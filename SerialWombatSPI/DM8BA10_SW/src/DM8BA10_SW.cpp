/*******************************************************************************
Written by Ilya 'road-t' Annikov © 2022

Based on :https://github.com/radenko/ht1622-arduino

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/

#include "DM8BA10_SW.h"

DM8BA10_SW::DM8BA10_SW(Charset* charset, SerialWombatSPI& spi_):spi(spi_),charset(charset)
{
    // pin setup

    // turn on oscillator
    sendCommand(CMD_OSC_ON);

    // clear the RAM before turning on CLD
    clearDisplay();

    // turn on LCD
    sendCommand(CMD_LCD_ON);
}

DM8BA10_SW::~DM8BA10_SW()
{
  if (charset)
    delete charset;
}

void DM8BA10_SW::clearDisplay()
{
    curPos = 0;
    this->allSegments(false);
}

byte DM8BA10_SW::setPos(int8_t newPos)
{
  if (newPos < 0)
    newPos = displaySize + newPos;

  if (!newPos)
    newPos = 0;
  
  if (newPos >= displaySize)
    newPos = displaySize - 1;
    
  curPos = newPos;
  
  return curPos;
}

void DM8BA10_SW::systemOscillator(bool on)
{
  sendCommand(on ? CMD_OSC_ON : CMD_OSC_OFF);
}

void DM8BA10_SW::LCD(bool on)
{
  sendCommand(on ? CMD_LCD_ON : CMD_LCD_OFF);
}

void DM8BA10_SW::backlight(bool on)
{
	/*TODO
    if (backlightPin > -1)
        digitalWrite(backlightPin, on ? HIGH : LOW);
	*/
}

void DM8BA10_SW::allSegments(bool on)
{
    // feel all the memory of display controller
    for (byte addr = 0; addr < 0x3F; addr += 4)
    {
        sendData(addr, (on ? 0xFFFF : 0x0));
    }
    
    delayMicroseconds(WRITE_DELAY);
}

void DM8BA10_SW::drawChar(word ch, byte pos)
{
  sendData(places[pos], ch);
}

void DM8BA10_SW::setChar(const byte ch, int8_t pos)
{
    word character = charset->Char(ch - charset->startingOffset());

    sendData(places[(pos < 0) ? curPos : pos], character, 16);
}

byte DM8BA10_SW::print(String text, int8_t pos)
{
  if (pos > -1)
      curPos = pos;
  
  byte written = 0;
  
  for (; curPos < displaySize; curPos++)
  {
    setChar(text[written++]);

    if (written >= text.length())
      break;
  }

  if (curPos >= displaySize)
    curPos = 0;

  return written;
}

void DM8BA10_SW::println(String text, Padding padType)
{
    auto resultString = padString(text, padType);
    
    for (curPos = 0; curPos < displaySize; curPos++)
    {
      setChar(resultString[curPos]);

      if (padType == Right && curPos >= resultString.length())
        break;
    }

    if (curPos == displaySize)
      curPos = 0;
}


void DM8BA10_SW::scroll(String text, word start)
{
    curPos = 0;
    word stringPos = start;

    do
    {
      setChar(text[stringPos++]);

      // if the string has ended, just add the space
      // and start all over again
      if (stringPos > text.length())
      {
        setChar(charset->startingOffset());
        
        stringPos = 0;
      }
    } while (++curPos < displaySize);
    
    if (curPos == displaySize)
      curPos = 0;
}

void DM8BA10_SW::resetPoints()
{
  // fill all the decimals-related
  // memory with zeroes
  sendData(pointClusters[2], 0);
  sendData(pointClusters[0], 0);
}

void DM8BA10_SW::setPoint(byte index)
{
  resetPoints();
  point(index);
}

void DM8BA10_SW::point(byte index, bool on)
{
  if (!pointClustersCount)
    return;
  
  byte bit = on ? 1 << ((index + pointClustersCount) % pointClustersCount) : 0;
  byte address = static_cast<byte>(floor(index / pointClustersCount)) * 2 + pointClusters[0];

  // exchange bits 0 and 2, to get continous index
  if (bit != 2)
    bit ^= 5;

  // just write accurately to 4-bi memory segment
  sendData(address, bit, 4);
}

String DM8BA10_SW::padString(String text, Padding padType)
{
    word textLen = text.length();
    byte padSize = 0;

    if (textLen < displaySize)
    {
      // here we gonna place prepared string
      auto buffer = new char[displaySize];
      
      // fill new string with spaces
      memset(buffer, charset->startingOffset(), displaySize);
    
      padSize = displaySize - textLen;
      
      if (padType == Both)
      {
        // if string length is odd it'll be biased to the right
        padSize = round(padSize / 2);
      }

      // copy source string to resulting, skipping padding bytes,
      memcpy(buffer + (padType != Right ? padSize : 0), text.c_str(), textLen);

      auto resultString = String(buffer);

      delete[] buffer;

      return resultString;
    }
    
   // avoid redundant copying of non-altered string
   return text;
}

// Private functions
void DM8BA10_SW::beginTransfer()
{
//    digitalWrite(csPin, LOW);
}

void DM8BA10_SW::endTransfer()
{
	/*
    digitalWrite(csPin, HIGH);
    */
}

void DM8BA10_SW::sendCommand(unsigned char cmd)
{  
    this->beginTransfer();
    
    this->sendBits(BIT_PREFIX_CMD, 3); // command code
    this->sendBits(cmd, 8); // command
    this->sendBits(1, 1,true); // padding bit, doesn't mean anything
    
    this->endTransfer();
}

void DM8BA10_SW::sendData(byte addr, word sdata, byte bits)
{
    this->beginTransfer();
    
    this->sendBits(BIT_PREFIX_DATA, 3); // data prefix
    this->sendBits(addr, 6); // address
    this->sendBits(sdata, bits,true); // data
    
    this->endTransfer();
}

void DM8BA10_SW::sendBits(word data, byte bits,bool endTransfer)
{
	uint16_t udata = (uint16_t) data;
	udata <<= (16-bits);
	uint8_t swapped[2];
	swapped[0] = (uint8_t)(udata >>8);
	swapped[1] = (uint8_t)udata;
	spi.transferPacketUpTo32Bits(swapped,NULL,bits,!endTransfer);
	/*
  word mask;
  mask = 1 << (bits - 1);
  
  for (byte i = bits; i > 0; i--)
  {
      // begin
      digitalWrite(this->wrPin, LOW);
      delayMicroseconds(WRITE_DELAY);

      // write
      data & mask ? digitalWrite(this->dataPin, HIGH) : digitalWrite(this->dataPin, LOW);
      delayMicroseconds(WRITE_DELAY);

      // end
      digitalWrite(this->wrPin, HIGH);
      delayMicroseconds(WRITE_DELAY);
      
      data <<= 1;
  }
  
  delayMicroseconds(WRITE_DELAY);
  */
}

uint16_t DM8BA10_SW::reverse_bits_16(uint16_t num) {
    uint16_t reversed = 0;
    for (int i = 0; i < 16; i++) {
        // Shift reversed left to make room, then OR with the LSB of num
        reversed = (reversed << 1) | (num & 1);
        num >>= 1; // Move to the next bit in num
    }
    return reversed;
}
