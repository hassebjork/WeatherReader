<?php
/*
	JavaScript minify:
	http://refresh-sf.com/
*/
include_once( 'db.php' );

ini_set('display_errors', 1);

define( 'MIN_GRAPH_DATA', 6 );
define( 'TEMPERATURE', 1 );
define( 'HUMIDITY', 2 );
define( 'WINDAVERAGE', 4 );
define( 'WINDGUST', 8 );
define( 'WINDDIRECTION', 16 );
define( 'RAINTOTAL', 32 );

define( 'SVG_COLOR_BG', '80a2e0' );
define( 'SVG_COLOR_WIND_ARROW', '2e8abb' );

$mysqli = new mysqli( DB_HOST, DB_USER, DB_PASS, DB_DATABASE );
$mysqli->set_charset( 'utf8' );

class Sensor {
	public  $id;
	public  $name;
	public  $team;
	public  $bat;
	public  $color;
	public  $type;
	public  $data = array();
	
	function __construct() {
		$this->current     = new stdClass;
		$this->max         = new stdClass;
		$this->min         = new stdClass;
	}

	static function &fetch_sensors() { 
		$sensors = array();
		$sql   = 'SELECT `id`, `name`, `team`, `type`, `color`, `battery` '
				.'FROM `wr_sensors` '
				.'ORDER BY `team`, `name`';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensors' );
		while( $row = $res->fetch_array() ) {
			$sensor = new Sensor;
			$sensor->id    = intval( $row['id'] );
			$sensor->name  = $row['name'];
			$sensor->team  = intval( $row['team'] );
			$sensor->type  = intval( $row['type'] );
			$sensor->color = dechex( $row['color'] );
			$sensor->bat   = intval( $row['battery'] );
			$sensors[$row['id']] = $sensor;
		}
		return $sensors;
	}
	
	static function &fetch_all( $days = 1 ) { 
		$sensors = Sensor::fetch_sensors();
		$temp = $humid = $wind = $rain = false;
		foreach ( $sensors as $sensor ) {
			if ( $sensor->type & TEMPERATURE )
				$temp = true;
			if ( $sensor->type & HUMIDITY )
				$humid = true;
			if ( $sensor->type & WINDDIRECTION )
				$wind = true;
			if ( $sensor->type & RAINTOTAL )
				$rain = true;
		}
		$data = array();
		if ( $temp )
			$data = Sensor::fetch_temperature( $data, $days );
		if ( $humid )
			$data = Sensor::fetch_humidity( $data, $days );
		if ( $wind )
			$data = Sensor::fetch_wind( $data, $days );
		if ( $temp )
			$data = Sensor::fetch_rain( $data, $sensors, $days );
		foreach ( $data as $key=>$val ) {
			// Skip sensors with team == 0
			if ( !array_key_exists( $key, $sensors ) ) {
				continue;
			} else if ( $sensors[$key]->team == 0 ) {
				unset( $sensors[$key] );
				continue;
			}
			if ( array_key_exists( 'max', $val ) ) {
				$sensors[$key]->max = @$val['max'];
				unset( $val['max'] );
			}
			if ( array_key_exists( 'min', $val ) ) {
				$sensors[$key]->min = @$val['min'];
				unset( $val['min'] );
			}
			foreach( $val as $d ) {
				$sensors[$key]->data[] = $d;
			}
		}
		return $sensors;
	}

