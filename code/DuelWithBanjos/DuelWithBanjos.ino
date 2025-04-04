// DuelWithBanjos 2024
// Author: https://github.com/Mark-MDO47/
//

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

// The idea is to play a song such as "Dueling Banjos" repeatedly on the speaker.
//    Alternatively we can play banjo Christmas music or even turn the music off.
//    Actually any sort of music or sound would be possible.
// Additionally we will control and fade the banjo player LED eyes.

// We will use UniRemote to control our activities - see https://github.com/Mark-MDO47/UniRemote
//    Because UniRemote uses ESP-NOW, DuelWithBanjos must run on an ESP32.
//    We will use an inexpensive ESP-WROOM-32 and this guides us on selection of UART and GPIO pins
//       https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
//       https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32_datasheet_en.pdf

// connections:
//
// YX5200/DFPlayer Sound Player
//   ESP32 Dev Module pin D-16   DPIN_HWSRL_RX   Arduino RX; YX5200 TX - 9600 Baud
//   ESP32 Dev Module pin D-17   DPIN_HWSRL_TX   Arduino TX; YX5200 RX - 9600 Baud
//   ESP32 Dev Module pin D-23   DPIN_AUDIO_BUSY YX5200 BUSY; HIGH when audio finishes
//
// LED PWM
//   ESP32 Dev Module pin D-18   left  splayer eye 1
//   ESP32 Dev Module pin D-19   left  player eye 2
//   ESP32 Dev Module pin D-32   right player eye 1
//   ESP32 Dev Module pin D-33   right player eye 2

// Attributions for the sounds are on the MicroSD card containing the sounds.
// The music is either purchased or else performed/recorded by me.
// Purchased music at this time is as follows:
//    Dueling Banjos, Curtis McPeake 2009, https://www.amazon.com/Dueling-Banjos-CURTIS-OTHERS-MCPEAKE/dp/B0001MZ854
//    Todd Taylor's Banjo Christmas, Todd Taylor 2015, https://www.amazon.com/dp/1574243217

// See https://github.com/Mark-MDO47/AudioPlayer-YX5200 for details on using the YX5200/DFPlayer.
//   I usually install a copy of the DFRobot.com DFPlayer code when using it, since I did a lot
//   of debugging to find how to use it on all the variants of the YX5200 I have seen.
//   The still unmodified DFRobot.com DFPlayer code files are DFRobotDFPlayerMini.*.
//   LICENSE_for_DFRobot_code.txt shows it is OK to do this and describes the legal boundaries
//   for correct usage.

#include "Arduino.h"              // general Arduino definitions plus uint8_t etc.
#include <FastLED.h>              // only for the convenient EVERY_N_MILLISECONDS() macro - too lazy to write my own...

#include <UniRemoteRcvr.h>        // for UniRemoteRcvr "library"

#include "HardwareSerial.h"       // to talk with the YX5200
#include "DFRobotDFPlayerMini.h"  // to communicate with the YX5200 audio player
#include "SoundNum.h"             // numbers for each sound

#include "LEDPinsPwm.h"           // to control LED eyes with Pulse Width Modulation


////////////////////////////////////////////////////////////////////////////////////////
// definitions for YX5200/DFPlayer and ESP-WROOM-32 serial port 

#define DPIN_HWSRL_RX   16  // HW-serial in  - talk to DFPlayer audio player (YX5200)
#define DPIN_HWSRL_TX   17  // HW-serial out - talk to DFPlayer audio player (YX5200)
#define DPIN_AUDIO_BUSY 23  // digital input - HIGH when audio finishes
DFRobotDFPlayerMini myDFPlayer;                                // to talk to YX5200 audio player
void DFsetup();                                                // how to initialize myDFPlayer
#define SOUND_DEFAULT_VOL     30  // default volume - 25 is pretty good
#define SOUND_BKGRND_VOL      20  // background volume
#define SOUND_ACTIVE_PROTECT 200  // milliseconds to keep SW twiddled sound active after doing myDFPlayer.play(mySound)
uint32_t gTimerForceSoundActv = 0;  // SOUND_ACTIVE_PROTECT until millis() >= this

#define DFCHANGEVOLUME 0 // zero does not change sound volume
// #define DFPRINTDETAIL 1 // if need detailed status from myDFPlayer (YX5200 communications)
#define DFPRINTDETAIL 0  // will not print detailed status from myDFPlayer
#if DFPRINTDETAIL // routine to do detailed debugging
  void DFprintDetail(uint8_t type, int value); // definition of call
