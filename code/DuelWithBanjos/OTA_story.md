# OTA:WEB Story

**Table Of Contents**
* [Top](#otaweb-story "Top")
* [My Adaptation of the ESP32 Example OTAWebUpdater.ino](#my-adaptation-of-the-esp32-example-otawebupdaterino "My Adaptation of the ESP32 Example OTAWebUpdater.ino")
* [The Story](#the-story "The Story")
  * [The Setup](#the-setup "The Setup")
  * [Send OTA-WEB Command](#send-ota\-web-command "Send OTA-WEB Command")
  * [Enter Login Credentials](#enter-login-credentials "Enter Login Credentials")
  * [Click on Choose File](#click-on-choose-file "Click on Choose File")
  * [Time to Click on the UPDATE Button](#time-to-click-on-the-update-button "Time to Click on the UPDATE Button")
  * [The Upload Completes, Banjo Player ESP32 Reboots and Disconnects from WiFi Router](#the-upload-completes,-banjo-player-esp32-reboots-and-disconnects-from-wifi-router "The Upload Completes, Banjo Player ESP32 Reboots and Disconnects from WiFi Router")

## My Adaptation of the ESP32 Example OTAWebUpdater.ino
The idea is that the OTA Web Updater is not started until we receive an ESP-NOW
command to initialize and start it up. Prior to that, we do not connect to any
WiFi SSID. We only use ESP-NOW which is point-to-point.

In this implementation, it will probably hang if it cannot connect to the specified WiFi SSID.

## The Story

### The Setup
My two little Banjo Players. We will re-program Over-The-Air the Banjo Player ESP32 (in the box).<br>
<img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/banjo_players_after.jpg" width="500" alt="My Banjo Players">

From the Arduino IDE, export the sketch you want to upload as **compiled binary**.<br>
<img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/Sketch_ExportCompiledBinary.png" width="500" alt="Export sketch as compiled binary">

The UniRemoteCYD and the card with the OTA:WEB command card for the Banjo Players. The OTA:WEB command causes the Banjo Players to connect to the WiFi Router and start the login page website for Over-The-Air loading.<br>
<img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/OTA_WEB_CMD_UniRemoteCYD.jpg" width="500" alt="UniRemoteCYD and OTA:WEB command card">

Before sending the OTA:WEB command, the Banjo Players have not connected to the WiFi router. A browser cannot find the website for OTA loading. <br>
| Browser Webpage |
| --- |
| <img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/Site01_B4_OTA_WEB_CMD_CannotBeReached.png" width="300" alt="Before sending the OTA:WEB command the website is not available."> |

### Send OTA-WEB Command
After  sending the OTA:WEB command, the Banjo Players connect to the WiFi router and start the OTA loading website login page. Refreshing the browser will show the page.<br>
| Browser Webpage |
| --- |
| <img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/Site02_After_OTA_WEB.png" width="300" alt="After sending the OTA:WEB command the website is available."> |

### Enter Login Credentials
After entering login credentials, the OTA upload page appears.<br>
| Browser Webpage |
| --- |
| <img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/Site03_AfterLogin.png" width="300" alt="After entering login credentials, the OTA upload page appears."> |

### Click on Choose File
After clicking on the Choose File button, a standard open-file dialog box opens. Choose the **"*.bin"** file you want to upload.<br>
| Browser Webpage |
| --- |
| <img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/Site04_ChoosingFileToLoad.png" width="500" alt="After clicking the choose-file button, a file brower opens."> |

### Time to Click on the UPDATE Button
The chosen file is displayed. Now click on the **UPDATE** button to start the upload.<br>
| Browser Webpage |
| --- |
| <img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/Site05_ReadyToLoadFile.png" width="300" alt="Now ready to click on the UPDATE button."> |

The upload progress is displayed.<br>
| Browser Webpage |
| --- |
| <img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/Site06_FileLoadInProgress.png" width="300" alt="The upload progress is displayed."> |

### The Upload Completes, Banjo Player ESP32 Reboots and Disconnects from WiFi Router
The Upload completes and the browser shows complete. The Banjo Player ESP32 reboots automatically. After reboot it does not connect to WiFi Router until it receives another OTA:WEB command.<br>
| Browser Webpage |
| --- |
| <img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/Site07_FileLoadComplete_AutoReboot.png" width="300" alt="The Upload Completes, Banjo Player ESP32 Reboots and Disconnects from WiFi Router."> |

Refreshing the webpages will show that the Banjo Players are not serving the webpage anymore. In fact, they are not connected to the WiFi router at all.<br>
| Browser Webpage |
| --- |
| <img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/Site08_AfterAutoReboot_CannotBeReached.png" width="300" alt="Banjo Player is not connected to the WiFi Router. Refresh of webpage shows not connected."> |
