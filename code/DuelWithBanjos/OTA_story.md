# OTA:WEB Story

**Table Of Contents**

## My Adaptation of the ESP32 Example OTAWebUpdater.ino
The idea is that the OTA Web Updater is not started until we receive an ESP-NOW
command to initialize and start it up. Prior to that, we do not connect to any
WiFi SSID. We only use ESP-NOW which is point-to-point.

In this implementation, it will probably hang if it cannot connect to the specified WiFi SSID.

## The Story

### The Setup
My two little Banjo Players. We will program the Banjo Player ESP32 in the box Over-The-Air<br>
<img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/banjo_players_after.jpg" width="500" alt="My Banjo Players">

The UniRemoteCYD and the card with the OTA:WEB command card for the Banjo Players. This command causes the Banjo Players to connect to the WiFi Router and start the login page website for Over-The-Air loading.<br>
<img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/OTA_WEB_CMD_UniRemoteCYD.jpg" width="500" alt="UniRemoteCYD and OTA:WEB command card">

Before sending the OTA:WEB command, the Banjo Players have not connected to the WiFi router. A browser cannot find the website for OTA loading. <br>
| Browser Webpage |
| --- |
| <img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/Site01_B4_OTA_WEB_CMD_CannotBeReached.png" width="300" alt="Before sending the OTA:WEB command the website is not available."> |

### After Sending OTA-WEB Command
After  sending the OTA:WEB command, the Banjo Players connect to the WiFi router and start the OTA loading website login page. Refreshing the browser will show the page.<br>
| Browser Webpage |
| --- |
| <img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/Site02_After_OTA_WEB.png" width="500" alt="After sending the OTA:WEB command the website is available."> |

### After Entering Login Credentials
After entering login credentials, the OTA upload page appears.<br>
| Browser Webpage |
| --- |
| <img src="https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/resources/images/Site03_AfterLogin.png" width="300" alt="After entering login credentials, the OTA upload page appears."> |
