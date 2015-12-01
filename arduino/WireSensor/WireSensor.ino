#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#define CHANNEL 22
#define NAME "Vinden"

#define SLEEP_8SEC_COUNT  6		/* Sleep intervall counted in aprox 8 sec */

#define OK                0
#define ERROR_CHECKSUM   -1
#define ERROR_TIMEOUT    -2
#define INVALID_VALUE    -999

#define DHT21            21
#define DHT22            22
#define DHT33            33
#define DHT44            44
#define AM2301           21
#define AM2302           22
#define DHTLIB_WAKEUP    1
#define DHTLIB_TIMEOUT (F_CPU/40000)

#define US_ROUNDTRIP_CM  57 

class DHT {
public:
	DHT( uint8_t pin, uint8_t type );
    int  read();
    void print();

    float   humidity;
    float   temperature;
	uint8_t _pin;
	uint8_t _type;

private:
    uint8_t bits[5];  // buffer to receive data
    int _readSensor();
};

class Switch {
public:
	Switch( uint8_t pin );
    uint8_t get()   { return digitalRead( _pin ); };
    void set()   { digitalWrite( _pin, HIGH ); };
    void reset() { digitalWrite( _pin, LOW ); };
    void print();
	
	uint8_t _pin;
};

class UltraSonic {
public:
	UltraSonic( uint8_t trigger, uint8_t echo );
    long read();
    void print();
	
	uint8_t trig_pin;
	uint8_t echo_pin;
};

/******************************************************************************
 * 
 * Class DHT
 * 
 ******************************************************************************/

DHT::DHT(uint8_t pin, uint8_t type) {
	_pin  = pin;
	_type = type;
}

int DHT::_readSensor() {
	// INIT BUFFERVAR TO RECEIVE DATA
	uint8_t mask = 128;
	uint8_t idx  = 0;

	uint8_t bit  = digitalPinToBitMask( _pin );
	uint8_t port = digitalPinToPort( _pin );
	volatile uint8_t *PIR = portInputRegister( port );

	// EMPTY BUFFER
	for ( uint8_t i = 0; i < 5; i++ ) bits[i] = 0;

	// REQUEST SAMPLE
	pinMode( _pin, OUTPUT );
	digitalWrite( _pin, LOW ); // T-be 
	delay( DHTLIB_WAKEUP );
	digitalWrite( _pin, HIGH );   // T-go
	delayMicroseconds( 40 );
	pinMode( _pin, INPUT );

	// GET ACKNOWLEDGE or TIMEOUT
	uint16_t loopCntLOW = DHTLIB_TIMEOUT;
	while ( ( *PIR & bit ) == LOW ) { // T-rel
		if ( --loopCntLOW == 0 ) return ERROR_TIMEOUT;
	}

	uint16_t loopCntHIGH = DHTLIB_TIMEOUT;
	while ( ( *PIR & bit ) != LOW ) { // T-reh
		if ( --loopCntHIGH == 0 ) return ERROR_TIMEOUT;
	}

	// READ THE OUTPUT - 40 BITS => 5 BYTES
	for ( uint8_t i = 40; i != 0; i-- ) {
		loopCntLOW = DHTLIB_TIMEOUT;
		while ( ( *PIR & bit ) == LOW ) {
			if ( --loopCntLOW == 0 ) return ERROR_TIMEOUT;
		}

		uint32_t t = micros();

		loopCntHIGH = DHTLIB_TIMEOUT;
		while ( ( *PIR & bit ) != LOW ) {
			if ( --loopCntHIGH == 0 ) return ERROR_TIMEOUT;
		}

		if ( ( micros() - t ) > 40 ) { 
			bits[idx] |= mask;
		}
		mask >>= 1;
		if ( mask == 0) {  // next byte?
			mask = 128;
			idx++;
		}
	}
	pinMode( _pin, OUTPUT );
	digitalWrite( _pin, HIGH );

	return OK;
}

int DHT::read() {
	int rv = _readSensor();
	if ( rv != OK ) {
		humidity    = INVALID_VALUE;  // invalid value, or is NaN prefered?
		temperature = INVALID_VALUE;  // invalid value
		return rv; // propagate error value
	}

	// CONVERT AND STORE
	humidity = word( bits[0], bits[1] ) * 0.1;
	temperature = word( bits[2] & 0x7F, bits[3] ) * 0.1;
	if ( bits[2] & 0x80 )
		temperature = -temperature;

	// TEST CHECKSUM
	if ( bits[4] == ( bits[0] + bits[1] + bits[2] + bits[3] ) )
		return OK;
	return ERROR_CHECKSUM;
}

void DHT::print() {
	read();
	Serial.print( "{\"type\":\"DHT" );
	Serial.print( _type );
	Serial.print( "\",\"id\":" );
	Serial.print( _pin );
	Serial.print( ",\"ch\":" );
	Serial.print( CHANNEL );
	if ( temperature != INVALID_VALUE ) {
		Serial.print( ",\"T\":" );
		Serial.print( temperature, 1 );
	}
	if ( humidity != INVALID_VALUE ) {
		Serial.print( ",\"H\":" );
		Serial.print( humidity, 1 );
	}
	Serial.println( "}" );
}