#else  // no DFPRINTDETAIL
  #define DFprintDetail(type, value) // nothing at all
#endif // #if DFPRINTDETAIL

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// structures for LEDPinsPwm to control the LED Eyes of the Banjo Players

// pin definitions - name must be must be g_pwm_pin_info, must be one row for each PWM LED output pin
//
pwm_pin_info g_pwm_pin_info[LED_PINS_PWM_NUM_PINS] = {
  { .pin_num=18, .ptrn_step_ptr = (pwm_led_ptrn_step *)0, .idx_curr_step=0, .curr_pwm_val=0, .prev_pwm_val = 0xFFFF, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_tick_incr = 0, .scaledtm_next_tick=0, .scaledtm_next_step=0 },
  { .pin_num=19, .ptrn_step_ptr = (pwm_led_ptrn_step *)0, .idx_curr_step=0, .curr_pwm_val=0, .prev_pwm_val = 0xFFFF, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_tick_incr = 0, .scaledtm_next_tick=0, .scaledtm_next_step=0 },
  { .pin_num=32, .ptrn_step_ptr = (pwm_led_ptrn_step *)0, .idx_curr_step=0, .curr_pwm_val=0, .prev_pwm_val = 0xFFFF, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_tick_incr = 0, .scaledtm_next_tick=0, .scaledtm_next_step=0 },
  { .pin_num=33, .ptrn_step_ptr = (pwm_led_ptrn_step *)0, .idx_curr_step=0, .curr_pwm_val=0, .prev_pwm_val = 0xFFFF, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_tick_incr = 0, .scaledtm_next_tick=0, .scaledtm_next_step=0 }
};

// pwm LED pattern definitions - name can be anything, can be as many pattern definitions as desired,
//         can have as many steps per pattern as desired
//
pwm_led_ptrn_step g_pwm_ptrn_open_eye[] = { 
  { .start_set_pwm=0,                     .step_incr=1,  .step_time=1757, .tick_time=5, .tick_pwm= 2},
  { .start_set_pwm=LED_PINS_PWM_NO_CHANGE, .step_incr=-1, .step_time=1000, .tick_time=5, .tick_pwm=-7}
};
pwm_led_ptrn_step g_pwm_ptrn_blink[] = { 
  { .start_set_pwm=0,                      .step_incr=1,  .step_time=450, .tick_time=5, .tick_pwm= 0},
  { .start_set_pwm=LED_PINS_PWM_MAX_VALUE, .step_incr=-1, .step_time=450, .tick_time=5, .tick_pwm= 0}
};

pwm_led_ptrn_step g_pwm_ptrn_sinelon0[] = { 
  { .start_set_pwm=0,                      .step_incr=2,  .step_time=500, .tick_time=5, .tick_pwm= 3},
  { .start_set_pwm=0,                      .step_incr=1,  .step_time=500, .tick_time=5, .tick_pwm= 3},
  { .start_set_pwm=LED_PINS_PWM_NO_CHANGE, .step_incr=-2, .step_time=500, .tick_time=5, .tick_pwm=-3}
};
pwm_led_ptrn_step g_pwm_ptrn_sinelon1[] = { 
  { .start_set_pwm=0,                      .step_incr=2,  .step_time=250, .tick_time=5, .tick_pwm= 0},
  { .start_set_pwm=0,                      .step_incr=1,  .step_time=500, .tick_time=5, .tick_pwm= 3},
  { .start_set_pwm=LED_PINS_PWM_NO_CHANGE, .step_incr=-2, .step_time=500, .tick_time=5, .tick_pwm=-3}
};
pwm_led_ptrn_step g_pwm_ptrn_sinelon2[] = {
  { .start_set_pwm=0,                      .step_incr=2,  .step_time=500, .tick_time=5, .tick_pwm= 0},
  { .start_set_pwm=0,                      .step_incr=1,  .step_time=500, .tick_time=5, .tick_pwm= 3},
  { .start_set_pwm=LED_PINS_PWM_NO_CHANGE, .step_incr=-2, .step_time=500, .tick_time=5, .tick_pwm=-3}
};
pwm_led_ptrn_step g_pwm_ptrn_sinelon3[] = {
  { .start_set_pwm=0,                      .step_incr=2,  .step_time=750, .tick_time=5, .tick_pwm= 0},
  { .start_set_pwm=0,                      .step_incr=1,  .step_time=500, .tick_time=5, .tick_pwm= 3},
  { .start_set_pwm=LED_PINS_PWM_NO_CHANGE, .step_incr=-2, .step_time=500, .tick_time=5, .tick_pwm=-3}
};
pwm_led_ptrn_step* g_pwm_ptrn_sinelon_ptrs[LED_PINS_PWM_NUM_PINS] = {g_pwm_ptrn_sinelon0, g_pwm_ptrn_sinelon1, g_pwm_ptrn_sinelon2, g_pwm_ptrn_sinelon3};

