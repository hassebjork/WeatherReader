// Source: http://jeelabs.net/attachments/download/49/Ook_OSV2.pde
//
// Oregon V2 decoder added - Dominique Pierre
// Oregon V3 decoder revisited - Dominique Pierre
// New code to decode OOK signals from weather sensors, etc.
// 2010-04-11 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id: ookDecoder.pde 5331 2010-04-17 10:45:17Z jcw $

const byte reverse_bits_lookup[] = {
	0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE,
	0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF
};

class DecodeOOK {
protected:
	byte total_bits, bits, flip, state, pos, data[25];

	virtual char decode (word width) =0;

public:

	enum { UNKNOWN, T0, T1, T2, T3, OK, DONE };

	DecodeOOK () { resetDecoder(); }

	virtual bool checkSum() { return true; }

	bool nextPulse (word width) {
		if (state != DONE)
		
			switch (decode(width)) {
				case -1: resetDecoder(); break;
				case 1:  done(); break;
			}
		return isDone();
	}

	bool isDone () const { return state == DONE; }

	const byte* getData (byte& count) const {
		count = pos;
		return data; 
	}

	void resetDecoder () {
		total_bits = bits = pos = flip = 0;
		state = UNKNOWN;
	}

	virtual void gotBit (char value) {
		data[pos] = (data[pos] << 1) | value;
		total_bits++;
		pos = total_bits >> 3;
		if (pos >= sizeof data) {
			resetDecoder();
			return;
		}
		state = OK;
	}

	// store a bit using Manchester encoding
	void manchester (char value) {
		flip ^= value; // manchester code, long pulse flips the bit
		gotBit(flip);
	}

	// move bits to the front so that all the bits are aligned to the end
	void alignTail (byte max =0) {
		// align bits
		if (bits != 0) {
			data[pos] >>= 8 - bits;
			for (byte i = 0; i < pos; ++i)
				data[i] = (data[i] >> bits) | (data[i+1] << (8 - bits));
			bits = 0;
		}
		// optionally shift bytes down if there are too many of 'em
		if (max > 0 && pos > max) {
			byte n = pos - max;
			pos = max;
			for (byte i = 0; i < pos; ++i)
				data[i] = data[i+n];
		}
	}

	void reverseBits () {
		for (byte i = 0; i < pos; ++i) {
			byte b = data[i];
			for (byte j = 0; j < 8; ++j) {
				data[i] = (data[i] << 1) | (b & 1);
				b >>= 1;
			}
		}
	}

	void reverseNibbles () {
		for (byte i = 0; i < pos; ++i)
			data[i] = (data[i] << 4) | (data[i] >> 4);
	}

	byte reverseByte ( byte b ) {
		return ( reverse_bits_lookup[b&0x0F] << 4 | reverse_bits_lookup[b>>4] );
	}

	void done () {
		while (bits)
			gotBit(0); // padding
		state = DONE;
	}
};

// 433 MHz decoders


class OregonDecoderV2 : public DecodeOOK {
public:
	
	byte max_bits;
	
	OregonDecoderV2() {
		max_bits = 160;
	}

	// add one bit to the packet data buffer
	virtual void gotBit (char value) {
		if( !( total_bits & 0x01 ) )
			data[pos] = (data[pos] >> 1) | (value ? 0x80 : 00);
		total_bits++;
		pos = total_bits >> 4;
		if (pos >= sizeof data) {
			resetDecoder();
			return;
		}
		state = OK;
	}

