/* Author: https://github.com/Mark-MDO47  March 20, 2025
 *  https://github.com/Mark-MDO47/DuelWithBanjos
 *
 */

/*
   Copyright 2024, 2025 Mark Olson

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


#ifndef DO_PINS_PWM_H

#define DO_PINS_PWM_H 1

#include "Arduino.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// generally useful definitions
//
#define NUMOF(x) (sizeof((x)) / sizeof((*x)))


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// modifiable constants for do_pins_pwm LEDC PWM library to control the LED Eyes of the Banjo Players
//
#define DO_PINS_PWM_NUM_PINS 4        // number of LED pins to control
#define DO_PINS_PWM_FREQ 500          // Arduino Uno is ~490 Hz. ESP32 example uses 5,000Hz
#define DO_PINS_PWM_VAL_NUM_BITS 8    // Use same resolution as Uno (8 bits, 0-255) but ESP32 can go up to 16 bits (some versions less)
#define TIME_SCALE_EQUAL 64           // pwm_pin_info factors for time_ entries: millisec = (time_<whatever> / TIME_SCALE_EQUAL)
                                      // this allows a relatively smooth scaling of time by setting
                                      // 2^32millisec / 64 is about 18 hours, so that will exceed any gig time for the banjo players

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// constants for do_pins_pwm LEDC PWM library to control the LED Eyes of the Banjo Players
//
#define DO_PINS_PWM_NO_CHANGE 0xFFFF  // no change to current pwm value when init pattern (.start_set_pwm or init function)
#define DO_PINS_PWM_USE_PTRN  0xFFFE  // use pwm value from pattern step when init pattern (init function only)
#define DO_PINS_PWM_MAX_VALUE ((1 << DO_PINS_PWM_VAL_NUM_BITS)-1) // max pwm value (255 if DO_PINS_PWM_VAL_NUM_BITS is 8)


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// structure typedefs for do_pins_pwm LEDC PWM library to control the LED Eyes of the Banjo Players
typedef struct {
  uint16_t  start_set_pwm; // if not DO_PINS_PWM_NO_CHANGE, set pwm value to this
  int16_t   step_incr;     // if # >= 0, increment step counter by #; if # < 0 go to step "-# - 1"
  uint16_t  step_time;     // how long (millisec B4 scaling) until this step is complete
  uint16_t  tick_time;     // how long (millisec B4 scaling) between each tick
  int16_t   tick_pwm;      // how far and what direction for pwm value each tick_time
} pwm_led_ptrn_step;

typedef struct {
  uint16_t pin_num;             // pin number executing this pattern
  pwm_led_ptrn_step * ptrn_step_ptr; // pointer to array of pwm_led_ptrn_step (steps)
  uint16_t idx_curr_step;       // index into array of steps for current step
  uint16_t curr_pwm_val;        // current pwm intensity from 0 to DO_PINS_PWM_MAX_VALUE
  uint16_t prev_pwm_val;        // previous pwm intensity from 0 to DO_PINS_PWM_MAX_VALUE
  uint32_t scale_factor;        // to stretch time. scale_factor == TIME_SCALE_EQUAL means no stretch
  uint32_t scaledtm_tick_incr;  // time (tick_time * TIME_SCALE_EQUAL) to increment scaledtm_next_tick
  uint32_t scaledtm_next_tick;  // time (millisec * TIME_SCALE_EQUAL) to start next tick
  uint32_t scaledtm_next_step;  // time (millisec * TIME_SCALE_EQUAL) to go to next step
} pwm_pin_info;

extern pwm_pin_info g_pwm_pin_info[DO_PINS_PWM_NUM_PINS];

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_pin_pwm_init_step_times() - calculates the scaled times for this step in the pattern
//    Called once each time a new step in a pattern is entered for a particular pin
//
// Things that are required when calling
//    p_pin_idx  - must be a valid pin idx in g_pwm_pin_info
//    also g_pwm_pin_info should be set up except for the scalled times
//
void do_pin_pwm_init_step_times(int p_pin_idx);

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
void do_pin_pwm_init_ptrn(int p_pin_idx, pwm_led_ptrn_step* p_ptrn_ptr, uint16_t p_idx_start_step = 0, uint32_t p_scale_factor=TIME_SCALE_EQUAL, uint16_t p_pwm_val_init=DO_PINS_PWM_USE_PTRN);

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
int16_t do_pins_pwm_init(uint16_t p_pwm_freq, uint16_t p_pwm_val_num_bits, uint16_t p_serial_debugging);

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_pins_pwm() - do calculations and set values for pwm for all pins based on g_pwm_pin_info
//
// should have called do_pins_pwm_init() and each g_pwm_pin_info[pin_idx] should have called do_pin_pwm_init_ptrn()
//
void do_pins_pwm();

#endif // DO_PINS_PWM_H