	static function &fetch_temperature( $data, $days = 1 ) {
		return Sensor::fetch_th( $data, $days, 'wr_temperature', 't' );
	}
	static function fetch_humidity( $data, $days = 1 ) {
		return Sensor::fetch_th( $data, $days, 'wr_humidity', 'h' );
	}
	static function &fetch_th( $data, $days = 1, $tbl, $var ) {
		$sql   = 'SELECT `sensor_id`, '
			. 'ROUND( AVG( `value` ), 1 ) AS `var`, ' 
			. 'CONCAT( DATE( `time` ), " ", HOUR( `time` ) ) AS `date` '
			. 'FROM  `' . $tbl . '` '
			. 'WHERE `time` > SUBDATE( NOW(), ' . $days . ' ) '
			. 'GROUP BY `date`, `sensor_id` '
			. 'ORDER BY `time` ASC; ';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data' );
		while( $row = $res->fetch_object() ) {
			$id = $row->sensor_id;
			unset( $row->sensor_id );
			if ( !isset( $data[$id] ) )
				$data[$id] = array();
			if ( !isset( $data[$id][$row->date] ) ) {
				$data[$id][$row->date] = new stdClass;
				$data[$id][$row->date]->d = $row->date;
			}
			$data[$id][$row->date]->$var = floatVal( $row->var );
		}
		
		$sql   = 'SELECT `sensor_id`, '
			. 'MAX( `value` ) AS `max`, '
			. 'MIN( `value` ) AS `min` '
			. 'FROM  `' . $tbl . '` '
			. 'WHERE `time` > SUBDATE( NOW(), ' . $days . ' ) '
			. 'GROUP BY `sensor_id`';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data: ' . $sql );
		while( $row = $res->fetch_object() ) {
			$id = $row->sensor_id;
			if ( !isset($data[$id]['max'] ) )
				$data[$id]['max'] = new stdClass;
			if ( !isset($data[$id]['min'] ) )
				$data[$id]['min'] = new stdClass;
			$data[$id]['max']->$var = $row->max;
			$data[$id]['min']->$var = $row->min;
		}
		return $data;
	}
	
	static function &fetch_wind( $data, $days = 1 ) {
		$sql   = 'SELECT `sensor_id`, '
			. 'ROUND( AVG( `speed` ), 1 ) AS `speed`, ' 
			. 'MAX( `gust` ) AS `gust`, `dir`, ' 
			. 'CONCAT( DATE( `time` ), " ", HOUR( `time` ) ) AS `date` '
			. 'FROM  `wr_wind` '
			. 'WHERE `time` > SUBDATE( NOW(), ' . $days . ' ) AND speed > 0 '
			. 'GROUP BY `date`, `sensor_id` '
			. 'ORDER BY `time` ASC; ';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data: ' );
		while( $row = $res->fetch_object() ) {
			$id = $row->sensor_id;
			unset( $row->sensor_id );
			$wind = new stdClass;
			$wind->s = floatVal( $row->speed );
			$wind->g = floatVal( $row->gust  );
			$wind->d = intVal( $row->dir );
			if ( !isset( $data[$id] ) )
				$data[$id] = array();
			if ( !isset( $data[$id][$row->date] ) ) {
				$data[$id][$row->date] = new stdClass;
				$data[$id][$row->date]->d = $row->date;
			}
			if ( !isset($data[$id]['cur'] ) )
				$data[$id]['cur'] = new stdClass;
			$data[$id][$row->date]->w = $wind;
			$data[$id]['cur']->w = $wind;
		}
		return $data;
	}

	static function &fetch_rain( $data, $sensors, $days = 1 ) {
		$sql   = 'SELECT `sensor_id`, MAX(`total`) AS `total`, CONCAT( DATE( `time` ), " ", HOUR( `time` ) ) AS `date` '
			. 'FROM  `wr_rain` '
			. 'WHERE `time` > SUBTIME( NOW(), "' . $days . ' 1:0:0" ) '
			. 'GROUP BY `sensor_id`, `date` '
			. 'ORDER BY `sensor_id`, `time` ASC; ';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data: ' );
		$start  = 0;
		$last   = -1;
		$sensor = -1;
		$id     = 0;
		while( $row = $res->fetch_object() ) {
			$id = $row->sensor_id;
			if ( $sensor != $id ) {
				$sensor = $id;
				$start  = $row->total;
				$last   = -1;
				$sensors[$id]->max->r = 0;
			}
			
			if ( $last > -1 ) { //&& $row->total > $last ) {
				if ( !isset( $data[$id] ) ) $data[$id] = array();
				if ( !isset( $data[$id][$row->date] ) ) {
					$data[$id][$row->date] = new stdClass;
					$data[$id][$row->date]->d = $row->date;
				}
				$data[$id][$row->date]->r = $row->total - $last;
				if ( $data[$id][$row->date]->r > $sensors[$id]->max->r )
					$sensors[$id]->max->r = $data[$id][$row->date]->r;
			}
			$last = $row->total;
			$sensors[$id]->current->r  = $row->total - $start;
		}
		return $data;
	}

