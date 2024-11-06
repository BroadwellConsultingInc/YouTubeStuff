#include <SerialWombat.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
SerialWombatChip sw;

SerialWombatQuadEnc_18AB Pin6QuadEnc(sw); // Your serial wombat chip may be named something else than sw
SerialWombatDebouncedInput Pin2DebouncedInput(sw); // Your serial wombat chip may be named something else than sw
SerialWombatDebouncedInput Pin5DebouncedInput(sw); // Your serial wombat chip may be named something else than sw
SerialWombatDebouncedInput Pin8DebouncedInput(sw); // Your serial wombat chip may be named something else than sw
SerialWombatPulseOnChange Pin19_PulseOnChange(sw); // Your serial wombat chip may be named something else than sw

void setup() {
  Serial.begin(115200);

  delay(100);
  Wire.begin();
  sw.begin(Wire, sw.find(true));
  sw.digitalWrite(9, LOW);
  sw.pinMode(9, OUTPUT);
  sw.digitalWrite(9, LOW);
  delay(1000); //Give the display time to power up
  display.begin(i2c_Address, true); // Address 0x3C default, initializes wire
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.display();


  Pin6QuadEnc.begin(6, //1st Pin
                    7, //2nd Pin
                    1, //DebouceTime in mS
                    true, //pull ups
                    QE_READ_MODE_t ::QE_ONHIGH_POLL //Mode
                   );
  Pin6QuadEnc.writeMinMaxIncrementTargetPin  ( 65535,//Minimum, 65535 = ignore
      0,//Maximum, 0 = ignore
      1, //Increment
      6 //Target Pin
                                             );
  Pin6QuadEnc.writeFrequencyPeriodmS  ( 1000  );// period in mS to count pulses to calculate frequency


  Pin2DebouncedInput.begin(2,  //Pin
                           1, //Debounce Ms
                           true, // Invert
                           true); // Pull Up enabled

  Pin5DebouncedInput.begin(5,  //Pin
                           1, //Debounce Ms
                           true, // Invert
                           true); // Pull Up enabled

  Pin8DebouncedInput.begin(8,  //Pin
                           1, //Debounce Ms
                           true, // Invert
                           true); // Pull Up enabled

  Pin19_PulseOnChange.begin(19,
                            SW_HIGH,
                            SW_LOW,
                            20,
                            10,
                            true,
                            0,
                            32768);
  Pin19_PulseOnChange.setEntryOnChange(0, 6);
  Pin19_PulseOnChange.setEntryOnIncrease(1, 2);
  Pin19_PulseOnChange.setEntryOnIncrease(2, 5);
  Pin19_PulseOnChange.setEntryOnIncrease(3, 8);


  Pin6QuadEnc.writePublicData(32768);
}

#define NUMBER_OF_VALUES 3
uint16_t values[NUMBER_OF_VALUES] = {100, 100, 100};
int currentPosition = 0;

uint16_t currentValue = 100;
bool refresh = true;
void loop() {

  int32_t newQE = Pin6QuadEnc.writePublicData(32768);

  if (newQE != 32768)
  {
    currentValue -= newQE - 32768;
    refresh = true;
  }

  Pin5DebouncedInput.readTransitionsState(true);
  if (Pin5DebouncedInput.transitions > 0)  // Knob Button Pushed
  {
    values[currentPosition] = currentValue;
  }

{
  bool currentPin2 = Pin2DebouncedInput.readTransitionsState(true);
  if (Pin2DebouncedInput.transitions > 0)
  {
    refresh = true;
    if (Pin2DebouncedInput.transitions == 1 && currentPin2)
    {
      // User is holding button
      currentPosition += Pin2DebouncedInput.transitions;
    }
    else
    {
    currentPosition += Pin2DebouncedInput.transitions/2;  // Divide by 2 because 2 transitions (up and down per push)
    }
    currentPosition %= NUMBER_OF_VALUES;
    currentValue = values[currentPosition];
  }
}
{
  bool currentPin8 = Pin8DebouncedInput.readTransitionsState(true);
  if (Pin8DebouncedInput.transitions > 0)
  {
    refresh = true;
     if (Pin8DebouncedInput.transitions == 1 && currentPin8)
    {
      // User is holding button
      currentPosition -= Pin8DebouncedInput.transitions;
    }
    else
    {
    currentPosition -= Pin8DebouncedInput.transitions/2;  // Divide by 2 because 2 transitions (up and down per push)
    }
    while (currentPosition < 0)
    {
      currentPosition += NUMBER_OF_VALUES;
    }
      
      currentValue = values[currentPosition];
  }
  }
  if (refresh)
  {
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.setTextSize(2);


    for (int i = 0; i < NUMBER_OF_VALUES; ++i)
    {

      if (currentPosition == i)
      {
        display.setTextSize(3);
        display.println(currentValue);
      }
      else
      {
        display.setTextSize(2);
        display.println(values[i]);
      }
    }

    display.display();
    refresh = false;
  }


  delay(5000);
}
