<?php
/*	
	SVG:
			http://tutorials.jenkov.com/svg
			
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
			if ( $sensor->type & BAROMETER )
				$barometer = true;
			if ( $sensor->type & DISTANCE )
				$distance = true;
			if ( $sensor->type & LEVEL )
				$level = true;
		}
		$data = array();
		if ( $temp )
			Sensor::fetch_temperature( $sensors, $days );
		if ( $humid )
			Sensor::fetch_humidity( $sensors, $days );
		if ( $wind )
			$data = Sensor::fetch_wind( $data, $days );
		if ( $rain )
			$data = Sensor::fetch_rain( $data, $sensors, $days );
		if ( $barometer )
			$data = Sensor::fetch_barometer( $data, $days );
		if ( $distance )
			$data = Sensor::fetch_distance( $data, $days );
		if ( $level )
			Sensor::fetch_level( $sensors, 5 );
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

	static function fetch_temperature( &$sensors, $days = 1 ) {
		$sql = 'SELECT `sensor_id`, `value`, UNIX_TIMESTAMP( `time` ) AS `date` '
			. 'FROM  `wr_temperature` '
			. 'WHERE `time` > SUBDATE( NOW(), ' . $days . ' ) '
			. 'ORDER BY `sensor_id`, `time` ASC;';
		$res = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data' );
		$ids = array();
		while( $row = $res->fetch_object() ) {
			if ( array_key_exists( $row->sensor_id, $sensors ) ) {
				$ids[$row->sensor_id] = true;
				if ( isset( $sensors[$row->sensor_id]->temp ) )
					$sensors[$row->sensor_id]->temp[] = $row->date - $sensors[$row->sensor_id]->temp[0];
				else {
					$sensors[$row->sensor_id]->temp = array();
					$sensors[$row->sensor_id]->temp[] = $row->date;
				}
				$sensors[$row->sensor_id]->temp[] = $row->value;
			}
		}
		foreach( $ids as $k => $v )
			$sensors[$k]->temp = Sensor::simplify( $sensors[$k]->temp, 1000 );
	}
	static function fetch_humidity( &$sensors, $days = 1 ) {
		$sql = 'SELECT `sensor_id`, `value`, UNIX_TIMESTAMP( `time` ) AS `date` '
			. 'FROM  `wr_humidity` '
			. 'WHERE `time` > SUBDATE( NOW(), ' . $days . ' ) '
			. 'ORDER BY `sensor_id`, `time` ASC;';
		$res = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data' );
		$ids = array();
		while( $row = $res->fetch_object() ) {
			if ( array_key_exists( $row->sensor_id, $sensors ) ) {
				$ids[$row->sensor_id] = true;
				if ( isset( $sensors[$row->sensor_id]->hum ) )
					$sensors[$row->sensor_id]->hum[] = $row->date - $sensors[$row->sensor_id]->hum[0];
				else {
					$sensors[$row->sensor_id]->hum = array();
					$sensors[$row->sensor_id]->hum[] = $row->date;
				}
				$sensors[$row->sensor_id]->hum[] = $row->value;
			}
		}
		foreach( $ids as $k => $v )
			$sensors[$k]->hum = Sensor::simplify( $sensors[$k]->hum, 1000 );
	}
	static function fetch_barometer( $data, $days = 1 ) {
		$sql   = 'SELECT `sensor_id`, '
			. 'ROUND( AVG( `value` ), 1 ) AS `var`, ' 
			. 'DATE_FORMAT( `time`, "%m%d%H" ) AS `date` '
			. 'FROM  `wr_barometer` '
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
			$data[$id][$row->date]->b = floatVal( $row->var );
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
			$data[$id]['max']->b = $row->max;
			$data[$id]['min']->b = $row->min;
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
		return $data;
	}
	
	static function fetch_level( &$sensors, $days = 8 ) {
		$sql = 'SELECT `sensor_id`, `value`, UNIX_TIMESTAMP( `time` ) AS `date` '
			. 'FROM  `wr_level` '
			. 'WHERE `time` > SUBDATE( NOW(), ' . $days . ' ) '
			. 'ORDER BY `sensor_id`, `time` ASC;';
		$res = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data' );
		$ids = array();
		while( $row = $res->fetch_object() ) {
			if ( array_key_exists( $row->sensor_id, $sensors ) ) {
				$ids[$row->sensor_id] = true;
				if ( isset( $sensors[$row->sensor_id]->level ) )
					$sensors[$row->sensor_id]->level[] = $row->date - $sensors[$row->sensor_id]->level[0];
				else {
					$sensors[$row->sensor_id]->level = array();
					$sensors[$row->sensor_id]->level[] = $row->date;
				}
				$sensors[$row->sensor_id]->level[] = $row->value;
			}
		}
		foreach( $ids as $k => $v )
			$sensors[$k]->level = Sensor::simplify( $sensors[$k]->level, 50000 );
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
			. 'GROUP BY HOUR(`time`), `sensor_id` '
			. 'ORDER BY `date`, `sensor_id` ASC;';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data' );
		while( $row = $res->fetch_object() ) {
			$sensors[$row->date][$row->sensor_id] = intVal( $row->var );
		}
 		return '"distance":' . json_encode( $sensors, JSON_NUMERIC_CHECK );
	}
	
	static function json_level( $days = 5 ) {
		$days = ( is_numeric( $days ) ? $days : 5 );
		$sensors = array();
		$sql   = 'SELECT `sensor_id`, `value` AS `var`, UNIX_TIMESTAMP( `time` ) AS `date` '
			. 'FROM  `wr_level` '
			. 'WHERE `time` > SUBDATE( NOW(), ' . $days . ' ) '
			. 'ORDER BY `sensor_id`, `time` ASC;';
		$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data' );
		while( $row = $res->fetch_object() ) {
			if ( array_key_exists( $row->sensor_id, $sensors ) )
				$sensors[$row->sensor_id][] = $row->date - $sensors[$row->sensor_id][0];
			else
				$sensors[$row->sensor_id][] = $row->date;
			$sensors[$row->sensor_id][] = $row->var;
		}
		$array = array();
		foreach( $sensors as $k => $v ) {
			$object = new stdClass;
			$object->id   = $k;
			$object->level = Sensor::simplify( $v, 50000 );
			$array[] = $object;
		}
 		return json_encode( $array, JSON_NUMERIC_CHECK );
	}
	
	static function simplify( $points, $maxArea ) {
		if ( count( $points ) < 7 ) {
			return $points;
		}
		$time      = $points[0];
		$points[0] = 0;
		do {
			$idx = 1;
			$minArea = abs( $points[1] * ( $points[2] - $points[4] )
				+  $points[3] * $points[4] -  $points[5] * $points[4] ) / 2;
			for ( $i = 2; $i < count( $points ) - 4 ; $i += 2 ) {
				$area = abs( $points[$i-1] * ( $points[$i] - $points[$i+2] )
						+ $points[$i+1] * ( $points[$i+2] - $points[$i-2] )
						+ $points[$i+3] * ( $points[$i-2] - $points[$i] ) ) / 2;
				if ( $area < $minArea ) {
					$minArea = $area;
					$idx = $i;
				}
			}
			array_splice( $points, $idx, 2 );
		} while( $minArea < $maxArea && count( $points ) > 6 );
		$points[0] = $time;
		return $points;
	}
	
}

if ( isset( $_REQUEST ) ) {
	if ( isset( $_REQUEST['all'] ) ) {
		header( 'Content-Type: application/json' );
		echo '{"sensors":' . json_encode( Sensor::fetch_all( $_REQUEST['all'] ), JSON_NUMERIC_CHECK | JSON_PRETTY_PRINT )
			. ',"time":"' . date('Y-m-d H:i:s' ) . '"'
			. ',"Clock":['
// 			. '{"id":2,"time":"' . date('Y-m-d H:i:s', mktime( date('H') + 1 ) ) . '","title":"Helsinki"},'
			. '{"id":1,"time":"' . date('Y-m-d H:i:s' ) . '","title":"Stockholm"}'
			. ']}';
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
	} else if ( isset( $_REQUEST['level'] ) ) {
		header( 'Content-Type: application/json' );
		echo Sensor::json_level( $_REQUEST['level'] );
		exit;
	} else if ( isset( $_REQUEST['total'] ) ) {
		header( 'Content-Type: application/json' );
		echo  Sensor::json_sensors() . ',' 
			. Sensor::json_temperature( $_REQUEST['total'] ) . ',' 
			. Sensor::json_humidity( $_REQUEST['total'] ) . ',' 
			. Sensor::json_barometer( $_REQUEST['total'] ) . ',' 
			. Sensor::json_rain( $_REQUEST['total'] ) . ',' 
			. Sensor::json_wind( $_REQUEST['total'] ) . ',' 
			. Sensor::json_distance( $_REQUEST['total'] ) . ',' 
			. Sensor::json_level( $_REQUEST['total'] * 10 );
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
	<link rel="shortcut icon" href="../favicon.ico">
	<link rel="icon" href="../favicon.ico">

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
		g.r_ruler line { opacity: .25; stroke:#0047e9; stroke-width:.5; } /* stroke-dasharray:3 3 */
		g.r_ruler text { text-anchor: middle; font-size: 8px; fill: #5555cd; opacity: 1; }
		polygon.r_graph { stroke: none; opacity: .25; }
		
		/* Distance */
		div.d_cur { color: #666666; top: 30px; text-align: center; left: 0px;  width: 100%; font-size: 22px; }
		div.d_widget        { width: 150px; height: 75px; font-weight: bold; position: relative; display: inline-block; } 
		div.d_widget > *    { position: absolute; display: inline; }
		div.d_widget .title { font-size: 24px; font-weight: normal; text-align: center; top: 5px; left: 0px; width: 100%; }
		polygon.d_graph { opacity: .25; }
		g.d_ruler       { stroke-width:1; opacity: .75; text-anchor: middle; font-size: 8px }
		
		.dial_bg1  { fill:url(#lg_bg) }
		.dial_bg2  { fill:black;opacity:0.8;fill-opacity:0.7;stroke:black;stroke-width:8 }
		.dial_glar { fill:url(#lg_glare1);opacity:0.7 }
		.dial_tick { fill:none;stroke:#b3b3b3;stroke-width:3;stroke-linecap:round }
		.dial      { fill:#ff9900 }
		.baro_doff  { fill:#404040 }
		.baro_don   { fill:#808080 }
		.ico_sun    { fill:#f0c40f }
		.ico_lgh    { fill:#f0c40f }
		.ico_cloud  { fill:#94a4a6 }
		.ico_rain   { fill:#3497db }
		.ico_star   { fill:none;stroke:#b3b3b3;stroke-width:3;opacity:.5 }
		.baro_dial_dig  { font-size:28px;fill:#cccccc;text-anchor:middle; }
		.dial_val       { font-size:28px;fill:#cccccc;text-anchor:middle; }
		.ano_dial_dig   { font-size:45px;fill:#cccccc;text-anchor:middle; }
		.clock_dial_dig {  font-size:45px;fill:#cccccc;text-anchor:middle; }
		
	</style>
	<script>
var tim1, tim2;

function doLoad() {
	Sensor.load();
	tim1 = setInterval( function() { Sensor.load(); }, 600000 );
// 	tim2 = setTimeout( function() {
// 		var yr = "http://www.yr.no/place/Sweden/V%C3%A4stra_G%C3%B6taland/Fritsla~2713656/avansert_meteogram.png";
// 		return setInterval( function() { 
// 			document.getElementById("yr").src = yr + "?r=" + Date.now() 
// 		}, 3600000 );
// 	}, ( 60 - (new Date()).getMinutes() ) * 60000 );
}
<?php readfile( 'sensors.js' ) ?>
window.onload  = doLoad;
window.onfocus = doLoad;
window.onblur  = function() { clearInterval(tim1); clearInterval(tim2); };
	</script>
</head>

<body>
	<div id="aTime" onclick="Sensor.load();" style="clear:both; display:block;">Fetching data</div>
<?php 
$width  = 150; 
$height = 150; 
?>
	<svg id="defsSVG" width="0" height="0" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
		<defs>
			<linearGradient id="windGradArrow" x1="0%" y1="0%"   x2="0%" y2="100%">
				<stop offset="0%" class="wArr1" />
				<stop offset="100%" class="wArr2" />
			</linearGradient>
			<linearGradient id="gradBg" x1="0%" y1="100%" x2="100%" y2="0%">
				<stop offset="0%" class="bg1" />
				<stop offset="100%" class="bg2" />
			</linearGradient>
			
			<linearGradient id="lg_glare1" x1="10%" y1="10%" x2="60%" y2="100%">
				<stop offset="0" style="stop-color:#ffffff;stop-opacity:0.6"/>
				<stop offset="1" style="stop-opacity:0"/>
			</linearGradient>
			<linearGradient id="lg_bg" x1="50%" y1="133%" x2="50%" y2="-25%">
				<stop offset="0"/>
				<stop style="stop-color:#ffffff" offset="1"/>
			</linearGradient>
			<g id="dial_bg">
				<circle r="188" cy="188" cx="188" class="dial_bg1"/>
				<circle r="175" cx="188" cy="188" class="dial_bg2"/>
			</g>
			<path id="dial_glare" class="dial_glar" d="M187,22c-92,0 -165,74 -165,168c0,45 18,86 46,117c15,-124 117,-220 241,-220c3,0 6,1 10,1c-38,-39 -80,-65 -132,-66Z"/>
			<path id="dial_normal" class="dial" d="M188,185c-4,0 -5,6 0,6c4,0 4,-6 0,-6Zm1,-157l2,150c11,5 10,17 0,21l0,7l-4,3l-4,-3l0,-7c-9,-4 -10,-16 0,-21l2,-150l2,-2Z"/>
			<path id="dial_short" class="dial" d="M188,185c-4,0 -5,6 0,6c4,0 4,-6 0,-6Zm1,-131l2,124c11,5 10,17 0,21v8l-4,3l-4,-3v-8c-9,-4 -10,-16 0,-21l2,-124l2,-3Z"/>
			<path id="dial_thin" class="dial" d="M188,185c-4,0 -5,6 0,6c4,0 4,-6 0,-6Zm0,-157l1,149c16,3 12,22 -1,22c-13,0 -17,-19 -1,-22l0,-149l1,-2Z"/>
			<g id="dial_icon_star" class="ico_star">
				<path d="M151,173l-16,-40l38,19l15,-71l15,71l38,-19l-16,40l69,15l-69,15l16,40l-38,-19l-15,71l-15,-71l-38,19l16,-40l-69,-15Z"/>
				<path d="M144,164a18,18 0 1,1 -36,0a18,18 0 1,1 36,0Z" transform="matrix(1.571,0,0,1.604,-9.585,-74.933)"/>
			</g>
			<g id="dial_icon_weather">
				<path class="ico_cloud" d="M208,89c10,-5 4,-23 -10,-16c-5,-16 -24,-13 -27,0c-3,12 7,16 7,16Zm-31,5c0,0 -12,-5 -12,-19c0,-18 26,-25 35,-8c18,-3 23,20 9,27Z"/>
				<path class="ico_cloud" d="M119,113c-9,-17 -35,-10 -35,8c0,14 12,19 12,19l1,-5c0,0 -10,-4 -7,-17c3,-12 22,-15 27,0c14,-7 20,12 10,16l1,5c14,-7 9,-31 -9,-27Z"/>
				<path class="ico_rain" d="M118,134c0,0 7,9 0,9c-7,0 0,-9 0,-9Z"/>
				<path class="ico_rain" d="M104,135c0,0 7,9 0,9c-7,0 0,-9 0,-9Z"/>
				<path class="ico_rain" d="M111,145c0,0 7,9 0,9c-7,0 0,-9 0,-9Z"/>
				<path class="ico_cloud" d="M101,214c-9,-17 -35,-10 -35,8c0,14 12,19 12,19l1,-5c0,0 -10,-4 -7,-17c3,-12 22,-15 27,0c14,-7 20,11 10,16l-6,7c26,-3 17,-32 -1,-28Z"/>
				<path class="ico_lgh" d="M89,225h12l-7,11h8l-14,16l2,-12h-8Z"/>
				<path class="ico_cloud" d="M249,153c0,0 -12,-5 -11,-18c1,-18 26,-24 34,-8c17,-3 22,20 8,26Zm30,-5c10,-5 4,-22 -10,-16c-5,-15 -24,-12 -26,-1c-3,12 7,16 7,16Z"/>
				<path class="ico_sun" d="M270,103c0,-3 6,-2 6,0v6c0,3 -6,3 -6,0Zm22,28c-3,0 -2,-6 0,-6h6c3,0 3,6 0,6Zm-4,-14c-2,2 -6,-2 -4,-4l5,-5c2,-2 6,3 4,4Zm-31,0c2,2 6,-2 4,-4l-5,-5c-2,-2 -6,3 -4,4Zm29,22c1,3 1,5 0,7l2,2c2,2 6,-2 4,-4l-5,-5c0,0 -1,-1 -2,-1Zm-20,-16c2,-2 4,-3 6,-3c6,0 9,5 8,10l5,5c4,-8 -1,-20 -13,-20c-5,0 -9,2 -11,5Z"/>
				<path class="ico_sun" d="M262,216c2,2 6,-2 4,-4l-4,-4c-2,-2 -6,2 -4,4Zm32,32c2,2 6,-2 4,-4l-4,-4c-2,-2 -6,2 -4,4Zm0,-32c-2,2 -6,-2 -4,-4l4,-4c2,-2 6,2 4,4Zm-32,32c-2,2 -6,-2 -4,-4l4,-4c2,-2 6,2 4,4Zm35,-17c-3,0 -2,-6 0,-6h6c3,0 2,6 0,6Zm-45,0c-3,0 -2,-6 0,-6h6c3,0 2,6 0,6Zm23,16c0,-3 6,-2 6,0v6c0,3 -6,2 -6,0Zm0,-45c0,-3 6,-2 6,0v6c0,3 -6,2 -6,0Zm3,17c-11,0 -11,18 0,18c11,0 11,-18 0,-18Zm0,-5c19,0 19,28 0,28c-19,0 -19,-28 0,-28Z"/>
			</g>
			<g id="dial_dig_compass" class="ano_dial_dig">
				<path class="dial_tick" d="M120,347l2,-5m131,-318l-2,5m97,223l-5,-2m-318,-131l5,2m34,188l4,-4m243,-244l-4,4m4,243l-4,-4m-244,-243l4,4m-41,187l5,-2m317,-133l-5,2m-89,226l-2,-5m-133,-317l2,5m-109,157h5m344,-1h-5m-169,175v-5m-1,-344v5"/>
				<text style="fill:#ff2a2a" y="65" x="188">N</text>
				<text x="188" y="344">S</text>
				<text x="53" y="205">W</text>
				<text x="329" y="205">E</text>
			</g>
			<g id="dial_dig_baro" class="baro_dial_dig">
				<path class="dial_tick" d="M206,14l-1,5m19,-3l-1,5m19,-1l-2,5m19,1l-2,5m-87,-19l1,5m-19,-2l1,5m-19,-1l2,5m-19,1l2,5m-34,13l3,4m203,278l-3,-4m-241,-34l4,-3m278,-203l-4,3m-310,119l5,-1m342,-37l-5,1m-328,-52l5,2m315,139l-5,-2m-272,-198l4,4m231,255l-4,-4m-243,-8l4,-4m255,-231l-4,4m-296,150l5,-1m336,-72l-5,1m-332,-18l5,2m327,105l-5,-2m-291,-168l4,4m256,229l-4,-4m-243,17l4,-4m229,-256l-4,4m-279,181l5,-2m327,-107l-5,2m-332,17l5,1m337,71l-5,-1m-307,-137l4,3m279,201l-4,-3m-240,43l3,-4m201,-279l-3,4m-259,209l5,-2m314,-141l-5,2m-328,52l5,1m342,35l-5,-1m-256,-168l3,5m173,297l-3,-5m-237,-233l5,3m298,171l-5,-3m-234,67l3,-5m171,-298l-3,5m-235,235l5,-3m297,-173l-5,3m-147,-90v5m-174,170h5m344,-1h-5"/>
				<text x="188" y="42">1010</text>
				<text y="-401" x="-68" transform="matrix(-0.866,0.5,-0.5,-0.866,0,0)">1060</text>
				<text y="-214" x="-255" transform="matrix(-0.866,-0.5,0.5,-0.866,0,0)">960</text>
				<text y="111" x="70" transform="matrix(0.866,-0.5,0.5,0.866,0,0)">1000</text>
				<text transform="matrix(0.5,-0.866,0.866,0.5,0,0)" x="-68" y="111">990</text>
				<text y="42" x="-186" transform="matrix(0,-1,1,0,0,0)">980</text>
				<text transform="matrix(-0.5,-0.866,0.866,-0.5,0,0)" x="-255" y="-77">970</text>
				<text transform="matrix(-0.5,0.866,-0.866,-0.5,0,0)" x="70" y="-401">1050</text>
				<text y="-333" x="188" transform="matrix(0,1,-1,0,0,0)">1040</text>
				<text transform="matrix(0.5,0.866,-0.866,0.5,0,0)" x="257" y="-214">1030</text>
				<text y="-77" x="257" transform="matrix(0.866,0.5,-0.5,0.866,0,0)">1020</text>
			</g>
			<g id="dial_dig_clock" class="clock_dial_dig">
				<path class="dial_tick" d="M35,98l4,2m298,171l-4,-3m-60,-236l-2,4m-171,298l3,-4m-3,-298l2,4m173,297l-2,-4m66,-234l-4,2m-297,173l4,-2m146,-260v5m1,344v-5m174,-170h-5m-344,1h5"/>
				<text x="188" y="65">12</text>
				<text x="188" y="344">6</text>
				<text x="53" y="205">9</text>
				<text x="329" y="205">3</text>
			</g>
			
			<g id="icon_bat">
				<polygon points="0,20 0,2 2.5,2 2.5,0 7.5,0 7.5,2 10,2 10,20 0,20"/>
				<polygon points="2,18 2,15 8,13 8,18 2,18" style="fill:#b30000; stroke:none;"/>
			</g>
			<g id="svgBg">
 				<rect x="0" y="0" width="100%" height="100%" rx="10" ry="10" /> 
			</g>
		</defs>
	</svg>
<!--	<a href="http://www.yr.no/place/Sweden/V%C3%A4stra_G%C3%B6taland/Fritsla~2713656/long.html" style="">
		<img src="http://www.yr.no/place/Sweden/V%C3%A4stra_G%C3%B6taland/Fritsla~2713656/avansert_meteogram.png" id="yr" />
	</a>-->
</body>
</html>
