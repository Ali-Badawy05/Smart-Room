# Smart Room IoT System

A smart-room IoT system built around an **ESP32-WROOM-32**. The firmware connects the room hardware to an MQTT broker and provides remote monitoring and control.

The system is designed to be controlled through:

- 📱 **Flutter Mobile Application**
- 🌐 **Flutter Web Dashboard**
- 🔌 **Physical switches connected to the ESP32**

The Flutter application and web dashboard communicate with the ESP32 through **MQTT**, allowing the user to monitor sensors, control relays, activate Away Mode, and receive motion alerts.

---

## Features

- 🌡️ Temperature and humidity monitoring using **DHT11**
- 🚶 Room presence / motion detection using **PIR**
- 💡 Control of **3 relays**
- 🔘 Physical switches for local relay control
- 📡 Wi-Fi connectivity with automatic reconnection
- ☁️ MQTT communication using **HiveMQ Cloud or another MQTT broker**
- 🚨 Motion alert when **Away Mode** is enabled
- 🔔 Buzzer alert for detected motion while Away Mode is active
- 💾 Relay states are saved in ESP32 non-volatile storage
- 🔄 OTA firmware updates
- 🌐 Built-in OTA web page
- ❤️ MQTT online/offline status and heartbeat
- 📱 Remote control through a Flutter mobile app
- 💻 Remote monitoring/control through a Flutter web dashboard

---

## System Architecture

```text
                    ┌─────────────────────┐
                    │   Flutter Mobile    │
                    │       App           │
                    └──────────┬──────────┘
                               │
                               │ MQTT
                               │
                    ┌──────────▼──────────┐
                    │    MQTT Broker      │
                    │   HiveMQ / Custom   │
                    └──────────┬──────────┘
                               │
                               │ MQTT
                               │
                    ┌──────────▼──────────┐
                    │        ESP32        │
                    │   Smart Room MCU    │
                    └─────┬─────┬─────┬───┘
                          │     │     │
                 ┌────────┘     │     └────────┐
                 │              │              │
              Sensors         Relays         Buzzer
                 │              │
             ┌───┴───┐      ┌───┴────┐
             │ DHT11 │      │ 3 Loads │
             │  PIR  │      │/Devices │
             └───────┘      └─────────┘

                    ┌─────────────────────┐
                    │    Flutter Web      │
                    │     Dashboard       │
                    └──────────┬──────────┘
                               │
                               │ MQTT
                               └──────────────► MQTT Broker
```

---

## Hardware

### Main Controller

- ESP32-WROOM-32

### Sensors and Outputs

| Component | GPIO |
|---|---:|
| Relay 1 | GPIO 26 |
| Relay 2 | GPIO 27 |
| Relay 3 | GPIO 14 |
| Switch 1 | GPIO 32 |
| Switch 2 | GPIO 33 |
| Switch 3 | GPIO 25 |
| DHT11 | GPIO 4 |
| PIR Sensor | GPIO 13 |
| Buzzer | GPIO 21 |

The relay module is configured as **active-low** by default.

---

## Firmware Structure

```text
src/
├── main.cpp
├── config.h
├── logger.h
│
├── wifi_manager.cpp
├── wifi_manager.h
│
├── mqtt_manager.cpp
├── mqtt_manager.h
│
├── sensors.cpp
├── sensors.h
│
├── relay_control.cpp
├── relay_control.h
│
├── buzzer.cpp
├── buzzer.h
│
├── ota_manager.cpp
└── ota_manager.h
```

### Main Modules

**`main.cpp`**

Coordinates the complete system and handles:

- Wi-Fi
- MQTT
- Sensors
- Relays
- Buzzer
- OTA
- Watchdog
- Publishing sensor/device updates

**`wifi_manager`**

Handles:

- Wi-Fi connection
- Automatic reconnection
- Connection monitoring
- DNS configuration

**`mqtt_manager`**

Handles:

- MQTT connection
- TLS connection
- MQTT authentication
- Automatic reconnect with exponential backoff
- MQTT subscriptions
- Publishing messages

**`sensors`**

Handles:

- DHT11 temperature/humidity readings
- PIR motion/presence detection
- Sensor validation
- Presence state changes

**`relay_control`**

Handles:

- Three relays
- Physical switches
- Switch debouncing
- MQTT relay commands
- Persistent relay states

**`buzzer`**

Handles the three-beep alert pattern when motion is detected during Away Mode.

**`ota_manager`**

Handles:

- OTA firmware updates
- OTA web interface
- mDNS access
- Remote version checking
- Firmware rollback validation

---

# MQTT Communication

The ESP32 communicates with the Flutter applications through MQTT.

The default device ID is:

```text
smart_room_01
```

The base topic is:

```text
smart_room/smart_room_01/
```

## Sensor Topics

### Temperature and Humidity

```text
smart_room/smart_room_01/sensor/dht
```

Example payload:

```json
{
  "temp": 25.4,
  "hum": 51.2
}
```

### Presence

```text
smart_room/smart_room_01/sensor/presence
```

Possible values:

```text
occupied
```

or

```text
empty
```

### Motion Alert

```text
smart_room/smart_room_01/alert/motion
```

Example:

```json
{
  "event": "motion_detected"
}
```

---

# Relay Control

There are three controllable relays.

## Relay 0

Command:

```text
smart_room/smart_room_01/relay/0/set
```

State:

```text
smart_room/smart_room_01/relay/0/state
```

## Relay 1

Command:

```text
smart_room/smart_room_01/relay/1/set
```

State:

```text
smart_room/smart_room_01/relay/1/state
```

