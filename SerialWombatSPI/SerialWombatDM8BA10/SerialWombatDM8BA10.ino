/*
 * This example illustrates segment animation.
 * Effects are cycling automatically, but
 * can be controlled with button if attached
 */

#include <DM8BA10_SW.h>
#include <charset/latin_basic.h>
#include <SerialWombat.h>
#include <Wire.h>
#include "sequences.h"

#define ARRAY_SIZE(a) sizeof(a) / sizeof(a[0])

// pins


#define BUTTON_PIN    0 // set to 0 if no button is attached

// display settings
#define PLACES_COUNT 10
#define REFRESH_RATE 400 // display update speed in ms

#define EFFECT_TEXT "Effect #"
#define LABEL_FRAMES 3

const Sequence* effects[] = { Snake, Spinstar, Valve, Typing, Loading };
const word effectLength[] = { ARRAY_SIZE(Snake), ARRAY_SIZE(Spinstar), ARRAY_SIZE(Valve), ARRAY_SIZE(Typing), ARRAY_SIZE(Loading) };

DM8BA10_SW* LCD;

volatile static byte effectIndex = 0;
volatile int8_t frame = -LABEL_FRAMES; // draw label for three frames

SerialWombatChip sw;
SerialWombatSPI spi(sw);
void setup()
{   
  Wire.begin();
  delay(100);

  sw.begin(0x6B);  // Assume a Serial Wombat chip on Address 0x6B.  You may need to change this.
  spi.begin(18, // Clock pin
            0, // SPI Mode 0
            17,// MOSI on pin 17
            255, // No MISO pin
            19);  // CS on pin 19
  
  LCD = new DM8BA10_SW(new LatinBasicCharset(), spi);
  LCD->backlight();

  if (BUTTON_PIN)
  {
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), switchEffect, RISING);
  }
}

void loop()
{
  static uint32_t lastUpd = 0;

  auto nowMs = millis();

  if (nowMs - lastUpd > REFRESH_RATE)
  {
    if (frame++ < 0)
    {
      LCD->println(EFFECT_TEXT + String(effectIndex + 1));
    }
    else
    {  
      if (frame >= effectLength[effectIndex])
      {
        frame = -LABEL_FRAMES;
        
        if (++effectIndex >= ARRAY_SIZE(effects))
          effectIndex = 0;

         LCD->allSegments(false); // display blank screen for a frame
      }
      else
      {
        auto chr = effects[effectIndex][frame];
        
        for (byte i = 0; i < PLACES_COUNT; i++)
        {
          LCD->drawChar(chr, i);
        }
      }
    }

    lastUpd = nowMs;
  }
}

void switchEffect()
{
  // reduce jitter
  static auto contactBounceReductor = millis();
  auto nowMs = millis();

  if (nowMs - contactBounceReductor > 200)
  {
    if (effectIndex++ >= ARRAY_SIZE(effects))
      effectIndex = 0;

    frame = -LABEL_FRAMES;
    
    contactBounceReductor = nowMs;
  }
}
