
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
// 		if ( sensors[sens].type & 1 )	// Temp
// 			SVG.createGraph( sensors[sens], "t" );
	}
}
function drawTemp( sensor ) {
	var i, t, x, dh, dx, dy, path = "", node, ruler;
	node = $i("sTemp" + sensor.id);
	if ( node == null )
		return;
	$c(node,"batt").style.visibility = ( sensor.bat == 0 ? "visible" : "hidden" );
	$c(node,"title").textContent = sensor.name;
	
	// Check minimum values
	if ( sensor.data.length < 4 ) {
		$c(node,"t_cur").textContent = "---°";
		$c(node,"t_min").textContent = "---°";
		$c(node,"t_max").textContent = "---°";
		return;
	}
	
	// Set current
	for ( i = sensor.data.length - 1; i >= 0; i-- ) {
		if ( typeof sensor.data[i].t !== "undefined" ) {
			$c(node,"t_cur").textContent = sensor.data[i].t;
			break;
		}
	}
	
	// Set Min/Max
	if ( typeof sensor.min.t !== "undefined" )
		$c(node,"t_min").textContent = sensor.min.t;
	if ( typeof sensor.max.t !== "undefined" )
		$c(node,"t_max").textContent = sensor.max.t;
	
	// Draw graph
	if ( sensor.data.length < 2 )	// Division by 0
		return;
	ruler = $c(node,"t_ruler");
	while( ruler.firstChild )
		ruler.removeChild(ruler.firstChild);
	t  = sensor.max.t - sensor.min.t;
	dh = node.clientHeight * .95;
	dx = node.clientWidth / ( sensor.data.length - 1 );
	dy = ( t == 0 ? 1 : node.clientHeight / t * .9 );
	for ( i = 0; i < sensor.data.length; i++ ) {
		x = Math.round(i*dx);
		if ( typeof sensor.data[i].t !== "undefined" ) {
			t = Math.round( dh - ( sensor.data[i].t - sensor.min.t ) * dy );
			path += x + "," + t + " ";
		}
		if ( typeof sensor.data[i].d !== "undefined" ) {
			hr = sensor.data[i].d.substring( sensor.data[i].d.indexOf( ' ' ) + 1 );
			if ( hr % 3 == 0 ) {
				ruler.appendChild( createText( x, node.clientHeight-2, hr ) );
			}
		}
	}
	path += node.clientWidth + "," + node.clientHeight + " " + "0," + node.clientHeight;
	$c(node,"t_graph").setAttribute("points", path);
}
function drawHumidity( sensor, node ) {
	var i, node;
	node = $i("sTemp" + sensor.id);
	if ( node == null )
		return;
	checkBatt( node, sensor.bat == 0 );
	$c(node,"title").textContent = sensor.name;
	if ( sensor.data.length < 4 ) {
		$c(node,"h_cur").textContent = "---";
		$c(node,"h_min").textContent = "---";
		$c(node,"h_max").textContent = "---";
		return;
	}
	if ( typeof sensor.max.h !== "undefined" )
		$c(node,"h_max").textContent = parseInt( sensor.max.h ) + "";
	if ( typeof sensor.min.h !== "undefined" )
		$c(node,"h_min").textContent = parseInt( sensor.min.h ) + "";
	for ( i = sensor.data.length - 1; i >= 0; i-- ) {
		if ( typeof sensor.data[i].h !== "undefined" ) {
			$c(node,"h_cur").textContent = parseInt( sensor.data[i].h ) + "%";
			break;
		}
	}
}
function drawWind( sensor ) {
	var i, wind, node, a;
	node = $i("defsSVG");
	if ( node == null )
		return;
	checkBatt( node, sensor.bat == 0 );
	for ( i = sensor.data.length - 1; i >= 0; i-- ) {
		if ( typeof sensor.data[i].w !== "undefined" ) {
			a = $c(node,"windArr");
			a.transform.baseVal.getItem(0).setRotate(sensor.data[i].w.d, a.x.baseVal.value, a.y.baseVal.value);
			$c(node,"windSpd").textContent = "Wind: " + sensor.data[i].w.s + " m/s";
			$c(node,"windDir").textContent = "Dir: "   + sensor.data[i].w.d + "°";
			$c(node,"windGst").textContent = "Gust: "  + sensor.data[i].w.g + " m/s";
			break;
		}
	}
}
function drawRain( sensor ) {
	var x, y, x1, x2, dx, dy, dh, ruler, hr, path = "";
	var node = $i("defsSVG");
	if ( node == null )
		return;
	checkBatt( node, sensor.bat == 0 );
	
	var rain = $i("rainSVG");
	if ( rain != null ) {
		var graph = $i("rainGraph" + sensor.id);
		if ( graph == null ) {
			graph = document.createElementNS("http://www.w3.org/2000/svg", "polygon");
			graph.setAttribute("id","rainGraph" + sensor.id);
			graph.setAttribute("class","r_graph team" + sensor.team );
			$c(rain,"r_graph").appendChild(graph);
		}
		var title = $i("rainTitle" + sensor.id);
		if ( title == null ) {
		}
	} else {
		return;
	}
	
	// Set value
	if ( typeof sensor.current.r !== "undefined" )
		$c(node,"r_cur").textContent = "Rain: " + sensor.current.r + " mm";
	
	ruler = $c(node,"r_ruler_txt");
	while( ruler.firstChild )
		ruler.removeChild(ruler.firstChild);

	// Draw graph
	if ( sensor.data.length < 2 )	// Division by 0
		return;
	dh = node.clientHeight - 10;
	dx = node.clientWidth / ( sensor.data.length - 1 );
	dy = node.clientHeight / 10;
	x1 = 0;
	for ( i = 0; i < sensor.data.length; i++ ) {
		x2 = Math.round( (i+1) * dx ) - 1;
		x  = Math.round( x1 + ( x2-x1) / 2 );
		if ( typeof sensor.data[i].r !== "undefined" ) {
			y  = Math.round( dh - sensor.data[i].r * dy );
			path += x1 + "," + node.clientHeight + ' '
				  + x1 + "," + y + ' '
				  + x2 + "," + y + ' '
				  + x2 + "," + node.clientHeight + ' ';
		}
		if ( typeof sensor.data[i].d !== "undefined" ) {
			hr = sensor.data[i].d.substring( sensor.data[i].d.indexOf( ' ' ) + 1 );
			if ( hr % 3 == 0 ) {
				ruler.appendChild( createText( x, node.clientHeight-2, hr ) );
			}
		}
		x1 = x2 + 1;
	}
	path += node.clientWidth + "," + node.clientHeight + " " + "0," + node.clientHeight;
	graph.setAttribute("points",path);
// 	debugger;
// 	console.log( sensor.id + ": " + path + "\n" );
}
function createLine( x1, x2, y1, y2 ) {
	var line = document.createElementNS("http://www.w3.org/2000/svg", "line");
	line.setAttribute("x1",x1);
	line.setAttribute("x2",x2);
	line.setAttribute("y1",y1);
	line.setAttribute("y2",y2);
	return line;
}
function createText( x, y, txt ) {
	var text = document.createElementNS("http://www.w3.org/2000/svg", "text");
	text.setAttribute("x",x);
	text.setAttribute("y",y);
	text.appendChild(document.createTextNode(txt));
	return 	text;
}
function checkBatt( node, value ) {
	$c(node,"batt").style.visibility = ( value ? "visible" : "hidden" );
}
// Get by class name
function $c( node, id ) {
	return node.getElementsByClassName( id )[0];
}
// Get by id
function $i( id ) {
	return document.getElementById( id );
}

