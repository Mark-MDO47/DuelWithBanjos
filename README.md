# DuelWithBanjos
Just for fun - tiny scarecrow skeletons playing banjo music, placed under the control of the UniRemote!
- https://github.com/Mark-MDO47/UniRemote

**Table Of Contents**
* [Top](#duelwithbanjos "Top")
* [Introduction](#introduction "Introduction")
* [Schematic](#schematic "Schematic")
* [License](#license "License")

## Introduction
[Top](#duelwithbanjos "Top")<br>
I have two country style skeleton banjo players approximately 14 inches tall ordered from Amazon.com but apparently that exact model is no longer available there.
I think they might be similar to this one:
- https://www.amazon.com/XINANDHAO-Halloween-Animated-Skeletons-Musicians/dp/B0DC56RL1F

The plan is to make them
- respond to controls from my UniRemote remote controller via ESP-NOW
- play music using a YX5200 Audio Player which reads sound files from a MicroSD card
- blink LED "eyes" using Pulse Width Modulation (PWM)

## Schematic
[Top](#duelwithbanjos "Top")<br>
This is the schematic of the interesting part of the design, including the ESP32 module, sound control, and LED control.
<img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/KiCad/Duel_schematic_01.png" width="800" alt="Schematic of ESP32 module, sound control, and LED control">

The ESP32 of course has I/O at 3.3V but the YX5200 uses 5V. I use resistor networks to drop the voltages from 5V to 3.3V and a SN74HCT125N quadruple bus buffer to raise voltages from 3.3V to 5V.

The 1K resistor in the path to the YX5200 RX line prevents coupling of digital noise into the sound output.

## License
[Top](#duelwithbanjos "Top")<br>
This repository has a LICENSE file for the Apache 2.0 License. There may be code included that I have modified from other open sources (such as Arduino, Espressif, SparkFun, Seeed Studio, DFRobot, RandomNerds, etc.). These other sources may possibly be licensed using a different license model. In such a case I will include some notation of this. Typically I will include verbatim the license in the included/modified source code, but alternatively there might be a LICENSE file in the source code area that points out exceptions to the Apache 2.0 License.
