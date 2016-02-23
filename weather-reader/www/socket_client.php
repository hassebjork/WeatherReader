<?php
include_once( 'db.php' );

$mysqli = new mysqli( DB_HOST, DB_USER, DB_PASS, DB_DATABASE );
$mysqli->set_charset( 'utf8' );

// http://www.binarytides.com/udp-socket-programming-in-php/
error_reporting(~E_WARNING);
 
$server = '192.168.1.234';
$port = 9876;
 
if ( !( $sock = socket_create( AF_INET, SOCK_DGRAM, 0 ) ) ) {
    $errorcode = socket_last_error();
    $errormsg = socket_strerror($errorcode);
     
    die("Couldn't create socket: [$errorcode] $errormsg \n");
}

echo "Socket created \n";
 
// $sql   = 'SELECT * '
// 		.'FROM `wr_test` '
// 		.'WHERE `sensor_id` = 51 '
// 		.'ORDER BY `time`';
$sql   = 'SELECT * '
		.'FROM `wr_temperature` '
		.'WHERE `sensor_id` = 48 '
		.'AND `time` < "2016-02-11 00:00:00" '
		.'ORDER BY `time`';
// $sql   = 'SELECT * '
// 		.'FROM `wr_barometer` '
// 		.'WHERE `sensor_id` = 60 '
// 		.'AND `time` < "2016-02-20 00:00:00" '
// 		.'ORDER BY `time`';
// $sql   = 'SELECT * '
// 		.'FROM `wr_humidity` '
// 		.'WHERE `sensor_id` = 50 '
// 		.'AND `time` < "2016-02-20 00:00:00" '
// 		.'ORDER BY `time`';
$res   = $GLOBALS['mysqli']->query( $sql ) or die( 'Error - failed to get sensors' );
while( $row = $res->fetch_array() ) {
//     //Take some input to send
//     echo 'Enter a message to send : ';
//     $input = fgets(STDIN);

// 	$input = '{"type":"US","id":11,"ch":21,"L":' . $row['value'] . ',"Q":' . $row['value'] . '}';
	$input = '{"type":"DHT21","id":7,"ch":22,"T":' . $row['value'] . ',"Q":' . $row['value'] . '}';
// 	$input = '{"type":"BMP85","id":75,"ch":6,"P":' . $row['value'] . ',"Q":' . $row['value'] . '}';
// 	$input = '{"type":"DHT21","id":13,"ch":21,"H":' . $row['value'] . ',"Q":' . $row['value'] . '}';
	// Send the message to the server
	if ( !socket_sendto( $sock, $input , strlen( $input ) , 0, $server , $port ) ) {
		$errorcode = socket_last_error();
		$errormsg = socket_strerror($errorcode);
			
		die("Could not send data: [$errorcode] $errormsg \n");
	}
	
	echo 'Sent: ' . $input . "\n";
	sleep( 1 );
// 	//Now receive reply from server and print it
// 	if ( socket_recv ( $sock , $reply , 2045, MSG_WAITALL ) === FALSE ) {
// 		$errorcode = socket_last_error();
// 		$errormsg = socket_strerror($errorcode);
// 			
// 		die("Could not receive data: [$errorcode] $errormsg \n");
// 	}
// 		
// 	echo "Reply : $reply";
}