/******************************************************************************
 * 
 * Class Switch
 * 
 ******************************************************************************/

Switch::Switch( uint8_t pin ) {
	_pin  = pin;
	pinMode( _pin, INPUT );
}

void Switch::print() {
	Serial.print( "{\"type\":\"SWITCH\",\"id\":" );
	Serial.print( _pin );
	Serial.print( ",\"ch\":" );
	Serial.print( CHANNEL );
	Serial.print( ",\"S\":\"" );
	Serial.print( ( digitalRead( _pin ) == 0 ? "Off" : "On" ) );
	Serial.println( "\"}" );
}

/******************************************************************************
 * 
 * Class UltraSonic distance sensor
 * 
 ******************************************************************************/
UltraSonic::UltraSonic( uint8_t trigger, uint8_t echo ) {
	trig_pin = trigger;
	echo_pin = echo;
	pinMode( trig_pin, OUTPUT );
	pinMode( echo_pin, INPUT );
}

long UltraSonic::read() {
	long distance;
	digitalWrite( trig_pin, LOW );
	delayMicroseconds( 2 );
	digitalWrite( trig_pin, HIGH );
	delayMicroseconds( 10 );
	digitalWrite( trig_pin, LOW );
	distance = pulseIn( echo_pin, HIGH ) / US_ROUNDTRIP_CM;
	if ( distance < 2 || distance > 400 )
		return INVALID_VALUE;
	return distance;
}

void UltraSonic::print() {
	long distance = read();
	Serial.print( "{\"type\":\"US\",\"id\":" );
	Serial.print( echo_pin );
	Serial.print( ",\"ch\":" );
	Serial.print( CHANNEL );
	if ( distance != INVALID_VALUE ) {
		Serial.print( ",\"D\":" );
		Serial.print( distance );
	}
	Serial.println( "}" );
}


/******************************************************************************
 * 
 * Sleep and interrupt routines
 * 
 ******************************************************************************/


volatile uint8_t sleep_count = 0;

/******************************************************************************
 *  Description: Watchdog Interrupt Service.
 *               This is executed when watchdog timed out.
 ******************************************************************************/
ISR( WDT_vect ) {
	sleep_count--;
}

void enterSleep( void ) {
	set_sleep_mode( SLEEP_MODE_PWR_SAVE );   /* EDIT: could also use SLEEP_MODE_PWR_DOWN for lowest power consumption. */
	sleep_enable();
	
	/* Now enter sleep mode. */
	sleep_mode();
	
	/* The program will continue from here after the WDT timeout*/
	sleep_disable(); /* First thing to do is disable sleep. */
	
	/* Re-enable the peripherals. */
	power_all_enable();
}

/******************************************************************************
 * 
 * Main program
 * 
 ******************************************************************************/

DHT dht[] = {
// 	DHT( pin, type1 ),				  		
// 	DHT( 4, DHT21 ),	/* Attic   NC  		
	DHT( 5, DHT21 ),	/* Attic   Ceiling	*/
	DHT( 6, DHT21 ),	/* Outdoor Lower	*/
	DHT( 7, DHT21 )		/* Outdoor Upper	*/
};

Switch sw[] = {
// 	Switch( pin ),							
// 	Switch( 8 ),	
// 	Switch( 9 )		
};

UltraSonic us[] = {
// 	UltraSonic( trigger, echo ),			
// 	UltraSonic( 10, 11 )					
};

void setup() {
	Serial.begin( 115200 );
	Serial.print( "[SR] Ch:" );
	Serial.print( CHANNEL );
	Serial.print( " Name:\"" );
	Serial.print( NAME );
	Serial.println( "\"" );
	
	uint8_t i, n;
	
	/* Clear the reset flag. */
	MCUSR &= ~(1<<WDRF);
	/* In order to change WDE or the prescaler, we need to
	 * set WDCE (This will allow updates for 4 clock cycles).*/
	WDTCSR |= (1<<WDCE) | (1<<WDE);
	/* set new watchdog timeout prescaler value */
	WDTCSR = 1<<WDP0 | 1<<WDP3; /* 8.0 seconds  */
	/* Enable the WD interrupt (note no reset). */
	WDTCSR |= _BV(WDIE);
}

void loop() {
	if ( sleep_count == 0 ) {
		uint8_t i, n;
		
		/* Read DHT */
		n = sizeof( dht ) / sizeof( DHT );
		for ( i = 0; i < n; i++ )
			dht[i].print();
		
		/* Read Switch */
		n = sizeof( sw ) / sizeof( Switch );
		for ( i = 0; i < n; i++ )
			sw[i].print();
		
		/* Read UltraSonic distance */
		n = sizeof( us ) / sizeof( UltraSonic );
		for ( i = 0; i < n; i++ )
			us[i].print();
		
		delay( 100 );
 		sleep_count = SLEEP_8SEC_COUNT;
		enterSleep();
	}
}
