#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>

#include <SerialWombat.h>

SerialWombatChip sw;
 //Put this line before setup()
                SerialWombatQuadEnc_18AB Pin19QuadEnc(sw); // Your serial wombat chip may be named something else than sw
//Put this line before setup()
                SerialWombatPWM_18AB Pin1PWM(sw); // Your serial wombat chip may be named something else than sw
void setup() {
#ifdef ARDUINO_ESP8266_GENERIC
  Wire.begin(2, 0); // ESP-01 - SDA GPIO2, SCL GPIO0
#else
  Wire.begin();
#endif

  Serial.begin(115200);

  delay(100);

  sw.begin(Wire, sw.find(true));

  
               
                //Add this line to  setup():
                Pin19QuadEnc.begin(19, //1st Pin
                18, //2nd Pin
                0, //DebouceTime in mS
                true, //pull ups
                 QE_READ_MODE_t ::QE_ONHIGH_INT //Mode
);

            Pin19QuadEnc.writeMinMaxIncrementTargetPin  ( 65535,//Minimum, 65535 = ignore
0,//Maximum, 0 = ignore
1, //Increment
19 //Target Pin 
);    

sw.pinMode(0,OUTPUT);
sw.digitalWrite(0,LOW);
Pin19QuadEnc.writeFrequencyPeriodmS ( 1000  );// period in mS to count pulses to calculate frequency  



                
                //Add this to  setup():
                                Pin1PWM.begin(1, //Pin Number
                                0, // Duty Cycle (out of 65535)
                                false); // Invert (subtracts duty cycle from 65535)

                               
 Pin1PWM.writePeriod_uS(1500); // Set period in uS
//put this line in setup.  Make this the last line after other
                    // Output Scaling configurations for this pin
                    Pin1PWM.writeScalingEnabled(true, //Enabled
                    18); //DataSource
//put this line in setup.
                    Pin1PWM.writePID(4000, //Kp
                    30000, //Ki
                    0, //Kd
                    1200,  //Target Value
                    SerialWombatAbstractScaledOutput::Period::PERIOD_64mS);//Period

}

void loop() {
  Serial.println(sw.readPublicData(18));
}
