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

// This can be updated via an "OTA:WEB" ESP-NOW command, which will cause us to open a web page
//   on the specified SSID allowing Over-The-Air software update. That capability is based on
//   the ESP32 example OTAWebUpdater.ino

// connections:
//
// YX5200/DFPlayer Sound Player
//   ESP32 Dev Module pin D-16   DPIN_HWSRL_RX   Arduino RX; YX5200 TX - 9600 Baud
//   ESP32 Dev Module pin D-17   DPIN_HWSRL_TX   Arduino TX; YX5200 RX - 9600 Baud
//   ESP32 Dev Module pin D-23   DPIN_AUDIO_BUSY YX5200 BUSY; HIGH when audio finishes
//
// LED PWM
//   ESP32 Dev Module pin D-18   left  player left eye  (green)
//   ESP32 Dev Module pin D-19   left  player right eye (red)
//   ESP32 Dev Module pin D-32   right player left eye  (green)
//   ESP32 Dev Module pin D-26   right player right eye (red) (33 exceeded my soldering capability)

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

#include "Arduino.h"                // general Arduino definitions plus uint8_t etc.
#include <FastLED.h>                // only for the convenient EVERY_N_MILLISECONDS() macro - too lazy to write my own...

// mdo_use_ota_webupdater https://github.com/Mark-MDO47/UniRemote/tree/master/code/mdo_use_ota_webupdater
// This does not connect to WiFi router until it gets a command
#define MDO_USE_OTA 1               // zero to not use, non-zero to use OTA ESP32 Over-The-Air software updates

// Visual Micro Over-The-Air Debugging https://www.visualmicro.com/page/OTA-Debugging-for-Espressif-ESP-Boards.aspx
// For this we need to connect to WiFi all the time
//    NOTE: somehow this interferes with ESP-NOW and also does not make OTA debug work. It does put the IP address on the router.
#define VM_OTA_DEBUG 0              // zero to not use, non-zero to use OTA ESP32 debugging with Visual Micro and Microsoft Visual Studio

#if (MDO_USE_OTA || VM_OTA_DEBUG)
#include "mdo_use_ota_webupdater.h" // for mdo_use_ota_webupdater "library"
#endif // (MDO_USE_OTA || VM_OTA_DEBUG)
#include <UniRemoteRcvr.h>          // for UniRemoteRcvr "library"



#include "HardwareSerial.h"         // to talk with the YX5200
#include "DFRobotDFPlayerMini.h"    // to communicate with the YX5200 audio player
#include "SoundNum.h"               // numbers for each sound

#include "LEDPinsPwm.h"             // to control LED eyes with Pulse Width Modulation


////////////////////////////////////////////////////////////////////////////////////////
// definitions for YX5200/DFPlayer and ESP-WROOM-32 serial port and volume controls

#define DPIN_HWSRL_RX   16  // HW-serial in  - talk to DFPlayer audio player (YX5200)
#define DPIN_HWSRL_TX   17  // HW-serial out - talk to DFPlayer audio player (YX5200)
#define DPIN_AUDIO_BUSY 23  // digital input - HIGH when audio finishes
DFRobotDFPlayerMini myDFPlayer;                                // to talk to YX5200 audio player
void DFsetup();                                                // how to initialize myDFPlayer
constexpr uint32_t SOUND_ACTIVE_PROTECT = 200; // milliseconds to keep SW twiddled sound active after doing myDFPlayer.play(mySound)
uint32_t gTimerForceSoundActv = 0;             // SOUND_ACTIVE_PROTECT until millis() >= this

// Problem with dynamically setting volume - seems to prevent starting a different song until the current one finishes
#define VOLUME_ALLOW_CHANGE 0    // 1 if can dynamically set volume

#define VOLUME_GSCALE_DIVISOR 100
uint32_t g_volume_gscale = VOLUME_GSCALE_DIVISOR; // volume global scale factor. range: 0 to 200

constexpr uint16_t SOUND_VOL_MIN     =  0;  // range 0 to 30
constexpr uint16_t SOUND_VOL_MAX     = 30;  // range 0 to 30
constexpr uint16_t SOUND_VOL_DEFAULT = 25;  // default volume - range 0 to 30
uint16_t g_current_volume_set = SOUND_VOL_DEFAULT; // 

// #define DFPRINTDETAIL 1 // if need detailed status from myDFPlayer (YX5200 communications)
#define DFPRINTDETAIL 0  // will not print detailed status from myDFPlayer

#if DFPRINTDETAIL // routine to do detailed debugging
  void DFprintDetail(uint8_t type, int value); // definition of call
#else  // no DFPRINTDETAIL
  #define DFprintDetail(type, value) // nothing at all
#endif // #if DFPRINTDETAIL

uint16_t g_global_volume_scaling = 100;  // integer percentage volume (range 0 through 200)
uint16_t g_unscaled_volume;              // volume active now before scaling (range 0 thru 30)

