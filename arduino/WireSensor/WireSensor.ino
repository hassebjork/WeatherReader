#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

// Sleep intervall counted in aprox 8 sec
#define INTERVALL 1

#define SENSOR_1  7
#define SENSOR_2  8
#define SENSOR_3  9
#define SENSOR_ENABLE 6
#define UL_ECHO   5
#define UL_TRIG   4
#define SWITCH    3
#define LED      13

volatile unsigned int f_wdt = 0;

/***************************************************
 *  Description: Watchdog Interrupt Service. This
 *               is executed when watchdog timed out.
 ***************************************************/
ISR( WDT_vect ) {
	f_wdt--;
}


void pin3Interrupt( void ) {
	detachInterrupt( 1 );
	static unsigned long debounce = 0;
	unsigned long time = millis();
	if ( time - debounce > 100 ) {
		digitalWrite( LED, 1 );
		sendSwitch( 4, 1 );
		sendSwitch( 4, 0 );
		debounce = time;
	}
}
/***************************************************
 *  Description: Enters the arduino into sleep mode.
 ***************************************************/
void enterSleep( void ) {
	attachInterrupt( 1, pin3Interrupt, RISING );
	delay( 100 );
	set_sleep_mode( SLEEP_MODE_PWR_DOWN );   /* Lowest: SLEEP_MODE_PWR_DOWN or higher: SLEEP_MODE_PWR_SAVE */
	sleep_enable();
	sleep_mode();			// Enter sleep mode

	/* The program will continue from here after the WDT timeout*/
	sleep_disable(); 		// First thing to do is disable sleep.
	power_all_enable();		// Re-enable the peripherals.
}

/***************************************************
 *  Description: Setup for the serial comms and the
 *                Watch dog timeout. 
 ***************************************************/
void setup() {
	Serial.begin(115200);
 	Serial.print("[WS]\n");
	delay( 100 );

	pinMode( SWITCH, INPUT );
	pinMode( SENSOR_ENABLE, OUTPUT );
	pinMode( LED, OUTPUT );
//	pinMode( UL_TRIG, OUTPUT );

	/*** Setup the WDT ***/
	MCUSR &= ~( 1<<WDRF );	// Clear the reset flag.
	/* In order to change WDE or the prescaler, we need to
	* set WDCE ( This will allow updates for 4 clock cycles ). */
	WDTCSR |= ( 1<<WDCE ) | ( 1<<WDE );
	/* set new watchdog timeout prescaler value */
	WDTCSR = 1<<WDP0 | 1<<WDP3; /* 8.0 seconds */
	/* Enable the WD interrupt ( note no reset ). */
	WDTCSR |= _BV( WDIE );
}

void sendSwitch( char id, char value ) {
	Serial.print( "WIRE\tS" );
	if ( id < 16 )
		Serial.print( '0' );
	Serial.print( id, HEX );
	if ( value < 16 )
		Serial.print( '0' );
	Serial.println( value, HEX );
}

/***************************************************
 *  Description: Main application loop.
 ***************************************************/
void loop() {
	int read;
//	long duration, distance;
	if( f_wdt < 1 ) {
		digitalWrite( SENSOR_ENABLE, 1 ); //!digitalRead( SENSOR_ENABLE ) );	//Toggle the LED
/*		
		digitalWrite( UL_TRIG, LOW );
		delayMicroseconds( 2 );
		digitalWrite( UL_TRIG, HIGH );
		delayMicroseconds( 10 );
		digitalWrite( UL_TRIG, LOW );
		duration = pulseIn( UL_ECHO, HIGH );
		distance = (duration/2) / 29.1;
		Serial.print( "WSPS\t" );
		if ( distance < 2 || distance > 400 ) {
    			Serial.println( "DIST:NULL" );
    		} else {
			Serial.print( "DIST:" );
    			Serial.println( distance );
		}
*/		
		delay( 500 );
		sendSwitch( 1, digitalRead( SENSOR_1 ) );
		sendSwitch( 2, digitalRead( SENSOR_2 ) );
		sendSwitch( 3, digitalRead( SENSOR_3 ) );
		delay( 100 );
		digitalWrite( SENSOR_ENABLE, 0 );
		f_wdt = INTERVALL;
	}
	enterSleep();
}
