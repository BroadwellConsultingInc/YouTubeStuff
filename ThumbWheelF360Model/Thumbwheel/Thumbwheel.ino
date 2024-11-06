#include <SerialWombat.h>

SerialWombatChip SW;

SerialWombatAnalogInput thumbWheel0(SW),thumbWheel1(SW),thumbWheel2(SW);
void setup() {
  Wire.begin();
  Serial.begin(115200);
  delay(200);
  SW.begin(Wire,0x6D); // Set your I2C address here...
  thumbWheel0.begin(1); // Pin 1
  thumbWheel1.begin(2); // Pin 2
  thumbWheel2.begin(3); // Pin 3
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  uint16_t adc = thumbWheel0.readPublicData();
  if (adc > 25000)
  {
    Serial.println("TW0 - R");
  }
  else if (adc > 15000)
  {
    Serial.println("TW0 - L");
  }
  else if (adc > 5000)
  {
    Serial.println("TW0 - Push");
  }

  adc = thumbWheel1.readPublicData();
  if (adc > 25000)
  {
    Serial.println("TW1 - R");
  }
  else if (adc > 15000)
  {
    Serial.println("TW1 - L");
  }
  else if (adc > 5000)
  {
    Serial.println("TW1 - Push");
  }

 adc = thumbWheel2.readPublicData();
  if (adc > 25000)
  {
    Serial.println("TW2 - R");
  }
  else if (adc > 15000)
  {
    Serial.println("TW2 - L");
  }
  else if (adc > 5000)
  {
    Serial.println("TW2 - Push");
  }

}
