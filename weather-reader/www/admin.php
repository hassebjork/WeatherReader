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
define( 'SWITCH', 64 );
define( 'BAROMETER', 128 );
define( 'DISTANCE', 256 );
define( 'LEVEL', 512 );
define( 'TEST', 1024 );

$mysqli = new mysqli( DB_HOST, DB_USER, DB_PASS, DB_DATABASE );
$mysqli->set_charset( 'utf8' );

$types = array(
	1 		=> 'temperature',
	2 		=> 'humidity',
	4		=> 'windaverage',
	8		=> 'windgust',
	16		=> 'winddirection',
	32		=> 'raintotal',
	64		=> 'switch',
	128		=> 'barometer',
	256 	=> 'distance',
	512 	=> 'level',
	1024	=> 'test',
);
$tables = array(
	1 		=> 'wr_temperature',
	2 		=> 'wr_humidity',
	4		=> 'wr_wind',
// 	8		=> 'wr_wind',
// 	16		=> 'wr_wind',
	32		=> 'wr_rain',
	64		=> 'wr_switch',
	128		=> 'wr_barometer',
	256 	=> 'wr_distance',
	512 	=> 'wr_level',
	1024	=> 'wr_test',
);

$sensors = get_sensors();

function remove_sensor() {
	$sensor = get_sensor( $_REQUEST['id'] );
	if ( !$sensor ) return;
	
	foreach( $GLOBALS['tables'] as $k => $t ) {
		$sql = 'DELETE FROM `' . $t . '` WHERE `sensor_id` = ' . $sensor->id;
		$res = $GLOBALS['mysqli']->query( $sql );
		if ( $res === TRUE )
			echo 'Table <i>' . $t . '</i>: Emptied<br />' . "\n";
		else if ( $res === FALSE )
			echo 'Failed to delete <i>' . $t . '</i><br />' . "\n";
	}
	$sql = 'DELETE FROM `wr_sensors` WHERE `id` = ' . $sensor->id;
	$res = $GLOBALS['mysqli']->query( $sql );
	if ( $res === TRUE )
		echo 'Sensor <i>' . $sensor->name . '</i> removed<br />' . "\n";
	else if ( $res === FALSE )
		echo 'Failed to remove <i>' . $sensor->name . '</i><br />' . "\n";
}

function remove_sensor_data() {
	$sensor = get_sensor( $_REQUEST['id'] );
	if ( !$sensor ) return;
	
	foreach( $GLOBALS['tables'] as $k => $t ) {
		if ( $sensor->type & $k ) {
			echo 'Skipping table: <i>' . $t . '</i><br />' . "\n";
		} else {
			$sql = 'DELETE FROM `' . $t . '` WHERE `sensor_id` = ' . $sensor->id;
			$res = $GLOBALS['mysqli']->query( $sql );
			if ( $res === TRUE )
				echo 'Table <i>' . $t . '</i>: Emptied<br />' . "\n";
			else if ( $res === FALSE )
				echo 'Failed to delete <i>' . $t . '</i><br />' . "\n";
			else
				echo 'Table: <i>' . $t . '</i> deleting ' . $res->affected_rows . ' rows<br />' . "\n";
		}
	}
}

function get_sensors() {
	$sensors = array();
	$sql   = 'SELECT * '
			.'FROM `wr_sensors` ';
	$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensors' );
	while( $row = $res->fetch_object() ) {
		$sensors[$row->id] = $row;
	}
	return $sensors;
}

function get_sensor( $id ) {
	$sql   = 'SELECT * FROM `wr_sensors` WHERE `id` = ' . $id;
	$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensors' );
	return $res->fetch_object();
}

function table_sensors( $name ) {
	$sensors = array();
	$sql   = 'SELECT DISTINCT `sensor_id` FROM `' . $name . '` ORDER BY `sensor_id`';
	$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensors' );
	while( $row = $res->fetch_row() ) {
		$sensors[] = $row[0];
	}
	return $sensors;
}

function remove_table_data() {
	$table  = $GLOBALS['tables'][$_REQUEST['tbl']];
	$id  = $_REQUEST['id'];
	$sql = 'DELETE FROM `' . $table . '` WHERE `sensor_id` = ' . $id;
	
	$res = $GLOBALS['mysqli']->query( $sql );
	if ( $res === TRUE )
		echo 'Removed sensor ' . $id . ' from <i>' . $table . '</i><br />' . "\n";
	else if ( $res === FALSE )
		echo 'Failed to removed sensor ' . $id . ' from <i>' . $table . '</i><br />' . "\n";
}

if ( isset( $_REQUEST['do'] ) ) {
	switch( $_REQUEST['do'] ) {
		case 'rsd':
			remove_sensor_data();
			break;
		case 'del':
			remove_sensor();
			break;
		case 'rtd':
			remove_table_data();
			break;
		default:
			break;
	}
}

header( 'Content-Type: text/html; charset=UTF-8' );
?>
<!DOCTYPE html>
<html>
<head>
	<meta charset="UTF-8"/>
	<meta name="viewport" content="width=device-width, user-scalable=yes" />
	
	<title>Administration</title>
	<link rel="shortcut icon" href="icon.ico">
	<link rel="icon" href="icon.ico">

	<style type="text/css" media="all">
		.odd { background-color: #ffe0e0 }
	</style>
	<script>
	</script>
</head>
<body>
	<div>
		<a href="admin.php">Home</a>
	</div>

	<table>
		<thead>
			<th>ID</th>
			<th>Name</th>
			<th>Team</th>
			<th>Type</th>
		</thead>
		<tbody>
<?php
$eo = 0;
foreach( $sensors as $s ) {
	echo "\t\t\t" . '<tr class="' . ( $eo++ % 2 == 1 ? 'odd' : '' ) . '">'
		. '<td>' . $s->id . '</td>'
		. '<td>' . $s->name . '</td>'
		. '<td>' . $s->team . '</td>'
		. '<td>';
	foreach ( $types as $k => $v )
		if ( $s->type & $k ) echo $v . ' ';
	echo '</td>'
		. '<td><a href="admin.php?do=del&id=' . $s->id . '">Delete</a> </td>'
		. '<td><a href="admin.php?do=rsd&id=' . $s->id . '">data</a> </td>'
		. '</tr>' . "\n";
}
?>
		</tbody>
	</table>
	
	<p></p>
	
	<table>
		<thead>
			<th>Name</th>
			<th>Sensors</th>
		</thead>
		<tbody>
<?php
foreach( $tables as $k => $t ) {
	echo "\t\t\t" . '<tr class="' . ( $eo++ % 2 == 1 ? 'odd' : '' ) . '">'
		. '<td>' . $t . '</td>'
		. '<td>';
	foreach ( table_sensors( $t ) as $sn ) {
		if ( isset( $sensors[$sn] ) ) {
			if ( $sensors[$sn]->type & $k )
				echo $sn . ' ';
			else
				echo '<a href="admin.php?do=rtd&tbl=' . $k . '&id=' . $sn . '">' . $sn . '</a> ';
		} else {
			echo '<a href="admin.php?do=rtd&tbl=' . $k . '&id=' . $sn . '">' . $sn . '</a> ';
		}
	}
	echo '</td>'
		. '</tr>' . "\n";
}
?>
		</tbody>
	</table>
</body>
</html>
