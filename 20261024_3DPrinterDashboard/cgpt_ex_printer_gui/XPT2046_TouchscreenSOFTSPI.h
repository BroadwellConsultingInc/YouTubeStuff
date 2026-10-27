/* Touchscreen library for XPT2046 Touch Controller Chip
 * Copyright (c) 2015, Paul Stoffregen, paul@pjrc.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef cgpt_ex_XPT2046_TouchscreenSOFTSPI_h_
#define cgpt_ex_XPT2046_TouchscreenSOFTSPI_h_

#include "Arduino.h"

#include "SoftSPI.h"

#if ARDUINO < 10600
#error "Arduino 1.6.0 or later (SPI library) is required"
#endif



class cgpt_ex_touchPoint {
public:
	cgpt_ex_touchPoint(void) : x(0), y(0), z(0) {}
	cgpt_ex_touchPoint(int16_t x, int16_t y, int16_t z) : x(x), y(y), z(z) {}
	bool operator==(cgpt_ex_touchPoint p) { return ((p.x == x) && (p.y == y) && (p.z == z)); }
	bool operator!=(cgpt_ex_touchPoint p) { return ((p.x != x) || (p.y != y) || (p.z != z)); }
	int16_t x, y, z;
};

class XPT2046_TouchscreenSOFTSPI {
public:
	constexpr XPT2046_TouchscreenSOFTSPI(uint8_t cspin, uint8_t tirq=255)
		: csPin(cspin), tirqPin(tirq) { }
	bool begin(SoftSPI* touchscreenSPI);
	cgpt_ex_touchPoint getPoint(SoftSPI* touchscreenSPI);
	bool tirqTouched();
	bool touched(SoftSPI *touchscreenSPI);
	void readData(SoftSPI *touchscreenSPI,uint16_t *x, uint16_t *y, uint8_t *z);
	bool bufferEmpty();
	uint8_t bufferSize() { return 1; }
	void setRotation(uint8_t n) { rotation = n % 4; }
// protected:
	volatile bool isrWake=true;

private:
	void update(SoftSPI *touchscreenSPI);
	uint8_t csPin, tirqPin, rotation=1;
	int16_t xraw=0, yraw=0, zraw=0;
	uint32_t msraw=0x80000000;
};

#ifndef cgpt_ex_softTouchIsrPrefix
  #if defined(ESP8266)
    #define cgpt_ex_softTouchIsrPrefix ICACHE_RAM_ATTR
  #elif defined(ESP32)
    // TODO: should this also be ICACHE_RAM_ATTR ??
    #define cgpt_ex_softTouchIsrPrefix IRAM_ATTR
  #else
    #define cgpt_ex_softTouchIsrPrefix
  #endif
#endif

#endif
