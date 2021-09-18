#include <stdint.h>

#define BOFF	2
#define BON	3

#define RS	3
#define EN	4

int bitmap[] = { 6, 7, 8, 13, 12, 11, 10, 9 };

#define ON_TIME 90

class interval {

	uint32_t start_millis;

public:
	interval() {
		start_millis = 0;
	}

	uint32_t milliseconds() {
		uint32_t now = millis();
		if (now < start_millis) // handle overflow
			return (UINT32_MAX - start_millis) + now;
		return now - start_millis;
	}

	uint32_t seconds() {
		return milliseconds() / 1000;
	}

	void reset() {
		start_millis = millis();
	}

};

interval timer;

boolean backlight_on;

void writeByte(byte b) {
	for (int i = 0; i < 8; i++)
		digitalWrite(bitmap[i], (b & (1 << i))? HIGH: LOW);
	digitalWrite(EN, HIGH);
	delayMicroseconds(1);
	digitalWrite(EN, LOW);
}

void writeCmd(byte b) {
	digitalWrite(RS, 0);
	writeByte(b);
	delayMicroseconds(5000);	// home & clear need long delay
}

void writeChar(byte b) {
	digitalWrite(RS, 1);
	writeByte(b);
	delayMicroseconds(200);
}

void writeStr(const char *s) {
	char c;
	while (c = *s++)
		writeChar(c & 0xff);
}

boolean readTouch() {
	int read = analogRead(A5);
	return read == 1023;
}

void backlightOn() {
	digitalWrite(A0, 255);
	timer.reset();
	backlight_on = true;
}

void backlightOff() {
	digitalWrite(A0, 0);
	backlight_on = false;
}

void setup() {
	Serial.begin(57600);
	
	for (int i = 0; i < 8; i++)
		pinMode(bitmap[i], OUTPUT);
	pinMode(EN, OUTPUT);
	pinMode(RS, OUTPUT);
		
	digitalWrite(RS, 0);
	writeByte(0x30);
	delayMicroseconds(5000);
	writeByte(0x30);
	delayMicroseconds(200);
	writeByte(0x30);
	delayMicroseconds(200);
	writeCmd(0x38);	// set interface length: DL=8 N=2 F=0
	
	// initialise the display ourselves anyway to
	// allow an already-running LCDd to talk to us
	writeCmd(0x08);	// blank screen w/o clearing it
	writeCmd(0x01);	// clear screen
	writeCmd(0x06);	// entry mode set (display shift off)
	writeCmd(0x0c);	// make cursor invisible
	
	// little banner
	writeCmd(0xc3);	 // set cursor position
	writeStr("Mock Pertelian");
	writeCmd(0xdd);	// set cursor position
	writeStr("Steve, 2021");

	backlightOn();	
}

void loop() {
	if (Serial.available() > 0) {
		byte b = Serial.read();
		if (b == 0xfe) {
			while (Serial.available() == 0);
			b = Serial.read();
			if (b == BOFF)
				backlightOff();
			else if (b == BON)
				backlightOn();
			else if (b < 0x20 || b >= 0x80) // only pass thru cursor cmds
				writeCmd(b);
		} else if (b >= 0x20)
			writeChar(b);
	} else if (backlight_on) {
		if (timer.seconds() > ON_TIME)
			backlightOff();
	} else if (readTouch())
		backlightOn();
}
