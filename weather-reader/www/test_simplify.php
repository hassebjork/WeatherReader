<?php
include_once( 'common.php' );

if ( isset( $_REQUEST['time'] ) )
	$time = intval( $_REQUEST['time'] );
else
	$time   = mktime(0, 0, 0, date('n'), date('j'), date('Y') );

$tomorrow  = $time + 86400;
$yesterday = $time - 86400;
$today     = mktime(0, 0, 0, date('n'), date('j'), date('Y') );
	
$data   = fetch_data( 'wr_temperature', 48, $time - 86400, $time );

// $k2     = kalman( $data[0]->y, .5, 2 );	// Barometer
$k2     = kalman( $data[0]->y, .01, 0.2 );	// Temperature
// $k2     = kalman( $data[0]->y, 0.05, 2 );		// Humidity
$kg2    = kalman_graph( $data, $k2 );

$point1 = array();
// $point1 = whyatt( $kg2, (int) count($kg2) / 20 );
$point1 = whyatt_qulity( $kg2, 250 );			// Temperature
// $point1 = whyatt_qulity( $kg2, 500 );			// Humidity

$graph  = graph( array( $data, $point1, $kg2 ) );

$dl = count( $data ) - 1;
$pl = count( $point1 ) - 1;

?>
<!DOCTYPE html>
<html>
<head>
	<meta charset="UTF-8"/>
	<meta name="viewport" content="width=device-width, user-scalable=yes" />
	
	<title>Temperature test</title>
	<link rel="shortcut icon" href="../icon.ico">
	<link rel="icon" href="../icon.ico">

	<style type="text/css" media="all">
	</style>
	<script>
	</script>
</head>

<body>
	<div>
		<a href="?time=<?= $yesterday ?>">Prev</a>
		<a href="?time=<?= $today ?>">Now</a>
		<?= $today < $tomorrow ? 'Next' : '<a href="?time=' . $tomorrow .'">Next</a>' ?>
	</div>
	
	<svg id="svgImg" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="100%" height="500" viewBox="0 0 1000 500">
		<defs></defs>
		<rect x="0" y="0" width="100%" height="100%" style="stroke:#000000; fill:#ffe0e0"/>
		<text x="5" y="15" style="fill:#ff0000;opacity:.5">Original data (<?= $dl ?> pt)</text>
		<text x="5" y="30" style="fill:#ff00ff">Kalman filter (<?= $dl ?> pt)</text>
		<text x="5" y="45" style="fill:#0000ff">Visvalingam-Whyatt (<?= $pl ?> pt)</text>
		<polyline id="orig" points="<?= $graph[0] ?>" style="stroke:#ff0000;stroke-width:1;stroke-opacity:.25;fill:none"/>
		<polyline id="wyat" points="<?= $graph[1] ?>" style="stroke:#0000ff;stroke-width:1;fill:none"/>
		<polyline id="kal2" points="<?= $graph[2] ?>" style="stroke:#ff00ff;stroke-width:1;fill:none"/>
		<text x="5" y="480" style="fill:#404040"><?= date( 'Y-m-d', $data[0]->x ) ?></text>
		<text x="5" y="495" style="fill:#404040"><?= date( 'H:i:s', $data[0]->x ) ?></text>
		<text x="995" y="495" style="fill:#404040;text-anchor:end"><?= date( 'H:i:s', $data[$dl]->x ) ?></text>
	</svg>
	
</body>
</html>
