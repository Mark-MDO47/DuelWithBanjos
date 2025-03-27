/* Author: https://github.com/Mark-MDO47  March 20, 2025
 *  https://github.com/Mark-MDO47/DuelWithBanjos
 *
 */

/*
   Copyright 2025 Mark Olson

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

/*
 * This code is being developed to provide DO_PINS_PWM LED Eye control for the
 * "DuelWithBanjos" project.
 *
 * It is a work-in-progress and incomplete at this time.
 *
 */

#include "do_pins_pwm.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// pin definitions - name must be must be g_pwm_pin_info, must be one row for each PWM LED output pin
//
pwm_pin_info g_pwm_pin_info[DO_PINS_PWM_NUM_PINS] = {
  { .pin_num=18, .ptrn_step_ptr = (pwm_led_ptrn_step *)0, .idx_curr_step=0, .curr_pwm_val=0, .prev_pwm_val = 0xFFFF, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_tick_incr = 0, .scaledtm_next_tick=0, .scaledtm_next_step=0 },
  { .pin_num=19, .ptrn_step_ptr = (pwm_led_ptrn_step *)0, .idx_curr_step=0, .curr_pwm_val=0, .prev_pwm_val = 0xFFFF, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_tick_incr = 0, .scaledtm_next_tick=0, .scaledtm_next_step=0 },
  { .pin_num=32, .ptrn_step_ptr = (pwm_led_ptrn_step *)0, .idx_curr_step=0, .curr_pwm_val=0, .prev_pwm_val = 0xFFFF, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_tick_incr = 0, .scaledtm_next_tick=0, .scaledtm_next_step=0 },
  { .pin_num=33, .ptrn_step_ptr = (pwm_led_ptrn_step *)0, .idx_curr_step=0, .curr_pwm_val=0, .prev_pwm_val = 0xFFFF, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_tick_incr = 0, .scaledtm_next_tick=0, .scaledtm_next_step=0 }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// pwm LED pattern definitions - name can be anything, can be as many pattern definitions as desired,
//         can have as many steps per pattern as desired
//
pwm_led_ptrn_step pwm_ptrn_open_eye[] = { 
  { .start_set_pwm=0,                     .step_incr=1,  .step_time=1757, .tick_time=5, .tick_pwm= 2},
  { .start_set_pwm=DO_PINS_PWM_NO_CHANGE, .step_incr=-1, .step_time=1000, .tick_time=5, .tick_pwm=-7}
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// setup()
void setup() {
  Serial.begin(115200);         // this serial communication is for general debug; set the USB serial port to 115,200 baud
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  delay(3000);
  Serial.println(""); // print a blank line in case there is some junk from power-on
  Serial.println("starting LEDC_steps");

  if (do_pins_pwm_init(DO_PINS_PWM_FREQ, DO_PINS_PWM_VAL_NUM_BITS, 0)) {
    while (1) ;
  } // end if error in initialization

} // end setup()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// loop()
void loop() {
  static uint32_t first_time = 1;

  if (0 != first_time) {
    for (int pin_idx = 0; pin_idx < NUMOF(g_pwm_pin_info); pin_idx += 1) {
      do_pin_pwm_init_ptrn(pin_idx, pwm_ptrn_open_eye, 0, TIME_SCALE_EQUAL + pin_idx*2, 0);
    } // end for each pin_idx
    first_time = 0;
  } // end if start next pattern time scale

  do_pins_pwm();
  delay(1);

} // end loop()