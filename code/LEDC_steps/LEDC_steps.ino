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

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// constants for LEDC PWM library to control the LED Eyes of the Banjo Players
#define PWM_NO_CHANGE 0xFFFF  // .start_set_pwm value for no change to current pwm value
#define PWM_NUM_PINS 4        // number of LED pins to control
#define PWM_FREQ 500          // Arduino Uno is ~490 Hz. ESP32 example uses 5,000Hz
#define PWM_VAL_NUM_BITS 8    // Use same resolution as Uno (8 bits, 0-255) but ESP32 can go up to 16 bits (some versions less)
#define PWM_MAX_VALUE ((1 << (PWM_VAL_NUM_BITS-1)) - 1) // max pwm value (255 if PWM_VAL_NUM_BITS is 8)
#define TIME_SCALE_EQUAL 64   // pwm_pin_info factors for time_ entries: millisec = (time_<whatever> / TIME_SCALE_EQUAL)
                              // this allows a relatively smooth scaling of time by setting
                              // 2^32millisec / 64 is about 18 hours, so that will exceed any gig time for the banjo players

typedef struct {
  uint16_t start_set_pwm; // if not PWM_NO_CHANGE, set pwm value to this ??? FIXME TODO should this be in the struct??? or pwm_pin_info???
  uint16_t step_time;     // how long (millisec B4 scaling) until this step is complete
  uint16_t  tick_time;    // how long (millisec B4 scaling) between each tick
  int16_t   tick_pwm;     // how far and what direction for pwm value each tick_time
} pwm_led_ptrn_step;

typedef struct {
  pwm_led_ptrn_step *step_ptr; // pointer to this step
  int16_t step_incr;           // if > 0, increment step counter by this
                               // if zero, go to beginning; if < 0, go to (-val) steps past beginning
} pwm_led_ptrn_step_cmd;

typedef struct {
  uint16_t pin_num;             // pin number executing this pattern
  uint16_t idx_curr_step;       // step within ptrn_step_cmd_ptr
  pwm_led_ptrn_step_cmd * ptrn_step_cmd_ptr; // pointer to ptrn_step_cmd
  uint16_t curr_pwm_val;        // pwm intensity from 0 to PWM_MAX_VALUE or else PWM_NO_CHANGE
  uint32_t scale_factor;        // to stretch time. scale_factor == TIME_SCALE_EQUAL means no stretch
  uint32_t scaledtm_tick_incr;  // time (tick_time * TIME_SCALE_EQUAL) to increment scaledtm_next_tick
  uint32_t scaledtm_next_tick;  // time (millisec * TIME_SCALE_EQUAL) to start next tick
  uint32_t scaledtm_next_step;  // time (millisec * TIME_SCALE_EQUAL) to go to next step
} pwm_pin_info;

pwm_pin_info g_pwm_pin_info[PWM_NUM_PINS] = {
  { .pin_num=18, .idx_curr_step=0, .ptrn_step_cmd_ptr = (pwm_led_ptrn_step_cmd *)0, .curr_pwm_val=0, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_next_tick=0, .scaledtm_next_step=0 },
  { .pin_num=19, .idx_curr_step=0, .ptrn_step_cmd_ptr = (pwm_led_ptrn_step_cmd *)0, .curr_pwm_val=0, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_next_tick=0, .scaledtm_next_step=0 },
  { .pin_num=32, .idx_curr_step=0, .ptrn_step_cmd_ptr = (pwm_led_ptrn_step_cmd *)0, .curr_pwm_val=0, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_next_tick=0, .scaledtm_next_step=0 },
  { .pin_num=33, .idx_curr_step=0, .ptrn_step_cmd_ptr = (pwm_led_ptrn_step_cmd *)0, .curr_pwm_val=0, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_next_tick=0, .scaledtm_next_step=0 },
};

pwm_led_ptrn_step pwm_ptrn_open_eye = { .start_set_pwm=PWM_NO_CHANGE, .step_time=37, .tick_time=5, .tick_pwm=7 };

