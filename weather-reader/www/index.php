<?php
include_once( 'db.php' );

ini_set('display_errors', 1);
header( 'Content-Type: text/html; charset=UTF-8' );

define( 'MIN_GRAPH_DATA', 6 );
define( 'TEMPERATURE', 1 );
define( 'HUMIDITY', 2 );
define( 'WINDAVERAGE', 4 );
define( 'WINDGUST', 8 );
define( 'WINDDIRECTION', 16 );
define( 'RAINTOTAL', 32 );

$mysqli = new mysqli( DB_HOST, DB_USER, DB_PASS, DB_DATABASE );
$mysqli->set_charset( 'utf8' );

class Groups {
	public $id_to_group = array();
	public $team = array();
	public $days  = 1;
	
	function __construct( $days = 1 ) {
		$this->days = $days;
		foreach( Sensor::fetch_all( $days ) as $v ) {
//  			var_dump( $v );
			$this->id_to_group[ $v->id] = $v->team;
			$this->team[ $v->team ][$v->id] = $v; 
		}
		ksort( $this->id_to_group );
	}
	
	function &get_sensor( $id ) {
		return $this->team[$this->id_to_group[$id]][$id];
	}
	
	function draw_graph() {
		foreach( $this->team as $no=>$team ) {
		
			$has_data[$no] = false;
			foreach( $team as $sensor ) {
				if ( $sensor->has_data() && count( $sensor->data ) > MIN_GRAPH_DATA ) {
					$has_data[$no] = true;
				}
			}
			
			if ( $has_data[$no] ) {
				echo "\t"  . '<div id="graph' . $no . '" class="graph">'  
				. "\n\t\t" . '<div class="graph_title">' . "\n";
				
				foreach( $team as $id=>$sensor ) {
					if ( $sensor->type & TEMPERATURE && $sensor->has_data() && count( $sensor->data ) > 8 ) {
						echo "\t\t\t" . '<div id="temp' . $id . '" class="temp">' . "\n\t\t\t\t" 
							. '<div class="t_name" style="color: #' . dechex( $sensor->color ) . '" title="' . $sensor->protocol . ':' . count( $sensor->data ) . '">' . $sensor->name . '</div>' . "\n\t\t\t\t"
							. '<div class="t_cur" title="No. ' . count( $sensor->data ) . '">' . @$sensor->cur->temp . '&deg;C</div> ' . "\n\t\t\t\t"
							. '<div class="t_cen">' 
							. ( $sensor->battery == 0 ? '<div class="batt0">&nbsp;</div>' :
							( isset( $sensor->min->humid ) ? '<div class="humid">' 
									. '<div class="h_max">' . $sensor->max->humid . '%</div>' 
									. '<div class="h_cur">' . $sensor->cur->humid . '%</div>' 
									. '<div class="h_min">' . $sensor->min->humid . '%</div></div>' : '' ) )
							. '</div> ' . "\n\t\t\t\t"
							. '<div class="t_max">' . @$sensor->max->temp . '&deg;C</div> ' . "\n\t\t\t\t"
							. '<div class="t_min">' . @$sensor->min->temp . '&deg;C</div> ' . "\n\t\t\t"
							. '</div>' . "\n";
					}
				}
				echo "\t\t</div>\n\t\t" . '<canvas id="graphCanvas' . $no .'" height="150" width="350"></canvas>' . "\n\t"
				.'</div>' . "\n\n";
			}
		}
	
		echo "\n\t" . '<script>' . "\n\t\t"
			. 'var lineOpts = {' . "\n\t\t\t"
			. 'bezierCurve : false,' . "\n\t\t\t"
			. 'pointDot : false,' . "\n\t\t\t"
			. 'animation : false,' . "\n\t\t\t"
			. 'scaleFontSize: 9' . "\n\t\t"
			. '}' . "\n\n";
		
		$date   = date( 'H', ( time() - $this->days * 24 * 3600 ) );
		$labels = array();
		for ( $i = 0; $i < 24 * $this->days; $i++ ) {
			$label    = ( $date + $i ) % 24;
			$labels[] = ( $label == 0 ? date('"l"') : $label );
		}

		foreach( $this->team as $no=>$team ) {
			if ( $has_data[$no] ) {
				echo "\n\t\t" . 'var lineChartData' . $no .' = {' . "\n\t\t\t" 
					. 'labels : [' . implode( $labels, ',' ) . '],' . "\n\t\t\t"
					. 'datasets : [';
				foreach( $team as $sensor ) {
					if ( $sensor->has_data() && count( $sensor->data ) > MIN_GRAPH_DATA )
						echo $sensor->js_graph();
				}
				echo "\n\t\t\t]\n\t\t}\n\t\t" 
					. 'var chart' . $no . ' = new Chart(document.getElementById("graphCanvas' . $no
					. '").getContext("2d")).Line(lineChartData' . $no . ',lineOpts);' . "\n";
			}
		}
		echo  "\t</script>\n";
	}
}