pwm_led_ptrn_step g_pwm_ptrn_off[] = {
  { .start_set_pwm=0,                      .step_incr=-1, .step_time=750, .tick_time=5, .tick_pwm= 0}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////
#if DFPRINTDETAIL
void DFprintDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!")); break;
    case WrongStack:
      Serial.println(F("Stack Wrong!")); break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!")); break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!")); break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!")); break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!"); break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!"); break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:")); Serial.print(value); Serial.println(F(" Play Finished!"));  break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      } // end switch (value)
      break;
    default:
      break;
  }  // end switch (type)
} // end DFprintDetail()
#endif // DFPRINTDETAIL

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// DFstartSound(tmpSoundNum, tmpVolume) - start tmpSoundNum if it is valid
//
// tmpSoundNum is the sound file number.
//
// Had lots of trouble with reliable operation using playMp3Folder. Came to conclusion
//    that it is best to use the most primitive of YX5200 commands.
// Also saw strong correlation of using YX5200 ACK and having even more unreliable
//    operation, so turned that off in DFinit.
// The code checking DPIN_AUDIO_BUSY still needs the delay but 200 millisec is probably overkill.
// There is code checking myDFPlayer.available() that can maybe also be removed now
//    that the dubugging for above is finished. Now that I am using myDFPlayer.play(),
//    it only seems to trigger when I interrupt a playing sound by starting another.
//    It is sort of interesting but not needed.
//
void  DFstartSound(uint16_t tmpSoundNum, uint16_t tmpVolume) {
  uint16_t idx;
  bool prevHI;
  uint16_t mySound = tmpSoundNum;
  static uint8_t init_minmax = 2;
  static uint32_t prev_msec;
  static uint32_t max_msec = 0;
  static uint32_t min_msec = 999999;
  uint32_t diff_msec = 0;
  uint32_t now_msec = millis();


#if DFCHANGEVOLUME
  myDFPlayer.volume(tmpVolume);  // Set volume value. From 0 to 30 - FIXME 25 is good
#if DFPRINTDETAIL
  if (myDFPlayer.available()) {
    Serial.print(F(" DFstartSound ln ")); Serial.print((uint16_t) __LINE__); Serial.println(F(" myDFPlayer problem after volume"));
    DFprintDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }
#endif // DFPRINTDETAIL
#endif // DFCHANGEVOLUME

  myDFPlayer.play(mySound); //play specific wav in SD: root directory ###.wav; number played is physical copy order; first one copied is 1
  // Serial.print(F("DEBUG DFstartSound myDFPlayer.play(")); Serial.print((uint16_t) mySound); Serial.println(F(")"));
  gTimerForceSoundActv = millis() + SOUND_ACTIVE_PROTECT; // handle YX5200 problem with interrupting play

  if (init_minmax) {
    init_minmax -= 1;
  }  else {
    diff_msec = now_msec - prev_msec;
    if (diff_msec > max_msec) {
      max_msec = diff_msec;
      // Serial.print(F("max_msec ")); Serial.println(max_msec);
    } else if (diff_msec < min_msec) {
      min_msec = diff_msec;
      // Serial.print(F("min_msec ")); Serial.println(min_msec);
    }
  }
  prev_msec = now_msec;
} // end DFstartSound()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// DFcheckSoundDone() - returns TRUE if previous sound is complete (DFPlayer/YX5200 is notBusy)
//
// notBusy means (HIGH == digitalRead(DPIN_AUDIO_BUSY)) && (millis() >= gTimerForceSoundActv)
//
//    DPIN_AUDIO_BUSY goes HIGH when sound finished, but may take a while to start being high after sound starts
//    gTimerForceSoundActv is millisec count we have to wait for to before checking DPIN_AUDIO_BUSY
//    so we have to be BOTH (DPIN_AUDIO_BUSY is HIGH) AND (current time is beyond gTimerForceSoundActv)
//
uint8_t DFcheckSoundDone() {
  return (HIGH == digitalRead(DPIN_AUDIO_BUSY)) && (millis() >= gTimerForceSoundActv);
} // end DFcheckSoundDone()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// DFsetup()
void DFsetup() {
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  pinMode(DPIN_AUDIO_BUSY,  INPUT_PULLUP); // HIGH when audio stops
  Serial2.begin(9600, SERIAL_8N1, DPIN_HWSRL_RX, DPIN_HWSRL_TX); // this is control to DFPlayer audio player 
  if (!myDFPlayer.begin(Serial2, false, true)) {  // Serial2 to communicate with mp3 player
    Serial.println(F("Unable to begin DFPlayer:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true){
      delay(1);
    }
  }
  myDFPlayer.EQ(DFPLAYER_EQ_BASS); // our speaker is quite small
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD); // location of sound files is MicroSD card
  myDFPlayer.volume(SOUND_DEFAULT_VOL);  // Set volume value. From 0 to 30 - FIXME 25 is good
  // delay(3000); // allow bluetooth connection to complete - not using bluetooth; tinny sound from small speaker adds to effect
  delay(1000); // allow DFPlayer to stabilize
  Serial.println(F("DFPlayer Mini online."));

} // end DFsetup()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// setup()
void setup() {
  Serial.begin(115200);         // this serial communication is for general debug; set the USB serial port to 115,200 baud
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  delay(3000);
  Serial.println(""); // print a blank line in case there is some junk from power-on
  Serial.println("starting DuelWithBanjos");

  // init UniRemoteRcvr - inits WiFi mode to WIFI_STA and inits ESP-NOW
  esp_err_t status_init_uni_remote_rcvr = uni_remote_rcvr_init();
  if (status_init_uni_remote_rcvr != ESP_OK) { // (== UNI_REMOTE_RCVR_OK)
    // handle error status
    Serial.println("Error Initializing uni_remote_rcvr!");
    // while (1) ; // don't need to hang; can still play music
  }

  // connect to Banjo Player LED eyes and initially turn off
  Serial.println("\nInitialize LEDPinsPwm");
  if (led_pins_pwm_init(LED_PINS_PWM_FREQ, LED_PINS_PWM_VAL_NUM_BITS)) {
    while (1) ;
  } // end if error in initialization
  for (int pin_idx = 0; pin_idx < NUMOF(g_pwm_pin_info); pin_idx += 1) {
    led_pin_pwm_init_ptrn(pin_idx, g_pwm_ptrn_blink, 0, TIME_SCALE_EQUAL /* + pin_idx*2 */, 0);
  } // end for each pin_idx
  led_pin_pwm_set_pwm_scale(1,5);

  // initialize the YX5200 DFPlayer audio player
  Serial.println("\nInitialize YX5200");
  DFsetup();

  Serial.println("\nDuelWithBanjos init complete...");

  // start the INTRO sound, then allow normal loop() processing
  DFstartSound(SOUNDNUM_DuelingBanjos, SOUND_DEFAULT_VOL);
/*
  while (!DFcheckSoundDone()) {
    delay(10); // wait for the INTRO sound to finish
  } // end while
  Serial.println("Intro Sound Complete");
*/
} // end setup()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_cmd_volume() - stub versions
//       returns: 0 if no error
//   process VOLUME: command if we understand it; call with p_cmd and p_param ALL UPPERCASE
//
// VOLUME:UP #
// VOLUME:DOWN #
// VOLUME:SET #
//
uint16_t do_cmd_volume(char* p_cmd, char* p_param) {
    Serial.printf("do_cmd_volume %s %s\n", p_cmd, p_param);
    return(0);
} // end do_cmd_volume()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_cmd_music() - stub versions
//       returns: 0 if no error
//   process MUSIC: command if we understand it; call with p_cmd and p_param ALL UPPERCASE
//
// MUSIC:SONG <name>   = (<name> = DUEL-BANJO DECK-HALLS WHAT-CHILD MERRY-GENTLEMEN TOWN-BETHLEHEM KING-WENCESLAS)
// MUSIC:TYPE <type>   = (<type> = DUEL CHRISTMAS CHOPIN ALL)
// MUSIC:OFF  <ignore> = (<ignore> = anything; I use OFF)
//
uint16_t do_cmd_music(char* p_cmd, char* p_param) {
    Serial.printf("do_cmd_music %s %s\n", p_cmd, p_param);
    return(0);
} // end do_cmd_music()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_cmd_eyes() - stub version
//       returns: 0 if no error
//   process EYES: command if we understand it; call with p_cmd and p_param ALL UPPERCASE
//
// EYES:PATTERN <coord>/<tscale>/<ptrn> = (<coord> = TOGETHER SEPARATE OPPOSITE - <coord> no effect for SINELON or OFF) (<tscale> = TBS FIXME TODO 64)  (<ptrn> = BLINK OPEN BL-OPN SINELON OFF)
// EYES:CYCLE   <coord>/<tscale>        = (<coord> = TOGETHER SEPARATE OPPOSITE - <coord> no effect for SINELON or OFF) (<tscale> = TBS FIXME TODO 64) 
// EYES:BRIGHT  <num>/<den>             = (<num> = numerator of fraction) (<den> = denominator of fraction) (NOTE: 0 <= num/den <= 1, den != 0)
//
uint16_t do_cmd_eyes(char* p_cmd, char* p_param) {
  char delimiters[] = "/ \t";
  char* param1;
  char* param2;
  char* param3;
  static char tmp_msg[ESP_NOW_MAX_DATA_LEN];

  strncpy(tmp_msg, p_param, ESP_NOW_MAX_DATA_LEN);
  param1 = strtok(tmp_msg, delimiters);
  if (param1 == NULL) { Serial.printf("ERROR ESP-NOW cmd %s %s bad parameter 1\n", p_cmd, p_param);  return(1); }
  param2 = strtok(NULL, delimiters);
  if (param2 == NULL) { Serial.printf("ERROR ESP-NOW cmd %s %s bad parameter 2\n", p_cmd, p_param);  return(1); }

  Serial.printf("do_cmd_eyes %s %s\n", p_cmd, p_param);
  if (NULL != strstr("EYES:PATTERN", p_cmd)) {
    // <coord>/<tscale>/<ptrn> FIXME TODO implement coord
    param3 = strtok(NULL, delimiters);
    if (param3 == NULL) { Serial.printf("ERROR ESP-NOW cmd %s %s bad parameter 3\n", p_cmd, p_param);  return(1); }
    uint16_t tscale = atoi(param2);
    if (NULL != strstr("BLINK", param3)) {
    } else if (NULL != strstr("OPEN", param3)) {
    } else if (NULL != strstr("BL-OPN", param3)) {
    } else if (NULL != strstr("SINELON", param3)) {
      for (int pin_idx = 0; pin_idx < NUMOF(g_pwm_pin_info); pin_idx += 1) {
        led_pin_pwm_init_ptrn(pin_idx, g_pwm_ptrn_sinelon_ptrs[pin_idx], 0, tscale, LED_PINS_PWM_USE_PTRN);
      } // end for each pin_idx
    } else if (NULL != strstr("OFF", param3)) {
      for (int pin_idx = 0; pin_idx < NUMOF(g_pwm_pin_info); pin_idx += 1) {
        led_pin_pwm_init_ptrn(pin_idx, g_pwm_ptrn_off, 0, TIME_SCALE_EQUAL, LED_PINS_PWM_USE_PTRN);
      } // end for each pin_idx
    } else { Serial.printf("ERROR ESP-NOW cmd %s %s no such pattern\n", p_cmd, p_param);  return(1); }
  } else if (NULL != strstr("EYES:CYCLE", p_cmd)) {
  } else if (NULL != strstr("EYES:BRIGHT", p_cmd)) {
  } else { Serial.printf("ERROR ESP-NOW cmd %s %s no such command\n", p_cmd, p_param);  return(1); }
  return(0);
} // end do_cmd_eyes()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_esp_now_command()
//       returns: nothing
//   process command if we understand it; call with my_message ALL UPPERCASE
//
//   "BANJO" is <CMD_VERIFIER> only at the start of the line before first comand
//     this just helps identify command destination on screen in UniRemote
//   each command is " ; <command> <param>"
//     do not place ";" at the end of the line
//   Example: "BANJO ; EYES:PATTERN TOGETHER/64/SINELON ; MUSIC:SONG DUEL-BANJO"
//
//       <CMD_VERIFIER>: "BANJO"
//       <command>: "EYES:PATTERN" <param>: "TOGETHER/SINELON"
//       <command>: "MUSIC:SONG"   <param>: "DUEL-BANJO"
//
// BANJO ; VOLUME:UP #
// BANJO ; VOLUME:DOWN #
// BANJO ; VOLUME:SET #
//
// BANJO ; MUSIC:SONG <name>   = (<name> = DUEL-BANJO DECK-HALLS WHAT-CHILD MERRY-GENTLEMEN TOWN-BETHLEHEM KING-WENCESLAS)
// BANJO ; MUSIC:TYPE <type>   = (<type> = DUEL CHRISTMAS CHOPIN ALL)
// BANJO ; MUSIC:OFF  <ignore> = (<ignore> = anything; I use OFF)
//
// BANJO ; EYES:PATTERN <coord>/<tscale>/<ptrn> = (<coord> = TOGETHER SEPARATE OPPOSITE - <coord> no effect for SINELON or OFF) (<tscale> = TBS FIXME TODO 64)  (<ptrn> = BLINK OPEN BL-OPN SINELON OFF)
// BANJO ; EYES:CYCLE   <coord>/<tscale>        = (<coord> = TOGETHER SEPARATE OPPOSITE - <coord> no effect for SINELON or OFF) (<tscale> = TBS FIXME TODO 64) 
// BANJO ; EYES:BRIGHT  <num>/<den>             = (<num> = numerator of fraction) (<den> = denominator of fraction) (NOTE: 0 <= num/den <= 1, den != 0)
//
typedef struct {
  const char * cmd;
  uint16_t cmd_idx;
} esp_now_cmd_t;
esp_now_cmd_t esp_now_cmds[] = {
  {.cmd = "VOLUME:UP",    .cmd_idx = 0x0000 },
  {.cmd = "VOLUME:DOWN",  .cmd_idx = 0x0001 },
  {.cmd = "VOLUME:SET",   .cmd_idx = 0x0002 },
  {.cmd = "MUSIC:SONG",   .cmd_idx = 0x0100 },
  {.cmd = "MUSIC:TYPE",   .cmd_idx = 0x0101 },
  {.cmd = "MUSIC:OFF",    .cmd_idx = 0x0102 },
  {.cmd = "EYES:PATTERN", .cmd_idx = 0x0200 },
  {.cmd = "EYES:CYCLE",   .cmd_idx = 0x0201 },
  {.cmd = "EYES:BRIGHT",  .cmd_idx = 0x0202 }
};
#define CMD_VERIFIER "BANJO"

