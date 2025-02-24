#include <SerialWombat.h>
uint32_t updateTime = 0;
SerialWombatChip sw;

     //Put this line before setup()
                SerialWombatQuadEnc_18AB Pin19QuadEnc(sw); // Your serial wombat chip may be named something else than sw
//Put this line before setup()
                SerialWombatHBridge_18AB Pin1HBridge(sw); // Your serial wombat chip may be named something else than sw
                
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

Pin19QuadEnc.writeFrequencyPeriodmS ( 1000  );// period in mS to count pulses to calculate frequency  


                
                //Add this to  setup():
                                Pin1HBridge.begin(1, //Pin Number
                                0, // Second Pin
                                1500,// PWM Period in uS
                                HBRIDGE_OFF_BOTH_LOW);   // Driver
                               
//put this line in setup.  Make this the last line after other
                    // Output Scaling configurations for this pin
                    Pin1HBridge.writeScalingEnabled(true, //Enabled
                    19); //DataSource
//Put the interpolation table in user RAM.
               {
                    uint16_t xytable[] = {
                0, 0,
32758, 27768,
32759, 32768,
32967, 32768,
32968, 37768,
65535, 65535,
}; 
 
            sw.writeUserBuffer(0, (uint8_t*)xytable, 24);
}

            Pin1HBridge.Enable2DLookupOutputScaling(0);
          
            //put this line in setup.
                    Pin1HBridge.writePID(20000, //Kp
                    9311, //Ki
                    15000, //Kd
                    13000,  //Target Value
                    SerialWombatAbstractScaledOutput::Period::PERIOD_8mS);//Period
      updateTime = millis();
}

uint16_t targetPosition  = 5000;
void loop() {

  if (millis() > updateTime + 5000)
  {
    targetPosition += 500;
  
    Pin1HBridge.writeScalingTargetValue (targetPosition);
    updateTime = millis();
  }
  Serial.println(Pin19QuadEnc.readPublicData());
}
