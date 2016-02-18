<?php
/*	
	SVG:
			http://tutorials.jenkov.com/svg
			
	JavaScript minify:
			http://refresh-sf.com/
	
	MySQL Archive:
			http://stackoverflow.com/a/7725900/4405465
			
	MySQL Median calc:
			http://stackoverflow.com/a/16680264/4405465
			http://stackoverflow.com/questions/6982808/mysql-median-value-per-month
			http://stackoverflow.com/questions/1291152/simple-way-to-calculate-median-with-mysql
			http://rpbouman.blogspot.se/2008/07/fast-single-pass-method-to-calculate.html
					
	TODO
	Pellets days of equidistance
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
define( 'SWITCH', 64 );
define( 'BAROMETER', 128 );
define( 'DISTANCE', 256 );
define( 'LEVEL', 512 );
define( 'TEST', 1024 );

$mysqli = new mysqli( DB_HOST, DB_USER, DB_PASS, DB_DATABASE );
$mysqli->set_charset( 'utf8' );

class Sensor {
	public  $id;
	public  $name;
	public  $team;
	public  $bat;
	public  $type;
	public  $data = array();
	
	function __construct() {
		$this->current     = new stdClass;
		$this->max         = new stdClass;
		$this->min         = new stdClass;
	}

	static function &fetch_sensors() { 
		$sensors = array();
		$sql   = 'SELECT `id`, `name`, `protocol`, `team`, `type`, `battery` '
				.'FROM `wr_sensors` '
				.'WHERE `team` > 0 '
				.'ORDER BY `team`, `name`';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensors' );
		while( $row = $res->fetch_array() ) {
			$sensor = new Sensor;
			$sensor->id       = intval( $row['id'] );
			$sensor->name     = $row['name'];
			$sensor->protocol = $row['protocol'];
			$sensor->team     = intval( $row['team'] );
			$sensor->type     = intval( $row['type'] );
			$sensor->bat      = intval( $row['battery'] );
			$sensors[$row['id']] = $sensor;
		}
		return $sensors;
	}
	
	static function &fetch_all( $days = 1 ) { 
		$days = ( is_numeric( $days ) ? $days : 1 );
		$sensors = Sensor::fetch_sensors();
		$temp = $humid = $wind = $rain = $distance = $barometer = false;
		foreach ( $sensors as $sensor ) {
			if ( $sensor->type & TEMPERATURE )
				$temp = true;
			if ( $sensor->type & HUMIDITY )
				$humid = true;
			if ( $sensor->type & WINDDIRECTION )
				$wind = true;
			if ( $sensor->type & RAINTOTAL )
				$rain = true;
			if ( $sensor->type & DISTANCE )
				$distance = true;
			if ( $sensor->type & BAROMETER )
				$barometer = true;
		}
		$data = array();
		if ( $temp )
			$data = Sensor::fetch_temperature( $data, $days );
		if ( $humid )
			$data = Sensor::fetch_humidity( $data, $days );
		if ( $wind )
			$data = Sensor::fetch_wind( $data, $days );
		if ( $rain )
			$data = Sensor::fetch_rain( $data, $sensors, $days );
		if ( $distance )
			$data = Sensor::fetch_distance( $data, 5 );
		if ( $barometer )
			$data = Sensor::fetch_barometer( $data, $days );
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
	static function fetch_barometer( $data, $days = 1 ) {
		return Sensor::fetch_th( $data, $days, 'wr_barometer', 'b' );
	}
	static function &fetch_th( $data, $days = 1, $tbl, $var ) {
		$sql   = 'SELECT `sensor_id`, '
			. 'ROUND( AVG( `value` ), 1 ) AS `var`, ' 
			. 'DATE_FORMAT( `time`, "%m%d%H" ) AS `date` '
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
			. 'DATE_FORMAT( `time`, "%m%d%H" ) AS `date` '
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
		$sql   = 'SELECT `sensor_id`, MAX(`total`) AS `total`, '
			. 'DATE_FORMAT( `time`, "%m%d%H" ) AS `date` '
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
				@$sensors[$id]->max->r = 0;
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
			@$sensors[$id]->current->r  = $row->total - $start;
		}
		return $data;
	}

	static function fetch_distance( $data, $days = 8 ) {
		$sql   = 'SELECT `sensor_id`, '
			. 'ROUND( AVG( `value` ), 0 ) AS `var`, ' 
			. 'DATE_FORMAT( `time`, "%m%d%H" ) AS `date` '
			. 'FROM  `wr_distance` '
			. 'WHERE `time` > SUBDATE( NOW(), ' . $days . ' ) '
// 			. 'GROUP BY `date`, `sensor_id` '
			. 'GROUP BY DATE(`time`), HOUR(`time`), `sensor_id` '
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
			$data[$id][$row->date]->v = intVal( $row->var );
		}
// 		$data[$id]['sql'] = $sql;
		return $data;
	}
	
	static function json_sensors() { 
		$sensors = array();
		$sql   = 'SELECT `id`, `name`, `protocol`, `team`, `type`, `battery` '
				.'FROM `wr_sensors` '
				.'ORDER BY `team`, `name` ASC';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensors' );
		while( $row = $res->fetch_object() ) {
			$sensor = new stdClass;
			$sensor->id       = intval( $row->id );
			$sensor->name     = $row->name;
			$sensor->protocol = $row->protocol;
			$sensor->team     = intval( $row->team );
			$sensor->type     = intval( $row->type );
			$sensor->bat      = intval( $row->battery );
			$sensors[]        = $sensor;
		}
		return '"sensors":' . json_encode( $sensors, JSON_NUMERIC_CHECK );
	}
	
	static function json_temperature( $days = 1 ) {
		$days = ( is_numeric( $days ) ? $days : 1 );
 		return '"temperature":' . Sensor::json_th( $days, 'wr_temperature' );
	}
	
	static function json_humidity( $days = 1 ) { 
		$days = ( is_numeric( $days ) ? $days : 1 );
 		return '"humidity":' . Sensor::json_th( $days, 'wr_humidity' );
	}
	
	static function json_barometer( $days = 1 ) { 
		$days = ( is_numeric( $days ) ? $days : 1 );
 		return '"barometer":' . Sensor::json_th( $days, 'wr_barometer' );
	}
	
	static function json_th( $days = 1, $tbl ) { 
		$sensors = array();
		$sql   = 'SELECT `sensor_id`, '
			. 'ROUND( AVG( `value` ), 1 ) AS `var`, ' 
			. 'DATE_FORMAT( `time`, "%m%d%H" ) AS `date` '
			. 'FROM  `' . $tbl . '` '
			. 'WHERE `time` > SUBDATE( NOW(), ' . $days . ' ) '
			. 'GROUP BY `date`, `sensor_id` '
			. 'ORDER BY `date`, `sensor_id` ASC;';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data' );
		while( $row = $res->fetch_object() ) {
			$sensors[$row->date][$row->sensor_id] = floatVal( $row->var );
		}
		
		$sql   = 'SELECT `sensor_id`, '
			. 'MAX( `value` ) AS `max`, '
			. 'MIN( `value` ) AS `min` '
			. 'FROM  `' . $tbl . '` '
			. 'WHERE `time` > SUBDATE( NOW(), ' . $days . ' ) '
			. 'GROUP BY `sensor_id` '
			. 'ORDER BY `sensor_id` ASC;';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data: ' . $sql );
		while( $row = $res->fetch_object() ) {
			$sensors['max'][$row->sensor_id] = floatVal( $row->max );
			$sensors['min'][$row->sensor_id] = floatVal( $row->min );
		}
 		return json_encode( $sensors, JSON_NUMERIC_CHECK );
	}
	
	static function json_rain( $days = 1 ) {
		$days = ( is_numeric( $days ) ? $days : 1 );
		$sensors = array();
		$sql   = 'SELECT `sensor_id`, MAX(`total`) AS `total`, '
			. 'DATE_FORMAT( `time`, "%m%d%H" ) AS `date` '
			. 'FROM  `wr_rain` '
			. 'WHERE `time` > SUBTIME( NOW(), "' . $days . ' 1:0:0" ) '
			. 'GROUP BY `sensor_id`, `date` '
			. 'ORDER BY `date`, `sensor_id` ASC;';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data: ' );
		while( $row = $res->fetch_object() ) {
			$sensors[$row->date][$row->sensor_id] = intVal( $row->total );
		}
 		return '"rain":' . json_encode( $sensors, JSON_NUMERIC_CHECK );
	}
	
	static function json_wind( $days = 1 ) {
		$days = ( is_numeric( $days ) ? $days : 1 );
		$sensors = array();
		$sql   = 'SELECT `sensor_id`, '
			. 'ROUND( AVG( `speed` ), 1 ) AS `speed`, ' 
			. 'MAX( `gust` ) AS `gust`, `dir`, ' 
			. 'DATE_FORMAT( `time`, "%m%d%H" ) AS `date` '
			. 'FROM  `wr_wind` '
			. 'WHERE `time` > SUBDATE( NOW(), ' . $days . ' ) '
			. 'GROUP BY `date`, `sensor_id` '
			. 'ORDER BY `date` ASC;';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data: ' );
		while( $row = $res->fetch_object() ) {
			$obj   = new stdClass;
			$obj->s = floatVal( $row->speed );
			$obj->g = floatVal( $row->gust );
			$obj->d = ( $row->dir == NULL ? $row->dir : intVal( $row->dir ) );
			$sensors[$row->date][$row->sensor_id] = $obj;
		}
 		return '"wind":' . json_encode( $sensors, JSON_NUMERIC_CHECK );
	}

	static function json_distance( $days = 8 ) {
		$days = ( is_numeric( $days ) ? $days : 8 );
		$sensors = array();
		$sql   = 'SELECT `sensor_id`, '
			. 'ROUND( AVG( `value` ), 1 ) AS `var`, ' 
			. 'DATE_FORMAT( `time`, "%m%d%H" ) AS `date` '
			. 'FROM  `wr_distance` '
			. 'WHERE `time` > SUBDATE( NOW(), ' . $days . ' ) '
// 			. 'GROUP BY `date`, `sensor_id` '
			. 'GROUP BY HOUR(`time`), `sensor_id` '
			. 'ORDER BY `date`, `sensor_id` ASC;';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data' );
		while( $row = $res->fetch_object() ) {
			$sensors[$row->date][$row->sensor_id] = intVal( $row->var );
		}
 		return '"distance":' . json_encode( $sensors, JSON_NUMERIC_CHECK );
	}
	
	static function draw_sensors() { 
		$sensors = Sensor::fetch_sensors();
		foreach ( $sensors as $sensor ) {
			// Skip sensors with team == 0
			if ( $sensor->team > 0 )
				$sensor->svg_head();
		}
	}
	
	function svg_head() {
		static $switch = 0;
		if ( $this->type & TEMPERATURE || $this->type & HUMIDITY ) {
			echo '<div id="sTemp' . $this->id .'" class="t_widget team' . $this->team . '">'
				.'<svg width="100%" height="100%" '
				.'xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">'
				.'<use xlink:href="#svgBg" class="widgetBg"/>'
				.'<polygon class="t_graph"/>'
				.'<use x="5" y="8" class="batt" style="visibility:hidden" xlink:href="#icon_bat"/>'
				.'<g class="t_ruler"/>'
				.'</svg>'
				.'<div class="title">' . $this->name . '</div>'
				.'<div class="t_cur"></div>'
				.'<div class="t_max"></div>'
				.'<div class="t_min"></div>'
				.'<div class="h_cur"></div>'
				.'<div class="h_max"></div>'
				.'<div class="h_min"></div>'
				.'</div>' . "\n";
		}
		if ( $this->type & DISTANCE ) {
			echo '<div id="sDist' . $this->id .'" class="d_widget team' . $this->team . '">'
				.'<svg width="100%" height="100%" '
				.'xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">'
				.'<use xlink:href="#svgBg" class="widgetBg"/>'
				.'<polygon class="d_graph"/>'
				.'<g class="d_ruler"/>'
				.'</svg>'
				.'<div class="title">' . $this->name . '</div>'
				.'<div class="d_cur"></div>'
				.'</div>' . "\n";
		}
	}
}

if ( isset( $_REQUEST ) ) {
	if ( isset( $_REQUEST['all'] ) ) {
		header( 'Content-Type: application/json' );
		echo '{"sensors":' . json_encode( Sensor::fetch_all( $_REQUEST['all'] ), JSON_NUMERIC_CHECK | JSON_PRETTY_PRINT )
			. ',"time":"' . date('Y-m-d H:i:s' ) . '"}';
		exit;
	} else if ( isset( $_REQUEST['ftp'] ) ) {
		$data = '{"sensors":' . json_encode( Sensor::fetch_all( 1 ), JSON_NUMERIC_CHECK  )
			. ',"time":"' . date('Y-m-d H:i:s' ) . '"}';
		$host = 'ftp://' . FTP_USER . ':' . FTP_PASS . '@' . FTP_SERVER . FTP_DIR . 'all.js';
		$stream = stream_context_create( array('ftp' => array('overwrite' => true ) ) );
		echo file_put_contents( $host, $data, 0, $stream );
		exit;
	} else if ( isset( $_REQUEST['sensors'] ) ) {
		header( 'Content-Type: application/json' );
		echo Sensor::json_sensors();
		exit;
	} else if ( isset( $_REQUEST['temperature'] ) ) {
		header( 'Content-Type: application/json' );
		echo Sensor::json_temperature( $_REQUEST['temperature'] );
		exit;
	} else if ( isset( $_REQUEST['humidity'] ) ) {
		header( 'Content-Type: application/json' );
		echo Sensor::json_humidity( $_REQUEST['humidity'] );
		exit;
	} else if ( isset( $_REQUEST['barometer'] ) ) {
		header( 'Content-Type: application/json' );
		echo Sensor::json_barometer( $_REQUEST['barometer'] );
		exit;
	} else if ( isset( $_REQUEST['rain'] ) ) {
		header( 'Content-Type: application/json' );
		echo Sensor::json_rain( $_REQUEST['rain'] );
		exit;
	} else if ( isset( $_REQUEST['wind'] ) ) {
		header( 'Content-Type: application/json' );
		echo Sensor::json_wind( $_REQUEST['wind'] );
		exit;
	} else if ( isset( $_REQUEST['distance'] ) ) {
		header( 'Content-Type: application/json' );
		echo Sensor::json_distance( $_REQUEST['distance'] );
		exit;
	} else if ( isset( $_REQUEST['total'] ) ) {
		header( 'Content-Type: application/json' );
		echo  Sensor::json_sensors() . ',' 
			. Sensor::json_temperature( $_REQUEST['total'] ) . ',' 
			. Sensor::json_humidity( $_REQUEST['total'] ) . ',' 
			. Sensor::json_barometer( $_REQUEST['total'] ) . ',' 
			. Sensor::json_rain( $_REQUEST['total'] ) . ',' 
			. Sensor::json_wind( $_REQUEST['total'] ) . ',' 
			. Sensor::json_distance( $_REQUEST['total'] * 10 );
		exit;
	}
}

header( 'Content-Type: text/html; charset=UTF-8' );
?>
<!DOCTYPE html>
<html>
<head>
	<meta charset="UTF-8"/>
	<meta name="viewport" content="width=device-width, user-scalable=yes" />
	
	<title>Weather</title>
	<link rel="shortcut icon" href="icon.ico">
	<link rel="icon" href="icon.ico">

	<style type="text/css" media="all">
		/* Global and common */
		@import url("http://fonts.googleapis.com/css?family=Droid+Sans");
		body	   { font-family: 'Droid Sans',Sans-serif; font-size: 14px; margin: 0px; padding: 0px; }
		text.title { font-size: 26px; text-anchor: middle; }
		svg        { float: left; }
		.team1     { color: #4e608b; fill: #4e608b; stroke: #4e608b; }
		.team2     { color: #4e608b; fill: #4e608b; stroke: #4e608b; }
		.team3     { color: #852d5f; fill: #852d5f; stroke: #852d5f; }
		.team4     { color: #136513; fill: #136513; stroke: #136513; }
		.team5     { color: #7e1818; fill: #7e1818; stroke: #7e1818; }
		.team6     { color: #7e4618; fill: #7e4618; stroke: #7e4618; }
		/* svg */
		text       { stroke-width: 0; }
		#icon_bat  { stroke: black; fill: white; opacity: .75; }
		.widgetBg  { fill:url(#gradBg); stroke:#2e8abb }
		stop.bg1   { stop-color: #80a2e0; stop-opacity: 1; }
		stop.bg2   { stop-color: #fff; stop-opacity: 1; }
		stop.wArr1 { stop-color: #2e8abb; stop-opacity: .5; }
		stop.wArr2 { stop-color: #fff; stop-opacity: .5; }
		
		/* Wind */
		/* svg */
		text.w_spd { font-size: 18px; fill: #cf696a; text-anchor: middle; }
		text.w_gst { font-size: 18px; fill: #cf696a; text-anchor: middle; }
		text.w_dir { font-size: 18px; fill: #cf696a; text-anchor: middle; }
		use.w_arr  { fill: url(#windGradArrow); stroke: #2e8abb; stroke-width: 1px; }
		
		/* Temperature */
		div.t_widget        { width: 150px; height: 75px; font-weight: bold; position: relative; display: inline-block; } 
		div.t_widget > *    { text-align: right; position: absolute; display: inline; }
		div.t_widget .title { font-size: 24px; font-weight: normal; text-align: center; top: 5px; left: 0px; width: 100%; }
		.t_cur { color: #666666; top: 30px; left:  5px;  width: 60px; font-size: 22px; }
		.t_max { color: #ff6060; top: 30px; left: 112px; width: 33px; }
		.t_min { color: #6060ff; top: 45px; left: 112px; width: 33px; }
		.t_cur::after { content: "°"; }
		.t_min::after { content: "°"; }
		.t_max::after { content: "°"; }
		.h_cur { color: #666666; top: 30px; left: 75px; text-align: left; font-size: 14px; }
		.h_min { color: #6060ff; top: 45px; left: 70px; font-size: smaller; }
		.h_max { color: #ff6060; top: 45px; left: 93px; text-align: left; font-size: small; }
		/* svg */
		polygon.t_graph { opacity: .25; }
		g.t_ruler       { stroke-width:1; opacity: .75; text-anchor: middle; font-size: 8px }
		
		/* Rain */
		/* svg */
		text.b_cur   { font-size: 18px; fill: green; text-anchor: middle; }
		text.r_cur   { font-size: 18px; fill: #5555cd; text-anchor: middle; }
		g.r_ruler    { opacity: .25; stroke:#0047e9; stroke-width:.5; } /* stroke-dasharray:3 3 */
		.r_ruler_txt { text-anchor: middle; font-size: 8px; fill: #5555cd; opacity: 1; }
		polygon.r_graph { stroke: none; opacity: .25; }
		
		/* Distance */
		div.d_widget        { width: 150px; height: 75px; font-weight: bold; position: relative; display: inline-block; } 
		div.d_widget > *    { position: absolute; display: inline; }
		div.d_widget .title { font-size: 24px; font-weight: normal; text-align: center; top: 5px; left: 0px; width: 100%; }
		.d_cur { color: #666666; top: 30px; text-align: center; left: 0px;  width: 100%; font-size: 22px; }
		polygon.d_graph { opacity: .25; }
		g.d_ruler       { stroke-width:1; opacity: .75; text-anchor: middle; font-size: 8px }
	</style>
	<script>
var tim1, tim2;
function doLoad() {
	loadSensor();
	tim1 = setInterval( function() { loadSensor(); }, 600000 );
	tim2 = setTimeout( function() {
		var yr = "http://www.yr.no/place/Sweden/V%C3%A4stra_G%C3%B6taland/Fritsla~2713656/avansert_meteogram.png";
		return setInterval( function() { 
			document.getElementById("yr").src = yr + "?r=" + Math.random() 
		}, 3600000 );
	}, ( 60 - (new Date()).getMinutes() ) * 60000 );
	
}
<?php readfile( 'sensors.js' ) ?>
window.onload  = doLoad;
window.onfocus = doLoad;
window.onblur  = function() { clearInterval(tim1); clearInterval(tim2); };
	</script>
</head>

<body><?php 
$width = 150; 
$height = 150; 
include( 'barometer.svg' );
?>
	<svg id="defsSVG" class="chart" width="<?= $width ?>" height="<?= $height ?>" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
		<defs>
			<linearGradient id="windGradArrow" x1="0%" y1="0%"   x2="0%" y2="100%">
				<stop offset="0%" class="wArr1" />
				<stop offset="100%" class="wArr2" />
			</linearGradient>
			<linearGradient id="gradBg" x1="0%" y1="100%" x2="100%" y2="0%">
				<stop offset="0%" class="bg1" />
				<stop offset="100%" class="bg2" />
			</linearGradient>
			<g id="icon_bat">
				<polygon points="0,20 0,2 2.5,2 2.5,0 7.5,0 7.5,2 10,2 10,20 0,20"/>
				<polygon points="2,18 2,15 8,13 8,18 2,18" style="fill:#b30000; stroke:none;"/>
			</g>
			<g id="svgBg">
 				<rect x="0" y="0" width="100%" height="100%" rx="10" ry="10" /> 
			</g>
			<g id="sArrow">
				<polygon points="0,-65 -25,-15 -5,-20 -20,65 0,60 20,65 5,-20 25,-15 0,-65" transform="rotate(0)" />
			</g>
		</defs>
		<use xlink:href="#svgBg" class="widgetBg" />
		<g id="rainSVG">
			<g class="r_graph"></g>
			<g class="r_ruler">
<?php
// Calculations based on sensors.js function drawRain
$dy = $height / 10;
$dh = $height - 10;
for ( $i = 1; $i <= 10; $i++ ) {
	echo "\t\t\t\t" . '<path d="M0 ' . intval( $dh - $i * $dy ) . ' L' . $width .' ' . intval( $dh - $i * $dy ) . '" '
		. ( $i%5 == 0 ? 'style="stroke-width: 1.3" 	' : '' ) . '/>' . "\n";
}
?>
			</g>
			<g class="r_ruler_txt"></g>
		</g>
		<use x="50%" y="50%" class="w_arr" xlink:href="#sArrow" transform="rotate(0 0,0)"/>
		<g id="baroSVG" style="opacity:.2"></g>
		<text x="50%" y="30" class="w_spd"></text>
		<text x="50%" y="50" class="w_dir"></text>
		<text x="50%" y="70" class="w_gst"></text>
		<text x="50%" y="100" class="r_cur"></text>
		<text x="50%" y="120" class="b_cur"></text>
		<use x="10" y="10" class="batt" xlink:href="#icon_bat" />
	</svg>
<?php
// rotate(60, 187, 187)
Sensor::draw_sensors();
?>

	<div id="aTime" onclick="loadSensor();" style="clear:both; display:block;">Fetching data</div>
	<a href="http://www.yr.no/place/Sweden/V%C3%A4stra_G%C3%B6taland/Fritsla~2713656/long.html" style="">
		<img src="http://www.yr.no/place/Sweden/V%C3%A4stra_G%C3%B6taland/Fritsla~2713656/avansert_meteogram.png" id="yr" />
	</a>
</body>
</html>
