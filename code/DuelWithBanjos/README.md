# Duel With Banjos

<img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/banjo_players_1024_768.jpg" width="500" alt="My Banjo Players">

**Table Of Contents**
* [Top](#duel-with-banjos "Top")
* [The Banjo Players](#the-banjo-players "The Banjo Players")
* [YX5200 Audio Player](#yx5200-audio-player "YX5200 Audio Player")
  * [LICENSE for DFRobot code](#license-for-dfrobot-code "LICENSE for DFRobot code")
* [ESP32 LEDC PWM - LED Dimmer Control](#esp32-ledc-pwm-\--led-dimmer-control "ESP32 LEDC PWM - LED Dimmer Control")
* [UniRemote and ESP-NOW](#uniremote-and-esp\-now "UniRemote and ESP-NOW")
* [ESP32](#esp32 "ESP32")

## The Banjo Players
[Top](#duel-with-banjos "Top")<br>
I have two country style skeleton scarecrow banjo players approximately 14 inches tall ordered from Amazon.com but apparently that exact model is no longer available there.
I think they might be similar to this one:
- https://www.amazon.com/XINANDHAO-Halloween-Animated-Skeletons-Musicians/dp/B0DC56RL1F

The ones I have each have a 3V power supply and two very bright red LED eyes. There is no other electronics involved in them.

## YX5200 Audio Player
[Top](#duel-with-banjos "Top")<br>
I am providing sound for my banjo players using the YX5200 Audio Player, which is widely available from many sources. A MicroSD card is used to store the sound files.
- https://github.com/Mark-MDO47/AudioPlayer-YX5200

![alt text](https://github.com/Mark-MDO47/RubberBandGun/blob/master/PartsInfo/YX5200_MP3player.png "Top view pin arrangement on YX5200 module")

### LICENSE for DFRobot code
[Top](#duel-with-banjos "Top")<br>
The DFRobot code (see https://www.dfrobot.com/) is not under the Apache License 2.0<br>
The license for the DFRobot code is copied into the file LICENSE_for_DFRobot_code.txt

## ESP32 LEDC PWM - LED Dimmer Control
[Top](#duel-with-banjos "Top")<br>
I am using the Espressif ESP32 Pulse Width Modulation (PWM) LED control to make the LED eyes do something interesting. This capability is similar to that available on Arduinos such as the Arduino Uno, but is specifically tailored to the ESP32. Be sure to use the new "pin-oriented" calling sequence instead of the earlier "channel-oriented" calling sequence.
- https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32/api-reference/peripherals/ledc.html
- https://github.com/espressif/arduino-esp32/blob/master/docs/en/migration_guides/2.x_to_3.0.rst#ledc

## UniRemote and ESP-NOW
[Top](#duel-with-banjos "Top")<br>
I am using UniRemote and its ESP-NOW WiFi capability as the remote control for this project
- https://github.com/Mark-MDO47/UniRemote

<img src="https://github.com/Mark-MDO47/UniRemote/blob/master/resources/images/UniRemote_overview.jpg" width="500" alt="Image of UniRemote breadboard overview">

This means I include the *.cpp and *.h from UniRemoteRcvrTemplate
- https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemoteRcvrTemplate

## ESP32
[Top](#duel-with-banjos "Top")<br>
Both the LEDC library and the UniRemote ESP-NOW usage mean that the processor for this project will be an ESP32. I will use some variant of the original ESP32 Devkit V1 ESP-WROOM-32 because they are inexpensive and I have several of them hanging around my mad scientist laboratory just waiting to be put to use.
