# DuelWithBanjos
Just for fun - tiny scarecrow skeletons playing banjo music, placed under the control of the UniRemote!
- https://github.com/Mark-MDO47/UniRemote

A quick 38-second video demonstration:
- https://youtu.be/nwAhtAX-E6E

A story-line showing how to do Over-The-Air (OTA) Web updates to the Banjo Player ESP32.
- https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/code/DuelWithBanjos/OTA_story.md

| My two little banjo players - BEFORE | My two little banjo players - AFTER<br>with new electronics and sound |
| --- | --- |
| <img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/banjo_players_1024_768.jpg" width="300" alt="My Banjo Players BEFORE"> | <img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/banjo_players_after.jpg" width="300" alt="My Banjo Players AFTER upgrade"> |
| **The new electronics on a prototype board** | **The new electronics inside the cardboard box** |
| <img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/Duel_ProtoBoard.png" width="300" alt="My ProtoBoard Banjo Player electronics"> | <img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/BanjoBox_inside.png" width="300" alt="Banjo Player electronics inside cardboard box"> |

**Table Of Contents**
* [Top](#duelwithbanjos "Top")
* [Introduction](#introduction "Introduction")
* [Schematic](#schematic "Schematic")
  * [Do-It-Yourself Layout Creator](#do\-it\-yourself-layout-creator "Do-It-Yourself Layout Creator")
* [Code](#code "Code")
* [Parts List](#parts-list "Parts List")
* [Items of Note](#items-of-note "Items of Note")
  * [Volume Control](#volume-control "Volume Control")
* [License](#license "License")

## Introduction
[Top](#duelwithbanjos "Top")<br>
I have two country style skeleton banjo players approximately 14 inches tall ordered from Amazon.com but apparently that exact model is no longer available there.
I think they might be similar to this one:
- https://www.amazon.com/XINANDHAO-Halloween-Animated-Skeletons-Musicians/dp/B0DC56RL1F

The plan is to make them
- respond to controls from my UniRemote remote controller via ESP-NOW
  - https://github.com/Mark-MDO47/UniRemote
- play music using a YX5200 Audio Player which reads sound files from a MicroSD card
  - https://github.com/Mark-MDO47/AudioPlayer-YX5200
- blink LED "eyes" using Pulse Width Modulation (PWM)
  - https://github.com/Mark-MDO47/DuelWithBanjos/tree/master/code/LEDPinsPwmTemplate
- support Over-The-Air (OTA) re-programming
  - https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/code/DuelWithBanjos/OTA_story.md
  - https://github.com/Mark-MDO47/UniRemote/blob/master/code/mdo_use_ota_webupdater/README.md
- support OTA debugging via the extension "Visual Micro" for Visual Studio Code
  - I am still working on it - getting ESP-NOW, ota_webupdater, and OTA debugging to play together
  - https://www.visualmicro.com/
  - https://code.visualstudio.com/
- Have volume control for the sound
  - I am still working on it - YX5200 volume control commands seem to wait for the start of the next sound and cause other effects
  - I will try this: https://www.adafruit.com/product/4286

## Schematic
[Top](#duelwithbanjos "Top")<br>
This is the schematic of the interesting part of the design, including the ESP32 module, sound control, and LED control.<br>
- The ESP32 of course has I/O at 3.3V but the YX5200 uses 5V. I use resistor networks to drop the voltages from 5V to 3.3V and a SN74HCT125N quadruple bus buffer to raise voltages from 3.3V to 5V.
- The 1K resistor in the path to the YX5200 RX line prevents coupling of digital noise into the sound output.
- I had to move to pin D26 because of my horrible soldering.
<img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/KiCad/Duel_schematic_01.png" width="800" alt="Schematic of ESP32 module, sound control, and LED control">

### Do-It-Yourself Layout Creator
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
I didn't know about this when I did my perfboard design for Dueling with Banjos but I wish I had used DIYLC instead of hacking away at my perfboard!
- https://github.com/bancika/diy-layout-creator
- https://github.com/bancika/diy-layout-creator/releases

Here is a nice YouTube showing how to use it
- Design Circuits on Stripboard or Veroboard for FREE with DIY Layout Creator by TekSparkz - https://www.youtube.com/watch?v=mzje3eHqi2E

## Code
[Top](#duelwithbanjos "Top")<br>
The code is starting to take shape; you can find it here
- https://github.com/Mark-MDO47/DuelWithBanjos/tree/master/code/DuelWithBanjos

I wanted my PWM LED control to be automatic in between commands but also to respond to commands. I made some code to do this. Once everything is initialized, you only need to call **led_pins_pwm()** periodically from loop() to have the LED eyes follow the requested time pattern.
- https://github.com/Mark-MDO47/DuelWithBanjos/tree/master/code/LEDPinsPwmTemplate

## Parts List
[Top](#duelwithbanjos "Top")<br>
Work In Progress...

| Title | Descrip | URL | each |
| --- | --- | --- | --- |
| Skeletons | 2 @ Pose-able banjo playing skeletons | https://www.amazon.com/XINANDHAO-Halloween-Animated-Skeletons-Musicians/dp/B0DC56RL1F | $10.00 |
| ESP32 | 1 @ ESP32 Devkit V1 ESP-WROOM-32 | https://www.amazon.com/Hosyond-ESP-WROOM-32-Development-Microcontroller-Compatible/dp/B0C7C2HQ7P/ref=sr_1_4 | $3.50 |
| YX5200 | 1 @ Sound Module with MicroSD (TF) card socket | https://www.amazon.com/Organizer-YX5200-DFPlayer-Supporting-Arduino/dp/B07XXYQBNS/ref=sr_1_1 | $2.00 |
| SN74HCT125N | 1 @ Non-inverting 5.5V buffer 14-pin DIP | https://www.digikey.com/en/products/detail/texas-instruments/SN74HCT125N/376860 | $0.63 |
| Proto Board | 1 @ 3.125" x 2.25" Prototype Board | from deep in my closet | $ who-knows? |
| speaker |  Metal Shell Round Internal Magnet Speaker 2W 8 Ohm approx 1 inch | https://www.amazon.com/dp/B0177ABRQ6 | $2.80 |
| JST SM 2.5mm | 2 @ pair JST SM 2.5mm 3-pin Male/Female connectors and cables for banjo players | https://www.amazon.com/mxuteuk-Connectors-Connector-Adapter-Electrical/dp/B0DHKC3ZSF/ref=sr_1_2?th=1 | $0.45 |
| resistors | 4 @ 200-Ohm 1/4 watt through-hole resistors | https://www.digikey.com/en/products/filter/through-hole-resistors/53?s=N4IgTCBcDaIE4FMDOBLJAXA9nEBdAvkA | $0.10 |
| resistors | 1 @ 1.0-KOhm 1/4 watt through-hole resistors | https://www.digikey.com/en/products/detail/stackpole-electronics-inc/CF14JT1K00/1741314 | $0.10 |
| resistors | 2 @ 3.3-KOhm 1/4 watt through-hole resistors | https://www.digikey.com/en/products/detail/stackpole-electronics-inc/CF14JT3K30/1741376 | $0.10 |
| resistors | 2 @ 6.65-KOhm 1/4 watt through-hole resistors | https://www.digikey.com/en/products/detail/stackpole-electronics-inc/RNF14FTD6K65/1682364 | $0.10 |
| on/off latch button | 1 @ ON/Off Switch Self-Lock Micro Push Button Switch DC 30V 1A | https://www.amazon.com/gp/product/B086L2GPGX | $0.20 |
| UBEC 5V | 1 @ 5V/6V HOBBYWING 3A UBEC | https://www.aliexpress.us/item/3256805502614547.html | $1.50 |
| 18650 Battery | 2 @ 3.7V 18650 & charger<br>URL has 4 batteries plus charger so price each is inflated | https://www.amazon.com/dp/B0CP6V26QX | $5.00 |
| 18650 2-slot holder | 18650 Battery Holder 2 Slot 3.7V 18650 Battery | https://www.amazon.com/dp/B09LC13D9P | $2.60 |
| JST SM 2.5mm | 1 @ pair JST SM 2.5mm 3-pin Male/Female connectors and cables for power connection | https://www.amazon.com/mxuteuk-Connectors-Connector-Adapter-Electrical/dp/B0DHKC3ZSF/ref=sr_1_2?th=1 | $0.45 |

## Items of Note
[Top](#duelwithbanjos "Top")<br>

### Volume Control
In experimenting I found that the volume control command on this particular YX5200 module
- only takes effect on the next start of a sound file
- causes commands to change sound file also wait for the next start of a sound file

YX5200 modules can vary so I am not sure if this applies to most or every module.

I decided to try a "digital potentiometer" or **digipot** to implement the volume control.
- Adafruit DS3502 I2C Digital 10K Potentiometer Breakout - https://www.adafruit.com/product/4286

I used Audacity to create a mono audio file with the frequency of "A" at 440 Hertz (just above middle C on the piano)
- https://en.wikipedia.org/wiki/Piano_key_frequencies

Looking at the SPK1/SPK2 sound output of the YX5200, I see that
1. both SPK1 and SPK2 are positive with respect to ground
2. they are phase-shifted 180 degrees such that the high point on one is the low point on the other
3. when connected to the speaker (which does not receive ground) it makes it appear to be a balanced signal around ground, with negative and positive movement
4. an alternative view would be to assume one signal (say SPK2) is ground; then it appears to be a signal with twice the amplitude of either individual signal to chip ground

Unfortunately it is hard to power the digipot with a ground based on SPK2, and it would violate the input signal parameters to use the normal ground and let the signal go to -1 volt (which would happen).

My current approach is to use two digipots, one for each SPK signal, and then combine the signals afterwards.

## License
[Top](#duelwithbanjos "Top")<br>
This repository has a LICENSE file for the Apache 2.0 License. There may be code included that I have modified from other open sources (such as Arduino, Espressif, SparkFun, Seeed Studio, DFRobot, RandomNerds, etc.). These other sources may possibly be licensed using a different license model. In such a case I will include some notation of this. Typically I will include verbatim the license in the included/modified source code, but alternatively there might be a LICENSE file in the source code area that points out exceptions to the Apache 2.0 License.