typedef struct {
  char*    song_name;   // points to name of song to play
  uint16_t soundnum;    // number of sound
  uint16_t song_volume; // song-suggested volume
} music_song_to_soundnum_t;
static music_song_to_soundnum_t g_music_song_to_soundnum[] = {
  { .song_name = (char*)"DUEL-BANJO",                    .soundnum = SOUNDNUM_DuelingBanjos,                             .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"SILENCE",                       .soundnum = SOUNDNUM_silence,                                   .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"DECK-HALLS",                    .soundnum = SOUNDNUM_DeckTheDuelingHalls,                       .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"WHAT-CHILD",                    .soundnum = SOUNDNUM_WhatChildIsThis,                           .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"MERRY-GENTLEMEN",               .soundnum = SOUNDNUM_GodRestYe,                                 .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"TOWN-BETHLEHEM",                .soundnum = SOUNDNUM_OLittleTownOf,                             .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"KING-WENCESLAS",                .soundnum = SOUNDNUM_GoodKingWenceslas,                         .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"CHOPIN-ETUDE-TRISTESSE",        .soundnum = SOUNDNUM_Chopin_Etude_10_03,                        .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"CHOPIN-NOCTURNE-E-FLAT",        .soundnum = SOUNDNUM_Chopin_Noct_55_2,                          .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"CHOPIN-ETUDE-REVOLUTIONARY",    .soundnum = SOUNDNUM_Chopin_Etude_10_12,                        .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"CHOPIN-NOCTURNE-D-FLAT",        .soundnum = SOUNDNUM_Chopin_Noct_27_2,                          .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"CHOPIN-NOCTURNE-G",             .soundnum = SOUNDNUM_Chopin_Noct_37_2,                          .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"CHOPIN-PRELUDE-RAINDROP",       .soundnum = SOUNDNUM_Chopin_Prelude_15,                         .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"PATRIOT-BATTLE-HYMN-REPUBLIC",  .soundnum = SOUNDNUM_Patriotic_Battle_Hymn_of_the_Republic,     .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"PATRIOT-AMERICA-THE-BEAUTIFUL", .soundnum = SOUNDNUM_Patriotic_America_the_Beautiful,           .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"PATRIOT-JOHNNY-MARCHING-HOME",  .soundnum = SOUNDNUM_Patriotic_When_Johnny_Comes_Marching_Home, .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"PATRIOT-MARINE-HYMN",           .soundnum = SOUNDNUM_Patriotic_Marine_Hymn,                     .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"PATRIOT-DIXIE",                 .soundnum = SOUNDNUM_Patriotic_Dixie,                           .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"PATRIOT-SHENANDOAH",            .soundnum = SOUNDNUM_Patriotic_Shenandoah,                      .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"PATRIOT-STAR-SPANGLED-BANNER",  .soundnum = SOUNDNUM_Patriotic_Star_Spangled_Banner,            .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"A440",                          .soundnum = SOUNDNUM_A440_SIN_WAVE,                             .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"C4641",                         .soundnum = SOUNDNUM_C4641_SIN_WAVE,                            .song_volume = SOUND_VOL_DEFAULT },
  { .song_name = (char*)"SOUNDNUM_ERR_INVALID",          .soundnum = SOUNDNUM_ERR_INVALID,                               .song_volume = SOUND_VOL_DEFAULT }
};
typedef struct {
  char*     type_name;
  uint16_t* list_array;
  uint16_t  num_in_list;
} music_type_to_music_list_t;

uint16_t g_music_type_array_duel[] = { SOUNDNUM_DuelingBanjos };
uint16_t g_music_type_array_christmas[] = {
  SOUNDNUM_DeckTheDuelingHalls,
  SOUNDNUM_WhatChildIsThis,
  SOUNDNUM_GodRestYe,
  SOUNDNUM_OLittleTownOf,
  SOUNDNUM_GoodKingWenceslas
};
uint16_t g_music_type_array_chopin[] = {
  SOUNDNUM_Chopin_Etude_10_03,
  SOUNDNUM_Chopin_Noct_55_2,
  SOUNDNUM_Chopin_Etude_10_12,
  SOUNDNUM_Chopin_Noct_27_2,
  SOUNDNUM_Chopin_Noct_37_2,
  SOUNDNUM_Chopin_Prelude_15
};
uint16_t g_music_type_array_patriot[] = {
  SOUNDNUM_Patriotic_Battle_Hymn_of_the_Republic,
  SOUNDNUM_Patriotic_America_the_Beautiful,
  SOUNDNUM_Patriotic_When_Johnny_Comes_Marching_Home,
  SOUNDNUM_Patriotic_Marine_Hymn,
  SOUNDNUM_Patriotic_Dixie,
  SOUNDNUM_Patriotic_Shenandoah,
  SOUNDNUM_Patriotic_Star_Spangled_Banner
};
uint16_t g_music_type_array_all[] = {
  SOUNDNUM_DuelingBanjos,
  SOUNDNUM_DeckTheDuelingHalls,
  SOUNDNUM_WhatChildIsThis,
  SOUNDNUM_GodRestYe,
  SOUNDNUM_OLittleTownOf,
  SOUNDNUM_GoodKingWenceslas,
  SOUNDNUM_Chopin_Etude_10_03,
  SOUNDNUM_Chopin_Noct_55_2,
  SOUNDNUM_Chopin_Etude_10_12,
  SOUNDNUM_Chopin_Noct_27_2,
  SOUNDNUM_Chopin_Noct_37_2,
  SOUNDNUM_Chopin_Prelude_15,
  SOUNDNUM_Patriotic_Battle_Hymn_of_the_Republic,
  SOUNDNUM_Patriotic_America_the_Beautiful,
  SOUNDNUM_Patriotic_When_Johnny_Comes_Marching_Home,
  SOUNDNUM_Patriotic_Marine_Hymn,
  SOUNDNUM_Patriotic_Dixie,
  SOUNDNUM_Patriotic_Shenandoah,
  SOUNDNUM_Patriotic_Star_Spangled_Banner
};

uint16_t g_music_type_array_all_plus_invalid[] = {
  SOUNDNUM_DuelingBanjos,
  SOUNDNUM_DeckTheDuelingHalls,
  SOUNDNUM_WhatChildIsThis,
  SOUNDNUM_GodRestYe,
  SOUNDNUM_OLittleTownOf,
  SOUNDNUM_GoodKingWenceslas,
  SOUNDNUM_Chopin_Etude_10_03,
  SOUNDNUM_Chopin_Noct_55_2,
  SOUNDNUM_Chopin_Etude_10_12,
  SOUNDNUM_Chopin_Noct_27_2,
  SOUNDNUM_Chopin_Noct_37_2,
  SOUNDNUM_Chopin_Prelude_15,
  SOUNDNUM_Patriotic_Battle_Hymn_of_the_Republic,
  SOUNDNUM_Patriotic_America_the_Beautiful,
  SOUNDNUM_Patriotic_When_Johnny_Comes_Marching_Home,
  SOUNDNUM_Patriotic_Marine_Hymn,
  SOUNDNUM_Patriotic_Dixie,
  SOUNDNUM_Patriotic_Shenandoah,
  SOUNDNUM_Patriotic_Star_Spangled_Banner,
  SOUNDNUM_A440_SIN_WAVE,
  SOUNDNUM_C4641_SIN_WAVE,
  SOUNDNUM_ERR_INVALID
};

