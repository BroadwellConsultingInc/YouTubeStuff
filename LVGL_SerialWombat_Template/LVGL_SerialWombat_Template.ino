#include <lvgl.h>
#include <SerialWombat.h>

SerialWombatChip sw;
SerialWombatAnalogInput  realSlider(sw),realSlider2(sw)  ;
SerialWombatServo swServo(sw);


static uint32_t last_lv_tick_ms = 0;

volatile uint16_t swFramesRun = 0;
volatile uint16_t swPotentiometer = 0;
volatile uint16_t swPotentiometer2 = 0;
volatile uint16_t swServoPosition = 0;


void setup() {
  Serial.begin(115200);
 
  // Start LVGL
  lvgl_setup();

  Wire.begin(22,27); // Set custom SDA and SCL pins based on Blue, Yellow wire (different from RandomNerdDisplay example)
  
  delay(100);
  sw.begin(Wire,sw.find(true));
  realSlider.begin(1);
  realSlider2.begin(2);
  swServo.attach(3);
  //Put LVGL init here
   cgpt_ex_ui_init();
}

volatile int16_t  swTemperatureC_x10;
void loop() {
  // LVGL tick handling
      uint32_t now = millis();
  uint32_t elapsed = now - last_lv_tick_ms;
  if (elapsed) {
    lv_tick_inc(elapsed);
    last_lv_tick_ms = now;
  }
   cgpt_ex_ui_loop();
  delay(5);

  //Put logic handling here
  swFramesRun = sw.readPublicData( SerialWombatDataSource::SW_DATA_SOURCE_FRAMES_RUN_LSW);  
  swPotentiometer = realSlider.readPublicData();
  swPotentiometer2 = realSlider2.readPublicData();
  swServo.writePublicData(swServoPosition);
}
