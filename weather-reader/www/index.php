<?php
include_once( 'db.php' );

ini_set('display_errors', 1);

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
						echo "\t\t\t" . '<div id="temp' . $id . '" class="sens">' . "\n\t\t\t\t" 
							. '<div class="s_name" style="color: #' . dechex( $sensor->color ) . '" title="' . $sensor->protocol . ':' . count( $sensor->data ) . '">' . $sensor->name . '</div>' . "\n\t\t\t\t"
							. '<div class="t_cur" title="No. ' . count( $sensor->data ) . '">' . @$sensor->cur->temp . '&deg;C</div> ' . "\n\t\t\t\t"
							. '<div class="s_cen">' 
							. ( $sensor->battery == 0 ? '<div class="batt0">&nbsp;</div>' : '' )
							. ( isset( $sensor->min->humid ) ? '<div class="humid">' 
									. '<div class="h_max">' . $sensor->max->humid . '%</div>' 
									. '<div class="h_cur">' . $sensor->cur->humid . '%</div>' 
									. '<div class="h_min">' . $sensor->min->humid . '%</div></div>' : '' )
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
	
		echo "\n\t" 
			. '<script>' . "\n\t\t"
			. 'var lineOpts = {' . "\n\t\t\t"
			. 'bezierCurve : false,' . "\n\t\t\t"
			. 'pointDot : false,' . "\n\t\t\t"
			. 'animation : false,' . "\n\t\t\t"
			. 'scaleFontSize: 9' . "\n\t\t"
			. '};' . "\n\n";
		
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
			. 'ROUND( AVG( value ), 1 ) AS temp, '
			. 'HOUR( time ) AS hour, '
			. 'DATE( time ) AS day '
			. 'FROM  `wr_temperature` '
			. 'WHERE time > SUBDATE( NOW(), ' . $days . ' ) '
			. 'AND sensor_id = ' . $this->id . ' '
			. 'GROUP BY day, hour '
			. 'ORDER BY time ASC; ';
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
			. 'fillColor: "rgba(' . ( ( $this->color&0xFF0000 ) >> 16 ) . ',' . ( ( $this->color&0xFF00 ) >> 8 ) . ',' . ( $this->color&0xFF ) . ',.1)",' . "\n\t\t\t\t\t"
			. 'strokeColor: "rgba(' . ( ( $this->color&0xFF0000 ) >> 16 ) . ',' . ( ( $this->color&0xFF00 ) >> 8 ) . ',' . ( $this->color&0xFF ) . ',.5)",' . "\n\t\t\t\t\t"
			. 'data : [' . implode( $temp, ',' ) . ']'. "\n\t\t\t\t"
			. '},';
	}
}

class Wind {
	public $current;
	
	function __construct() {
		$this->current = Wind::fetch_current();
	}
	
	static function fetch_current() {
		$sql   = 'SELECT `id`, `speed`, `gust`, `dir`, `samples`, `time`, '
			.'UNIX_TIMESTAMP(`time`) AS `date` '
			.'FROM `wr_wind` ORDER BY `time` DESC LIMIT 1';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( $sql );
		return $res->fetch_object();
	}
	
	static function dir_to_text( $dir ) {
		if ( 337.5 >= $dir && $dir < 22.5 )
			return 'N';
		else if ( 22.5 >= $dir && $dir < 67.5 )
			return 'NE';
		else if ( 67.5 >= $dir && $dir < 112.5 )
			return 'E';
		else if ( 112.5 >= $dir && $dir < 157.5 )
			return 'SE';
		else if ( 157.5 >= $dir && $dir < 202.5 )
			return 'S';
		else if ( 202.5 >= $dir && $dir < 247.5 )
			return 'SW';
		else if ( 247.5 >= $dir && $dir < 292.5 )
			return 'W';
		return 'NW';
	}
}

if ( isset( $_REQUEST ) ) {
	if ( isset( $_REQUEST['wind'] ) ) {
		header( 'Content-Type: application/json' );
		$wind = new Wind();
		echo '{"wind":' . json_encode( $wind->fetch_current() ) . '}';
	// 	echo '{"wind":{"id":"500","speed":"1.0","gust":"1.0","dir":"180","samples":"5","time":"2015-03-01 09:03:04","date":"1425196984"}}';
		exit;
	}
}
$isMobile = preg_match("/(android|avantgo|blackberry|bolt|boost|cricket|docomo|fone|hiptop|mini|mobi|palm|phone|pie|tablet|up\.browser|up\.link|webos|wos)/i", $_SERVER["HTTP_USER_AGENT"]);

