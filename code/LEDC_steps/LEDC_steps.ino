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
// generally useful definitions
#define NUMOF(x) (sizeof((x)) / sizeof((*x)))

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// constants for LEDC PWM library to control the LED Eyes of the Banjo Players
#define PWM_NO_CHANGE 0xFFFF  // no change to current pwm value when init pattern (.start_set_pwm or init function)
#define PWM_USE_PTRN  0xFFFE  // use pwm value from pattern step when init pattern (init function only)
#define PWM_NUM_PINS 4        // number of LED pins to control
#define PWM_FREQ 500          // Arduino Uno is ~490 Hz. ESP32 example uses 5,000Hz
#define PWM_VAL_NUM_BITS 8    // Use same resolution as Uno (8 bits, 0-255) but ESP32 can go up to 16 bits (some versions less)
#define PWM_MAX_VALUE ((1 << PWM_VAL_NUM_BITS)-1) // max pwm value (255 if PWM_VAL_NUM_BITS is 8)
#define TIME_SCALE_EQUAL 64   // pwm_pin_info factors for time_ entries: millisec = (time_<whatever> / TIME_SCALE_EQUAL)
                              // this allows a relatively smooth scaling of time by setting
                              // 2^32millisec / 64 is about 18 hours, so that will exceed any gig time for the banjo players

typedef struct {
  uint16_t  start_set_pwm; // if not PWM_NO_CHANGE, set pwm value to this ??? FIXME TODO should this be in the struct??? or pwm_pin_info???
  int16_t   step_incr;     // if # >= 0, increment step counter by #; if # < 0 go to step "-# - 1"
  uint16_t  step_time;     // how long (millisec B4 scaling) until this step is complete
  uint16_t  tick_time;     // how long (millisec B4 scaling) between each tick
  int16_t   tick_pwm;      // how far and what direction for pwm value each tick_time
} pwm_led_ptrn_step;

typedef struct {
  uint16_t pin_num;             // pin number executing this pattern
  pwm_led_ptrn_step * ptrn_step_ptr; // pointer to array of pwm_led_ptrn_step (steps)
  uint16_t idx_curr_step;       // index into array of steps for current step
  uint16_t curr_pwm_val;        // current pwm intensity from 0 to PWM_MAX_VALUE
  uint32_t scale_factor;        // to stretch time. scale_factor == TIME_SCALE_EQUAL means no stretch
  uint32_t scaledtm_tick_incr;  // time (tick_time * TIME_SCALE_EQUAL) to increment scaledtm_next_tick
  uint32_t scaledtm_next_tick;  // time (millisec * TIME_SCALE_EQUAL) to start next tick
  uint32_t scaledtm_next_step;  // time (millisec * TIME_SCALE_EQUAL) to go to next step
} pwm_pin_info;

pwm_pin_info g_pwm_pin_info[PWM_NUM_PINS] = {
  { .pin_num=18, .ptrn_step_ptr = (pwm_led_ptrn_step *)0, .idx_curr_step=0, .curr_pwm_val=0, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_tick_incr = 0, .scaledtm_next_tick=0, .scaledtm_next_step=0 },
  { .pin_num=19, .ptrn_step_ptr = (pwm_led_ptrn_step *)0, .idx_curr_step=0, .curr_pwm_val=0, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_tick_incr = 0, .scaledtm_next_tick=0, .scaledtm_next_step=0 },
  { .pin_num=32, .ptrn_step_ptr = (pwm_led_ptrn_step *)0, .idx_curr_step=0, .curr_pwm_val=0, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_tick_incr = 0, .scaledtm_next_tick=0, .scaledtm_next_step=0 },
  { .pin_num=33, .ptrn_step_ptr = (pwm_led_ptrn_step *)0, .idx_curr_step=0, .curr_pwm_val=0, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_tick_incr = 0, .scaledtm_next_tick=0, .scaledtm_next_step=0 }
};