class Sensor {
	public  $id;
	public  $name;
	public  $protocol;
	public  $sensor_id;
	public  $team;
	public  $battery;
	public  $color;
	public  $type;
	public  $data        = array();
	public  $days        = NULL;
	private $has_data    = NULL;
	
	function __construct() {
		$this->max        = new stdClass;
		$this->min        = new stdClass;
		$this->cur        = new stdClass;
		$this->min->temp  = NULL;
		$this->max->temp  = NULL;
		$this->cur->temp  = NULL;
		$this->min->humid = NULL;
		$this->max->humid = NULL;
		$this->cur->humid = NULL;
	}

	function has_data() {
		if ( $this->has_data == NULL )
			$this->has_data = ( count( $this->data ) > 0 );
		return $this->has_data;
	}
	
	static function has_temperature() {
		return ( $this->type & TEMPERATURE );
	}
	
	static function has_humidity() {
		return ( $this->type & HUMIDITY );
	}
	
	static function &fetch_sensors() { 
		$sensors = array();
		
		$sql   = 'SELECT * FROM `wr_sensors` ORDER BY `team`, `name`;';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensors' );
		while( $row = $res->fetch_array() ) {
			$sensors[$row['id']] = Sensor::from_array( $row );
		}
		return $sensors;
	}
	
	static function &fetch_all( $days = 1 ) { 
		$sensors = Sensor::fetch_sensors();
		
		// Temperature data
		$sql   = 'SELECT sensor_id, '
			. 'ROUND( AVG( value ), 1 ) AS temp, ' 
			. 'HOUR( time ) AS hour, '
			. 'DATE( time ) AS day '
			. 'FROM  `wr_temperature` '
			. 'WHERE time > SUBDATE( NOW(), ' . $days . ' ) '
			. 'GROUP BY day, hour, sensor_id '
			. 'ORDER BY time ASC; ';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data' );
		while( $row = $res->fetch_object() ) {
			$id = $row->sensor_id;
			unset( $row->sensor_id );
			if ( !$sensors[$id]->type & TEMPERATURE )
				unset( $row->temp );
			$sensors[$id]->data[] = $row;
		}
		
		// Temperature max/min & current values
		$sql   = 'SELECT sensor_id, '
			. 'MAX( `value` ) AS tmax, MIN( `value` ) AS tmin '
			. 'FROM  `wr_temperature` '
			. 'WHERE time > SUBDATE( NOW(), ' . $days . ' ) '
			. 'GROUP BY sensor_id';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data: ' . $sql );
		while( $row = $res->fetch_object() ) {
			$id = $row->sensor_id;
			$sensors[$row->sensor_id]->days = $days;
			if ( $sensors[$id]->type & TEMPERATURE ) {
				$sensors[$id]->min->temp = $row->tmin;
				$sensors[$id]->max->temp = $row->tmax;
				$sensors[$id]->cur->temp = $sensors[$id]->data[count($sensors[$id]->data)-1]->temp;
			}
		}
		
		// Humidity max/min values
		$sql   = 'SELECT sensor_id, '
			. 'MAX( `value` ) AS hmax, MIN( `value` ) AS hmin '
			. 'FROM  `wr_humidity` '
			. 'WHERE time > SUBDATE( NOW(), ' . $days . ' ) '
			. 'GROUP BY sensor_id';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data: ' . $sql );
		while( $row = $res->fetch_object() ) {
			$id = $row->sensor_id;
			$sensors[$row->sensor_id]->days = $days;
			if ( $sensors[$id]->type & HUMIDITY ) {
				$sensors[$id]->min->humid = $row->hmin;
				$sensors[$id]->max->humid = $row->hmax;
			}
		}
		
		// Humidity current value
		$sql   = 'SELECT sensor_id, value FROM '
			. '( SELECT sensor_id, value FROM `wr_humidity` ORDER BY id DESC ) '
			. 'AS x GROUP BY sensor_id';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data: ' . $sql );
		while( $row = $res->fetch_object() ) {
			$id = $row->sensor_id;
			$sensors[$row->sensor_id]->days = $days;
			if ( $sensors[$id]->type & HUMIDITY ) {
				$sensors[$id]->cur->humid = $row->value;
			}
		}

		return $sensors;
	}

