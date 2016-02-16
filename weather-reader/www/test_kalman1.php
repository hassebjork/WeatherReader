<?php
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

$data  = array();

$sql   = 'SELECT *, UNIX_TIMESTAMP(`time`) AS unix '
	. 'FROM  `wr_test` '
	. 'WHERE `time` > "2016-02-15 00:00:00" '
	. 'ORDER BY `time` ASC;';
$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensor data: ' . $sql );

$value_max = -999;
$value_min = PHP_INT_MAX;
$unix_max = -999;
$unix_min = PHP_INT_MAX;

while( $row = $res->fetch_object() ) {
	$data[] = $row;
	if ( $row->value > $value_max ) $value_max = $row->value;
	if ( $row->value < $value_min ) $value_min = $row->value;
	if ( $row->unix  > $unix_max )  $unix_max  = $row->unix;
	if ( $row->unix  < $unix_min )  $unix_min  = $row->unix;
}

// http://home.wlu.edu/~levys/kalman_tutorial/
function &kalman( $initialValue, $slope, $noise ) {
	$k = new stdClass;
	$k->a = $slope;			// Slope of the curve for next prediction
	$k->r = $noise;			// Sensor noise
	$k->x = $initialValue;	// Initial value
	$k->p = 1.0;			// Prediction error
	$k->k = 0.0;			// Kalman gain
	return $k;
}

function kalman_filter( &$data, $value ) {
	// Predict
	$data->x = $data->x * $data->a;
	$data->p = $data->a * $data->p * $data->a;
	
	// Update
	$data->k = $data->p == 0 ? 1 : $data->p / ($data->p + $data->r);
	$data->x = $data->x + $data->k * ($value - $data->x);
	$data->p = (1.0 - $data->k) * $data->p;
	
	return $data->x;
}

function &kalman2( $initValue, $process, $noise ) {
	// http://www.magesblog.com/2014/12/measuring-temperature-with-my-arduino.html
	$k = new stdClass;
	$k->a = $process;		// Process variance
	$k->r = $noise;			// Measurement uncertainty
	$k->x = $initValue;		// Initial value
	$k->p = 1.0;			// Prediction error
	$k->k = 0.0;			// Kalman gain
	return $k;
	// varM // Measurement uncertainty
	// varP // Process variance
}

function kalman_filter2( &$data, $value ) {
	// http://www.magesblog.com/2014/12/measuring-temperature-with-my-arduino.html
	// Predict
	$data->p = $data->p + $k->a;
	
	// Update
	$data->k = $data->p / ($data->p + $k->r);    
	$data->x = $data->x + $data->k * ($value - $data->x);
	$data->p = (1 - $data->k) * $data->p;
}

header( 'Content-Type: text/html; charset=UTF-8' );
?>
<!DOCTYPE html>
<html>
<head>
	<meta charset="UTF-8"/>
	<meta name="viewport" content="width=device-width, user-scalable=yes" />
	
	<title>Kalman Test</title>
	<link rel="shortcut icon" href="../icon.ico">
	<link rel="icon" href="../icon.ico">

	<style type="text/css" media="all">
	</style>
	<script>
	</script>
</head>

<body>
	<svg id="svgImg" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="100%" height="500" viewBox="0 0 1000 500">
		<defs></defs>
		<rect x="0" y="0" width="100%" height="100%" style="stroke:#000000; fill:#ffe0e0"/>
<?php
$length = count($data) - 1;
$a = 1.00019;
$r = 15;

$height = 500;
$width  = 1000;

$sv = $value_max - $value_min;
$su = $unix_max  - $unix_min;

$dh = $height * .05;
$dw = $width  * .05;

$dy = $height / $sv * .9;
$dx = $width  / $su;

$w      = $data[0]->value;
$kalman = kalman( $data[0]->value, $a, $r );
$k2     = kalman2( $data[0]->value, .01, .5 );
$max_noise = 0;
$min_noise = PHP_INT_MAX;
$_orig  = '';
$_kalm  = '';
$_slop  = '';
$_k = '';
$_z = '';
$_x = '';
$_w = '';
$_p = '';
$_g = '';

for( $i = 0; $i < $length; $i++ ) {
	$w *= $a;
	$x  = (int) ( ( $data[$i]->unix - $unix_min ) * $dx );
	$k  = kalman_filter( $kalman, $data[$i]->value );
	$_orig .= $x . ',' . (int) ( ( $data[$i]->value - $value_min ) * $dy + $dh )  . ' ';
	$_slop .= $x . ',' . (int) ( ( $w - $value_min ) * $dy + $dh )  . ' ';
	$_kalm .= $x . ',' . (int) ( ( $k - $value_min ) * $dy + $dh )  . ' ';
	$noise = $w - $data[$i]->value;
	$max_noise = ( $noise > $max_noise ? $noise : $max_noise );
	$min_noise = ( $noise < $min_noise ? $noise : $min_noise );
	
	$_k .= '<td>' . $i . '</td>';
	$_z .= '<td>' . $data[$i]->value . '</td>';
	$_x .= '<td>' . round( $k, 1 ) . '</td>';
	$_w .= '<td>' . round( $w, 1 ) . '</td>';
	$_p .= '<td>' . round( $kalman->p, 2 ) . '</td>';
	$_g .= '<td>' . round( $kalman->k, 2 ) . '</td>';
}

?>
		<polyline id="orig" points="<?= $_orig ?>" style="stroke:#ff0000;stroke-width:3;fill:none"/>
		<polyline id="slop" points="<?= $_slop ?>" style="stroke:#00ff00;stroke-width:2;fill:none"/>
		<polyline id="kalm" points="<?= $_kalm ?>" style="stroke:#0000ff;stroke-width:2;fill:none"/>
		<polyline id="strg" points="0,<?= (int)( ( $data[0]->value - $value_min ) * $dy + $dh ) ?> 1000,<?= (int)( ( $data[$length]->value - $value_min ) * $dy + $dh ) ?>" style="stroke:#606060;stroke-width:1;fill:none"/>
	</svg>
	
	<table>
		<tr>
			<th>start/end</th><td><?= $data[0]->value ?>-<?= $data[$length]->value ?></td>
			<th>values span</th><td><?= $value_min ?>-<?= $value_max ?></td>
			<th>values No</th><td><?= $length ?></td>
		</tr><tr>
			<th>set slope</th><td><?= $a ?></td>
			<th>set noise</th><td><?= $r ?></td>
			<th>noise range</th><td><?= round( $max_noise, 1 ) ?><?= round( $min_noise, 1 ) ?></td>
		</tr>
	</table>
	<div style="overflow-x:scroll;">
	<table border="1">
		<tr><td>Iteration</td><?= $_k ?></tr>
		<tr><td>Measured</td><?= $_z ?></tr>
		<tr><td>Estimate</td><?= $_x ?></tr>
		<tr><td>Linear</td><?= $_w ?></tr>
		<tr><td>Error</td><?= $_p ?></tr>
		<tr><td>Gain</td><?= $_g ?></tr>
	</table>
	</div>
</body>
</html>
