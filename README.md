# TinkerTuner

## Overview

TinkerTuner is an internet radio for the M5Stack Core2. It streams stations
listed on the SD card, displays station and stream information, and supports touch
controls. An optional gesture sensor can provide hands-free controls.

- Tap to mute or unmute; swipe left/right to change station and up/down to
  adjust volume.
- With the gesture sensor (`PAJ7620U2`), move left/right to change station,
  up/down to adjust volume, or forward to mute/unmute.

## Quick Start

1. Prepare an SD card with `stations.m3u` and `wifi-credentials.txt` in its
   root directory. The repository's `stations.m3u` is an example playlist;
   edit it to add your stations.
   Optionally add stadion logos as PNG files (120 x 120 pixels)
2. Connect the Core2 over USB and, from this sketch directory, compile and
   upload with Arduino CLI:

   ```sh
   arduino-cli compile --profile m5stack-core2 .
   arduino-cli upload --profile m5stack-core2 -p <serial-port> .
   ```

3. Insert the SD card and start the Core2. The radio connects to the first
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

For setup 1 using the Core2 internal speaker, leave `USE_MAX98357A_PORTS_BC` set to `0`.
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

For setup 2's external I²S output, set `USE_MAX98357A_PORTS_BC` to `1` in
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

Create `wifi-credentials.txt` in the SD card root. Add up to three networks
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

Credentials are stored in plaintext: keep the file private and do not commit
or share a populated copy. There is no embedded-credential fallback. Each
network is tried for up to 15 seconds; if none connect, the sketch cycles
through the list again after a pause. If the SD card or credentials file is
unavailable, the sketch reports the problem over Serial and checks again
periodically. Wi-Fi attempts run from the main loop, keeping display and
touch controls responsive.

## Credits

Audio playback is provided by
[ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S) &ndash; Big thanks and kudos!

## References

* [Internet radio stations (de)](https://wiki.ubuntuusers.de/Internetradio/Stationen/)