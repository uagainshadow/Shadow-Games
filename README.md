SHADOW GAMES Firmware Specification

Unified RF Awareness Dashboard

Target Platform: M5Stack Cardputer ADV (ESP32-S3) with official LoRa HAT

Project Goal

Shadow Games is an all-in-one situational awareness dashboard designed for the Cardputer.

Instead of opening several different utilities, the user sees a single live dashboard where multiple RF monitoring modules operate simultaneously.

The interface is inspired by:

1940s radar consoles
WWII command centers
CIA operations rooms
UFO tracking displays
Green phosphor CRT monitors
The interface should feel like old military equipment rather than modern cyberpunk.

 

 

Home Dashboard

The dashboard refreshes approximately 10 times per second.

Each module occupies its own panel.

Example layout:

+----------------------------------------------------+

|                 SHADOW GAMES                       |

+----------------------------------------------------+

 

 Radar Display

 

 WiFi Activity

 Bluetooth Activity

 

 Deauthentication Monitor

 

 Nearby Flipper Devices

 

 Camera / Smart Glass Detection

 

 RFID / Card Skimmer Detection

 

 GPS Status

 

 LoRa Status

 

 Battery

 

 CPU Usage

 

 Temperature

 

 Notification Feed

 

 Bottom Navigation

 

 

Module 1

WiFi Monitor

Displays

Nearby AP count

Channel

RSSI

Encryption

Clients

Signal history

Radar animation

Should continuously scan 2.4 GHz.

 

Module 2

Bluetooth Monitor

Displays

Classic Bluetooth

BLE

Nearby device count

RSSI

Estimated distance

Manufacturer

Radar sweep

Known device icons

 

Module 3

Deauthentication Detection

Continuously monitor

802.11 Management Frames

Detect

Deauthentication frames

Disassociation frames

Beacon floods

Suspicious packet rate

Display

Live graph

Alerts

Time

Channel

MAC

Attack intensity

 

Module 4

Nearby Device Detection

Show nearby devices that advertise recognizable identifiers (such as Bluetooth names or Wi-Fi management frames). The firmware should avoid claiming it can reliably identify a specific product unless there is a documented, observable signature to do so.

Display

Signal

Distance estimate

Vendor

Last Seen

 

Module 5

Camera / Smart Device Awareness

Monitor for observable wireless activity associated with nearby wireless devices (for example, certain Wi-Fi or Bluetooth advertisements). The firmware should not claim it can definitively detect hidden cameras or smart glasses solely from RF activity, because many devices use randomized identifiers and there is no universal RF signature for “camera glasses.”

 

 

Module 6

RFID Activity

Monitor

13.56 MHz reader activity using compatible external hardware.

If future hardware supports NFC sniffing:

Display

Field strength

Activity

Alert level

Timestamp

 

Module 7

GPS

Display

Latitude

Longitude

Heading

Altitude

Speed

Satellites

Compass

Mini map

 

Module 8

LoRa

Compatible with official Cardputer ADV LoRa HAT.

Display

Frequency

RSSI

SNR

Packets

TX/RX

Network status

Radio health

Interface Style

Everything should resemble

1945 radar equipment

Military typewriters

Green CRT

Oscilloscopes

Vintage aviation

No neon cyberpunk.

Animations should be subtle.

Theme

Green phosphor

Amber warning lights

Paper dossier graphics

Typewriter fonts

Military labels

Radar sweep

Noise texture

Glow

 

Boot Sequence

Shadow Games

 

Initializing CPU

 

Loading RF Drivers

 

Loading WiFi

 

Loading Bluetooth

 

Loading GPS

 

Loading LoRa

 

Loading Dashboard

 

Ready

Menu

Dashboard

WiFi

Bluetooth

Devices

RF Monitor

GPS

LoRa

Logs

Settings

About

 

Settings

Brightness

Sound

Refresh Rate

Power Saving

GPS

WiFi

Bluetooth

LoRa

Logging

Theme

 

Logging

Save

GPS tracks

Detected devices

WiFi scan history

Bluetooth history

RF events

Alerts

CSV

JSON

 

Alerts

Animated warning banner

Sound

Flash border

Timestamp

Log entry

Performance Goals

Target

20–30 FPS UI

<70% CPU

Smooth animations

Responsive keyboard

Battery optimized

Software Architecture

ESP-IDF

 

Main Task

 

UI Task

 

WiFi Task

 

Bluetooth Task

 

GPS Task

 

LoRa Task

 

Logger Task

 

Storage Task

 

Power Manager

 

Message Queue

 

Shared Event Bus

Each subsystem should run independently and publish status updates to a central event bus so the dashboard remains responsive even if one module is busy.

A couple of important notes on the implementation:

I would not instruct another AI to hard-code GPIO assignments, partition tables, flash modes, flash frequencies, or upload parameters without first confirming the exact Cardputer ADV revision and LoRa HAT model. Those details must come from the manufacturer’s hardware documentation or board definition to avoid creating firmware that won’t boot.
Likewise, features such as reliable detection of “Flipper Zero devices,” “camera glasses,” or “card skimmers” have technical limitations. The firmware should present them as detection of observable wireless activity or known signatures where applicable, rather than claiming guaranteed identification.<img width="1536" height="1024" alt="shadowGamesV1" src="https://github.com/user-attachments/assets/c75bcdbe-d810-4d68-997f-3700414b61d5" />