	static function &from_array( &$row ) {
		$sensor = new Sensor;
		$sensor->id        = intval( $row['id'] );
		$sensor->name      = str_replace( ' ', '&nbsp;', $row['name'] );
		$sensor->protocol  = $row['protocol'];
		$sensor->sensor_id = intval( $row['sensor_id'] );
		$sensor->team      = intval( $row['team'] );
		$sensor->type      = intval( $row['type'] );
		$sensor->color     = intval( $row['color'] );
		$sensor->battery   = intval( $row['battery'] );
		return $sensor;
	}

	function fetch_data( $days = 1 ) {
		$row = 0;
		$this->data = array();
		$sql   = 'SELECT '
			. ( ( $this->type & TEMPERATURE ) == 0 ? '' : 'ROUND( AVG( value ), 1 ) AS temp, ' )
			. 'HOUR( time ) AS hour, '
			. 'DATE( time ) AS day '
			. 'FROM  `wr_temperature` '
			. 'WHERE time > SUBDATE( NOW(), ' . $days . ' ) '
			. 'AND sensor_id = ' . $this->id . ' '
			. 'GROUP BY day, hour '
			. 'ORDER BY time ASC; '
			. '# Config: ' . $this->type 
			. ':' . ( $this->type & TEMPERATURE );
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data' );
		
		$this->has_data = ( $res->num_rows > 0 );
		$this->days     = $days;
		if ( !$this->has_data ) {
			$this->days = null;
			return;
		}
		
		if ( ( $this->type & TEMPERATURE ) > 0 )
			$this->temp->max = $this->temp->min = NULL; 
		if ( ( $this->type & HUMIDITY ) > 0 )
			$this->hum->max = $this->hum->min = NULL;
		
		while( $row = $res->fetch_object() ) {
			$this->data[] = $row;
			if ( ( $this->type & TEMPERATURE ) > 0 ) { 
				$this->temp->cur = $row->temp;
				if ( $row->temp > $this->temp->max || $this->temp->max == NULL )
					$this->temp->max = $row->temp;
				if ( $row->temp < $this->temp->min || $this->temp->min == NULL )
					$this->temp->min = $row->temp;
			}
			if ( ( $this->type & HUMIDITY ) > 0 ) { 
				$this->hum->cur = $row->hum;
				if ( $row->hum > $this->hum->max || $this->hum->max == NULL )
					$this->hum->max = $row->hum;
				if ( $row->hum < $this->hum->min || $this->hum->min == NULL )
					$this->hum->min = $row->hum;
			}
// 			var_dump( $row );
		}
	}

