
var teams;

function loadSensor( url ) {
	var req = new XMLHttpRequest();
	req.onreadystatechange = function() {
		if ( req.readyState == 4 ) {
			var obj = JSON.parse( req.responseText );
			if ( typeof obj.sensors !== 'undefined' )
				updateSensors( obj.sensors );
		}
	}
	req.open( "GET", url, true );
	req.send();
}
function updateSensors( sensors ) {
	var sens, head, h, m, d = new Date();
	h = d.getHours();
	m = d.getMinutes();
	$i("aTime").innerHTML = ( h < 10 ? "0" : "" ) + h + ":" + ( m < 10 ? "0" : "" ) + m;
	for ( sens in sensors ) {
		if ( sensors[sens].type & 1 )	// Temp
			drawTemp( sensors[sens] );
		if ( sensors[sens].type & 2 )	// Humid
			drawHumidity( sensors[sens] );
		if ( sensors[sens].type & 16 )	// Wind
			drawWind( sensors[sens] );
		if ( sensors[sens].type & 32 )	// Rain
			drawRain( sensors[sens] );
	}
}
function drawTemp( sensor ) {
	var i, t, dh, dx, dy, path = "", node;
	node = $i("sTemp" + sensor.id);
	$c(node,"batt").style.visibility = ( sensor.bat == 0 ? "visible" : "hidden" );
	$c(node,"title").textContent = sensor.name;
	for ( i = sensor.data.length - 1; i >= 0; i-- ) {
		if ( typeof sensor.data[i].t !== "undefined" ) {
			$c(node,"t_cur").textContent = sensor.data[i].t + "°C";
			break;
		}
	}
	if ( typeof sensor.min.t !== "undefined" )
		$c(node,"t_min").textContent = sensor.min.t + "°C";
	if ( typeof sensor.max.t !== "undefined" )
		$c(node,"t_max").textContent = sensor.max.t + "°C";
	if ( sensor.data.length < 2 )	// Division by 0
		return;
	t  = sensor.max.t - sensor.min.t;
	dh = node.clientHeight * .9;
	dx = node.clientWidth / ( sensor.data.length - 1 );
	dy = ( t == 0 ? 1 : node.clientHeight / t * .8 );
 	console.log( sensor.id + ": " + "[min: " + sensor.min.t + " max: " + sensor.max.t + " t: " + t + " dh:" + dh + " dx:" + dx + " dy:" + dy + "] " );
	for ( i = 0; i < sensor.data.length; i++ ) {
		if ( typeof sensor.data[i].t !== "undefined" ) {
			t = Math.round( dh - ( sensor.data[i].t - sensor.min.t ) * dy );
			path += Math.round(i*dx) + "," + t + " ";
		}
	}
	path += node.clientWidth + "," + node.clientHeight + " " + "0," + node.clientHeight;
	$c(node,"t_graph").setAttribute("points", path);
}
function drawHumidity( sensor, node ) {
	var i, node;
	node = $i("sTemp" + sensor.id);
	$c(node,"batt").style.visibility = ( sensor.bat == 0 ? "visible" : "hidden" );
	$c(node,"title").textContent = sensor.name;
	$c(node,"humi").style.visibility = "visible";
	if ( typeof sensor.max.h !== "undefined" )
		$c(node,"h_max").textContent = sensor.max.h + " %";
	if ( typeof sensor.min.h !== "undefined" )
		$c(node,"h_min").textContent = sensor.min.h + " %";
	for ( i = sensor.data.length - 1; i >= 0; i-- ) {
		if ( typeof sensor.data[i].h !== "undefined" ) {
			$c(node,"h_cur").textContent = sensor.data[i].h + " %";
			break;
		}
	}
}
function drawWind( sensor ) {
	var i, wind, node, a;
	node = $i("sWind" + sensor.id);
	for ( i = sensor.data.length - 1; i >= 0; i-- ) {
		if ( typeof sensor.data[i].w !== "undefined" ) {
			a = $c(node,"windArr");
			a.transform.baseVal.getItem(0).setRotate(sensor.data[i].w.d, a.x.baseVal.value, a.y.baseVal.value);
			$c(node,"windSpd").textContent = "Speed: " + sensor.data[i].w.s + " m/s";
			$c(node,"windDir").textContent = "Dir: "   + sensor.data[i].w.d + "°";
			$c(node,"windGst").textContent = "Gust: "  + sensor.data[i].w.g + " m/s";
			break;
		}
	}
// 	debugger;
// 	head.getElementsByClassName("batt")[0].style.visibility = ( sensor[sens].battery == 0 ? "visible" : "hidden" );
}
function drawRain( sensor ) {
//  	debugger;
	var y, x1, x2, path = "";
	var node = $i("sRain" + sensor.id);
	$c(node,"batt").style.visibility = ( sensor.bat == 0 ? "visible" : "hidden" );
	$c(node,"title").textContent = sensor.name;
	if ( typeof sensor.current.r !== "undefined" )
		$c(node,"r_cur").textContent = "24h: " + sensor.current.r + " mm";
	if ( sensor.data.length < 2 )	// Division by 0
		return;
	dh = node.clientHeight * .95;
	dx = node.clientWidth / ( sensor.data.length - 1 );
	dy = node.clientHeight / ( sensor.max.r > 0 ? sensor.max.r * .8 : 1 );
	for ( i = 0; i < sensor.data.length; i++ ) {
		if ( typeof sensor.data[i].r !== "undefined" ) {
			x1 = Math.round( i*dx );
			x2 = Math.round( (i+1) * dx ) - 1;
			y  = Math.round( dh - sensor.data[i].r * dy );
			path += x1 + "," + node.clientHeight + ' '
				  + x1 + "," + y + ' '
				  + x2 + "," + y + ' '
				  + x2 + "," + node.clientHeight + ' ';
		}
	}
	path += node.clientWidth + "," + node.clientHeight + " " + "0," + node.clientHeight;
// 	console.log( sensor.id + ": " + path + "\n" );
	$c(node,"r_graph").setAttribute("points", path);
}
// Get by class name
function $c( node, id ) {
	return node.getElementsByClassName( id )[0];
}
// Get by id
function $i( id ) {
	return document.getElementById( id );
}