	static function draw_sensors() { 
		$sensors = Sensor::fetch_sensors();
		foreach ( $sensors as $sensor ) {
			// Skip sensors with team == 0
			if ( $sensor->team > 0 )
				$sensor->svg_head();
		}
	}
	
	function svg_head( $tabs = "\t" ) {
		$width = 150; $height = 75;
		if ( $this->type & TEMPERATURE || $this->type & HUMIDITY )
			echo $tabs
				.'<svg id="sTemp' . $this->id .'" class="c_head" width="'.$width.'px" height="'.$height.'px" '
				.'xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">' . "\n\t" . $tabs
				.'<use xlink:href="#svgBg" />' . "\n\t" . $tabs
				.'<text x="49%" y="24" class="title" style="fill:#' . $this->color . '"></text>' . "\n\t" . $tabs
				.'<g class="graph">' . "\n\t\t" . $tabs
				.'<polygon class="t_graph" points="0,'.($height/2).' '.$width.','.($height/2).' '.$width.','.$height.' 0,'.$height.'" style="fill:#' . $this->color . ';opacity:.25" />' . "\n\t" . $tabs
				.'</g>' . "\n\t" . $tabs
				.'<use x="1%" y="18" class="batt" xlink:href="#sBat" style="opacity:.5;visibility:hidden" />' . "\n\t" . $tabs
				.'<g class="temp">' . "\n\t\t" . $tabs
				.'<text x="49%" y="48" class="t_cur"></text>' . "\n\t\t" . $tabs
				.'<text x="98%" y="38" class="t_max"></text>' . "\n\t\t" . $tabs
				.'<text x="98%" y="50" class="t_min"></text>' . "\n\t" . $tabs
				.'</g>' . "\n\t" . $tabs
				.'<g class="humi" style="visibility: hidden">' . "\n\t\t" . $tabs
				.'<text x="54%" y="36" class="h_max"></text>' . "\n\t\t" . $tabs
				.'<text x="54%" y="44" class="h_cur"></text>' . "\n\t\t" . $tabs
				.'<text x="54%" y="52" class="h_min"></text>' . "\n\t" . $tabs
				.'</g>' . "\n" . $tabs
				.'</svg>' . "\n";
		if ( $this->type & WINDDIRECTION )
			echo $tabs
				.'<svg id="sWind' . $this->id .'" width="'.$width.'px" height="'.$width.'px" '
				.'xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">' . "\n\t" . $tabs
				.'<use xlink:href="#svgBg" />' . "\n\t" . $tabs
				.'<use x="'.($width/2).'" y="'.($width/2).'" class="windArr" xlink:href="#sArrow" transform="rotate(0 0,0)"/>' . "\n\t" . $tabs
				.'<text x="50%" y="25%" class="windSpd"></text>' . "\n\t" . $tabs
				.'<text x="50%" y="50%" class="windDir"></text>' . "\n\t" . $tabs
				.'<text x="50%" y="75%" class="windGst"></text>' . "\n\t" . $tabs
				.'<use x="10" y="10" class="batt" xlink:href="#sBat" style="opacity:.5;visibility:hidden" />' . "\n\t" . $tabs
				.'</svg>' . "\n" . $tabs;
		if ( $this->type & RAINTOTAL )
			echo $tabs
				.'<svg id="sRain' . $this->id .'" width="'.$width.'px" height="'.$height.'px" '
				.'xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">' . "\n\t" . $tabs
				.'<use xlink:href="#svgBg" />' . "\n\t" . $tabs
				.'<use x="1%" y="18" class="batt" xlink:href="#sBat" style="opacity:.5;visibility:hidden" />' . "\n\t" . $tabs
				.'<polygon class="r_graph" points="0,'.($height-1).' '.$width.','.($height-1).' '.$width.','.$height.' 0,'.$height.'" style="fill:#0047e9;opacity:.25" />' . "\n\t" . $tabs
				.'<text x="49%" y="24" class="title" style="fill:#' . $this->color . '"></text>' . "\n\t" . $tabs
				.'<text x="49%" y="48" class="r_cur"></text>' . "\n\t" . $tabs
				.'</svg>' . "\n" . $tabs;
	}
}

