///////////////////////////////////////////////////////////////////////////////////////////////////
// RadioTypes.h
//
// Declares shared station, credential, and control data types.
//
// TinkerTuner -- I2S internet radio for M5Stack Core2
// https://github.com/matthias-bs/TinkerTuner
//
// First released: October 26, 2026
//
// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright (c) 2026 Matthias Prinke
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Arduino.h>

struct Station {
    String name;
    String url;
    String logoPath;
};

struct WifiCredential {
    String ssid;
    String password;
};

struct PersistentSettings {
    uint32_t stationIndex = 0;
    uint8_t volume = 3;
};

enum class InputAction {
    NextStation,
    PreviousStation,
    VolumeUp,
    VolumeDown,
    ToggleMute
};
