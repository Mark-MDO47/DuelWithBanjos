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
 * This code is being developed to provide PWM LED Eye control for the
 * "DuelWithBanjos" project.
 *
 * It is a work-in-progress and incomplete at this time.
 *
 */


uint32_t g_time_msec_now = 0;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// constants for LEDC PWM library to control the LED Eyes of the Banjo Players
#define PWM_NO_CHANGE 0xFFFF  // .start_set_pwm value for no change to current pwm value
#define PWM_NUM_PINS 4        // number of LED pins to control
#define PWM_FREQ 500          // Arduino Uno is ~490 Hz. ESP32 example uses 5,000Hz
#define PWM_VAL_NUM_BITS 8    // Use same resolution as Uno (8 bits, 0-255) but ESP32 can go up to 16 bits (some versions less)
#define PWM_MAX_VALUE ((1 << (PWM_VAL_NUM_BITS-1)) - 1) // max pwm value (255 if PWM_VAL_NUM_BITS is 8)
#define TIME_SCALE_EQUAL 64   // pwm_pin_info factors for time_ entries: millisec = (time_<whatever> / TIME_SCALE_EQUAL)
                              // this allows a relatively smooth scaling of time by setting 

typedef struct {
  uint16_t total_time;    // time for this step in msec
  uint16_t start_set_pwm; // if not PWM_NO_CHANGE, set pwm value to this
  uint8_t  tick_time;     // how long (millisec) between each tick
  int8_t   tick_pwm;      // how far and what direction for pwm value each tick_time
} pwm_led_ptrn_step;

typedef struct {
  pwm_led_ptrn_step *step_ptr; // pointer to this step
  int16_t step_incr;           // if > 0, increment step counter by this
                               // if zero, go to beginning; if < 0, go to (-val) steps past beginning
} pwm_led_ptrn_step_cmd;

typedef struct {
  uint8_t pin_num;             // pin number executing this pattern
  uint8_t idx_curr_step;       // step within ptrn_step_cmd_ptr
  pwm_led_ptrn_step_cmd * ptrn_step_cmd_ptr; // pointer to ptrn_step_cmd
  uint16_t curr_pwm_val;      // pwm intensity from 0 to PWM_MAX_VALUE
  uint32_t scale_factor;      // to stretch time. scale_factor == TIME_SCALE_EQUAL means no stretch
  uint32_t time_next_tick;    // time (millisec * TIME_SCALE_EQUAL) to start next tick
  uint32_t time_end_step;     // time (millisec * TIME_SCALE_EQUAL) to go to next step
} pwm_pin_info;

pwm_pin_info g_pwm_pin_info[PWM_NUM_PINS] = {
  { .pin_num=18, .idx_curr_step=0, .ptrn_step_cmd_ptr = (pwm_led_ptrn_step_cmd *)0, .curr_pwm_val=0, .scale_factor=TIME_SCALE_EQUAL, .time_next_tick=0, .time_end_step=0 },
  { .pin_num=19, .idx_curr_step=0, .ptrn_step_cmd_ptr = (pwm_led_ptrn_step_cmd *)0, .curr_pwm_val=0, .scale_factor=TIME_SCALE_EQUAL, .time_next_tick=0, .time_end_step=0 },
  { .pin_num=32, .idx_curr_step=0, .ptrn_step_cmd_ptr = (pwm_led_ptrn_step_cmd *)0, .curr_pwm_val=0, .scale_factor=TIME_SCALE_EQUAL, .time_next_tick=0, .time_end_step=0 },
  { .pin_num=33, .idx_curr_step=0, .ptrn_step_cmd_ptr = (pwm_led_ptrn_step_cmd *)0, .curr_pwm_val=0, .scale_factor=TIME_SCALE_EQUAL, .time_next_tick=0, .time_end_step=0 },
};


pwm_led_ptrn_step pwm_ptrn_open_eye = { .total_time=1000, .start_set_pwm=PWM_NO_CHANGE, .tick_time=5, .tick_pwm=7 };


pwm_led_ptrn_step_cmd g_pwm_ptrn_blink_array[] = {
   { .step_ptr=&pwm_ptrn_open_eye, .step_incr=0 }
}; // pwm_ptrn_blink


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_pin_pwm()
void do_pin_pwm() {
  for (int pin_idx = 0; pin_idx < PWM_NUM_PINS; pin_idx += 1) {
    if (g_pwm_pin_info[pin_idx].time_end_step <= g_time_msec_now) {
      // go to next step
    } else if (g_pwm_pin_info[pin_idx].time_next_tick <= g_time_msec_now) {
      // do next tick
    } else {
      ledcWrite(g_pwm_pin_info[pin_idx].pin_num, 5);
    }
  }
} // end do_pin_pwm()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// setup()
void setup() {
  // connect to Banjo Player LED eyes and initially turn off
  for (int pin_idx = 0; pin_idx < PWM_NUM_PINS; pin_idx += 1) {
    // connect a pin to a channel at a pwm value frequency, and a PWM resolution (1 - 16 bits)
    if (!ledcAttach(g_pwm_pin_info[pin_idx].pin_num, PWM_FREQ, PWM_VAL_NUM_BITS)) {
      Serial.println("ERROR - could not attach pin to LEDC library");
    }
    ledcWrite(g_pwm_pin_info[pin_idx].pin_num, 0); // initially set to off
  } // end connect all pins to Banjo Player LED eyes
} // end setup()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// loop()
void loop() {
  int pin_idx; // loop counter for the pins

  g_time_msec_now = millis();


} // end loop()