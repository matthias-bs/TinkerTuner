///////////////////////////////////////////////////////////////////////////////////////////////////
// SettingsStore.cpp
//
// Loads and saves persistent radio settings using ESP32 preferences.
//
// TinkerTuner -- I2S internet radio for M5Stack Core2
//
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

#include "SettingsStore.h"

#include "RadioConfig.h"

PersistentSettings SettingsStore::load() {
    PersistentSettings settings;
    settings.volume = radio_config::defaultVolume;
    settings.gestureEnabled = true;
    if (!preferences_.begin(radio_config::preferencesNamespace, false)) {
      Serial.println("Could not open NVS preferences; using default radio settings");
      return settings;
    }
    ready_ = true;
    settings.volume = preferences_.getUChar("volume", radio_config::defaultVolume);
    settings.stationIndex = preferences_.getUInt("station", 0);
    settings.gestureEnabled = preferences_.getBool("gesture", true);
    if (settings.volume > 21) {
      Serial.printf("Stored volume %u is invalid; using default\n", radio_config::defaultVolume);
      settings.volume = radio_config::defaultVolume;
      dirty_ = true;
      changedAt_ = millis();
    }
    return settings;
}

void SettingsStore::markChanged() {
    if (!ready_) {
      return;
    }
    dirty_ = true;
    changedAt_ = millis();
}

void SettingsStore::saveIfDue(uint8_t volume, size_t stationIndex, bool hasStations, bool gestureEnabled) {
    if (!ready_ || !dirty_ || millis() - changedAt_ < radio_config::preferencesSaveDelayMs) {
      return;
    }

    dirty_ = false;
    const size_t volumeBytes = preferences_.putUChar("volume", volume);
    if (volumeBytes != sizeof(uint8_t)) {
      Serial.println("Could not persist volume preference");
    }
    if (hasStations) {
      const size_t stationBytes = preferences_.putUInt("station", stationIndex);
      if (stationBytes != sizeof(uint32_t)) {
        Serial.println("Could not persist station preference");
      }
    }
    const size_t gestureBytes = preferences_.putBool("gesture", gestureEnabled);
    if (gestureBytes != sizeof(bool)) {
      Serial.println("Could not persist gesture preference");
    }
}
