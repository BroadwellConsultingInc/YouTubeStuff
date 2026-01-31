#include <SerialWombat.h>

SerialWombatChip sw60;

#define GRIPPER_RELEASE_POSITION 1000
//Put this line before setup()
SerialWombatServo_18AB gripperServo(sw60); // Your serial wombat chip may be named something else than sw
SerialWombatAnalogInput gripperCurrent(sw60); // Your serial wombat chip may be named something else than sw
void setup() {
#ifdef ARDUINO_ESP8266_GENERIC
  Wire.begin(2, 0); // ESP-01 - SDA GPIO2, SCL GPIO0
#else
  Wire.begin();
#endif

  Serial.begin(115200);

  delay(100);

  sw60.begin(Wire, 0x60);

  releaseGrip();

  //Put this line before setup()

  //Add this line to  setup():
  gripperCurrent.begin(4, //Pin Number
                       16,  // Samples per Average
                       65408,//Filter Constant
                       AnalogInputPublicDataOutput::AnalogInputPublicDataOutput_Averaged); //Public data output

}

void releaseGrip()
{
  //Add this line to  setup():
  gripperServo.attach(0,//Pin
                      500, //Minimum Pulse Time
                      2500, //MaximumPulse time
                      false);  //Reverse//put this line in setup.

  gripperServo.writePublicData(GRIPPER_RELEASE_POSITION);

}
void grip()
{



  //Add this line to  setup():
  gripperServo.attach(0,//Pin
                      500, //Minimum Pulse Time
                      2500, //MaximumPulse time
                      false);  //Reverse//put this line in setup.
  gripperServo.writeScalingTargetValue(33600);
  gripperServo.writeRamp(100, //Slow Increment
                         350, //Fast/slow threshold
                         1500, //Fast Increment
                         SerialWombatAbstractScaledOutput::Period::PERIOD_32mS,//Period
                         SerialWombatAbstractScaledOutput::RampMode::RAMP_MODE_BOTH); // Ramp mode
  //put this line in setup.  Make this the last line after other
  // Output Scaling configurations for this pin
  gripperServo.writeScalingEnabled(true, //Enabled
                                   4); //DataSource

}
void loop() {
  delay(10000);
  grip();
  delay(10000);
  releaseGrip();
}