pwm_led_ptrn_step pwm_ptrn_open_eye[] = { 
  { .start_set_pwm=0,             .step_incr=1,  .step_time=1757, .tick_time=5, .tick_pwm=2},
  { .start_set_pwm=PWM_NO_CHANGE, .step_incr=-1, .step_time=1750, .tick_time=5, .tick_pwm=-7}
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// dbg_display_step()
void dbg_display_step(int p_pin_idx) {

  pwm_pin_info * my_pin_info = &g_pwm_pin_info[p_pin_idx];
  pwm_led_ptrn_step * my_step_ptr = &my_pin_info->ptrn_step_ptr[my_pin_info->idx_curr_step];

  Serial.printf("dbg_display_step my_pin_info[pin_idx=%d]\n", p_pin_idx);
  Serial.printf("  pin_num=%d idx_curr_step=%d curr_pwm_val=%d scale_factor=%ld\n",my_pin_info->pin_num,my_pin_info->idx_curr_step,my_pin_info->curr_pwm_val,my_pin_info->scale_factor);
  Serial.printf("  scaledtm_tick_incr=%ld scaledtm_next_tick=%ld scaledtm_next_step=%ld\n",my_pin_info->scaledtm_tick_incr,my_pin_info->scaledtm_next_tick,my_pin_info->scaledtm_next_step);
  Serial.printf("dbg_display_step my_step_ptr[idx_curr_step=%d]\n",my_pin_info->idx_curr_step);
  Serial.printf("  start_set_pwm=%d step_incr=%d step_time=%d tick_time=%d tick_pwm=%d\n",my_step_ptr->start_set_pwm,my_step_ptr->step_incr,my_step_ptr->step_time,my_step_ptr->tick_time,my_step_ptr->tick_pwm);
  Serial.println(" ");
} // end dbg_display_step()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_pin_pwm_init_step_times()
//    Called once each time a new step in a pattern is entered for a particular pin
//
void do_pin_pwm_init_step_times(int p_pin_idx) {
  uint32_t time_msec_now = millis();

  pwm_pin_info * my_pin_info = &g_pwm_pin_info[p_pin_idx];
  pwm_led_ptrn_step * my_step_ptr = &my_pin_info->ptrn_step_ptr[my_pin_info->idx_curr_step];

  my_pin_info->scaledtm_tick_incr = my_pin_info->scale_factor * my_step_ptr->tick_time;
  my_pin_info->scaledtm_next_tick = my_pin_info->scale_factor * (my_step_ptr->tick_time + time_msec_now);
  my_pin_info->scaledtm_next_step = my_pin_info->scale_factor * (my_step_ptr->step_time + time_msec_now);
} // end do_pin_pwm_init_step_times()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_pin_pwm_init_ptrn() - initialize a pwm_pin_info entry for a particular pattern
//    Called once each time a new pattern is set for a particular p_pin_idx in g_pwm_pin_info
//
// Things that are required when calling
//    p_pin_idx  - must be a valid pin idx in g_pwm_pin_info
//    p_ptrn_ptr - must point to an array of steps, with at least (1+p_idx_start_step) steps in the array
// Things that can be adjusted when calling this for a new pattern
//    p_idx_start_step - usually 0, occasionally 1, I don't know why it would be > 1
//    p_scale_factor   - = TIME_SCALE_EQUAL means use the pattern millisec counts as-is
//                       > TIME_SCALE_EQUAL means go faster than pattern by factor (p_scale_factor/TIME_SCALE_EQUAL)
//                       < TIME_SCALE_EQUAL means go slower than pattern by factor (p_scale_factor/TIME_SCALE_EQUAL)
//                       = 0 (special case) means stay on step p_idx_start_step forever and never tick
//    p_pwm_val_init   - = PWM_NO_CHANGE means leave curr_pwm_val alone, do not change it from current value
//                       = PWM_USE_PTRN  means store the pattern start_set_pwm into curr_pwm_val
//                       = other         means store the parameter p_pwm_val_init into curr_pwm_val
//
void do_pin_pwm_init_ptrn(int p_pin_idx, pwm_led_ptrn_step* p_ptrn_ptr, uint16_t p_idx_start_step = 0, uint32_t p_scale_factor=TIME_SCALE_EQUAL, uint16_t p_pwm_val_init=PWM_USE_PTRN) {

  if (( 0 > p_pin_idx ) || (NUMOF(g_pwm_pin_info) <= p_pin_idx)) {
    Serial.printf("ERROR do_pin_pwm_init_ptrn param p_pin_idx=%d is out of range\n",p_pin_idx);
    return;
  }
  if (((pwm_led_ptrn_step*) 0) == p_ptrn_ptr) {
    Serial.printf("ERROR do_pin_pwm_init_ptrn param p_ptrn_ptr=0 is out of range\n");
    return;
  }
  if (1 < p_idx_start_step) {
    Serial.printf("Warning do_pin_pwm_init_ptrn param p_idx_start_step=%d is unusual\n",p_idx_start_step);
  }

  pwm_pin_info* my_pin_info = &g_pwm_pin_info[p_pin_idx];

  my_pin_info->ptrn_step_ptr = &p_ptrn_ptr[p_idx_start_step]; // initializing pattern
  my_pin_info->idx_curr_step = p_idx_start_step;
  pwm_led_ptrn_step * my_step_ptr = &my_pin_info->ptrn_step_ptr[my_pin_info->idx_curr_step];
  if (PWM_NO_CHANGE != p_pwm_val_init) {
    if (PWM_USE_PTRN == p_pwm_val_init) {
      // the step itself might say not to change the pwm
      uint16_t tmp_pwm = my_step_ptr->start_set_pwm;
      if (PWM_NO_CHANGE != tmp_pwm) {
        my_pin_info->curr_pwm_val = PWM_MAX_VALUE & tmp_pwm;
      }
    } else { // (PWM_USE_PTRN != p_pwm_val_init)
      my_pin_info->curr_pwm_val = PWM_MAX_VALUE & p_pwm_val_init;
    }
  }
  my_pin_info->scale_factor = p_scale_factor;
  do_pin_pwm_init_step_times(p_pin_idx);
  ledcWrite(my_pin_info->pin_num, my_pin_info->curr_pwm_val);
} // end do_pin_pwm_init_ptrn()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_pins_pwm()
void do_pins_pwm() {
  uint32_t time_msec_now = millis();
  uint32_t time_scaled_now = time_msec_now * TIME_SCALE_EQUAL;
  static uint32_t num_calls = 0;

  num_calls = (num_calls+1) % 1000;

  if (0 == num_calls) {
    Serial.printf("do_pins_pwm time_msec_now=%ld time_scaled_now=%ld\n",time_msec_now,time_scaled_now);
  }

  for (int pin_idx = 0; pin_idx < NUMOF(g_pwm_pin_info); pin_idx += 1) {
    pwm_pin_info * my_pin_info = &g_pwm_pin_info[pin_idx];
    pwm_led_ptrn_step * my_step_ptr = &my_pin_info->ptrn_step_ptr[my_pin_info->idx_curr_step];

    if (!(pin_idx || num_calls)) dbg_display_step(pin_idx);

    if (my_pin_info->scaledtm_next_step <= time_scaled_now) {
      // go to next step
      if (my_step_ptr->step_incr >= 0) {
        // increment step
        my_pin_info->idx_curr_step += my_step_ptr->step_incr;
      } else {
        // go to step offset from start of pattern (-1 --> 0, -2 --> 1, etc.)
        my_pin_info->idx_curr_step = -my_step_ptr->step_incr -1;
      }
      my_step_ptr = &my_pin_info->ptrn_step_ptr[my_pin_info->idx_curr_step];
      // now that idx_curr_step is changed, we need to set the scaled times for the new step for this pin_idx
      do_pin_pwm_init_step_times(pin_idx);
      // also need to update pwm value from step
      // the step itself might say not to change the pwm
      uint32_t tmp_pwm = my_step_ptr->start_set_pwm;
      if (PWM_NO_CHANGE != tmp_pwm) {
        my_pin_info->curr_pwm_val = PWM_MAX_VALUE & tmp_pwm;
      }
      // write the pwm value for this step change
      ledcWrite(my_pin_info->pin_num, my_pin_info->curr_pwm_val);
    } else if (my_pin_info->scaledtm_next_tick <= time_scaled_now) {
      // do next tick: calc time to end of tick and adjust pwm by one tick_pwm
      my_pin_info->scaledtm_next_tick += my_pin_info->scaledtm_tick_incr;
      // always good practice to only update state vars to valid values
      uint32_t tmp_pwm = my_pin_info->curr_pwm_val + my_step_ptr->tick_pwm;
      if (0 <= my_step_ptr->tick_pwm) {
        if (PWM_MAX_VALUE < tmp_pwm) tmp_pwm = PWM_MAX_VALUE;
      } else {
        if (PWM_MAX_VALUE < tmp_pwm) tmp_pwm = 0;
      }
      my_pin_info->curr_pwm_val = tmp_pwm;
      ledcWrite(my_pin_info->pin_num, my_pin_info->curr_pwm_val);
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
  delay(3000);
  Serial.println(""); // print a blank line in case there is some junk from power-on
  Serial.println("starting LEDC_steps");


  // connect to Banjo Player LED eyes and initially turn off
  for (int pin_idx = 0; pin_idx < NUMOF(g_pwm_pin_info); pin_idx += 1) {
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
  static uint32_t first_time = 1;

  if (0 != first_time) {
    for (int pin_idx = 0; pin_idx < NUMOF(g_pwm_pin_info); pin_idx += 1) {
      do_pin_pwm_init_ptrn(pin_idx, pwm_ptrn_open_eye, 0, TIME_SCALE_EQUAL + pin_idx*2, 0);
    } // end for each pin_idx
    for (int pin_idx = 0; pin_idx < NUMOF(g_pwm_pin_info); pin_idx += 1) { dbg_display_step(pin_idx); }
    first_time = 0;
  } // end if start next pattern time scale

  do_pins_pwm();
  delay(1);

} // end loop()