if ( isset( $_REQUEST ) ) {
	if ( isset( $_REQUEST['all'] ) ) {
		header( 'Content-Type: application/json' );
		echo '{"sensors":' . json_encode( Sensor::fetch_all( $_REQUEST['all'] ) )
			. ',"time":"' . date('Y-m-d H:i:s' ) . '"}';
		exit;
	}
}

header( 'Content-Type: text/html; charset=UTF-8' );
$isMobile = preg_match("/(android|avantgo|blackberry|bolt|boost|cricket|docomo|fone|hiptop|mini|mobi|palm|phone|pie|tablet|up\.browser|up\.link|webos|wos)/i", $_SERVER["HTTP_USER_AGENT"]);
?>
<!DOCTYPE html>

<html>
<head>
	<meta charset="UTF-8"/>
	<meta name="viewport" content="width=device-width, height=device-height, user-scalable=yes" />
	
	<title>Sensors</title>
	<link rel="shortcut icon" href="icon.ico">
	<link rel="icon" href="icon.ico">

	<style type="text/css" media="all">
		body	   { font-family: Arial,Sans-serif; font-size: 12px; margin: 0px; padding: 0px; }
		text.title { font-size: 24px; text-anchor: middle; }
		text.windSpd { font-size: 18px; fill: #34637b; text-anchor: middle; }
		text.windGst { font-size: 18px; fill: #34637b; text-anchor: middle; }
		text.windDir { font-size: 18px; fill: #34637b; text-anchor: middle; }
		text.t_cur { font-size: 20px; text-anchor: end; fill: #666666; }
		text.t_min { text-anchor: end; fill: #6060ff; }
		text.t_max { text-anchor: end; fill: #ff6060; }
		text.h_cur { font-size: 8px; text-anchor: start; fill: #666666; }
		text.h_min { font-size: 8px; text-anchor: start; fill: #6060ff; }
		text.h_max { font-size: 8px; text-anchor: start; fill: #ff6060; }
		text.r_cur { font-size: 18px; text-anchor: middle; fill: #666666; }
		stop.bg1 { stop-color:#80a2e0; stop-opacity: 1; }
		stop.bg2 { stop-color:#fff; stop-opacity: 1; }
		stop.wArr1 { stop-color:#2e8abb; stop-opacity: 1; }
		stop.wArr2 { stop-color:#fff; stop-opacity: 1; }
	</style>
	<script>
<?php readfile( 'sensors.js' ) ?>
		window.onload = function() {
			loadSensor( "/weather/?all=1" );
<?php if ( !$isMobile ) { ?>
			var tim1 = setInterval( function() {
				loadSensor( "/weather/?all=1" );
			}, 600000 );
<?php } ?>
		}
	</script>
</head>

<body>
	<svg id="defsSVG" class="chart" width="0" height="0" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" style="visibility:hidden;">
		<defs>
			<linearGradient id="windGradArrow" x1="0%" y1="0%"   x2="0%" y2="100%">
				<stop offset="0%" class="wArr1" />
				<stop offset="100%" class="wArr2" />
			</linearGradient>
			<linearGradient id="gradBg" x1="0%" y1="100%" x2="100%" y2="0%">
				<stop offset="0%" class="bg1" />
				<stop offset="100%" class="bg2" />
			</linearGradient>
			<g id="sBat">
				<polygon points="0,20 0,2 2.5,2 2.5,0 7.5,0 7.5,2 10,2 10,20 0,20" style="fill: white; stroke: black; stroke-width; 1px;" />
				<polygon points="2,18 2,15 8,13 8,18 2,18" style="fill: #b30000; stroke: none;">
				</polygon>
			</g>
			<g id="svgBg">
 				<rect x="0" y="0" width="100%" height="100%" rx="10" ry="10" style="fill:url(#gradBg);stroke:#2e8abb" /> 
			</g>
			<g id="sArrow">
				<polygon points="0,-65 -25,-15 -5,-20 -20,65 0,60 20,65 5,-20 25,-15 0,-65" style="fill: url(#windGradArrow); stroke: #<?= SVG_COLOR_WIND_ARROW ?>; stroke-width; 5px; " transform="rotate(0)" />
			</g>
		</defs>
	</svg>
<?php
Sensor::draw_sensors();
?>
	<a id="aTime" href="#" onclick="loadSensor('/weather/?all=1');return false;" style="display:block;">Update</a>
</body>
</html>
