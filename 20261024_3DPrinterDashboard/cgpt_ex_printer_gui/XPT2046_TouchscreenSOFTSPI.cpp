#include "cgpt_ex_config.h"
#if cgpt_ex_enableCydSoftTouch
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

#include "XPT2046_TouchscreenSOFTSPI.h"

#define Z_THRESHOLD     400
#define Z_THRESHOLD_INT	75
#define MSEC_THRESHOLD  3


static XPT2046_TouchscreenSOFTSPI 	*cgpt_ex_softTouchIsrObject;
void cgpt_ex_softTouchIsr(void);

bool XPT2046_TouchscreenSOFTSPI::begin(SoftSPI *touchscreenSPI)
{
	touchscreenSPI->begin();
    touchscreenSPI->setDataMode(SPI_MODE0);
    touchscreenSPI->setBitOrder(MSBFIRST);
	pinMode(csPin, OUTPUT);
	digitalWrite(csPin, HIGH);
	if (255 != tirqPin) {
		pinMode( tirqPin, INPUT );
		cgpt_ex_softTouchIsrObject = this;
        attachInterrupt(digitalPinToInterrupt(tirqPin), cgpt_ex_softTouchIsr, FALLING);
	}
	return true;
}

cgpt_ex_softTouchIsrPrefix
void cgpt_ex_softTouchIsr( void )
{
	XPT2046_TouchscreenSOFTSPI *o = cgpt_ex_softTouchIsrObject;
	if (o) o->isrWake = true;
}

cgpt_ex_touchPoint XPT2046_TouchscreenSOFTSPI::getPoint(SoftSPI *touchscreenSPI)
{
	update(touchscreenSPI);
	return cgpt_ex_touchPoint(xraw, yraw, zraw);
}

bool XPT2046_TouchscreenSOFTSPI::tirqTouched()
{
	return (isrWake);
}

bool XPT2046_TouchscreenSOFTSPI::touched(SoftSPI *touchscreenSPI)
{
	update(touchscreenSPI);
	return (zraw >= Z_THRESHOLD);
}

void XPT2046_TouchscreenSOFTSPI::readData(SoftSPI *touchscreenSPI,uint16_t *x, uint16_t *y, uint8_t *z)
{
	update(touchscreenSPI);
	*x = xraw;
	*y = yraw;
	*z = zraw;
}

bool XPT2046_TouchscreenSOFTSPI::bufferEmpty()
{
	return ((millis() - msraw) < MSEC_THRESHOLD);
}

static int16_t besttwoavg( int16_t x , int16_t y , int16_t z ) {
  int16_t da, db, dc;
  int16_t reta = 0;
  if ( x > y ) da = x - y; else da = y - x;
  if ( x > z ) db = x - z; else db = z - x;
  if ( z > y ) dc = z - y; else dc = y - z;

  if ( da <= db && da <= dc ) reta = (x + y) >> 1;
  else if ( db <= da && db <= dc ) reta = (x + z) >> 1;
  else reta = (y + z) >> 1;   //    else if ( dc <= da && dc <= db ) reta = (x + y) >> 1;

  return (reta);
}

// TODO: perhaps a future version should offer an option for more oversampling,
//       with the RANSAC algorithm https://en.wikipedia.org/wiki/RANSAC

void XPT2046_TouchscreenSOFTSPI::update(SoftSPI *touchscreenSPI)
{
	int16_t data[6];

	if (!isrWake) return;
	uint32_t now = millis();
	if (now - msraw < MSEC_THRESHOLD) return;

	// Pure software SPI; no hardware beginTransaction.
	digitalWrite(csPin, LOW);
	touchscreenSPI->transfer(0xB1 /* Z1 */);
	int16_t z1 = touchscreenSPI->transfer16(0xC1 /* Z2 */) >> 3;

	int z = z1 + 4095;

	int16_t z2 = touchscreenSPI->transfer16(0x91 /* X */) >> 3;

	z -= z2;
	if (z >= Z_THRESHOLD) {

		touchscreenSPI->transfer16(0x91 /* X */); // dummy X measure, 1st is always noisy
		data[0] = touchscreenSPI->transfer16(0xD1 /* Y */) >> 3;
		data[1] = touchscreenSPI->transfer16(0x91 /* X */) >> 3; // make 3 x-y measurements
		data[2] = touchscreenSPI->transfer16(0xD1 /* Y */) >> 3;
		data[3] = touchscreenSPI->transfer16(0x91 /* X */) >> 3;
	}
	else data[0] = data[1] = data[2] = data[3] = 0;	// Compiler warns these values may be used unset on early exit.

	data[4] = touchscreenSPI->transfer16(0xD0 /* Y */) >> 3; // Last Y touch power down
	data[5] = touchscreenSPI->transfer16(0) >> 3;
	digitalWrite(csPin, HIGH);
	// Pure software SPI; no hardware endTransaction.
	if (z < 0) z = 0;
	if (z < Z_THRESHOLD) { //	if ( !touched ) {
		zraw = 0;
		if (z < Z_THRESHOLD_INT) { //	if ( !touched ) {
			if (255 != tirqPin) isrWake = false;
		}
		return;
	}
	zraw = z;

	// Average pair with least distance between each measured x then y
	int16_t x = besttwoavg( data[0], data[2], data[4] );
	int16_t y = besttwoavg( data[1], data[3], data[5] );

	if (z >= Z_THRESHOLD) {
		msraw = now;	// good read completed, set wait
		switch (rotation) {
		  case 0:
			xraw = 4095 - y;
			yraw = x;
			break;
		  case 1:
			xraw = x;
			yraw = y;
			break;
		  case 2:
			xraw = y;
			yraw = 4095 - x;
			break;
		  default: // 3
			xraw = 4095 - x;
			yraw = 4095 - y;
		}
	}
}

#endif
