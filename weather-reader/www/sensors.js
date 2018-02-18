var BJORK = BJORK || ( window.location.href.indexOf( 'bjork.es' ) > 1 );

var Sensor = Sensor || {
	retry: 0,
	
	status: function ( txt ) {
		document.getElementById("aTime").innerHTML = txt;
		document.getElementById("aTime").className = "error";
		var t = setTimeout( function() { 
			document.getElementById("aTime").innerHTML = "";
			document.getElementById("aTime").className = "";
		}, 3000 );
	},
	
	load: function() {
		var req = new XMLHttpRequest();
		req.onreadystatechange = function() {
			if ( req.readyState == 4 ) {
				if ( req.status == 0 ) {
					Sensor.load();
					Sensor.status( "Error connecting. Retrying!" );
					return;
				} else if ( req.status != 200 ) {
					Sensor.load();
					Sensor.status( "Error: " + req.status );
					return;
				}
				Sensor.retry = 0;
				var obj = JSON.parse( req.responseText );
				if ( typeof obj.sensors !== 'undefined' )
					Sensor.update( obj.sensors );
				if ( typeof obj.time !== 'undefined' )
					document.getElementById("aTime").innerHTML = "Updated " + obj.time.substring(11, 16);
				if ( typeof obj.Clock !== 'undefined' )
					Sensor.Clock.draw( obj.Clock );
			}
		}
		req.open( "GET", ( BJORK ? "/weather/all.js?ckattempt=1&r=" : "/weather/?all=1&r=" ) + Date.now(), true );
		req.send();
	},
	
	update: function( sensors ) {
		var sens;
		for ( sens in sensors ) {
			if ( sensors[sens].type & 128 )	// Barometer
				Sensor.Baro.draw( sensors[sens] );
			if ( sensors[sens].type & 16 )	// Wind
				Sensor.Wind.draw( sensors[sens] );
			if ( sensors[sens].type & 1 )	// Temp
				Sensor.Temp.draw( sensors[sens] );
			if ( sensors[sens].type & 2 )	// Humid
				Sensor.Humid.draw( sensors[sens] );
			if ( sensors[sens].type & 32 )	// Rain
				Sensor.Rain.draw( sensors[sens] );
			if ( sensors[sens].type & 256 )	// Distance
				Sensor.Dist.draw( sensors[sens] );
			if ( sensors[sens].type & 512 )	// Level
				Sensor.Level.draw( sensors[sens] );
		}
		var evt = new CustomEvent('sensorupdate', {} );
		window.dispatchEvent( evt );
	},
	
	batt: function( node, value ) {
		if ( node != null && value != null )
			node.style.visibility = ( value == 0 ? "visible" : "hidden" );
	},
	
	emptyRuler: function( ruler ) {
		while( ruler.firstChild )
			ruler.removeChild(ruler.firstChild);
		return ruler;
	},
	
	svgLine: function( x1, x2, y1, y2 ) {
		var line = document.createElementNS("http://www.w3.org/2000/svg", "line");
		line.setAttribute("x1",x1);
		line.setAttribute("x2",x2);
		line.setAttribute("y1",y1);
		line.setAttribute("y2",y2);
		return line;
	},
	
	svgText: function( x, y, txt ) {
		var text = document.createElementNS("http://www.w3.org/2000/svg", "text");
		text.setAttribute("x",x);
		text.setAttribute("y",y);
		text.appendChild(document.createTextNode(txt));
		return text;
	},
	
	Temp: {
		data: [],
		
		make: function( sensor ) {
			var div = document.createElement( 'div' );
			div.setAttribute( 'id', 'sTemp' + sensor.id );
			div.setAttribute( 'class', 't_widget team' + sensor.team );
			div.setAttribute( 'data-team', sensor.team );
			div.setAttribute( 'style', 'display:none' );
			div.innerHTML = '<svg width="100%" height="100%" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">'
			+ '<use xlink:href="#svgBg" class="widgetBg"/>'
			+ '<polygon class="t_graph"/>'
			+ '<use x="5" y="8" class="batt" style="visibility:hidden" xlink:href="#icon_bat"/>'
			+ '<g class="t_ruler"/>'
			+ '</svg>'
			+ '<div class="title" title="' + sensor.protocol + '">' 
			+ sensor.name + '</div>'
			+ '<div class="t_cur"></div>'
			+ '<div class="t_max"></div>'
			+ '<div class="t_min"></div>'
			+ '<div class="h_cur"></div>'
			+ '<div class="h_max"></div>'
			+ '<div class="h_min"></div>'
			+ '</div>' + "\n";
			document.body.appendChild( div );
			var divs = div.getElementsByTagName( "div" );
			this.data[sensor.id] = {
				div:   div,
				title: divs[divs.length-7],
				t_cur: divs[divs.length-6],
				t_max: divs[divs.length-5],
				t_min: divs[divs.length-4],
				h_cur: divs[divs.length-3],
				h_max: divs[divs.length-2],
				h_min: divs[divs.length-1],
				graph: div.getElementsByTagName( "polygon" )[0],
				batt : div.getElementsByTagName( "use" )[1],
				ruler: div.getElementsByTagName( "g" )[0],
			};
			return div;
		},
		
		draw: function( sensor ) {
			var i, t, x, dh, dx, dy, path = "", node, ruler;
			node = ( this.data[sensor.id] ? this.data[sensor.id].div : Sensor.Temp.make( sensor ) );
			if ( sensor.data.length < 6 ) {
				node.style.display = "none";
				return;
			}
			node.style.display = "inline-block"
			Sensor.batt( this.data[sensor.id].batt, sensor.bat );
			this.data[sensor.id].title.textContent = sensor.name;
			this.data[sensor.id].title.title       = sensor.protocol;
			
			// Set current
			for ( i = sensor.data.length - 1; i >= 0; i-- ) {
				if ( typeof sensor.data[i].t !== "undefined" ) {
					this.data[sensor.id].t_cur.textContent = sensor.data[i].t;
					break;
				}
			}
			
			// Set Min/Max
			if ( typeof sensor.min.t !== "undefined" )
				this.data[sensor.id].t_min.textContent = sensor.min.t;
			if ( typeof sensor.max.t !== "undefined" )
				this.data[sensor.id].t_max.textContent = sensor.max.t;
			
			// Draw graph
			ruler = Sensor.emptyRuler( this.data[sensor.id].ruler );
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
					hr = sensor.data[i].d % 100;
					if ( hr % 3 == 0 ) {
						ruler.appendChild( Sensor.svgText( x, node.clientHeight-2, hr ) );
					}
				}
			}
			path += node.clientWidth + "," + node.clientHeight + " " + "0," + node.clientHeight;
			this.data[sensor.id].graph.setAttribute("points", path);
		},
	},
	
	Humid: {
		draw: function ( sensor ) {
			var i, node;
			node = ( Sensor.Temp.data[sensor.id] ? Sensor.Temp.data[sensor.id].div : Sensor.Temp.make( sensor ) );
			Sensor.batt( Sensor.Temp.data[sensor.id].batt, sensor.bat );
			Sensor.Temp.data[sensor.id].title.textContent = sensor.name;
			if ( sensor.data.length < 4 ) {
				Sensor.Temp.data[sensor.id].h_cur.textContent = "---";
				Sensor.Temp.data[sensor.id].h_min.textContent = "---";
				Sensor.Temp.data[sensor.id].h_max.textContent = "---";
				return;
			}
			
			if ( typeof sensor.max.h !== "undefined" )
				Sensor.Temp.data[sensor.id].h_max.textContent = sensor.max.h;
			if ( typeof sensor.min.h !== "undefined" )
				Sensor.Temp.data[sensor.id].h_min.textContent = sensor.min.h;
			for ( i = sensor.data.length - 1; i >= 0; i-- ) {
				if ( typeof sensor.data[i].h !== "undefined" ) {
					Sensor.Temp.data[sensor.id].h_cur.textContent = sensor.data[i].h + "%";
					break;
				}
			}
		}
	},
	
	Wind: {
		data: {},
		
		make: function( sensor ) {
			var div = document.createElement( 'div' );
			div.setAttribute( 'id', 'sAneom' + sensor.id );
			div.setAttribute( 'data-team', 0 );
			div.setAttribute( 'style', 'display:none' );
			div.innerHTML = '<svg width="150" height="150" viewBox="0 0 378 378" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">'
			+ '<use xlink:href="#dial_bg"/>'
			+ '<use xlink:href="#dial_icon_star"/>'
			+ '<use xlink:href="#dial_dig_compass"/>'
			+ '<g class="dial_val">'
			+ '<text x="187" y="95">Speed</text>'
			+ '<text id="aneo_speed" x="187" y="125">0.5 m/s</text>'
			+ '<text x="187" y="260">Gust</text>'
			+ '<text id="aneo_gust" x="187" y="290">1.5 m/s</text>'
			+ '</g>'
			+ '<use xlink:href="#dial_normal"/>'
			+ '<use xlink:href="#dial_glare"/>'
			+ '</svg>';
			document.body.insertBefore( div, document.body.childNodes[2] );
			var divs = div.getElementsByTagName( "text" );
			var div2 = div.getElementsByTagName( "use" );
			this.data[sensor.id] = {
				div:   div,
				speed: divs[1],
				gust:  divs[3],
				dial:  div2[div2.length-2],
			};
			return div;
		},
		
		draw: function ( sensor ) {
			var i, wind, node, a;
			node = ( sensor.id in this.data ? this.data[sensor.id].div : Sensor.Wind.make( sensor ) );
			
			if ( sensor.data.length < 6 ) {
				node.style.display = "none";
				return;
			}
			node.style.display = "inline-block"
			
			for ( i = sensor.data.length - 1; i >= 0; i-- ) {
				if ( typeof sensor.data[i].w !== "undefined" ) {
					this.data[sensor.id].dial.setAttribute("transform", "rotate(" + sensor.data[i].w.d + ", 187, 187)" );
					this.data[sensor.id].speed.textContent = sensor.data[i].w.s + " m/s";
					this.data[sensor.id].gust.textContent  = sensor.data[i].w.g + " m/s";
					break;
				}
			}
		},
	},
	
	Clock: {
		data: {},
		
		timer: setInterval( function() {
			var key, time;
			for ( key in Sensor.Clock.data ) {
				time = new Date( (new Date()).getTime() + Sensor.Clock.data[key].offset );
				var clock_hrs = time.getHours();
				var clock_min = time.getMinutes();
				var clock_sec = time.getSeconds();
				Sensor.Clock.data[key].dial_fast.setAttribute("transform", "rotate(" + ( clock_sec * 6 ) + ", 187, 187)" );
				if ( Sensor.Clock.data[key].min != clock_min ) {
					Sensor.Clock.data[key].dial_long.setAttribute("transform", "rotate(" + ( clock_min * 6 ) + ", 187, 187)" );
					Sensor.Clock.data[key].min = clock_min;
					if ( Sensor.Clock.data[key].hrs != clock_hrs ) {
						Sensor.Clock.data[key].dial_short.setAttribute("transform", "rotate(" + ( clock_hrs % 12 * 30 + clock_min / 2 ) + ", 187, 187)" );
						Sensor.Clock.data[key].hrs = clock_hrs;
					}
				}
			}
		}, 1000 ),
		
		make: function( sensor ) {
			var div = document.createElement( 'div' );
			div.setAttribute( 'id', 'sClock' + sensor.id );
			div.setAttribute( 'data-team', 0 );
			div.innerHTML = '<svg width="150" height="150" viewBox="0 0 378 378" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">'
			+ '<use xlink:href="#dial_bg"/>'
			+ '<use xlink:href="#dial_dig_clock"/>'
			+ '<text x="187" y="125" class="dial_val">' + sensor.title + '</text>'
			+ '<text x="187" y="270" class="dial_val"></text>'
			+ '<use xlink:href="#dial_normal"/>'
			+ '<use xlink:href="#dial_short"/>'
			+ '<use xlink:href="#dial_thin"/>'
			+ '<use xlink:href="#dial_glare"/>'
			+ '</svg>';
			document.body.insertBefore( div, document.body.childNodes[2] );
			var divs = div.getElementsByTagName( "use" );
			this.data[sensor.id] = {
				div:        div,
				title:      div.getElementsByTagName( "text" )[0],
				text:       div.getElementsByTagName( "text" )[1],
				dial_fast:  divs[divs.length-2],
				dial_short: divs[divs.length-3],
				dial_long:  divs[divs.length-4],
				offset:     (new Date( sensor.time)).getTime() - (new Date()).getTime(),
				hrs:        0,
				min:        0,
			};
			return div;
		},
		
		draw: function( sensors ) {
			var key, time;
			for ( key in sensors ) {
				if ( sensors[key] && !Sensor.Clock.data[sensors[key].id] )
					this.make( sensors[key] );
				time = new Date( (new Date()).getTime() + Sensor.Clock.data[sensors[key].id].offset );
				var clock_hrs = time.getHours();
				var clock_min = time.getMinutes();
				var clock_sec = time.getSeconds();
				Sensor.Clock.data[sensors[key].id].dial_long.setAttribute("transform", "rotate(" + ( clock_min * 6 ) + ", 187, 187)" );
				Sensor.Clock.data[sensors[key].id].dial_short.setAttribute("transform", "rotate(" + ( clock_hrs % 12 * 30 + clock_min / 2 ) + ", 187, 187)" );
				Sensor.Clock.data[sensors[key].id].dial_fast.setAttribute("transform", "rotate(" + ( clock_sec * 6 ) + ", 187, 187)" );
			}
		},
	},
	
	Baro: {
		data: {},
		
		make: function( sensor ) {
			var div = document.createElement( 'div' );
			div.setAttribute( 'id', 'sBaro' + sensor.id );
			div.setAttribute( 'data-team', 0 );
			div.setAttribute( 'style', 'display:none' );
			div.innerHTML = '<svg width="150" height="150" viewBox="0 0 378 378" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">'
			+ '<use xlink:href="#dial_bg"/>'
			+ '<use xlink:href="#dial_dig_baro"/>'
			+ '<use xlink:href="#dial_icon_weather"/>'
			+ '<g transform="translate(132,280)"/>'
			+ '<text class="dial_val" x="187" y="270"></text>'
			+ '<use xlink:href="#dial_normal"/>'
			+ '<use xlink:href="#dial_glare"/>'
			+ '</svg>'
			document.body.insertBefore( div, document.body.childNodes[2] );
			var divs = div.getElementsByTagName( "use" );
			this.data[sensor.id] = {
				div:  div,
				text: div.getElementsByTagName( "text" )[0],
				disp: div.getElementsByTagName( "g" )[0],
				dial: divs[divs.length-2],
			};
			return div;
		},
		
		draw: function ( sensor ) {
			// Normal display 950-1050. Graph range is >= 7 mmHg
			var x, y, dh, dx, dy, node, baro, path = "";	// Old
			var i, j, dv, dx, dy, y_max = -9999, y_min = 9999, data = [], disp = [];
			var DISPLAY_WIDTH = 12, DISPLAY_HEIGHT = 5;
			
			node = ( sensor.id in this.data ? this.data[sensor.id].div : Sensor.Baro.make( sensor ) );
			if ( sensor.data.length < 7 ) {
				node.style.display = "none";
				return;
			}
			node.style.display = "inline-block";
			
			// Barometer dial and text
			if ( sensor.data.length > 1 && typeof sensor.data[sensor.data.length-1].b !== "undefined" ) {
				this.data[sensor.id].dial.setAttribute("transform", "rotate(" + ( ( sensor.data[sensor.data.length-1].b - 1010 ) * 3 ) + ", 187, 187)" );
				this.data[sensor.id].text.textContent = sensor.data[sensor.data.length-1].b + " hPa";
			}
			
			// Create display grid
			node = this.data[sensor.id].disp;
			if ( node.childNodes.length < 1 ) {
				for ( i = 0; i < DISPLAY_WIDTH; i++ ) {
					for ( j = 0; j < DISPLAY_HEIGHT; j++ ) {
						var box = document.createElementNS("http://www.w3.org/2000/svg", "rect");
						box.setAttribute("x", ( i * 9 ) );
						box.setAttribute("y", ( j * 7 ) );
						box.setAttribute("width",  6);
						box.setAttribute("height", 5);
						box.setAttribute("class","baro_doff")
						node.appendChild(box);
					}
				}
			}
			
			// Display grid
			if ( sensor.data.length > 1 ) {
				for ( var i = 0; i < sensor.data.length; i++ ) {
					if ( typeof sensor.data[i].b !== "undefined" ) {
						data.push( sensor.data[i].b );
						if ( sensor.data[i].b > y_max ) y_max = sensor.data[i].b;
						if ( sensor.data[i].b < y_min ) y_min = sensor.data[i].b;
					}
				}
				
				dv = y_max - y_min;
				dv = ( dv > 7 ? dv : 7 );
				dx = data.length / DISPLAY_WIDTH;
				dy = (DISPLAY_HEIGHT-1) / dv;
				for ( i = 0; i < data.length; i += dx ) {
					disp.push( DISPLAY_HEIGHT - 1 - Math.round( ( data[Math.floor(i)] - y_min + dv/2 ) * dy ) );
				}
				
				dv = 0;
				for ( i = 0; i < DISPLAY_WIDTH; i++ ) {
					for ( j = 0; j < DISPLAY_HEIGHT; j++) {
						node.childNodes[dv++].setAttribute( "class", disp[i] <= j ? "baro_don" : "baro_doff" );
					}
				}
			}
		},
	},
	
	Dist: {
		data: {},
		
		make: function( sensor ) {
			var div = document.createElement( 'div' );
			div.setAttribute( 'id', 'sDist' + sensor.id );
			div.setAttribute( 'class', 't_widget team' + sensor.team );
			div.setAttribute( 'data-team', sensor.team );
			div.setAttribute( 'style', 'display:none' );
			div.innerHTML = '<svg width="100%" height="100%" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">'
				+ '<use xlink:href="#svgBg" class="widgetBg"/>'
				+ '<polygon class="d_graph"/>'
				+ '<g class="d_ruler"/>'
				+ '</svg>'
				+ '<div class="title" title="' + sensor.protocol + '">' 
				+ sensor.name + '</div>'
				+ '<div class="d_cur"></div>'
				+ '</div>' + "\n";
			document.body.appendChild( div );
			var divs = div.getElementsByTagName( "div" );
			this.data[sensor.id] = {
				div:   div,
				d_cur: divs[2],
				graph: div.getElementsByTagName( "polygon" )[0],
				ruler: div.getElementsByTagName( "g" )[0],
			};
			return div;
		},
		
		draw: function( sensor ) {
			var store = { max: 120, min: 4, level: 0, percent: 0 };
			var i, x, y, dh, t, dx, dy, path = "", node, ruler;
			var hr   = -1;
			
			node = ( sensor.id in this.data ? this.data[sensor.id].div : Sensor.Dist.make( sensor ) );
			if ( sensor.data.length < 6 ) {
				node.style.display = "none";
				return;
			}
			node.style.display = "inline-block"
			
			// Remove ruler
			ruler = Sensor.emptyRuler( this.data[sensor.id].ruler );
			
			// Set current
			for ( i = sensor.data.length - 1; i >= 0; i-- ) {
				if ( typeof sensor.data[i].v !== "undefined" ) {
					t = Math.round( 100 - ( sensor.data[i].v - store.min ) / ( store.max - store.min ) * 100 );
					t = t < 0 ? 0 : t;
					t = t > 100 ? 100 : t;
					this.data[sensor.id].d_cur.textContent = t + " %";
					break;
				}
			}
			dh = node.clientHeight * .95;
			dx = node.clientWidth / ( sensor.data.length - 1 );
			dy = node.clientHeight / store.max * .9;
			for ( i = 0; i < sensor.data.length; i++ ) {
				x = Math.round(i*dx);
				if ( typeof sensor.data[i].v !== "undefined" ) {
					y = Math.round( dh - ( store.max - sensor.data[i].v ) * dy );
					path += x + "," + ( y > 0 ? y : 0 ) + " ";
				}
				if ( typeof sensor.data[i].d !== "undefined" ) {
					t = Math.floor( sensor.data[i].d / 100 ) % 100;
					if ( t != hr ) {
						hr = t;
						ruler.appendChild( Sensor.svgLine( x, x, 0, node.clientHeight-10 ) );
						ruler.appendChild( Sensor.svgText( x, node.clientHeight-2, hr ) );
					}
				}
			}
			path += node.clientWidth + "," + node.clientHeight + " " + "0," + node.clientHeight;
			this.data[sensor.id].graph.setAttribute("points", path);
		}
	},
	
	Level: {
		data: {},
		
		make: function( sensor ) {
			var div = document.createElement( 'div' );
			div.setAttribute( 'id', 'sLevl' + sensor.id );
			div.setAttribute( 'class', 't_widget team' + sensor.team );
			div.setAttribute( 'data-team', sensor.team );
			div.setAttribute( 'style', 'display:none' );
			div.innerHTML = '<svg width="100%" height="100%" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">'
				+ '<use xlink:href="#svgBg" class="widgetBg"/>'
				+ '<polygon class="d_graph"/>'
				+ '<g class="d_ruler"/>'
				+ '</svg>'
				+ '<div class="title" title="' + sensor.protocol + '">' 
				+ sensor.name + '</div>'
				+ '<div class="d_cur"></div>'
				+ '</div>' + "\n";
			document.body.appendChild( div );
			var divs = div.getElementsByTagName( "div" );
			this.data[sensor.id] = {
				div:   div,
				d_cur: divs[1],
				graph: div.getElementsByTagName( "polygon" )[0],
				ruler: div.getElementsByTagName( "g" )[0],
			};
			return div;
		},
		
		draw: function( sensor ) {
			var store = { max: 120, min: 4, level: 0, percent: 0 };
			var i, x, y, dh, t, dx, dy, path = "", node, ruler;
			var hr   = -1;
			node = ( this.data[sensor.id] ? this.data[sensor.id].div : Sensor.Level.make( sensor ) );
			if ( sensor.data.length < 6 ) {
				node.style.display = "none";
				return;
			}
			node.style.display = "inline-block"
// 			Sensor.batt( this.data[sensor.id].batt, sensor.bat );
			this.data[sensor.id].title.textContent = sensor.name;
			this.data[sensor.id].title.title       = sensor.protocol;
			
			// Remove ruler
			ruler = Sensor.emptyRuler( this.data[sensor.id].ruler );
			
			// Set current
			for ( i = sensor.data.length - 1; i >= 0; i-- ) {
				if ( typeof sensor.data[i].l !== "undefined" ) {
					t = Math.round( 100 - ( sensor.data[i].l - store.min ) / ( store.max - store.min ) * 100 );
					t = t < 0 ? 0 : t;
					t = t > 100 ? 100 : t;
					this.data[sensor.id].d_cur.textContent = t + " %";
					break;
				}
			}
			dh = node.clientHeight;
			dx = node.clientWidth / ( sensor.data.length - 1 );
			dy = node.clientHeight / store.max;
			for ( i = 0; i < sensor.data.length; i++ ) {
				x = Math.round(i*dx);
				if ( typeof sensor.data[i].l !== "undefined" ) {
					y = Math.round( dh - ( store.max - sensor.data[i].l ) * dy );
					path += x + "," + ( y > 0 ? y : 0 ) + " ";
				}
				if ( typeof sensor.data[i].d !== "undefined" ) {
					t = Math.floor( sensor.data[i].d / 100 ) % 100;
					if ( t != hr ) {
						hr = t;
						ruler.appendChild( Sensor.svgLine( x, x, 0, node.clientHeight-10 ) );
						ruler.appendChild( Sensor.svgText( x, node.clientHeight-2, hr ) );
					}
				}
			}
			path += node.clientWidth + "," + node.clientHeight + " " + "0," + node.clientHeight;
			this.data[sensor.id].graph.setAttribute("points", path);
		}
	},
	
	Rain: {
		data: {},
		
		make: function( sensor ) {
			var div = document.createElement( 'div' );
			div.setAttribute( 'id', 'sRain' + sensor.id );
			div.setAttribute( 'class', 't_widget team' + sensor.team );
			div.setAttribute( 'data-team', sensor.team );
			div.setAttribute( 'style', 'display:none' );
			div.innerHTML = '<svg width="100%" height="100%" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">'
			+ '<use xlink:href="#svgBg" class="widgetBg"/>'
			+ '<polygon class="r_graph team"' + sensor.team + '/>'
			+ '<g class="r_ruler"></g>'
			+ '</svg>'
			+ '<div class="title" title="' + sensor.protocol + '">' 
			+ sensor.name + '</div>';
			document.body.appendChild( div );
			this.data[sensor.id] = {
				div:   div,
				graph: div.getElementsByTagName( "polygon" )[0],
				text:  div.getElementsByTagName( "div" )[0],
				ruler: div.getElementsByTagName( "g" )[0],
			};
			return div;
		},
		
		draw: function( sensor ) {
			var x, y, x1, x2, dx, dy, dh, ruler, hr, path = "", height = 6;
			var node = ( sensor.id in this.data ? this.data[sensor.id].div : Sensor.Rain.make( sensor ) );
			if ( sensor.data.length < 6 ) {
				node.style.display = "none";
				return;
			}
			node.style.display = "inline-block"
			Sensor.batt( this.data[sensor.id].batt, sensor.bat );
			
			// Set value
			if ( typeof sensor.current.r !== "undefined" )
				this.data[sensor.id].text.innerHTML = "Rain<br>" + sensor.current.r + " mm";
			ruler = Sensor.emptyRuler( this.data[sensor.id].ruler );
			
			// Draw graph
			var graph = this.data[sensor.id].graph;
			dh = node.clientHeight - 10;
			dx = node.clientWidth / ( sensor.data.length - 1 );
			dy = node.clientHeight / height;
			x1 = 0;
			for ( i = 1; i < height; i++ ) {
				y = dh - i * dy;
				this.data[sensor.id].ruler.appendChild( Sensor.svgLine( 0, node.clientWidth, y, y ) );
			}
			
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
					hr = sensor.data[i].d % 100;
					if ( hr % 3 == 0 ) {
						ruler.appendChild( Sensor.svgText( x, node.clientHeight-2, hr ) );
					}
				}
				x1 = x2 + 1;
			}
			path += node.clientWidth + "," + node.clientHeight + " " + "0," + node.clientHeight;
			graph.setAttribute("points",path);
		},
	},
};

