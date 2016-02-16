<?php
include_once( 'common.php' );

function &slope_graph( &$data, &$a ) {
	$w = $data[0]->y;
	$point = array();
	foreach( $data as $row ) {
		$w *= $a;
		$point[] = new Point( $row->x, $w ); 
	}
	return $point;
}


$a      = 1.00019;
$time   = mktime(0, 0, 0, 2, 15, 2016 );
$data   = fetch_data( 'wr_test', 51, $time, mktime() );
$dl     = count( $data ) - 1;

$k1     = kalman( $data[0]->y, $a, 15 );
$kg1    = kalman_graph( $data, $k1 );

$k2     = kalman2( $data[0]->y, .0005, 10 );
$kg2    = kalman2_graph( $data, $k2 );
// $kg2    = array();

$sgrap  = slope_graph( $data, $a );
$graph  = graph( array( $data, $kg1, $kg2, array( $data[0], $data[$dl] ), $sgrap ) );

?>
<!DOCTYPE html>
<html>
<head>
	<meta charset="UTF-8"/>
	<meta name="viewport" content="width=device-width, user-scalable=yes" />
	
	<title>Kalman test</title>
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
		<polyline id="orig" points="<?= $graph[0] ?>" style="stroke:#ff0000;stroke-width:1;fill:none"/>
		<polyline id="kal1" points="<?= $graph[1] ?>" style="stroke:#0000ff;stroke-width:2;fill:none"/>
		<polyline id="kal2" points="<?= $graph[2] ?>" style="stroke:#ff00ff;stroke-width:1;fill:none"/>
		<polyline id="strg" points="<?= $graph[3] ?>" style="stroke:#606060;stroke-width:1;fill:none"/>
		<polyline id="slop" points="<?= $graph[4] ?>" style="stroke:#00ff00;stroke-width:1;fill:none"/>
	</svg>
	<?= date( 'Y-m-d H:i:s', $data[0]->x ) . ' and ' . date( 'Y-m-d H:i:s', $data[$dl]->x ) ?>
</body>
</html>