header( 'Content-Type: text/html; charset=UTF-8' );

?>
<!DOCTYPE html>

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
		.r		{ text-align: right }
		
		div.graph  { display: inline-block; vertical-align: top; }
		div.sens   { display: inline-block; }
		div.s_name { font-size: 24px; text-align: center; }
		div.s_cen  { height: 28px; display: inline-block; float: left; }
		
		div.t_cur  { width: 48%; font-size: 24px; color: #666; float: left; display: block; text-align: right; padding-right: 10px}
		div.t_min, div.t_max { display: block; text-align: right; float:right; }
		div.t_min  { color: #6060ff; }
		div.t_max  { color: #ff6060; }
		
		div.humid  { font-size: 8px; text-align: center; background-color: #c6dcf3; }
		div.h_min  { color: #6060ff; }
		div.h_max  { color: #ff6060; }
		div.h_cur  { color: #666; font-weight: bold; }

		div.batt0, div.batt1, div.batt2, div.batt3 { background-image: url("bs16.png"); background-repeat: no-repeat; width: 16px; height: 16px; display: block;  }
		div.batt3 { background-position:   0px 0px; }
		div.batt2 { background-position: -16px 0px; }
		div.batt1 { background-position: -32px 0px; }
		div.batt0 { background-position: -48px 0px; }

		#wind { font-size: 24px; color: #2e8abb; text-align: center; width: 350px; height: 150px; }
	</style>
	<script src="Chart.min.js"></script><!--http://www.chartjs.org/ ver 0.2-->
	<script>
		function getHttpRequest() {
			try {
				var req = new XMLHttpRequest();
			} catch (e) {	// IE
				try {
					req = new ActiveXObject("Msxml2.XMLHTTP");
				} catch (e) {
					req = new ActiveXObject("Microsoft.XMLHTTP");
				}
			}
			return req;
		}
		function updateWind( wind ) {
			document.getElementById("svg_wind_speed").textContent = wind.speed + " m/s";
			document.getElementById("svg_wind_dir").textContent   = wind.dir + "°";
			document.getElementById("svg_wind_arrow").transform.baseVal.getItem(0).setRotate(wind.dir,25,65);
		}
		function loadWind() {
			var req = getHttpRequest();
			req.onreadystatechange = function() {
				if ( req.readyState == 4 ) {
					var obj = JSON.parse( req.responseText );
					if ( obj.wind != 'undefined' )
						updateWind( obj.wind );
				}
			}
			req.open( "GET", "/weather/?wind=0", true );
			req.send();
		}
		window.onload = function() {
<?php if ( !$isMobile ) { ?>
			var tim = setInterval( function() {
				loadWind();
			}, 60000 );
<?php } ?>
		}
	</script>
</head>

<body>
<?php
	
$groups  = new Groups( 1 );
$groups->draw_graph();

$wind = new Wind();
if ( isset( $wind->current->speed ) ) {
	$color_bg = '80a2e0'; $color_text = '2e8abb'; $color_arrow = '2e8abb';
?>
	<div id="wind">
		<div id="wind_title">Wind</div>
		<svg id="windSVG" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
			<defs>
				<linearGradient id="windGradArrow" x1="0%" y1="0%" x2="0%" y2="100%"><stop offset="0%" style="stop-color:#<?= $color_arrow ?>; stop-opacity: 1" /><stop offset="100%" style="stop-color:#fff; stop-opacity: 1" /></linearGradient>
				<linearGradient id="windGradBg" x1="0%" y1="100%" x2="0%" y2="0%"><stop offset="0%" style="stop-color:#<?= $color_bg ?>; stop-opacity: 1" /><stop offset="100%" style="stop-color:#fff; stop-opacity: 1" /></linearGradient>
			</defs>
			<rect id="svg_wind_bg" width="350" height="150" style="fill:url(#windGradBg);" />
			<g transform="translate(150,10)"><polygon id="svg_wind_arrow" points="25,0 0,50 20,45 5,130 25,125 45,130 30,45 50,50 25,0" style="fill: url(#windGradArrow); stroke: #<?= $color_arrow ?>; stroke-width; 5px; " transform="rotate(<?= $wind->current->dir ?> 25,65)" /></g>
			<text id="svg_wind_speed" x="175" y="24" text-anchor="middle" style="fill:#<?= $color_text ?>"><?= $wind->current->speed ?> m/s</text>
			<text  id="svg_wind_dir"x="175" y="48" text-anchor="middle" style="fill:#<?= $color_text ?>"><?= $wind->current->dir ?>&deg;</text>
		</svg>
	</div>
<?php } ?>
</body>
</html>
