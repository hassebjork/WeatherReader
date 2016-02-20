<?php
include_once( 'common.php' );

$time  = mktime(0, 0, 0, 2, 15, 2016 );
$data  = fetch_data( 'wr_test', 51, $time, mktime() );

$k1    = kalman( $data[0]->y, .001, 10 );
$k2    = kalman( $data[0]->y, .005, 20 );
$k3    = kalman( $data[0]->y, .005, 10 );
$k4    = kalman( $data[0]->y, .008, 40 );

$kg1   = kalman_graph( $data, $k1 );
$kg2   = kalman_graph( $data, $k2 );
$kg3   = kalman_graph( $data, $k3 );
$kg4   = kalman_graph( $data, $k4 );

$graph = graph( array( $data, $kg1, $kg2, $kg3, $kg4 ) );

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
</head>

<body>
	<svg id="svgImg" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="100%" height="500" viewBox="0 0 1000 500">
		<defs></defs>
		<rect x="0" y="0" width="100%" height="100%" style="stroke:#000000; fill:#ffe0e0" onmousemove="moveMouse(evt)"/>
		<line x1="0" x2="0" y1="0" y2="500" id="ruler" style="stroke:#000000;stroke-width:1" />
		<text x="5" y="15" style="fill:#ff0000;fill-opacity:.5;opacity:1" onclick="toggle('kal0', this)">Original data</text>
		<text x="5" y="30" style="fill:#ff00ff" onclick="toggle('kal1', this)">Kalman filter 1</text>
		<text x="5" y="45" style="fill:#0000ff" onclick="toggle('kal2', this)">Kalman filter 2</text>
		<text x="5" y="60" style="fill:#606060" onclick="toggle('kal3', this)">Kalman filter 3</text>
		<text x="5" y="75" style="fill:#00ff00" onclick="toggle('kal4', this)">Kalman filter 4</text>
		<polyline id="kal0" points="<?= $graph[0] ?>" style="stroke:#ff0000;stroke-width:1;stroke-opacity:.5;fill:none;opacity:1"/>
		<polyline id="kal1" points="<?= $graph[1] ?>" style="stroke:#ff00ff;stroke-width:1;stroke-opacity:.75;fill:none;opacity:1"/>
		<polyline id="kal2" points="<?= $graph[2] ?>" style="stroke:#0000ff;stroke-width:1;stroke-opacity:.75;fill:none;opacity:1"/>
		<polyline id="kal3" points="<?= $graph[3] ?>" style="stroke:#606060;stroke-width:1;stroke-opacity:.75;fill:none;opacity:1"/>
		<polyline id="kal4" points="<?= $graph[4] ?>" style="stroke:#00ff00;stroke-width:1;stroke-opacity:.75;fill:none;opacity:1"/>
		<text x="5" y="480" style="fill:#404040"><?= date( 'Y-m-d', $data[0]->x ) ?></text>
		<text x="5" y="495" style="fill:#404040"><?= date( 'H:i:s', $data[0]->x ) ?></text>
		<text x="995" y="480" style="fill:#404040;text-anchor:end"><?= date( 'Y-m-d', $data[0]->x ) ?></text>
		<text x="995" y="495" style="fill:#404040;text-anchor:end"><?= date( 'H:i:s', $data[count($data)-1]->x ) ?></text>
		<text x="500" y="495" id="mm" style="fill:#404040:text-anchor:middle"></text>
		<script>
			var m_move = document.getElementById("mm");
			var ruler  = document.getElementById("ruler");
			function toggle( id, src ) {
				var obj = document.getElementById(id);
				if ( obj.style.opacity == 1 ) {
					obj.style.opacity = 0;
					src.style.opacity = 0.5;
				} else {
					obj.style.opacity = 1;
					src.style.opacity = 1;
				}
			}
			function moveMouse(evt){
				var x = ( evt.pageX - evt.currentTarget.getBoundingClientRect().left );
				m_move.textContent = x;
				ruler.setAttribute( "x1", x );
				ruler.setAttribute( "x2", x );
			}
		</script>
	</svg>
</body>
</html>