	virtual char decode (word width) {
		if (200 <= width && width < 1200) {
			byte w = width >= 700;
			switch (state) {
				case UNKNOWN:
					if (w != 0) {
						// Long pulse
						++flip;
					} else if (32 <= flip) {
						// Short pulse, start bit
						flip = 0;
						state = T0;
					} else {
						// Reset decoder
						return -1;
					}
					break;
				case OK:
					if (w == 0) {
						// Short pulse
						state = T0;
					} else {
						// Long pulse
						manchester(1);
					}
					break;
				case T0:
					if (w == 0) {
						// Second short pulse
						manchester(0);
					} else {
						// Reset decoder
						return -1;
					}
					break;
			}
		} else {
			return -1;
		}
		
		if ( pos == 2 ) {
			if ( data[0] == 0x1A ) {
				if ( data[1] == 0x89 )			// WRGR800
					max_bits = 176;
				else if ( data[1] == 0x99 )		// WRGR800
					max_bits = 176;
			} else if ( data[0] == 0x2A ) {
				if ( data[1] == 0x19 )			// RCR800 cs8
					max_bits = 184;
				else if ( data[1] == 0x1D )		// RGR918
					max_bits = 168;
			} else if ( data[0] == 0x5A ) {
				if ( data[1] == 0x5D )			// BTHR918
					max_bits = 176;
				else if ( data[1] == 0x6D )		// BTHR918N
					max_bits = 192;
			} else if ( ( data[0] == 0x8A || data[0] == 0x9A ) && data[1] == 0xEC ) {	// RTGR328N
				max_bits = 208;
			} else if ( ( data[0] == 0xDA ) && ( data[1] == 0x78 ) ) {	// UVN800
				max_bits = 144;
			} else if ( data[0] == 0xEA ) {
				if ( data[1] == 0x4C )			// TH132N cs1
					max_bits = 136;
				else if ( data[1] == 0x7C )		// UV138 cs1
					max_bits = 240;
			} else {
				max_bits = 160;
			}
		}
		return total_bits == max_bits ? 1: 0;
	}
	
	virtual bool checkSum() {
		byte s = 0;
		for ( byte i = 0; i < pos-2 ; i++ )
			s += ( data[i] & 0xF ) + ( data[i] >> 4 );
		return ( s - 10 ) == data[pos-2];
	}
};

class OregonDecoderV3 : public DecodeOOK {
public:
	OregonDecoderV3() {}

	// add one bit to the packet data buffer
	virtual void gotBit (char value) {
		data[pos] = (data[pos] >> 1) | (value ? 0x80 : 00);
		total_bits++;
		pos = total_bits >> 3;
		if (pos >= sizeof data) {
			resetDecoder();
			return;
		}
		state = OK;
	}

	virtual char decode (word width) {
		if (200 <= width && width < 1200) {
			byte w = width >= 700;
			switch (state) {
				case UNKNOWN:
					if (w == 0)
						++flip;
					else if (32 <= flip) {
						flip = 1;
						manchester(1);
					} else
						return -1;
					break;
				case OK:
					if (w == 0)
						state = T0;
					else
						manchester(1);
					break;
				case T0:
					if (w == 0)
						manchester(0);
					else
						return -1;
					break;
			}
		} else {
			return -1;
		}
		return  total_bits == 80 ? 1: 0;
	}
};

class VentusDecoder : public DecodeOOK {
public:
	
	VentusDecoder () {}
	
	// see also http://www.tfd.hu/tfdhu/files/wsprotocol/auriol_protocol_v20.pdf
	virtual char decode (word width) {
		switch (state ) {
			case UNKNOWN:	// Preample
				if (  8500 < width && width < 9500 )
					state = OK;
				else
					return -1;
				break;
			case OK:		// Signal On
				if ( 450 <= width && width < 550)
					state = T0;
				else
					return -1;
				break;
			case T0:		// Signal off = bit
				if ( 1800 < width && width < 4400 ) {
					byte w = ( width > 3000 );
					gotBit( w );
					if ( total_bits > 35 ) {
						data[pos] = data[pos] << 4;
						++pos;
// 						checkSum();
						return 1;
					}
				} else if (  8500 < width && width < 9500 ) {
					done();
					state = OK;
					return 0;
				} else
					return -1;
				break;
			default:
				return -1;
		}
		return 0;
	}

	virtual bool checkSum() {
		byte s, t;
		bool rain = ( ( data[1] & 0x7F ) == 0x6C );
		s = ( rain ? 0x7 : 0xF );
		for ( byte i = 0; i < 8; i++ ) {
			if ( i%2 )
				t = reverse_bits_lookup[ ( data[i/2] & 0xF )];
			else
				t = reverse_bits_lookup[ ( data[i/2] >> 4  )];
			s += ( rain ? t : -t );
		}
		return ( s & 0x0F ) == reverse_bits_lookup[( data[4] >> 4 )];
	}
};