music_type_to_music_list_t g_music_type_to_music_list_duel      = { .type_name=(char*)"DUEL",      .list_array=&g_music_type_array_duel[0],      .num_in_list = NUMOF(g_music_type_array_duel) };
music_type_to_music_list_t g_music_type_to_music_list_christmas = { .type_name=(char*)"CHRISTMAS", .list_array=&g_music_type_array_christmas[0], .num_in_list = NUMOF(g_music_type_array_christmas) };
music_type_to_music_list_t g_music_type_to_music_list_chopin    = { .type_name=(char*)"CHOPIN",    .list_array=&g_music_type_array_chopin[0],    .num_in_list = NUMOF(g_music_type_array_chopin) };
music_type_to_music_list_t g_music_type_to_music_list_patriot   = { .type_name=(char*)"PATRIOT",   .list_array=&g_music_type_array_patriot[0],   .num_in_list = NUMOF(g_music_type_array_patriot) };
music_type_to_music_list_t g_music_type_to_music_list_all       = { .type_name=(char*)"ALL",       .list_array=&g_music_type_array_all[0],       .num_in_list = NUMOF(g_music_type_array_all) };

music_type_to_music_list_t* g_music_type_to_music_list_array[] = {
  &g_music_type_to_music_list_duel,
  &g_music_type_to_music_list_christmas,
  &g_music_type_to_music_list_chopin,
  &g_music_type_to_music_list_patriot,
  &g_music_type_to_music_list_all
};

constexpr uint16_t MUSIC_MODE_SINGLE_SONG  = 0;
constexpr uint16_t MUSIC_MODE_TYPE_OF_SONG = 1;
// constexpr uint16_t MUSIC_MODE_OFF       = 2; //  this is implemented by using SOUNDNUM_silence
uint16_t g_music_song_to_soundnum_idx_playing_now;              // kept current in all modes
uint16_t g_music_mode = MUSIC_MODE_SINGLE_SONG;
uint16_t g_music_soundnum_single_song = SOUNDNUM_DuelingBanjos; // only used if MUSIC_MODE_SINGLE_SONG
music_type_to_music_list_t* g_music_type_list;                  // only used if MUSIC_MODE_TYPE_OF_SONG
uint16_t g_music_type_list_idx_playing_now;                     // only used if MUSIC_MODE_TYPE_OF_SONG

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// structures for LEDPinsPwm to control the LED Eyes of the Banjo Players

// pin definitions - name must be must be g_pwm_pin_info, must be one row for each PWM LED output pin
//
// [0] = left  skeleton, left eye
// [1] = left  skeleton, right eye
// [2] = right skeleton, left eye
// [3] = right skeleton, right eye
//
pwm_pin_info g_pwm_pin_info[LED_PINS_PWM_NUM_PINS] = {
  { .pin_num=18, .ptrn_step_ptr = (pwm_led_ptrn_step *)0, .idx_curr_step=0, .curr_pwm_val=0, .prev_pwm_val = 0xFFFF, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_tick_incr = 0, .scaledtm_next_tick=0, .scaledtm_next_step=0 },
  { .pin_num=19, .ptrn_step_ptr = (pwm_led_ptrn_step *)0, .idx_curr_step=0, .curr_pwm_val=0, .prev_pwm_val = 0xFFFF, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_tick_incr = 0, .scaledtm_next_tick=0, .scaledtm_next_step=0 },
  { .pin_num=32, .ptrn_step_ptr = (pwm_led_ptrn_step *)0, .idx_curr_step=0, .curr_pwm_val=0, .prev_pwm_val = 0xFFFF, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_tick_incr = 0, .scaledtm_next_tick=0, .scaledtm_next_step=0 },
  { .pin_num=26, .ptrn_step_ptr = (pwm_led_ptrn_step *)0, .idx_curr_step=0, .curr_pwm_val=0, .prev_pwm_val = 0xFFFF, .scale_factor=TIME_SCALE_EQUAL, .scaledtm_tick_incr = 0, .scaledtm_next_tick=0, .scaledtm_next_step=0 }
};

