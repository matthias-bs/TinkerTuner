> [!CAUTION]
> Parts of this project were created or assisted by AI tools.

# TinkerTuner

## Overview

TinkerTuner is an internet radio for the M5Stack Core2. It streams stations
listed on the SD card, displays station and stream information, and supports touch
controls. An optional gesture sensor (`PAJ7620U2`) can provide hands-free controls.

- Tap to mute or unmute; swipe left/right to change station and up/down to
  adjust volume.
- If a gesture sensor is connected, press button C to enable or disable gesture
  controls. The setting is saved in preferences.
- With the gesture sensor, move left/right to change station,
  up/down to adjust volume, or forward to mute/unmute.
  A short lockout after each gesture prevents the hand's withdrawal from being
  interpreted as a second gesture.

## Quick Start

1. Prepare the WiFi credentials using one of these options:
   - Put `wifi-credentials.txt` in the SD card's root directory.
   - For the internal-flash variant, put `wifi-credentials.txt` in the
     repository's `data/` directory and upload it with the Arduino IDE's
     **ESP32 LittleFS Data Upload** tool as described below.
2. Prepare the SD card with `stations.m3u` in its root directory. The
   repository's `stations.m3u` is an example playlist; edit it to add your
   stations. Optionally add station logos as PNG files (120 x 120 pixels).
3. Connect the Core2 over USB and, from this sketch directory, compile and
   upload with Arduino CLI:

   ```sh
   arduino-cli compile --profile m5stack-core2 .
   arduino-cli upload --profile m5stack-core2 -p <serial-port> .
   ```

4. Insert the SD card and start the Core2. The radio connects to the first
   available configured Wi-Fi network and starts a station from the playlist.

The board profile and library dependencies are declared in `sketch.yaml`.
The playlist uses `#EXTINF` station-name lines followed by an `http://` or
`https://` stream URL. Up to 20 stations are loaded.

## Options

| Setup | Description | Status |
| --- | --- | --- |
| 1. Minimal | Core2 with its internal speaker. | Supported by the default I²S pin configuration. |
| 2. Prototype | Optional gesture sensor and/or external I²S output via M5GO2. | Supported; the gesture sensor is on Port A, and the M5GO2 provides Port B/C I²S connections. |
| 3. Prototype | Optional gesture sensor and/or external I²S output via M5 Proto Module. | TBD |
| 4. Final | Custom gesture sensor and I²S interface module. | TBD |

### Setup 1 &ndash; Minimal

For setup 1 using the Core2 internal speaker, leave `USE_I2S_PORTS_BC` set to `0`.
These I²S connections are internal to the board; no wiring is needed:

| I²S signal | Core2 GPIO |
| --- | --- |
| `BCLK` / `BCK` | `GPIO12` |
| `LRC` / `WS` / `LRCLK` | `GPIO0` |
| Data output (`DOUT`) | `GPIO2` |

### Setup 2 &ndash; Prototype

> ![WARNING]
> Make sure your `PAJ7620U2` gesture sensor module allows 5V supply voltage
> before connecting it to Port A.

For setup 2's external I²S output, set `USE_I2S_PORTS_BC` to `1` in
`src/RadioConfig.h`. 

Wire the I²S output device as follows:

| Core2/M5GO2 connection | I²S output device |
| --- | --- |
| Port B `G26` | `BCLK` / `BCK` |
| Port C `RX` (`G13`) | `LRC` / `WS` / `LRCLK` |
| Port C `TX` (`G14`); Data output (`DOUT`) | Data input (`DIN`) |
| Port B or C `GND` | `GND` |

Connect grounds and follow the output device's power and load requirements.
Use a suitable external regulated supply if the Core2/M5GO2 5 V rail cannot provide the voltage or current required by the amplifier. Use speakers within the amplifier's supported impedance and power ratings.

### Setup 3 & 4

Setup 3 and 4 have not been implemented yet.

The project's final target is a separate class D amplifier connected via I2S. Candidates are:
* PCM5102A + TPA3116D2 based board
* TASM5805M based board (e.g. [Louder](https://github.com/sonocotta/raspberry-media-center#louder-raspberry-pi-media-center-and-hats))

## WiFi Credentials

Create `wifi-credentials.txt` in the internal flash filesystem at its root, or
in the SD card root as a fallback. The internal flash file is tried first and
is preferred because it adds a little security. Add up to three networks
in preference order, with an `SSID=` line followed by a `PASSWORD=` line
for each:

```ini
SSID=Home WiFi
PASSWORD=your-password

# An empty password is an open network.
SSID=Guest network
PASSWORD=
```

Lines beginning with `#` are comments; blank lines are allowed. Use uppercase
keys. Values are trimmed and split at the first `=`, so passwords can contain
additional equals signs. SSIDs must be 1–32 bytes; passwords may be empty or
up to 64 bytes.

### Uploading credentials to internal flash with Arduino IDE

Create a `data/` directory in the repository if it does not already exist and
put `wifi-credentials.txt` directly in it:

```text
TinkerTuner/
└── data/
    └── wifi-credentials.txt
```

Install [arduino-littlefs-upload](https://github.com/earlephilhower/arduino-littlefs-upload)
for Arduino IDE 2.2.1 or newer: copy its release VSIX file to
`~/.arduinoIDE/plugins/` on Linux/macOS or
`C:\Users\<username>\.arduinoIDE\plugins\` on Windows, creating the directory
if needed, then restart the IDE. Select the M5Stack Core2 board and its serial
port, and close Serial Monitor or any other program using the port. Open the
command palette with **Ctrl+Shift+P** (**Cmd+Shift+P** on macOS), then select
**Upload LittleFS to Pico/ESP8266/ESP32**.
The tool uploads the contents of `data/` to the board's internal LittleFS
filesystem. Reset the board after the upload. The application then reads
`/wifi-credentials.txt` from internal flash before checking the SD card.

Credentials are stored in plaintext: keep the file private and do not commit
or share a populated copy. If the internal flash file is missing or invalid,
the SD card file is used. Each
network is tried for up to 15 seconds; if none connect, the sketch cycles
through the list again after a pause. If the SD card or credentials file is
unavailable, the sketch reports the problem over Serial and checks again
periodically. Wi-Fi attempts run from the main loop, keeping display and
touch controls responsive.

## Credits

Audio playback is provided by
[ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S) &ndash; Big thanks and kudos!

## References

* [arduino-littlefs-upload](https://github.com/earlephilhower/arduino-littlefs-upload)
* [Internet radio stations (de)](https://wiki.ubuntuusers.de/Internetradio/Stationen/)
* [M5Stack Unit Gesture](https://docs.m5stack.com/en/unit/Unit_Gesture)