pwm_led_ptrn_step_cmd g_pwm_ptrn_blink_array[] = {
   { .step_ptr=&pwm_ptrn_open_eye, .step_incr=0 }
}; // pwm_ptrn_blink


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// dbg_display_step()
void dbg_display_step(int p_pin_idx) {

  pwm_pin_info * my_pin_info = &g_pwm_pin_info[p_pin_idx];
  pwm_led_ptrn_step_cmd * my_step_cmd = my_pin_info->ptrn_step_cmd_ptr;
  pwm_led_ptrn_step * my_step_ptr = my_step_cmd->step_ptr;

  Serial.printf("dbg_display_step my_pin_info[pin_idx=%d]\n", p_pin_idx);
  Serial.printf("  pin_num=%d idx_curr_step=%d curr_pwm_val=%d scale_factor=%ld\n",my_pin_info->pin_num,my_pin_info->idx_curr_step,my_pin_info->curr_pwm_val,my_pin_info->scale_factor);
  Serial.printf("  scaledtm_tick_incr=%ld scaledtm_next_tick=%ld scaledtm_next_step=%ld\n",my_pin_info->scaledtm_tick_incr,my_pin_info->scaledtm_next_tick,my_pin_info->scaledtm_next_step);
  Serial.printf("dbg_display_step my_step_cmd->step_incr=%d\n",my_step_cmd->step_incr);
  Serial.printf("dbg_display_step my_step_ptr\n");
  Serial.printf("  start_set_pwm=%d step_time=%d tick_time=%d tick_pwm=%d\n",my_step_ptr->start_set_pwm,my_step_ptr->step_time,my_step_ptr->tick_time,my_step_ptr->tick_pwm);
  Serial.println(" ");
} // end dbg_display_step()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_pin_pwm_init_step_times()
void do_pin_pwm_init_step_times(int p_pin_idx) {
  uint32_t time_msec_now = millis();

  pwm_led_ptrn_step * my_step_ptr = g_pwm_pin_info[p_pin_idx].ptrn_step_cmd_ptr->step_ptr;
  g_pwm_pin_info[p_pin_idx].scaledtm_tick_incr = g_pwm_pin_info[p_pin_idx].scale_factor * my_step_ptr->tick_time;
  g_pwm_pin_info[p_pin_idx].scaledtm_next_tick = g_pwm_pin_info[p_pin_idx].scale_factor * (my_step_ptr->tick_time + time_msec_now);
  g_pwm_pin_info[p_pin_idx].scaledtm_next_step = g_pwm_pin_info[p_pin_idx].scale_factor * (my_step_ptr->step_time + time_msec_now);
} // end do_pin_pwm_init_step_times()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_pin_pwm_init_step()
void do_pin_pwm_init_step(int p_pin_idx, pwm_led_ptrn_step_cmd *p_ptrn_step_cmd_ptr, uint16_t p_idx_start_step = 0, uint32_t p_scale_factor=TIME_SCALE_EQUAL, uint16_t p_pwm_val_init=PWM_NO_CHANGE) {
  g_pwm_pin_info[p_pin_idx].idx_curr_step = p_idx_start_step;
  g_pwm_pin_info[p_pin_idx].ptrn_step_cmd_ptr = p_ptrn_step_cmd_ptr;
  g_pwm_pin_info[p_pin_idx].curr_pwm_val = p_pwm_val_init;
  g_pwm_pin_info[p_pin_idx].scale_factor = p_scale_factor;
  do_pin_pwm_init_step_times(p_pin_idx);
} // end do_pin_pwm_init_step()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_pin_pwm()
void do_pins_pwm() {
  uint32_t time_msec_now = millis();
  uint32_t time_scaled_now = time_msec_now * TIME_SCALE_EQUAL;

  for (int pin_idx = 0; pin_idx < PWM_NUM_PINS; pin_idx += 1) {
    if (g_pwm_pin_info[pin_idx].scaledtm_next_step <= time_scaled_now) {
      // go to next step
    } else if (g_pwm_pin_info[pin_idx].scaledtm_next_tick <= time_scaled_now) {
      // do next tick
      g_pwm_pin_info[pin_idx].scaledtm_next_tick += 1; // TODO FIXME
    } else {
      ledcWrite(g_pwm_pin_info[pin_idx].pin_num, 5);
    }
  }
} // end do_pins_pwm()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// setup()
void setup() {
  Serial.begin(115200);         // this serial communication is for general debug; set the USB serial port to 115,200 baud
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  delay(1500);
  Serial.println(""); // print a blank line in case there is some junk from power-on
  Serial.println("starting LEDC_steps");


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
#define MAX_TICK_COUNT 1000 // just for testing
void loop() {
  static int loop_count = 0;
  static uint32_t tick_count = MAX_TICK_COUNT+100;

  if (MAX_TICK_COUNT < tick_count) {
    for (int pin_idx = 0; pin_idx < PWM_NUM_PINS; pin_idx += 1) {
      do_pin_pwm_init_step(pin_idx, g_pwm_ptrn_blink_array, 0, (loop_count+1)*TIME_SCALE_EQUAL + pin_idx*13, 0);
      
    } // end for each pin_idx
    for (int pin_idx = 0; pin_idx < PWM_NUM_PINS; pin_idx += 1) { dbg_display_step(pin_idx); }
    tick_count = 0;
    loop_count = (loop_count + 1) % 4;
  } // end if start next pattern time scale

  // do_pins_pwm();
} // end loop()