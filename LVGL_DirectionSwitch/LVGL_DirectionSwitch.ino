#include <lvgl.h>
#include <SerialWombat.h>

PCB0041_Remcon remcon;
SerialWombatChip cyd64,cyd65;
SerialWombatMatrixKeypad swMatrixKeypad(cyd64);



uint32_t last_lv_tick_ms = 0;
extern void cgpt_ex_setup_calculator(void);
extern void cgpt_ex_loop_task();
extern void lvgl_setup();
void setup() {
  Serial.begin(115200);
 


  Wire.begin(22,27); // Set custom SDA and SCL pins based on Blue, Yellow wire (different from RandomNerdDisplay example)
  
  delay(100);
  cyd64.begin(0x64);
  cyd64.begin(0x65);
  
  swMatrixKeypad.begin(0,0,1,2,3,4,5,6,7);
  lvgl_setup();

  Serial.println("LVGL SETUP COMPLETE!");
  delay(1000);
  //Put LVGL init here
    cgpt_ex_setup_calculator();
   
}


 void loop()
{
  // LVGL tick handling
      uint32_t now = millis();
  uint32_t elapsed = now - last_lv_tick_ms;
  if (elapsed) {
    lv_tick_inc(elapsed);
    last_lv_tick_ms = now;
  }
  lv_timer_handler();
  
  delay(5);
}
 