// http://codereview.stackexchange.com/a/40324
function test() {
	var svg = new SVGGraph();
	
}

/* Skicka först sensor objekten
 * Skicka sensor data sorterat efter klockslag, senaste värdet först
 * Sätt en variabel för "Senaste klockslag" på x-axeln
 * Sätt en variabel för antal dagar som ska visas
 */
function SVGGraph( team, sensors ) {
	
	this.canvas = null;
	
	function createCanvas( w, h, id ) {
		this.canvas = document.createElementNS( "http://www.w3.org/2000/svg", "svg" );
		document.body.appendChild( canvas );
		canvas.setAttribute( "id", id );
		canvas.setAttribute( "width",  w + "px" );
		canvas.setAttribute( "height", h + "px" );
	}
	function createGraph( sensor, property ) {
		var max = Number.MIN_VALUE;
		var min = Number.MAX_VALUE;
		var dh, dx, dy, i;
		for ( i = 0; i < sensor.data.length; i++ ) {
			if ( max < sensor.data[i][property] )
				max = sensor.data[i][property];
			if ( min > sensor.data[i][property] )
				min = sensor.data[i][property];
		}
	}
	function createRect( x, y, w, h, fill, stroke ) {
		var rect = document.createElementNS( "http://www.w3.org/2000/svg", "rect" );
		rect.setAttribute( "x", x );
		rect.setAttribute( "y", y );
		rect.setAttribute( "width",  w );
		rect.setAttribute( "height", h );
		rect.setAttribute( "style", "fill:" + fill + ";stroke:" + stroke );
		this.canvas.appendChild( rect );
	}
}