class FineOffsetDecoder : public DecodeOOK {
public:
	// https://github.com/NetHome/Coders/blob/master/src/main/java/nu/nethome/coders/decoders/FineOffsetDecoder.java
	// https://github.com/lucsmall/BetterWH2/blob/master/BetterWH2.ino
	FineOffsetDecoder() { }

    virtual char decode (word width) {
		if (width < 400 || width > 1600)
			return -1;
		
		switch (state) {
			case UNKNOWN:	// Signal start (on)
				if (width < 600) {
					state = OK;
				} else
					return -1;
				break;
			case OK:		// Signal off
				if (900 < width && width < 1100) {
					++flip;
					state = T1;
				} else
					return -1;
				break;
			case T1:		// Signal on = bit
				if (width < 600 && flip > 10) {
					gotBit(1);
				} else if ( width > 1350 ) {
					flip = 11;
					gotBit(0);
				} else {
					return -1;
				}
				
				// Temperature & humidity
				if ( total_bits == 40 && ( data[0] & 0xF0 ) != 0x30 )
    					return 1;
				// Rain
				else if ( total_bits == 56 )
					return 1;
				break;
			default:
				return -1;
		}
        return 0;
    }
    
	virtual bool checkSum() {
		char len, i, mix, inbyte, crc = 0;

		// Indicated changes are from reference CRC-8 function in OneWire library
		for ( len = 0; len < pos - 1; len ++ ) {
			inbyte = data[len];
			for ( i = 8; i; i-- ) {
				mix = ( crc ^ inbyte ) & 0x80; // changed from & 0x01
				crc <<= 1; // changed from right shift
				if ( mix ) 
					crc ^= 0x31;// changed from 0x8C;
				inbyte <<= 1; // changed from right shift
			}
		}
		return ( ( crc & 0xFF ) == data[pos-1] );
	}
};

struct os_timedate {
	byte sec;
	byte min;
	byte hour;
	byte mday;
	byte month;
	byte wday;
	byte year;
};

OregonDecoderV2 orscV2;
OregonDecoderV3 orscV3;
VentusDecoder   ventus;
FineOffsetDecoder fineOffset;

#define PORT 2

volatile word pulse;

#if defined(__AVR_ATmega1280__)
void ext_int_1(void) {
#else
ISR(ANALOG_COMP_vect) {
#endif
	static word last;
	// determine the pulse length in microseconds, for either polarity
	pulse = micros() - last;
	last += pulse;
}

void reportSerial (const char* s, class DecodeOOK& decoder) {
	if ( !decoder.checkSum() )
		return;
	byte pos;
	const byte* data = decoder.getData(pos);
	Serial.print(s);
	Serial.print(' ');
	for (byte i = 0; i < pos; ++i) {
		Serial.print(data[i] >> 4, HEX);
		Serial.print(data[i] & 0x0F, HEX);
	}
	Serial.println();

	decoder.resetDecoder();
}


void setup () {
	Serial.begin(115200);
	Serial.println("\n[WeatherReader]\n");

#if !defined(__AVR_ATmega1280__)
	pinMode(13 + PORT, INPUT);  // use the AIO pin
	digitalWrite(13 + PORT, 1); // enable pull-up

	// use analog comparator to switch at 1.1V bandgap transition
	ACSR = _BV(ACBG) | _BV(ACI) | _BV(ACIE);

	// set ADC mux to the proper port
	ADCSRA &= ~ bit(ADEN);
	ADCSRB |= bit(ACME);
	ADMUX = PORT - 1;
#else
	attachInterrupt(1, ext_int_1, CHANGE);

	DDRE  &= ~_BV(PE5);
	PORTE &= ~_BV(PE5);
#endif
}

void loop () {
	cli();
	word p = pulse;

	pulse = 0;
	sei();

	//if (p != 0){ Serial.print(++i); Serial.print('\n');}

	if (p != 0) {
		if (orscV2.nextPulse(p))
			reportSerial("OSV2", orscV2);  
		if (orscV3.nextPulse(p))
			reportSerial("OSV3", orscV3);
		if (ventus.nextPulse(p))
			reportSerial("VENT", ventus);
		if (fineOffset.nextPulse(p))
			reportSerial("FINE", fineOffset);
	}
}

