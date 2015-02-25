#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#define SENSOR_1 10
#define SENSOR_2 11
#define SENSOR_3 12
#define SENSOR_ENABLE 13

volatile int f_wdt=1;

/***************************************************
 *  Description: Watchdog Interrupt Service. This
 *               is executed when watchdog timed out.
 ***************************************************/
ISR( WDT_vect ) {
	f_wdt--;
}


/***************************************************
 *  Description: Enters the arduino into sleep mode.
 ***************************************************/
void enterSleep( void ) {
	set_sleep_mode( SLEEP_MODE_PWR_SAVE );   /* EDIT: could also use SLEEP_MODE_PWR_DOWN for lowest power consumption. */
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

	pinMode( SENSOR_ENABLE,OUTPUT );

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

/***************************************************
 *  Description: Main application loop.
 ***************************************************/
void loop() {
	int read;
	if( f_wdt < 1 ) {
		digitalWrite( SENSOR_ENABLE, 1 ); //!digitalRead( SENSOR_ENABLE ) );	//Toggle the LED
		delay( 500 );
		read = 0;
		read = digitalRead( SENSOR_1 ) + digitalRead( SENSOR_2 ) * 2 + digitalRead( SENSOR_3 ) * 4;
		Serial.print( "WSPS\t" );
		Serial.print( read, HEX );
		Serial.print( "\n" );
		digitalWrite( SENSOR_ENABLE, 0 );
		delay( 100 );
		f_wdt = 6;
	}
	enterSleep();
}