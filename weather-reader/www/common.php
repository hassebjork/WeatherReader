<?php
header( 'Content-Type: text/html; charset=UTF-8' );

include_once( 'db.php' );

ini_set('display_errors', 1);

define( 'TEMPERATURE', 1 );
define( 'HUMIDITY', 2 );
define( 'WINDAVERAGE', 4 );
define( 'WINDGUST', 8 );
define( 'WINDDIRECTION', 16 );
define( 'RAINTOTAL', 32 );
define( 'SWITCH', 64 );
define( 'BAROMETER', 128 );
define( 'DISTANCE', 256 );
define( 'LEVEL', 512 );
define( 'TEST', 1024 );

$mysqli = new mysqli( DB_HOST, DB_USER, DB_PASS, DB_DATABASE );
$mysqli->set_charset( 'utf8' );

class Point {
	public $x;
	public $y;
	function __construct( $x=NULL, $y=NULL ) {
		if ( !$this->x ) $this->x = floatval( $x );
		if ( !$this->y ) $this->y = floatval( $y );
	}
	static function areaOfTriangle( $a, $b, $c ) {
		$area  = $a->x * ( $b->y - $c->y )
			   + $b->x * ( $c->y - $a->y )
			   + $c->x * ( $a->y - $b->y );
		return abs( $area / 2 );
	}
	static function lastKey( &$array, $index ) {	

		return key( array_slice( $array, $index, 1, TRUE ) );	
	}
}

function &fetch_data( $table, $id, $from, $to ) {
	$data  = array();

	$sql   = 'SELECT `value` AS `y`, UNIX_TIMESTAMP(`time`) AS `x` '
		. 'FROM  `' . $table . '` '
		. 'WHERE `sensor_id` = ' . $id . ' '
		. 'AND `time` BETWEEN "' . date( 'Y-m-d H:i:s', $from ) . '" AND "' . date( 'Y-m-d H:i:s', $to )  . '" '
		. 'ORDER BY `time` ASC;';
	$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data: ' . $sql );
	
	while( $row = $res->fetch_object( 'Point' ) ) {
		$data[] = $row;
	}
	return $data;
}

function graph( $data, $width=1000, $height=500 ) {
	$y_max = -PHP_INT_MAX;
	$y_min = PHP_INT_MAX;
	$x_max = -PHP_INT_MAX;
	$x_min = PHP_INT_MAX;
	
	foreach( $data as $set ) {
		foreach( $set as $row ) {
			if ( $row->y > $y_max ) $y_max = $row->y;
			if ( $row->y < $y_min ) $y_min = $row->y;
			if ( $row->x > $x_max )  $x_max  = $row->x;
			if ( $row->x < $x_min )  $x_min  = $row->x;
		}
	}
	
	$sv = $y_max - $y_min;
	$su = $x_max - $x_min;

	$dh = $height * .025;
	$dw = $width  * .025;

	$dy = $sv == 0 ? 1 : $height / $sv * .95;
	$dx = $su == 0 ? 1 : $width  / $su;

	$graph = array();
	
	foreach( $data as $k => $set ) {
		$graph[$k] = '';
		foreach( $set as $point ) {
			$x = (int) ( ( $point->x - $x_min ) * $dx );
			$graph[$k] .= $x . ',' . (int) ( ( $y_max - $point->y ) * $dy + $dh )  . ' ';
		}
	}
	
	return $graph;
}

// https://github.com/emcconville/point-reduction-algorithms/blob/master/src/Algorithms/VisvalingamWhyatt.php
function &whyatt( $points, $target ) {
	$kill = count($points) - $target;
	while ( $kill-- > 0 ) {
		$idx = 1;
		$minArea = Point::areaOfTriangle(
			$points[0],
			$points[1],
			$points[2]
		);
		foreach (range(2, Point::lastKey( $points, -2)) as $segment) {
			$area = Point::areaOfTriangle(
				$points[$segment - 1],
				$points[$segment],
				$points[$segment + 1]
			);
			if ( $area < $minArea ) {
				$minArea = $area;
				$idx = $segment;
			}
		}
		array_splice($points, $idx, 1);
	}
	return $points;
}

function &whyatt_qulity( $points, $maxArea ) {
	if ( count( $points ) < 4 )
		return $points;
	do {
		$idx = 1;
		$minArea = Point::areaOfTriangle(
			$points[0],
			$points[1],
			$points[2]
		);
		foreach (range(2, Point::lastKey( $points, -2)) as $segment) {
			$area = Point::areaOfTriangle(
				$points[$segment - 1],
				$points[$segment],
				$points[$segment + 1]// $sgrap  = slope_graph( $data, $a );
// $graph  = graph( array( $data, $kg2, $kg1, array( $data[0], $data[$dl] ), $sgrap ) );

			);
			if ( $area < $minArea ) {
				$minArea = $area;
				$idx = $segment;
			}
		}
		array_splice($points, $idx, 1);
	} while( $minArea < $maxArea && count( $points ) > 3 );
	return $points;
}

// https://www.youtube.com/watch?v=biY7F-tLwE8
// https://gist.github.com/Zymotico/836c5d82d5b52a2a3695
// http://www.magesblog.com/2014/12/measuring-temperature-with-my-arduino.html
function &kalman( $initialValue, $process_noise, $sensor_noise ) {
	$k = new stdClass;
	$k->r          = $sensor_noise;		// Sensor variance (0.3001871157)
	$k->Pn         = $process_noise;	// Process noise (1e-8)
	$k->P          = 1.0;				// Prediction error
	$k->Xe         = $initialValue;		// Kalman estimate
	return $k;
}
function kalman_filter( &$data, $value ) {
	// Predict
	$Pc = $data->P + $data->Pn;
	// Update
	$G  = $Pc == 0 ? 1 : $Pc / ( $Pc + $data->r );
	$data->P  = ( 1 - $G ) * $Pc;
	$data->Xe = $G * ( $value-$data->Xe ) + $data->Xe;
	return $data->Xe;
}
function &kalman_graph( &$data, &$kalman ) {
	$point = array();
	foreach( $data as $row ) {
		$point[] = new Point( $row->x, kalman_filter( $kalman, $row->y ) ); 
	}
	return $point;
}