// pwm LED pattern definitions - name can be anything, can be as many pattern definitions as desired,
//         can have as many steps per pattern as desired
//
pwm_led_ptrn_step g_pwm_ptrn_open_eye[] = { 
  { .start_set_pwm=0,                      .step_incr=1,  .step_time=1400, .tick_time=5, .tick_pwm= 0},
  { .start_set_pwm=0,                      .step_incr=1,  .step_time=1757, .tick_time=5, .tick_pwm= 2},
  { .start_set_pwm=LED_PINS_PWM_NO_CHANGE, .step_incr=-2, .step_time=1000, .tick_time=5, .tick_pwm=-7}
};
pwm_led_ptrn_step g_pwm_ptrn_blink[] = { 
  { .start_set_pwm=LED_PINS_PWM_MAX_VALUE, .step_incr=1,  .step_time=450, .tick_time=5, .tick_pwm= 0},
  { .start_set_pwm=0,                      .step_incr=1,  .step_time=450, .tick_time=5, .tick_pwm= 0},
  { .start_set_pwm=LED_PINS_PWM_MAX_VALUE, .step_incr=-2, .step_time=450, .tick_time=5, .tick_pwm= 0}
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

uint32_t g_eyes_bright; // MS-16bits is <num>, LS-16bits is <den> for led_pin_pwm_set_pwm_scale()



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
// DFscaleVolume(p_SoundNum, p_Volume) - compute scaled volume
//
// p_SoundNum is the sound file number, range 1 to SOUNDNUM_MAX_VALID; may use in future version
// p_Volume   is the requested vol, range 0 to 30
//
// for simple global scaled volume
//     VOLUME_GSCALE_DIVISOR is 100, g_volume_gscale has range: 0 to 200
// else
//     VOLUME_GSCALE_DIVISOR is 1, g_volume_gscale is 1, and code is #if'd out
//
uint16_t DFscaleVolume(uint16_t p_SoundNum, uint16_t p_Volume) {
  uint16_t scaled_volume = p_Volume;

  // simple global scaled volume
  scaled_volume = (scaled_volume * g_volume_gscale) / VOLUME_GSCALE_DIVISOR;

  if (30 < scaled_volume) // ensure valid argument
    scaled_volume = 30;

  return(scaled_volume);
} // end DFscaleVolume()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// DFstartSound(p_SoundNum, p_Volume) - start p_SoundNum if it is valid
//
// p_SoundNum is the sound file number, range 1 to SOUNDNUM_MAX_VALID
// p_Volume   is the requested vol, range 0 to 30
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
void  DFstartSound(uint16_t p_SoundNum, uint16_t p_Volume) {
  uint16_t idx;
  bool prevHI;
  uint16_t my_SoundNum = p_SoundNum;
  static uint8_t init_minmax = 2;
  static uint32_t prev_msec;
  static uint32_t max_msec = 0;
  static uint32_t min_msec = 999999;
  uint32_t diff_msec = 0;
  uint32_t now_msec = millis();
  uint16_t scaled_volume;

  if ((0 == my_SoundNum) || (SOUNDNUM_MAX_VALID < my_SoundNum)) {
    my_SoundNum = SOUNDNUM_ERR_INVALID; // give error sound that soundnum was invalid
    Serial.printf("ERROR DFstartSound called with %d (MAX is %d) replaced with error soundnum\n", p_SoundNum, SOUNDNUM_MAX_VALID);
    
#ifdef BE_OBNOXIOUS
    // repeat error message until someone uses UniRemote to send a command
    g_music_mode = MUSIC_MODE_SINGLE_SONG;
    g_music_soundnum_single_song = my_SoundNum;
#else // not BE_OBNOXIOUS
    // give error sound once, then start cycling from type == ALL
    //    when error sound ends, it will start the list ALL from the start
    uint16_t idx = NUMOF(g_music_type_to_music_list_array) - 1; // music type == ALL
    g_music_mode = MUSIC_MODE_TYPE_OF_SONG;
    g_music_type_list = g_music_type_to_music_list_array[idx];
    g_music_type_list_idx_playing_now = g_music_type_to_music_list_array[idx]->num_in_list - 1;
#endif // BE_OBNOXIOUS

    // always up to date
    g_music_song_to_soundnum_idx_playing_now = find_music_idx_from_soundnum(my_SoundNum);
  }

#if VOLUME_ALLOW_CHANGE
  // if we allow changing volume
  scaled_volume = DFscaleVolume(my_SoundNum, p_Volume);
  myDFPlayer.volume(scaled_volume);  // Set volume value. Range from 0 to 30
#endif // VOLUME_ALLOW_CHANGE

#if DFPRINTDETAIL
  if (myDFPlayer.available()) {
    Serial.print(F(" DFstartSound ln ")); Serial.print((uint16_t) __LINE__); Serial.println(F(" myDFPlayer problem after volume"));
    DFprintDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }
#endif // DFPRINTDETAIL

  myDFPlayer.play(my_SoundNum); //play specific wav in SD: root directory ###.wav; number played is physical copy order; first one copied is 1
  // Serial.print(F("DEBUG DFstartSound myDFPlayer.play(")); Serial.print((uint16_t) my_SoundNum); Serial.println(F(")"));
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
  myDFPlayer.volume(g_current_volume_set);  // Set volume value. From 0 to 30 - FIXME 25 is good
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

#if VM_OTA_DEBUG
  // Connect to WiFi network
  WiFi.begin(g_ssid, g_password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(g_ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
#endif // VM_OTA_DEBUG

  // connect to Banjo Player LED eyes and initially turn off
  Serial.println("\nInitialize LEDPinsPwm");
  if (led_pins_pwm_init(LED_PINS_PWM_FREQ, LED_PINS_PWM_VAL_NUM_BITS)) {
    while (1) ;
  } // end if error in initialization
  for (int pin_idx = 0; pin_idx < NUMOF(g_pwm_pin_info); pin_idx += 1) {
    led_pin_pwm_init_ptrn(pin_idx, g_pwm_ptrn_blink, 0, TIME_SCALE_EQUAL, 0);
  } // end for each pin_idx
  led_pin_pwm_set_pwm_scale(0x00010005); // 1/5 power

  // initialize the YX5200 DFPlayer audio player
  Serial.println("\nInitialize YX5200");
  DFsetup();

  Serial.println("\nDuelWithBanjos init complete...");

  // start the first sound, then allow normal loop() processing
  g_music_type_list = &g_music_type_to_music_list_duel; // just in case, make it valid
  g_music_mode = MUSIC_MODE_SINGLE_SONG;
  g_music_soundnum_single_song = SOUNDNUM_DuelingBanjos;
  g_music_song_to_soundnum_idx_playing_now = find_music_idx_from_soundnum(SOUNDNUM_DuelingBanjos);
  DFstartSound(g_music_soundnum_single_song, g_current_volume_set);

  // we don't need any setup for mdo_use_ota_webupdater

} // end setup()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_cmd_volume() - stub versions
//       returns: 0 if no error
//   process VOLUME: command if we understand it; call with p_cmd and p_param ALL UPPERCASE
//
// VOLUME:UP #       range: 0-30 move g_current_volume_set up by #
// VOLUME:DOWN #     range: 0-30 move g_current_volume_set down by #
// VOLUME:SET #      range: 0-30 set g_current_volume_set to #
// VOLUME:GSCALE #   range: 0-200 (percent) set g_volume_gscale to #
//
uint16_t do_cmd_volume(char* p_cmd, char* p_param) {
  Serial.printf("do_cmd_volume %s %s\n", p_cmd, p_param);
  uint16_t curr_soundnum = g_music_song_to_soundnum[g_music_song_to_soundnum_idx_playing_now].soundnum;
  int16_t num = (int16_t) atoi(p_param); // int can be + or -
  if (num < 0) num = 0;
  if (NULL != strstr("VOLUME:GSCALE", p_cmd)) {
    if (200 < num) num = 200;
    if (0   > num) num = 0;
    g_volume_gscale = (uint32_t) num; // gscale is uint32_t to force int calcs to uint32_t
  } else {
    if (SOUND_VOL_MIN > num) num = SOUND_VOL_MIN;
    if (SOUND_VOL_MAX < num) num = SOUND_VOL_MAX;
    if (NULL != strstr("VOLUME:UP", p_cmd)) {
      num = g_current_volume_set + num;
      if (SOUND_VOL_MIN > num) num = SOUND_VOL_MIN;
      if (SOUND_VOL_MAX < num) num = SOUND_VOL_MAX;
      g_current_volume_set = (uint16_t) num;
    } else if (NULL != strstr("VOLUME:DOWN", p_cmd)) {
      num = g_current_volume_set - num;
      if (SOUND_VOL_MIN > num) num = SOUND_VOL_MIN;
      if (SOUND_VOL_MAX < num) num = SOUND_VOL_MAX;
      g_current_volume_set = (uint16_t) num;
    } else if (NULL != strstr("VOLUME:SET", p_cmd)) {
      g_current_volume_set = (uint16_t) num;
    }
  }
  myDFPlayer.volume(DFscaleVolume(curr_soundnum, g_current_volume_set));  // Set volume value. Range from 0 to 30

  return(0);
} // end do_cmd_volume()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// find_musictype_idx_from_soundnum() - MUSIC:xxx utility
//     returns 0xFFFF if not found, else idx within g_music_song_to_soundnum
// 
uint16_t find_musictype_idx_from_soundnum(uint16_t p_soundnum, music_type_to_music_list_t *p_mtype_2mlist) {
  uint16_t ret_idx = 0xFFFF;

  for (uint16_t idx = 0; idx < p_mtype_2mlist->num_in_list; idx += 1) {
    if (p_mtype_2mlist->list_array[idx] == p_soundnum) {
      ret_idx = idx;
      break;
    }
  }

  return(ret_idx);
} // end find_musictype_idx_from_soundnum()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// find_music_idx_from_soundnum() - MUSIC:xxx utility
//     returns 0xFFFF if not found, else idx within g_music_song_to_soundnum
// 
uint16_t find_music_idx_from_soundnum(uint16_t p_soundnum) {
  uint16_t ret_idx = 0xFFFF;

  for (uint16_t idx = 0; idx < NUMOF(g_music_song_to_soundnum); idx += 1) {
    if (g_music_song_to_soundnum[idx].soundnum == p_soundnum) {
      ret_idx = idx;
      break;
    }
  }

  return(ret_idx);
} // end find_music_idx_from_soundnum()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_cmd_music() - MUSIC:xxx command
//       returns: 0 if no error
//   process MUSIC: command if we understand it; call with p_cmd and p_param ALL UPPERCASE
//
// MUSIC:SONG <name>   = (<name> = SILENCE DUEL-BANJO DECK-HALLS WHAT-CHILD MERRY-GENTLEMEN TOWN-BETHLEHEM KING-WENCESLAS
//                                 CHOPIN-ETUDE-TRISTESSE CHOPIN-NOCTURNE-E-FLAT CHOPIN-ETUDE-REVOLUTIONARY
//                                 CHOPIN-NOCTURNE-D-FLAT CHOPIN-NOCTURNE-G CHOPIN-PRELUDE-RAINDROP
//                                 PATRIOT-BATTLE-HYMN-REPUBLIC PATRIOT-AMERICA-THE-BEAUTIFUL PATRIOT-JOHNNY-MARCHING-HOME
//                                 PATRIOT-MARINE-HYMN PATRIOT-DIXIE PATRIOT-SHENANDOAH PATRIOT-STAR-SPANGLED-BANNER)
// MUSIC:TYPE <type>   = (<type> = DUEL CHRISTMAS CHOPIN PATRIOT ALL)
// MUSIC:NEXT <ignore> = start next song from ALL and set mode to MUSIC:SONG
// MUSIC:SKIP <ignore> = if MUSIC:TYPE skip to next song in this type
//                       if MUSIC:SONG do same thing as MUSIC:NEXT
//
uint16_t do_cmd_music(char* p_cmd, char* p_param) {
  uint16_t found = 0xFFFF;
  uint16_t idx = 0;

  if (NULL != strstr("MUSIC:SONG", p_cmd)) {
    for (idx = 0; idx < NUMOF(g_music_song_to_soundnum); idx += 1) {
      if (NULL != strstr(p_param, g_music_song_to_soundnum[idx].song_name)) {
        found = idx;
        g_music_mode = MUSIC_MODE_SINGLE_SONG;
        g_music_soundnum_single_song = g_music_song_to_soundnum[idx].soundnum;
        g_music_song_to_soundnum_idx_playing_now = find_music_idx_from_soundnum(g_music_soundnum_single_song);
        DFstartSound(g_music_soundnum_single_song, g_current_volume_set);
        break;
      }
    } // end for
    if (0xFFFF == found) {
      Serial.printf("ERROR: unknown MUSIC SONG in command %s %s\n", p_cmd, p_param);
      return(1);
    }
  } else if (NULL != strstr("MUSIC:TYPE", p_cmd)) {
    for (idx = 0; idx < NUMOF(g_music_type_to_music_list_array); idx += 1) {
      if (NULL != strstr(p_param, g_music_type_to_music_list_array[idx]->type_name)) {
        found = idx;
        g_music_mode = MUSIC_MODE_TYPE_OF_SONG;
        g_music_type_list = g_music_type_to_music_list_array[idx];
        g_music_type_list_idx_playing_now = 0;
        uint16_t tmp_soundnum = g_music_type_list->list_array[g_music_type_list_idx_playing_now];
        g_music_song_to_soundnum_idx_playing_now = find_music_idx_from_soundnum(tmp_soundnum);
        DFstartSound(tmp_soundnum, g_current_volume_set);
        break;
      }
    } // end for
    if (0xFFFF == found) {
      Serial.printf("ERROR: unknown MUSIC TYPE in command %s %s\n", p_cmd, p_param);
      return(1);
    }
  } else if ((NULL != strstr("MUSIC:NEXT", p_cmd)) || (NULL != strstr("MUSIC:SKIP", p_cmd))) {
    uint16_t tmp_idx = 0xFFFF;
    uint16_t tmp_soundnum = g_music_song_to_soundnum[g_music_song_to_soundnum_idx_playing_now].soundnum;
    if ((NULL != strstr("MUSIC:NEXT", p_cmd)) || (MUSIC_MODE_SINGLE_SONG == g_music_mode)) {
      // here to force MUSIC_MODE_SINGLE_SONG and skip to next in g_music_type_to_music_list_all
      g_music_mode = MUSIC_MODE_SINGLE_SONG;
      g_music_type_list = &g_music_type_to_music_list_all; // we use it temporarily in this routine
    } else {
      // here with SKIP and in MUSIC_MODE_TYPE_OF_SONG
      // g_music_mode already set and g_music_type_list already set
    }

    // note that 1+0xFFFF (return from not found) -> 0, which is valid for all lists
    tmp_idx = 1 + find_musictype_idx_from_soundnum(tmp_soundnum, g_music_type_list);
    if (tmp_idx >= g_music_type_list->num_in_list) {
      tmp_idx = 0;
    }
    tmp_soundnum = g_music_type_list->list_array[tmp_idx];

    // g_music_song_to_soundnum_idx_playing_now is ALWAYS up to date
    g_music_song_to_soundnum_idx_playing_now = find_music_idx_from_soundnum(tmp_soundnum);
    DFstartSound(tmp_soundnum, g_current_volume_set);
  } else {
    Serial.printf("ERROR: unknown MUSIC command %s %s\n", p_cmd, p_param);
    return(1);
  }
  // if we get here, we understood the command
  return(0);
} // end do_cmd_music()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_cmd_eyes() - EYES:xxx command
//       returns: 0 if no error
//   process EYES: command if we understand it; call with p_cmd and p_param ALL UPPERCASE
//
// EYES:PATTERN <coord>/<tscale>/<ptrn>     = (<coord> = TOGETHER SEPARATE OPPOSITE - <coord> no effect for SINELON or OFF) (<tscale> = TBS FIXME TODO 64)  (<ptrn> = BLINK OPEN BL-OPN SINELON OFF)
// EYES:CYCLE   <coord>/<tscale>            = (<coord> = TOGETHER SEPARATE OPPOSITE - <coord> no effect for SINELON or OFF) (<tscale> = TBS FIXME TODO 64) 
// EYES:BRIGHT  <num>/<den>                 = (<num> = numerator of fraction) (<den> = denominator of fraction) (NOTE: 0 <= num/den <= 1, den != 0)
// EYES:MVPINS  <pin1>/<pin2>/<pin3>/<pin4> = (<pin# = DIO pin number (ex: 18) for pin_idx=#)
//
uint16_t do_cmd_eyes(char* p_cmd, char* p_param) {

constexpr uint16_t EYES_COORD_TOGETHER = 0x0000;
constexpr uint16_t EYES_COORD_OPPOSITE = 0x0001;
constexpr uint16_t EYES_COORD_SEPARATE = 0x0002;
#define NUM_EYES_COORD 3

  char delimiters[] = "/ \t";
  char* param1;
  char* param2;
  char* param3;
  char* param4;
  static char tmp_msg[ESP_NOW_MAX_DATA_LEN];
  static uint16_t start_idx_coord[NUM_EYES_COORD][LED_PINS_PWM_NUM_PINS] = {
    { 0, 0, 0, 0 }, // TOGETHER
    { 0, 1, 0, 1 }, // OPPOSITE
    { 0, 1, 1, 0 }  // SEPARATE
  };

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

    // get numeric version of <coord>
    uint16_t coord_num = 0;
    if (NULL != strstr("OPPOSITE", param1))            coord_num = EYES_COORD_OPPOSITE;
    else if (NULL != strstr("SEPARATE", param1))       coord_num = EYES_COORD_SEPARATE;
    else /* if (NULL != strstr("TOGETHER", param1)) */ coord_num = EYES_COORD_TOGETHER;

    // get tscale - expected between 0 through about 80
    uint16_t tscale = atoi(param2);

    // handle the patterns
    if (NULL != strstr("BLINK", param3)) {
      for (int pin_idx = 0; pin_idx < NUMOF(g_pwm_pin_info); pin_idx += 1) {
        led_pin_pwm_init_ptrn(pin_idx, g_pwm_ptrn_blink, start_idx_coord[coord_num][pin_idx], tscale, LED_PINS_PWM_USE_PTRN);
      } // end for each pin_idx
    } else if (NULL != strstr("OPEN", param3)) {
      for (int pin_idx = 0; pin_idx < NUMOF(g_pwm_pin_info); pin_idx += 1) {
        led_pin_pwm_init_ptrn(pin_idx, g_pwm_ptrn_open_eye, start_idx_coord[coord_num][pin_idx], tscale, LED_PINS_PWM_USE_PTRN);
      } // end for each pin_idx
    } else if (NULL != strstr("BL-OPN", param3)) {
      for (int pin_idx = 0; pin_idx < NUMOF(g_pwm_pin_info); pin_idx += 1) {
        if (pin_idx % 2) {
          led_pin_pwm_init_ptrn(pin_idx, g_pwm_ptrn_blink, start_idx_coord[coord_num][pin_idx], tscale, LED_PINS_PWM_USE_PTRN);
        } else {
          led_pin_pwm_init_ptrn(pin_idx, g_pwm_ptrn_open_eye, start_idx_coord[coord_num][pin_idx], tscale, LED_PINS_PWM_USE_PTRN);
        }
      } // end for each pin_idx
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
    // FIXME TODO EYES:CYCLE
    Serial.printf("ERROR ESP-NOW cmd %s %s not yet implemented\n", p_cmd, p_param);
    return(1);
  } else if (NULL != strstr("EYES:BRIGHT", p_cmd)) {
    led_pin_pwm_set_pwm_scale(((uint32_t)atoi(param1))<<16 | ((uint32_t)atoi(param2))); // param1/param2 power
  } else if (NULL != strstr("EYES:MVPINS", p_cmd)) {
    param3 = strtok(NULL, delimiters);
    if (param3 == NULL) { Serial.printf("ERROR ESP-NOW cmd %s %s bad parameter 3\n", p_cmd, p_param);  return(1); }
    param4 = strtok(NULL, delimiters);
    if (param4 == NULL) { Serial.printf("ERROR ESP-NOW cmd %s %s bad parameter 4\n", p_cmd, p_param);  return(1); }
    // FIXME TODO EYES:MVPINS - maybe not important anymore...
    Serial.printf("ERROR ESP-NOW cmd %s %s not yet implemented\n", p_cmd, p_param);
    return(1);
  } else { Serial.printf("ERROR ESP-NOW cmd %s %s no such command\n", p_cmd, p_param);  return(1); }
  return(0);
} // end do_cmd_eyes()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_cmd_ota() - Over-The-Air software update command
//       returns: 0 if no error
//   process OTA: command if we understand it; call with p_cmd ALL UPPERCASE, p_param Upper/Lower case
//
// OTA:WEB <password>                 = (<password> = valid password to start OTA)
//
uint16_t do_cmd_ota(char* p_cmd, char* p_param) {
  uint16_t ret_val = 0;

#if MDO_USE_OTA // if using Over-The-Air software updates
  if ((NULL != strstr(p_cmd, "OTA:WEB")) && (NULL != strstr(p_param, WIFI_OTA_ESP_NOW_PWD))) {
    // This is the correct parameter for code that is using ESP-NOW but not connecting to router (already in WiFi STA mode but no IP address)
#if VM_OTA_DEBUG
    // if doing VM_OTA_DEBUG then already did START_OTA_WEB_BEGIN_WIFI
    mdo_ota_web_request(START_OTA_WEB_INIT_MDNS | START_OTA_WEB_INIT_UPDATER_WEBPAGE); // loop() will handle it
#else // not VM_OTA_DEBUG
    // if NOT doing VM_OTA_DEBUG then must do START_OTA_WEB_BEGIN_WIFI now
    mdo_ota_web_request(START_OTA_WEB_BEGIN_WIFI | START_OTA_WEB_INIT_MDNS | START_OTA_WEB_INIT_UPDATER_WEBPAGE); // loop() will handle it
#endif // VM_OTA_DEBUG
    Serial.printf("\nOTA Web Updater REQUESTED\n");
  } else {
    Serial.printf("\nERROR: bad OTA:WEB command\n");
    ret_val = 1;
  }
#endif // MDO_USE_OTA if using Over-The-Air software updates

  return(ret_val);
} // end do_cmd_ota()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// do_esp_now_command()
//       returns: nothing
//   process command if we understand it; call with my_message Upper/Lower case
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
// BANJO ; OTA:WEB <password>
//
// BANJO ; VOLUME:UP #       range: 0-30 move g_current_volume_set up by #
// BANJO ; VOLUME:DOWN #     range: 0-30 move g_current_volume_set down by #
// BANJO ; VOLUME:SET #      range: 0-30 set g_current_volume_set to #
// BANJO ; VOLUME:GSCALE #   range: 0-200 (percent) set g_volume_gscale to #
//
// BANJO ; MUSIC:SONG <name>   = (<name> = SILENCE DUEL-BANJO DECK-HALLS WHAT-CHILD MERRY-GENTLEMEN TOWN-BETHLEHEM KING-WENCESLAS
//                                         CHOPIN-ETUDE-TRISTESSE CHOPIN-NOCTURNE-E-FLAT CHOPIN-ETUDE-REVOLUTIONARY
//                                         CHOPIN-NOCTURNE-D-FLAT CHOPIN-NOCTURNE-G CHOPIN-PRELUDE-RAINDROP
//                                         PATRIOT-BATTLE-HYMN-REPUBLIC PATRIOT-AMERICA-THE-BEAUTIFUL PATRIOT-JOHNNY-MARCHING-HOME
//                                         PATRIOT-MARINE-HYMN PATRIOT-DIXIE PATRIOT-SHENANDOAH PATRIOT-STAR-SPANGLED-BANNER)
// BANJO ; MUSIC:TYPE <type>   = (<type> = DUEL CHRISTMAS CHOPIN PATRIOT ALL)
// BANJO ; MUSIC:NEXT <ignore> = start next song from ALL and set mode to MUSIC:SONG
//
// BANJO ; EYES:PATTERN <coord>/<tscale>/<ptrn> = (<coord> = TOGETHER SEPARATE OPPOSITE - <coord> no effect for SINELON or OFF) (<tscale> = TBS FIXME TODO 64)  (<ptrn> = BLINK OPEN BL-OPN SINELON OFF)
// BANJO ; EYES:CYCLE   <coord>/<tscale>        = (<coord> = TOGETHER SEPARATE OPPOSITE - <coord> no effect for SINELON or OFF) (<tscale> = TBS FIXME TODO 64) 
// BANJO ; EYES:BRIGHT  <num>/<den>             = (<num> = numerator of fraction) (<den> = denominator of fraction) (NOTE: 0 <= num/den <= 1, den != 0)
// EYES:MVPINS  <pin1>/<pin2>/<pin3>/<pin4>     = (<pin# = DIO pin number (ex: 18) for pin_idx=#)
//
constexpr uint16_t ESP_NOW_PARAM_MASK_CMD_TYPE = 0x00FF;
constexpr uint16_t ESP_NOW_PARAM_MASK_CMD_BITS = 0xFF00;
constexpr uint16_t ESP_NOW_PARAM_UC            = 0x0100; // upper case for parameter
typedef struct {
  const char * cmd;
  uint16_t cmd_idx;
} esp_now_cmd_t;
esp_now_cmd_t esp_now_cmds[] = {
  {.cmd = "VOLUME:UP",     .cmd_idx = 0x0000 | ESP_NOW_PARAM_UC },
  {.cmd = "VOLUME:DOWN",   .cmd_idx = 0x0000 | ESP_NOW_PARAM_UC },
  {.cmd = "VOLUME:SET",    .cmd_idx = 0x0000 | ESP_NOW_PARAM_UC },
  {.cmd = "VOLUME:GSCALE", .cmd_idx = 0x0000 },
  {.cmd = "MUSIC:SONG",    .cmd_idx = 0x0001 | ESP_NOW_PARAM_UC },
  {.cmd = "MUSIC:TYPE",    .cmd_idx = 0x0001 | ESP_NOW_PARAM_UC },
  {.cmd = "MUSIC:NEXT",    .cmd_idx = 0x0001 | ESP_NOW_PARAM_UC },
  {.cmd = "MUSIC:SKIP",    .cmd_idx = 0x0001 | ESP_NOW_PARAM_UC },
  {.cmd = "EYES:PATTERN",  .cmd_idx = 0x0002 | ESP_NOW_PARAM_UC },
  {.cmd = "EYES:CYCLE",    .cmd_idx = 0x0002 | ESP_NOW_PARAM_UC },
  {.cmd = "EYES:BRIGHT",   .cmd_idx = 0x0002 | ESP_NOW_PARAM_UC },
  {.cmd = "EYES:MVPINS",   .cmd_idx = 0x0002 | ESP_NOW_PARAM_UC },
  {.cmd = "OTA:WEB",       .cmd_idx = 0x0003 }
};
#define CMD_VERIFIER "BANJO"

typedef uint16_t(*do_cmd_type_t)(char*, char*);

do_cmd_type_t esp_now_cmd_ptrs[] = { &do_cmd_volume, &do_cmd_music, &do_cmd_eyes, &do_cmd_ota };

uint16_t do_esp_now_command(uint16_t rcvd_len, char* my_message) {
  // all commands to banjo players start with "BANJO" a.k.a. CMD_VERIFIER
  uint16_t ret_val = 1; // error unless complete
  char delimiters[] = " ";
  char* token;
  char* param_token;

  static char tmp_msg[ESP_NOW_MAX_DATA_LEN+1];
  strncpy(tmp_msg, my_message, ESP_NOW_MAX_DATA_LEN);
  token = strtok(tmp_msg, delimiters);
  if (token == NULL) { Serial.printf("ERROR ESP-NOW CMD_VERIFIER missing, not %s\n", CMD_VERIFIER);  return(1); }
  else if (NULL == strstr(strupr(token), CMD_VERIFIER)) { Serial.printf("ERROR ESP-NOW CMD_VERIFIER %s not %s\n", token, CMD_VERIFIER);  return(1); }
  Serial.printf("CMD_VERIFIER: |%s| seen: |%s|\n", token, CMD_VERIFIER);

  while (1) {
    token = strtok(NULL, delimiters);
    if ((token == NULL) || (NULL == strstr(token, ";"))) { ret_val = 0; break; }
    token = strupr(strtok(NULL, delimiters));
    if ((token == NULL) || (NULL == strstr(token, ":"))) break;
    param_token = strtok(NULL, delimiters);
    if (param_token == NULL) break;
    Serial.printf("cmd: |%s| param |%s|\n", token, param_token);
    int not_found = 1;
    for (int i = 0; (i < NUMOF(esp_now_cmds)) && not_found; i += 1) {
      if (!strcmp(token, esp_now_cmds[i].cmd)) {
        Serial.printf("  found %s %s cmd_idx:0x%04x\n", token, param_token, esp_now_cmds[i].cmd_idx);
        uint16_t cmd_type = esp_now_cmds[i].cmd_idx & ESP_NOW_PARAM_MASK_CMD_TYPE;
        uint16_t cmd_bits  = esp_now_cmds[i].cmd_idx & ESP_NOW_PARAM_MASK_CMD_BITS;
        if (cmd_bits & ESP_NOW_PARAM_UC)
          esp_now_cmd_ptrs[cmd_type](token, strupr(param_token));
        else
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
    do_esp_now_command(rcvd_len, my_message);
  }
  EVERY_N_MILLISECONDS( 5 ) { 
    led_pins_pwm(); // if needed, output brightness of LED
  } // end EVERY_N_MILLISECONDS 5

  EVERY_N_MILLISECONDS( 50 ) { 
    if (DFcheckSoundDone()) {
      if (MUSIC_MODE_TYPE_OF_SONG == g_music_mode) {
        // loop through array of songs
        uint16_t tmp_soundnum = 0xFFFF;
        uint16_t tmp_idx = g_music_type_list_idx_playing_now + 1;
        if (tmp_idx >= g_music_type_list->num_in_list) tmp_idx = 0;
        g_music_type_list_idx_playing_now = tmp_idx;
        tmp_soundnum = g_music_type_list->list_array[g_music_type_list_idx_playing_now];
        g_music_song_to_soundnum_idx_playing_now = find_music_idx_from_soundnum(tmp_soundnum);
        DFstartSound(tmp_soundnum, g_current_volume_set);
      } else /* if (MUSIC_MODE_SINGLE_SONG == g_music_mode) */ {
        // restart sound
        g_music_song_to_soundnum_idx_playing_now = find_music_idx_from_soundnum(g_music_soundnum_single_song);
        DFstartSound(g_music_soundnum_single_song, g_current_volume_set);
      }
    }
  } // end EVERY_N_MILLISECONDS 50

#if MDO_USE_OTA // if using Over-The-Air software updates
  EVERY_N_MILLISECONDS( 50 ) { 
    // if using Over-The-Air software updates
    mdo_ota_web_loop();
  }
#endif // MDO_USE_OTA if using Over-The-Air software updates

} // end loop()