	function js_graph( $type = TEMPERATURE ) {
		if ( ( $this->type & $type ) == 0 )
			return;
		if ( $this->data == NULL )
			$this->fetch_data();
			
		if ( !is_array( $this->data ) ) {
			return '';
		}

		$hour = intval( date( 'H' ) );
		$temp = array();
		foreach ( $this->data as $no => $data ) {
			while( $hour < $data->hour ) {
				$temp[] = null;
				$hour = ( $hour + 1 ) % 24;
			}
			switch( $type ) {
				case TEMPERATURE:
					$temp[] = $data->temp;
					break;
				case HUMIDITY:
					$temp[] = $data->hum;
					break;
				default:
					break;
			}
			$hour = ( $hour + 1 ) % 24;
		}
		
		return "\n\t\t\t\t" . '{ // ' . $this->name . " (" . count( $temp ) . ")\n\t\t\t\t\t"
			. 'fillColor: "rgba(' . ( ( $this->color&0xFF0000 ) >> 16 ) . ',' . ( ( $this->color&0xFF00 ) >> 8 ) . ',' . ( $this->color&0xFF ) . ',0.1)",' . "\n\t\t\t\t\t"
			. 'strokeColor: "rgba(' . ( ( $this->color&0xFF0000 ) >> 16 ) . ',' . ( ( $this->color&0xFF00 ) >> 8 ) . ',' . ( $this->color&0xFF ) . ',1)",' . "\n\t\t\t\t\t"
			. 'data : [' . implode( $temp, ',' ) . ']'. "\n\t\t\t\t"
			. '},';
	}
}

?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html>
<head>
	<meta charset="UTF-8"/>
	<meta http-equiv="refresh" content="<?= ( 3600 - intval( date( 'i') ) * 60 ) ?>"/>
	<meta name="viewport" content="width=device-width, height=device-height, user-scalable=yes" />
	
	<title>Temperature</title>
	<link rel="shortcut icon" href="icon.ico">
	<link rel="icon" href="icon.ico">

	<style type="text/css" media="all">
		body	{ font-family: Arial,Sans-serif; font-size: 12px; margin: 0px; padding: 0px; }
		td 		{ vertical-align: top; padding: 2px; }
		th 		{ vertical-align: top; padding: 2px; }
		.r		{ text-align: right }
		.even	{ background-color: #ffffff }
		.odd	{ background-color: #e0e0e0 }
		div.graph  { display: inline-block; vertical-align: top; }
		div.temp   { display: inline-block; }
		div.t_name { font-size: 24px; text-align: center; }
		div.t_cur  { width: 48%; font-size: 24px; color: #666; float: left; display: block; text-align: right; }
		div.t_min  { color: #6060ff; display: block; text-align: right; float:right; }
		div.t_max  { color: #ff6060; display: block; text-align: right; float:right; }
		div.t_cen  { height: 28px; display: inline-block; float: left; }
		div.humid  { font-size: 8px; text-align: center; background-color: #c6dcf3; }
		div.h_min  { color: #6060ff; }
		div.h_max  { color: #ff6060; }
		div.h_cur  { color: #666; font-weight: bold; }
		#temp td {text-align: right; padding: 0px; }
		.batt3 { background-position:   0px 0px; background-image: url("bs16.png"); background-repeat: no-repeat; width: 16px; height: 16px; display: block; }
		.batt2 { background-position: -16px 0px; background-image: url("bs16.png"); background-repeat: no-repeat; width: 16px; height: 16px; display: block; }
		.batt1 { background-position: -32px 0px; background-image: url("bs16.png"); background-repeat: no-repeat; width: 16px; height: 16px; display: block; }
		.batt0 { background-position: -48px 0px; background-image: url("bs16.png"); background-repeat: no-repeat; width: 16px; height: 16px; display: block; }

	</style>
	<script src="Chart.min.js"></script>
</head>

<body>
<?php
	
// $sensors = Sensor::fetch_all();
$groups  = new Groups( 1 );
$groups->draw_graph();

?>
</body>
</html>