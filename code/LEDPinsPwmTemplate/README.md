# LEDPinsPwmTemplate - My Method to Use LEDC PWM

**Table Of Contents**
* [Top](#ledpinspwmtemplate-\--my-method-to-use-ledc-pwm "Top")
* [Intro](#intro "Intro")
* [Constants](#constants "Constants")
  * [Modifiable Constants](#modifiable-constants "Modifiable Constants")
  * [Do Not Change These Constants](#do-not-change-these-constants "Do Not Change These Constants")
* [Data Structures](#data-structures "Data Structures")
  * [Typedef for Pattern Step](#typedef-for-pattern-step "Typedef for Pattern Step")
  * [Typedef for Pins Controlled by LEDPinsPwmTemplate](#typedef-for-pins-controlled-by-ledpinspwmtemplate "Typedef for Pins Controlled by LEDPinsPwmTemplate")
* [Function Declarations](#function-declarations "Function Declarations")
  * [led_pins_pwm_init - in setup do hardware initialization](#led_pins_pwm_init-\--in-setup-do-hardware-initialization "led_pins_pwm_init - in setup do hardware initialization")
  * [led_pin_pwm_init_ptrn - in loop or setup start a pattern for a particular pin](#led_pin_pwm_init_ptrn-\--in-loop-or-setup-start-a-pattern-for-a-particular-pin "led_pin_pwm_init_ptrn - in loop or setup start a pattern for a particular pin")
  * [led_pins_pwm - in loop calculate PWM values and if changed output to hardware](#led_pins_pwm-\--in-loop-calculate-pwm-values-and-if-changed-output-to-hardware "led_pins_pwm - in loop calculate PWM values and if changed output to hardware")
  * [led_pin_pwm_set_pwm_scale - in loop or setup set global scale factor for PWM value](#led_pin_pwm_set_pwm_scale-\--in-loop-or-setup-set-global-scale-factor-for-pwm-value "led_pin_pwm_set_pwm_scale - in loop or setup set global scale factor for PWM value")
* [Debugging Functions](#debugging-functions "Debugging Functions")
  * [led_pin_pwm_set_dbg_enable - enable or disable debugging output](#led_pin_pwm_set_dbg_enable-\--enable-or-disable-debugging-output "led_pin_pwm_set_dbg_enable - enable or disable debugging output")
  * [led_pin_pwm_int_dbg_step - display debug info on state of pin and steps structures](#led_pin_pwm_int_dbg_step-\--display-debug-info-on-state-of-pin-and-steps-structures "led_pin_pwm_int_dbg_step - display debug info on state of pin and steps structures")

## Intro
[Top](#ledpinspwmtemplate-\--my-method-to-use-ledc-pwm "Top")<br>
This code is a vehicle for me to come up  with a general way to use LEDC PWM. I wanted to have a data structure for a time-sequence pattern that could be applied to a set of pins.

Many Arduinos, including the Arduino UNO and the ESP-32, can use LEDPinsPwm to control timing of LEDs. However, some Arduinos are more restrictive on things such as the PWM frequency allowed, so check your Arduino documentation.

LEDPinsPwmTemplate.ino is a near-minimum implementation of use of LEDPinsPwm. It has the following required parts:
- **#include "LEDPinsPwm.h"**
  - **optional**: you can edit LEDPinsPwm.h to change "modifiable" constants as outlined in that file. For instance, you can redefine LED_PINS_PWM_NUM_PINS to something other than 4.
- make the table **pwm_pin_info g_pwm_pin_info[LED_PINS_PWM_NUM_PINS]** to point to the pins you will be having LEDPinsPwm control. You must use this name and it must be global scope.
- provide storage for uint32_t **g_eyes_bright**. MS-16bits is <num>, LS-16bits is <den> for last call to led_pin_pwm_set_pwm_scale()
- make at least one **pwm_led_ptrn_step pattern**. This would be an array of pwm_led_ptrn_step, even if there is only one entry. These patterns can have any name you want; they will be referenced later
- in calls to led_pin_pwm_init_ptrn().
- in **setup()**, call **Serial.begin()** so that you can see error output and/or debug output in the serial monitor.
- in **setup()**, call **led_pins_pwm_init()** to connect to the hardware pins to control.
- in **loop()** (preferred) or **setup()**, call **led_pin_pwm_init_ptrn()** to set a pattern for each pin in **g_pwm_pin_info[]**. Only do this when initially setting or changing patterns for a pin.
- in **loop()**, call **led_pins_pwm()** to perform the patterns on the pins over time.
  - **optional**: later on you can call **led_pin_pwm_init_ptrn()** to set a new pattern for a pin or restart the existing pattern for a pin.
  - **optional**: later on you can call **led_pin_pwm_set_pwm_scale()** to set a global scale factor for the PWM values.

The following excerpts from LEDPinsPwmTemplate.ino illustrate the above steps.

```C
#include "LEDPinsPwm.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// pin definitions - name must be must be g_pwm_pin_info, must be one row for each PWM LED output pin
//
pwm_pin_info g_pwm_pin_info[LED_PINS_PWM_NUM_PINS] = {
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
  { .start_set_pwm=0,                      .step_incr=1,  .step_time=1757, .tick_time=5, .tick_pwm= 2},
  { .start_set_pwm=LED_PINS_PWM_NO_CHANGE, .step_incr=-1, .step_time=1000, .tick_time=5, .tick_pwm=-7}
};

uint32_t g_eyes_bright; // MS-16bits is <num>, LS-16bits is <den> for last call to led_pin_pwm_set_pwm_scale()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// setup()
void setup() {
  Serial.begin(115200);         // this serial communication is for general debug; set the USB serial port to 115,200 baud
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  delay(3000);
  Serial.println(""); // print a blank line in case there is some junk from power-on
  Serial.println("starting LEDPinsPwmTemplate");

  if (led_pins_pwm_init(LED_PINS_PWM_FREQ, LED_PINS_PWM_VAL_NUM_BITS)) {
    while (1) ;
  } // end if error in initialization

} // end setup()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// loop()
void loop() {
  static uint32_t first_time = 1;
  static uint32_t time_msec_first_loop = 0;
  static uint16_t prev_ten_sec = 65535;
  static pwm_led_ptrn_step* prev_ptrn_ptr = (pwm_led_ptrn_step *) 0;

  uint32_t time_msec_this_loop = millis();
  uint16_t ten_secs = (time_msec_this_loop-time_msec_first_loop) / 10000;

  // this if statement should be entered once every 10 seconds and the first time loop() is executed
  if  ((0 != first_time) || (ten_secs != prev_ten_sec)) {
    if (0 != first_time) {
      time_msec_first_loop = millis();
      ten_secs = (time_msec_this_loop-time_msec_first_loop) / 10000; // probably same answer but not if setup() takes too long
    }
    
    // this section executes every 10 seconds

    // it is optional to adjust global scaling while operating LEDs but we demonstrate it here
    led_pin_pwm_set_pwm_scale(1,5); // this is a good max brightness in my breadboard setup; could have been done is setup()
    // commented out below is a way to adjust the brightness to fade away
    //    this would set pwm global scaling to 1/1, 1/2, 1/3, 1/4, ... 1/(n)
    // led_pin_pwm_set_pwm_scale(1,1+ten_secs);

    // it is optional to change LED pattern while operating LEDs but we demonstrate it here
    // pattern changes every 10 seconds
    if (pwm_ptrn_open_eye != prev_ptrn_ptr) prev_ptrn_ptr = pwm_ptrn_open_eye;
    else                                    prev_ptrn_ptr = pwm_ptrn_blink;
    for (int pin_idx = 0; pin_idx < NUMOF(g_pwm_pin_info); pin_idx += 1) {
      // commented out time scale manipulation would make pins operate on different time scales
      led_pin_pwm_init_ptrn(pin_idx, prev_ptrn_ptr, 0, TIME_SCALE_EQUAL /* + pin_idx*2 */, 0);
    } // end for each pin_idx
    
    first_time = 0; // first_time only once
    prev_ten_sec = ten_secs;
  } // end if once every 10 seconds

  led_pins_pwm(); // if needed, adjust brightness of LED
  delay(1);
} // end loop()
```

## Constants
[Top](#ledpinspwmtemplate-\--my-method-to-use-ledc-pwm "Top")<br>

### Modifiable Constants
[Top](#ledpinspwmtemplate-\--my-method-to-use-ledc-pwm "Top")<br>
```C
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// modifiable constants for led_pins_pwm LEDC PWM library to control the LED Eyes of the Banjo Players
//
#define LED_PINS_PWM_NUM_PINS 4        // number of LED pins to control
#define LED_PINS_PWM_FREQ 500          // Arduino Uno is ~490 Hz. ESP32 example uses 5,000Hz
#define LED_PINS_PWM_VAL_NUM_BITS 8    // Use same resolution as Uno (8 bits, 0-255) but ESP32 can go up to 16 bits (some versions less)
#define TIME_SCALE_EQUAL 64           // pwm_pin_info factors for time_ entries: millisec = (time_<whatever> / TIME_SCALE_EQUAL)
                                      // this allows a relatively smooth scaling of time by setting
                                      // 2^32millisec / 64 is about 18 hours, so that will exceed any gig time for the banjo players
```

### Do Not Change These Constants
[Top](#ledpinspwmtemplate-\--my-method-to-use-ledc-pwm "Top")<br>
```C
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// constants for led_pins_pwm LEDC PWM library to control the LED Eyes of the Banjo Players
//
#define LED_PINS_PWM_NO_CHANGE 0xFFFF  // no change to current pwm value when init pattern (.start_set_pwm or init function)
#define LED_PINS_PWM_USE_PTRN  0xFFFE  // use pwm value from pattern step when init pattern (init function only)
#define LED_PINS_PWM_MAX_VALUE ((1 << LED_PINS_PWM_VAL_NUM_BITS)-1) // max pwm value (255 if LED_PINS_PWM_VAL_NUM_BITS is 8)
```

## Data Structures

### Typedef for Pattern Step
[Top](#ledpinspwmtemplate-\--my-method-to-use-ledc-pwm "Top")<br>
```C
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// structure typedefs for led_pins_pwm LEDC PWM library to control the LED Eyes of the Banjo Players
typedef struct {
  uint16_t  start_set_pwm; // if not LED_PINS_PWM_NO_CHANGE, set pwm value to this
  int16_t   step_incr;     // if # >= 0, increment step counter by #; if # < 0 go to step "-# - 1"
  uint16_t  step_time;     // how long (millisec B4 scaling) until this step is complete
  uint16_t  tick_time;     // how long (millisec B4 scaling) between each tick
  int16_t   tick_pwm;      // how far and what direction for pwm value each tick_time
} pwm_led_ptrn_step;
```

### Typedef for Pins Controlled by LEDPinsPwmTemplate
[Top](#ledpinspwmtemplate-\--my-method-to-use-ledc-pwm "Top")<br>
```C
typedef struct {
  uint16_t pin_num;             // pin number executing this pattern
  pwm_led_ptrn_step * ptrn_step_ptr; // pointer to array of pwm_led_ptrn_step (steps)
  uint16_t idx_curr_step;       // index into array of steps for current step
  uint16_t curr_pwm_val;        // current pwm intensity from 0 to LED_PINS_PWM_MAX_VALUE
  uint16_t prev_pwm_val;        // previous pwm intensity from 0 to LED_PINS_PWM_MAX_VALUE
  uint32_t scale_factor;        // to stretch time. scale_factor == TIME_SCALE_EQUAL means no stretch
  uint32_t scaledtm_tick_incr;  // time (tick_time * TIME_SCALE_EQUAL) to increment scaledtm_next_tick
  uint32_t scaledtm_next_tick;  // time (millisec * TIME_SCALE_EQUAL) to start next tick
  uint32_t scaledtm_next_step;  // time (millisec * TIME_SCALE_EQUAL) to go to next step
} pwm_pin_info;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// we need the user to prepare this struct for us
//
extern pwm_pin_info g_pwm_pin_info[LED_PINS_PWM_NUM_PINS];
```

## Function Declarations
[Top](#ledpinspwmtemplate-\--my-method-to-use-ledc-pwm "Top")<br>

### led_pins_pwm_init - in setup do hardware initialization
[Top](#ledpinspwmtemplate-\--my-method-to-use-ledc-pwm "Top")<br>
```C
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// led_pins_pwm_init() - initialize pwm pins and set value to zero
//
// parameters (* means optional, has default as shown in declaration):
//   p_pwm_freq          - Frequency to use in Hertz. Usually between 500 and 5,000
//   p_pwm_val_num_bits  - Number of bits for value. Usually between 8 and 14
//                         If this is  8 then pwm_val would be between 0 and 255
//                         If this is 12 then pwm_val would be between 0 and 4095
// * p_num_pwm_scale     - numerator for final pwm scaling
// * p_den_pwm_scale     - denominator for final pwm scaling - should NOT be zero
// * p_serial_debugging  - zero for no serial debugging, 1 for serial debugging
//
int16_t led_pins_pwm_init(uint16_t p_pwm_freq, uint16_t p_pwm_val_num_bits, uint16_t p_num_pwm_scale = 1, uint16_t p_den_pwm_scale = 1, uint16_t p_serial_debugging = 0);
```

### led_pin_pwm_init_ptrn - in loop or setup start a pattern for a particular pin
[Top](#ledpinspwmtemplate-\--my-method-to-use-ledc-pwm "Top")<br>
```C
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// led_pin_pwm_init_ptrn() - initialize a pwm_pin_info entry for a particular pattern
//    Called once each time a new pattern is set for a particular p_pin_idx in g_pwm_pin_info
//
// parameters (* means optional, has default as shown in declaration):
//    p_pin_idx        - must be a valid pin idx in g_pwm_pin_info
//    p_ptrn_ptr       - must point to an array of steps, with at least (1+p_idx_start_step) steps in the array
// *  p_idx_start_step - usually 0, occasionally 1, I don't know why it would be > 1
// *  p_scale_factor   - = TIME_SCALE_EQUAL means use the pattern millisec counts as-is
//                       > TIME_SCALE_EQUAL means go faster than pattern by factor (p_scale_factor/TIME_SCALE_EQUAL)
//                       < TIME_SCALE_EQUAL means go slower than pattern by factor (p_scale_factor/TIME_SCALE_EQUAL)
//                       = 0 (special case) means stay on step p_idx_start_step forever and never tick
// *  p_pwm_val_init   - = LED_PINS_PWM_NO_CHANGE means leave curr_pwm_val alone, do not change it from current value
//                       = LED_PINS_PWM_USE_PTRN  means store the pattern start_set_pwm into curr_pwm_val
//                       = other         means store the parameter p_pwm_val_init into curr_pwm_val
//
void led_pin_pwm_init_ptrn(int p_pin_idx, pwm_led_ptrn_step* p_ptrn_ptr, uint16_t p_idx_start_step = 0, uint32_t p_scale_factor=TIME_SCALE_EQUAL, uint16_t p_pwm_val_init=LED_PINS_PWM_USE_PTRN);
```

### led_pins_pwm - in loop calculate PWM values and if changed output to hardware
[Top](#ledpinspwmtemplate-\--my-method-to-use-ledc-pwm "Top")<br>
```C
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// led_pins_pwm() - do calculations and output values for pwm for all pins based on g_pwm_pin_info
//
// should have called led_pins_pwm_init() and each g_pwm_pin_info[pin_idx] should have called led_pin_pwm_init_ptrn()
//
void led_pins_pwm();
```

### led_pin_pwm_set_pwm_scale - in loop or setup set global scale factor for PWM value 
[Top](#ledpinspwmtemplate-\--my-method-to-use-ledc-pwm "Top")<br>
```C
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// led_pin_pwm_set_pwm_scale() - set valid global values for pwm scaling during operation
//
// parameters:
//   MS-16bits p_pwm_scale  - numerator for final pwm scaling
//   LS-16bits p_pwm_scale  - denominator for final pwm scaling - should NOT be zero
//
extern uint32_t g_eyes_bright; // MS-16bits is <num>, LS-16bits is <den> for last call to led_pin_pwm_set_pwm_scale()
void led_pin_pwm_set_pwm_scale(uint32_t p_pwm_scale);
```

## Debugging Functions
[Top](#ledpinspwmtemplate-\--my-method-to-use-ledc-pwm "Top")<br>

### led_pin_pwm_set_dbg_enable - enable or disable debugging output
[Top](#ledpinspwmtemplate-\--my-method-to-use-ledc-pwm "Top")<br>
```C
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// led_pin_pwm_set_dbg_enable() - routine for debugging
//
//   p_enable - nonzero to enable led_pin_pwm debug prints; zero to disable
//
void led_pin_pwm_set_dbg_enable(uint16_t p_enable);
```

### led_pin_pwm_int_dbg_step - display debug info on state of pin and steps structures
[Top](#ledpinspwmtemplate-\--my-method-to-use-ledc-pwm "Top")<br>
```C
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// led_pin_pwm_int_dbg_step() - internal routine for debugging, display steps at important steps
//
void led_pin_pwm_int_dbg_step(int p_pin_idx); // to keep the compiler happy
```