typedef uint16_t(*do_cmd_type_t)(char*, char*);

do_cmd_type_t esp_now_cmd_ptrs[] = { &do_cmd_volume, &do_cmd_music, &do_cmd_eyes };

uint16_t do_esp_now_command(uint16_t rcvd_len, char* my_message) {
  // all commands to banjo players start with "BANJO" a.k.a. CMD_VERIFIER
  uint16_t ret_val = 1; // error unless complete
  char delimiters[] = " ";
  char* token;
  char* param_token;

  static char tmp_msg[ESP_NOW_MAX_DATA_LEN];
  strncpy(tmp_msg, my_message, ESP_NOW_MAX_DATA_LEN);
  token = strtok(tmp_msg, delimiters);
  if (token == NULL) { Serial.printf("ERROR ESP-NOW CMD_VERIFIER missing, not %s\n", CMD_VERIFIER);  return(1); }
  else if (NULL == strstr(token, CMD_VERIFIER)) { Serial.printf("ERROR ESP-NOW CMD_VERIFIER %s not %s\n", token, CMD_VERIFIER);  return(1); }
  Serial.printf("CMD_VERIFIER: |%s| seen: |%s|\n", token, CMD_VERIFIER);

  while (1) {
    token = strtok(NULL, delimiters);
    if ((token == NULL) || (NULL == strstr(token, ";"))) { ret_val = 0; break; }
    token = strtok(NULL, delimiters);
    if ((token == NULL) || (NULL == strstr(token, ":"))) break;
    param_token = strtok(NULL, delimiters);
    if (param_token == NULL) break;
    Serial.printf("cmd: |%s| param |%s|\n", token, param_token);
    int not_found = 1;
    for (int i = 0; (i < NUMOF(esp_now_cmds)) && not_found; i += 1) {
      if (!strcmp(token, esp_now_cmds[i].cmd)) {
        Serial.printf("  found %s %s cmd_idx:0x%04x\n", token, param_token, esp_now_cmds[i].cmd_idx);
        uint16_t cmd_type = (esp_now_cmds[i].cmd_idx >> 8) & 0xFF;
        uint16_t cmd_idx  = esp_now_cmds[i].cmd_idx & 0xFF;
        esp_now_cmd_ptrs[cmd_type](token, param_token);
        not_found = 0;
      }
    }
    if (not_found) { Serial.printf("ERROR ESP-NOW cmd %s not in list\n", token); return(1); }
  }
  if (0 == ret_val) { Serial.printf("ESP-NOW command done: %s\n", my_message); }
  return(ret_val);
} // end do_esp_now_command()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// loop()
//
void loop() {
  static char my_message[ESP_NOW_MAX_DATA_LEN];     // received message
  static uint8_t sender_mac_addr[ESP_NOW_ETH_ALEN]; // sender MAC address
  static uint32_t my_message_num = 0;               // increments for each msg received unless UNI_REMOTE_RCVR_ERR_CBUF_MSG_DROPPED
  static uint16_t count_eyes = 0;
  uint16_t rcvd_len = 0; // the length of the message/command. If zero, no message.

  // get any message received. If 0 == rcvd_len, no message.
  esp_err_t msg_status = uni_remote_rcvr_get_msg(&rcvd_len, &my_message[0], &sender_mac_addr[0], &my_message_num);

#ifdef HANDLE_CERTAIN_UNLIKELY_ERRORS
  // we can get an error even if no message
  if (msg_status != UNI_REMOTE_RCVR_OK) { // (== ESP_OK)
    // handle error status here

    // these error codes come from set/clear flags; clear so can detect next time
    if ((UNI_REMOTE_RCVR_ERR_CBUF_MSG_DROPPED == msg_status) || (UNI_REMOTE_RCVR_ERR_MSG_TOO_BIG == msg_status)) {
      uni_remote_rcvr_clear_extended_status_flags();
    }
  }
#endif // HANDLE_CERTAIN_UNLIKELY_ERRORS

  // we can get a message with or without an error; see above uni_remote_rcvr_clear_extended_status_flags()
  // If 0 == rcvd_len, no message.
  if (rcvd_len > 0) {
    // process command
    do_esp_now_command(rcvd_len, strupr(my_message));
  }
  EVERY_N_MILLISECONDS( 5 ) { 
    led_pins_pwm(); // if needed, output brightness of LED
  } // end EVERY_N_MILLISECONDS 5

  EVERY_N_MILLISECONDS( 50 ) { 
    if (DFcheckSoundDone()) {
      // restart sound
      DFstartSound(SOUNDNUM_DuelingBanjos, SOUND_DEFAULT_VOL);
    } // end if DFcheckSoundDone
  } // end EVERY_N_MILLISECONDS 50

/*
  EVERY_N_MILLISECONDS( 20000 ) {
    count_eyes ^= 1;
    for (int pin_idx = 0; pin_idx < NUMOF(g_pwm_pin_info); pin_idx += 1) {
      if (count_eyes ^ (pin_idx%2)) {
        led_pin_pwm_init_ptrn(pin_idx, g_pwm_ptrn_open_eye, 0, TIME_SCALE_EQUAL, 0);
        Serial.printf("  pin_id=%d count_eyes=%d ^=%d msec=%ld open_eye\n", pin_idx, count_eyes, count_eyes ^ (pin_idx%2),millis());
      }
      else {
        led_pin_pwm_init_ptrn(pin_idx, g_pwm_ptrn_blink, 0, TIME_SCALE_EQUAL, 0);
        Serial.printf("  pin_id=%d count_eyes=%d ^=%d msec=%ld blink\n", pin_idx, count_eyes, count_eyes ^ (pin_idx%2),millis());
      }
    } // end for each pin_idx

    for (int pin_idx = 0; pin_idx < NUMOF(g_pwm_pin_info); pin_idx += 1) {
      led_pin_pwm_int_dbg_step(pin_idx);
    }
  }
 */
} // end loop()