## Relay 2

Command:

```text
smart_room/smart_room_01/relay/2/set
```

State:

```text
smart_room/smart_room_01/relay/2/state
```

### Accepted command values

The ESP32 accepts:

```text
ON
OFF
```

and also:

```text
1
0
TRUE
FALSE
```

Relay states are published as:

```text
ON
```

or:

```text
OFF
```

State messages are retained by MQTT so the Flutter interfaces can receive the latest known state.

---

# Away Mode

Away Mode is controlled through:

```text
smart_room/smart_room_01/away_mode/set
```

Possible values:

```text
ON
OFF
```

The current state is published to:

```text
smart_room/smart_room_01/away_mode/state
```

When Away Mode is enabled and the PIR sensor detects presence:

1. The ESP32 publishes a motion alert.
2. The buzzer starts a three-beep pattern.
3. The Flutter application/web dashboard can receive the alert through MQTT.

---

# Device Status

The ESP32 publishes its status through:

```text
smart_room/smart_room_01/status
```

Normal state:

```text
online
```

If the MQTT connection is lost, the broker will receive:

```text
offline
```

through the MQTT Last Will and Testament mechanism.

The ESP32 also sends an online heartbeat approximately every **30 seconds**.

---

# OTA Updates

The firmware supports two OTA mechanisms.

## 1. OTA Web Page

After the ESP32 connects to Wi-Fi, the OTA page is available at:

```text
http://<ESP32-IP>/update
```

mDNS is also configured using:

```text
http://smart-room.local/update
```

The OTA page allows firmware to be uploaded remotely without connecting the ESP32 to the computer using USB.

## 2. MQTT OTA Trigger

The ESP32 subscribes to:

```text
smart_room/smart_room_01/ota/check
```

Publishing a message to this topic causes the ESP32 to check the configured firmware version URL.

The version information is expected to contain:

```json
{
  "version": "1.2.0",
  "url": "https://example.com/firmware.bin"
}
```

---

# Flutter Application and Web Dashboard

The system is intended to have two Flutter interfaces:

### Flutter Mobile App

The mobile application can provide:

- Live temperature
- Live humidity
- Room presence
- Relay ON/OFF controls
- Away Mode control
- Motion alerts
- Device online/offline status
- OTA/device information

### Flutter Web Dashboard

The web interface provides the same MQTT-based control and monitoring functionality through a browser.

Both interfaces communicate with the same MQTT broker, meaning the ESP32 can be controlled from either the mobile application or the web dashboard.

```text
Flutter App ────────┐
                    │
Flutter Web ────────┼── MQTT Broker ── ESP32
                    │
Other MQTT Clients ─┘
```

---

# Software Requirements

For the ESP32 firmware:

- Arduino IDE or PlatformIO
- ESP32 board support
- ESP32-WROOM-32
- C++ / Arduino framework

Required libraries include:

- WiFi
- PubSubClient
- WiFiClientSecure
- DHT sensor library
- Preferences
- HTTPClient
- HTTPUpdate
- WebServer
- ESPmDNS
- ElegantOTA

For the user interface:

- Flutter SDK
- Dart
- MQTT-compatible Flutter package/client
- MQTT broker credentials

---

# Configuration

Before uploading the firmware, edit:

```text
src/config.h
```

Configure:

```cpp
#define WIFI_SSID     "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

#define MQTT_HOST     "YOUR_MQTT_BROKER"
#define MQTT_PORT     8883
#define MQTT_USER     "YOUR_MQTT_USERNAME"
#define MQTT_PASS     "YOUR_MQTT_PASSWORD"
```

You can also change:

- Device ID
- GPIO pins
- Number of relays
- DHT type
- Sensor limits
- Timing values
- OTA settings

---

# Security Notice

**Do not upload real Wi-Fi, MQTT, or OTA passwords to GitHub.**

The original firmware configuration contains credentials. Before making this repository public:

1. Replace the credentials with placeholders.
2. Change any credentials that were previously exposed.
3. Do not commit passwords or private broker credentials.
4. Consider using a separate `config.example.h` file for GitHub.

Example:

```cpp
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

#define MQTT_HOST     "YOUR_MQTT_HOST"
#define MQTT_USER     "YOUR_MQTT_USERNAME"
#define MQTT_PASS     "YOUR_MQTT_PASSWORD"
```

Also review the TLS configuration before deploying the system in production. The current firmware uses an insecure TLS client configuration for MQTT/OTA connections.

---

# How It Works

1. The ESP32 starts and initializes the relays, sensors, buzzer, Wi-Fi, MQTT and OTA services.
2. The ESP32 connects to the configured Wi-Fi network.
3. It connects to the MQTT broker using TLS and MQTT authentication.
4. The ESP32 publishes sensor readings and device states.
5. The Flutter mobile application and Flutter web dashboard subscribe to the MQTT topics.
6. Users can control the relays and Away Mode from either Flutter interface.
7. Physical switches can also control the relays directly.
8. When Away Mode is active, detected motion generates an MQTT alert and activates the buzzer.
9. Firmware can be updated remotely through the OTA web interface or MQTT-triggered OTA checking.

---

# Project Status

The ESP32 firmware implements the core smart-room controller, MQTT communication, sensor monitoring, relay control, alerts, persistent relay states, Wi-Fi recovery, and OTA functionality.

The overall system is designed around a **Flutter mobile application + Flutter web dashboard + ESP32 + MQTT broker** architecture.

---

# License

Add your preferred license here, for example:

```text
MIT License
```

If this project is intended for private/academic use, you can replace this section with the appropriate project ownership and usage terms.
