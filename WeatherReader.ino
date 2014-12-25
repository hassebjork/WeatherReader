// Source: http://jeelabs.net/attachments/download/49/Ook_OSV2.pde
//
// Oregon V2 decoder added - Dominique Pierre
// Oregon V3 decoder revisited - Dominique Pierre
// New code to decode OOK signals from weather sensors, etc.
// 2010-04-11 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id: ookDecoder.pde 5331 2010-04-17 10:45:17Z jcw $

class DecodeOOK {
protected:
	byte total_bits, bits, flip, state, pos, data[25];

	virtual char decode (word width) =0;

public:

	enum { UNKNOWN, T0, T1, T2, T3, OK, DONE };

	DecodeOOK () { resetDecoder(); }

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

	// add one bit to the packet data buffer

	virtual void gotBit (char value) {
		total_bits++;
		byte *ptr = data + pos;
		*ptr = (*ptr >> 1) | (value << 7);

		if (++bits >= 8) {
			bits = 0;
			if (++pos >= sizeof data) {
				resetDecoder();
				return;
			}
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

	void done () {
		while (bits)
			gotBit(0); // padding
		state = DONE;
	}
};

// 433 MHz decoders


class OregonDecoderV2 : public DecodeOOK {
public:
	OregonDecoderV2() {}

	// add one bit to the packet data buffer
	virtual void gotBit (char value) {
		if(!(total_bits & 0x01))
		{
			data[pos] = (data[pos] >> 1) | (value ? 0x80 : 00);
		}
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
		return total_bits == 160 ? 1: 0;
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

		total_bits++;
		byte *ptr = data + pos;
		*ptr = (*ptr >> 1) | (value << 7);

		if (++bits >= 8) {
			bits = 0;
			if (++pos >= sizeof data) {
				resetDecoder();
				return;
			}
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


OregonDecoderV2 orscV2;
OregonDecoderV3 orscV3;

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
	byte pos;
	const byte* data = decoder.getData(pos);
	Serial.print(s);
	Serial.print(' ');
	for (byte i = 0; i < pos; ++i) {
		Serial.print(data[i] >> 4, HEX);
		Serial.print(data[i] & 0x0F, HEX);
	}

	// Serial.print(' ');
	// Serial.print(millis() / 1000);
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
	static int i = 0;
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
	}
}

