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

#include "DoPinsPwm.h"

static uint16_t g_do_pin_pwm_dbg_step = 1; // non-zero to do debug prints


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_pin_pwm_dbg_step() - for debugging, display steps at important steps
//
void do_pin_pwm_dbg_step(int p_pin_idx);
void do_pin_pwm_dbg_step(int p_pin_idx) {
  static int first_time = 1;

  if (g_do_pin_pwm_dbg_step) {
    pwm_pin_info * my_pin_info = &g_pwm_pin_info[p_pin_idx];
    pwm_led_ptrn_step * my_step_ptr = &my_pin_info->ptrn_step_ptr[my_pin_info->idx_curr_step];

    if (first_time) {
      Serial.printf("do_pin_pwm_dbg_step:my_pin_info\tpin_num\tidx_curr_step\tcurr_pwm_val\tprev_pwm_val\tscale_factor");
      Serial.printf("\tscaledtm_tick_incr\tscaledtm_next_tick\tscaledtm_next_step");
      Serial.printf("\tdo_pin_pwm_dbg_step:my_step_ptr\tidx_curr_step");
      Serial.printf("\tstart_set_pwm\tstep_incr\tstep_time\ttick_time\ttick_pwm\n");
    }

    Serial.printf("\t%d\t%d\t%d\t%ld", /* g_millis, */ my_pin_info->pin_num, my_pin_info->idx_curr_step, my_pin_info->curr_pwm_val, my_pin_info->prev_pwm_val, my_pin_info->scale_factor);
    Serial.printf("\t%ld\t%ld\t%ld", my_pin_info->scaledtm_tick_incr, my_pin_info->scaledtm_next_tick, my_pin_info->scaledtm_next_step);
    Serial.printf("\txxx\t%d", my_pin_info->idx_curr_step);
    Serial.printf("\t%d\t%d\t%d\t%d\t%d\n", my_step_ptr->start_set_pwm, my_step_ptr->step_incr, my_step_ptr->step_time, my_step_ptr->tick_time, my_step_ptr->tick_pwm);

    first_time = 0; // so we can turn this on and off
  } // end if g_do_pin_pwm_dbg_step
} // end do_pin_pwm_dbg_step()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_pin_pwm_init_step_times() - calculates the scaled times for this step in the pattern
//    Called once each time a new step in a pattern is entered for a particular pin
//
// Things that are required when calling
//    p_pin_idx  - must be a valid pin idx in g_pwm_pin_info
//    also g_pwm_pin_info should be set up except for the scalled times
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
//    p_pwm_val_init   - = DO_PINS_PWM_NO_CHANGE means leave curr_pwm_val alone, do not change it from current value
//                       = DO_PINS_PWM_USE_PTRN  means store the pattern start_set_pwm into curr_pwm_val
//                       = other         means store the parameter p_pwm_val_init into curr_pwm_val
//
void do_pin_pwm_init_ptrn(int p_pin_idx, pwm_led_ptrn_step* p_ptrn_ptr, uint16_t p_idx_start_step, uint32_t p_scale_factor, uint16_t p_pwm_val_init) {

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
  if (DO_PINS_PWM_NO_CHANGE != p_pwm_val_init) {
    if (DO_PINS_PWM_USE_PTRN == p_pwm_val_init) {
      // the step itself might say not to change the pwm
      uint16_t tmp_pwm = my_step_ptr->start_set_pwm;
      if (DO_PINS_PWM_NO_CHANGE != tmp_pwm) {
        my_pin_info->curr_pwm_val = DO_PINS_PWM_MAX_VALUE & tmp_pwm;
      }
    } else { // (DO_PINS_PWM_USE_PTRN != p_pwm_val_init)
      my_pin_info->curr_pwm_val = DO_PINS_PWM_MAX_VALUE & p_pwm_val_init;
    }
  }
  my_pin_info->scale_factor = p_scale_factor;
  do_pin_pwm_init_step_times(p_pin_idx);
  ledcWrite(my_pin_info->pin_num, my_pin_info->curr_pwm_val);
  my_pin_info->prev_pwm_val = my_pin_info->curr_pwm_val;
} // end do_pin_pwm_init_ptrn()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_pins_pwm_init() - initialize pwm pins and set value to zero
//
// parameters:
//   p_pwm_freq         - Frequency to use in Hertz. Usually between 500 and 5,000
//   p_pwm_val_num_bits - Number of bits for value. Usually between 8 and 14
//                        If this is  8 then pwm_val would be between 0 and 255
//                        If this is 12 then pwm_val would be between 0 and 4095
//   p_serial_debugging - zero for no serial debugging, 1 for serial debugging
//
int16_t do_pins_pwm_init(uint16_t p_pwm_freq, uint16_t p_pwm_val_num_bits, uint16_t p_serial_debugging) {
  int16_t ret_val = 0; // 0 is no error

  g_do_pin_pwm_dbg_step = p_serial_debugging;
  // connect to Banjo Player LED eyes and initially turn off
  for (int pin_idx = 0; pin_idx < NUMOF(g_pwm_pin_info); pin_idx += 1) {
    // connect a pin to a channel at a pwm value frequency, and a PWM resolution (1 - 16 bits)
    if (!ledcAttach(g_pwm_pin_info[pin_idx].pin_num, DO_PINS_PWM_FREQ, DO_PINS_PWM_VAL_NUM_BITS)) {
      Serial.printf("ERROR - could not attach pin %d to LEDC library", g_pwm_pin_info[pin_idx].pin_num);
      ret_val = 1; // 0 is no error
    } else {
      ledcWrite(g_pwm_pin_info[pin_idx].pin_num, 0); // initially set to off
    }
  } // end connect all pins to Banjo Player LED eyes

  return(ret_val); // 0 is no error
} // end do_pins_pwm_init()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_pins_pwm() - do calculations and set values for pwm for all pins based on g_pwm_pin_info
//
// should have called do_pins_pwm_init() and each g_pwm_pin_info[pin_idx] should have called do_pin_pwm_init_ptrn()
//
void do_pins_pwm() {
  uint32_t time_msec_now = millis();
  uint32_t time_scaled_now = time_msec_now * TIME_SCALE_EQUAL;
  static uint32_t num_calls = 0;

  num_calls = (num_calls+1) % 1000;

  for (int pin_idx = 0; pin_idx < NUMOF(g_pwm_pin_info); pin_idx += 1) {
    pwm_pin_info * my_pin_info = &g_pwm_pin_info[pin_idx];
    pwm_led_ptrn_step * my_step_ptr = &my_pin_info->ptrn_step_ptr[my_pin_info->idx_curr_step];

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
      if (DO_PINS_PWM_NO_CHANGE != tmp_pwm) {
        my_pin_info->curr_pwm_val = DO_PINS_PWM_MAX_VALUE & tmp_pwm;
      }
      // write the pwm value for this step change
      if (my_pin_info->prev_pwm_val != my_pin_info->curr_pwm_val) {
        ledcWrite(my_pin_info->pin_num, my_pin_info->curr_pwm_val);
      }
      // end if step change happened
    } else if (my_pin_info->scaledtm_next_tick <= time_scaled_now) {
      // do next tick: calc time to end of tick and adjust pwm by one tick_pwm
      my_pin_info->scaledtm_next_tick += my_pin_info->scaledtm_tick_incr;
      // always good practice to only update state vars to valid values
      uint32_t tmp_pwm = my_pin_info->curr_pwm_val + my_step_ptr->tick_pwm;
      if (0 <= my_step_ptr->tick_pwm) {
        if (DO_PINS_PWM_MAX_VALUE < tmp_pwm) tmp_pwm = DO_PINS_PWM_MAX_VALUE;
      } else {
        if (DO_PINS_PWM_MAX_VALUE < tmp_pwm) tmp_pwm = 0;
      }
      my_pin_info->curr_pwm_val = tmp_pwm;
      if (my_pin_info->prev_pwm_val != my_pin_info->curr_pwm_val) {
        ledcWrite(my_pin_info->pin_num, my_pin_info->curr_pwm_val);
      }
    } // end if tick change happened
    my_pin_info->prev_pwm_val = my_pin_info->curr_pwm_val;
  } // end for loop
} // end do_pins_pwm()
