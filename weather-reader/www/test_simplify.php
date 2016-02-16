<?php
include_once( 'common.php' );

function &whyatt2( $points, $maxArea ) {
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
				$points[$segment + 1]
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

if ( isset( $_REQUEST['time'] ) )
	$time = intval( $_REQUEST['time'] );
else
	$time   = mktime(0, 0, 0, date('n'), date('j'), date('Y') );

$data   = fetch_data( 'wr_temperature', 48, $time - 86400, $time );

$k2     = kalman2( $data[0]->y, .01, 0.2 );
$kg2    = kalman2_graph( $data, $k2 );

$point1 = array();
// $point1 = whyatt( $kg2, (int) count($kg2) / 20 );
$point1 = whyatt2( $kg2, 35 );

$graph  = graph( array( $data, $point1, $kg2 ) );

$dl = count( $data ) - 1;
$pl = count( $point1 ) - 1;

?>
<!DOCTYPE html>
<html>
<head>
	<meta charset="UTF-8"/>
	<meta name="viewport" content="width=device-width, user-scalable=yes" />
	
	<title>Test temperature</title>
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
		<polyline id="orig" points="<?= $graph[0] ?>" style="stroke:#ff0000;stroke-width:1;stroke-opacity:.25;fill:none"/>
		<polyline id="wyat" points="<?= $graph[1] ?>" style="stroke:#0000ff;stroke-width:1;fill:none"/>
		<polyline id="kal2" points="<?= $graph[2] ?>" style="stroke:#ff00ff;stroke-width:1;fill:none"/>
	</svg>
	
	<div>
		<?= date( 'Y-m-d H:i:s', $data[0]->x ) . ' and ' . date( 'Y-m-d H:i:s', $data[$dl]->x ) ?><br/>
		Points: <?= $dl . ( $pl > 2 ? ' (' . $pl . ')' : '' ) ?>
	</div>
	<a href="?time=<?= $time - 86400 ?>">Prev</a>
	<a href="?time=<?= mktime(0, 0, 0, date('n'), date('j'), date('Y') ) ?>">Now</a>
	<a href="?time=<?= $time + 86400 ?>">Next</a>
</body>
</html>
