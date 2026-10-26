///////////////////////////////////////////////////////////////////////////////////////////////////
// WifiManager.cpp
//
// Loads Wi-Fi credentials and manages network connection and retry handling.
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

#include "WifiManager.h"

#include "RadioConfig.h"
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>

bool RadioWifiManager::loadCredentials(bool sdReady) {
    return loadCredentialsFile(sdReady);
}

bool RadioWifiManager::loadCredentialsFile(bool sdReady) {
    credentials_.clear();
    lastFileLoadAt_ = millis();
    if (!sdReady) {
      Serial.println("Cannot load WiFi credentials: SD card is unavailable");
      configDisplayMessage_ = "WiFi config unavailable";
      return false;
    }

    File credentialFile = SD.open(radio_config::wifiCredentialsPath, FILE_READ);
    if (!credentialFile) {
      Serial.printf("WiFi credentials file not found: %s\n", radio_config::wifiCredentialsPath);
      configDisplayMessage_ = "WiFi config missing";
      return false;
    }
    if (credentialFile.size() > 4096) {
      Serial.printf("WiFi credentials file is too large: %s\n", radio_config::wifiCredentialsPath);
      credentialFile.close();
      configDisplayMessage_ = "WiFi config invalid";
      return false;
    }

    String pendingSsid;
    bool hasPendingSsid = false;
    while (credentialFile.available()) {
      String line = credentialFile.readStringUntil('\n');
      line.trim();
      if (!line.length() || line.startsWith("#")) {
        continue;
      }

      const int separator = line.indexOf('=');
      if (separator < 0) {
        Serial.println("Ignoring malformed WiFi setting without '='");
        continue;
      }
      String key = line.substring(0, separator);
      String value = line.substring(separator + 1);
      key.trim();
      value.trim();

      if (key == "SSID") {
        if (hasPendingSsid) {
          Serial.println("Ignoring WiFi entry without a PASSWORD setting");
        }
        pendingSsid = value;
        hasPendingSsid = true;
        if (!pendingSsid.length() || pendingSsid.length() > 32) {
          Serial.println("Ignoring WiFi entry with an invalid SSID length");
          pendingSsid = "";
          hasPendingSsid = false;
        }
        continue;
      }
      if (key != "PASSWORD") {
        Serial.printf("Ignoring unknown WiFi setting: %s\n", key.c_str());
        continue;
      }
      if (!hasPendingSsid) {
        Serial.println("Ignoring PASSWORD setting without a preceding SSID");
        continue;
      }
      if (value.length() > 64) {
        Serial.println("Ignoring WiFi entry with an overlong password");
        hasPendingSsid = false;
        pendingSsid = "";
        continue;
      }
      if (credentials_.size() >= radio_config::maxWifiCredentials) {
        Serial.printf("WiFi credential limit reached (%u); remaining entries ignored\n",
                      static_cast<unsigned>(radio_config::maxWifiCredentials));
        break;
      }
      WifiCredential credential;
      credential.ssid = pendingSsid;
      credential.password = value;
      credentials_.push_back(credential);
      pendingSsid = "";
      hasPendingSsid = false;
    }
    credentialFile.close();

    if (hasPendingSsid) {
      Serial.println("Ignoring final WiFi entry without a PASSWORD setting");
    }
    if (credentials_.empty()) {
      Serial.println("WiFi credentials file contains no valid network entries");
      configDisplayMessage_ = "WiFi config invalid";
      return false;
    }
    configDisplayMessage_ = "";
    Serial.printf("Loaded %u WiFi credential entr(y/ies)\n",
                  static_cast<unsigned>(credentials_.size()));
    return true;
}

void RadioWifiManager::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    nextAttemptAt_ = millis();
}

WifiEvent RadioWifiManager::update(bool& sdReady) {
    const uint32_t now = millis();
    if (WiFi.isConnected()) {
      if (!connected_) {
        connected_ = true;
        attemptActive_ = false;
        ESP_LOGI("TinkerTuner", "Connected to WiFi");
        return WifiEvent::Connected;
      }
      return WifiEvent::None;
    }

    WifiEvent event = WifiEvent::None;
    if (connected_) {
      connected_ = false;
      Serial.println("WiFi connection lost; reconnecting");
      attemptIndex_ = 0;
      nextAttemptAt_ = now + radio_config::wifiRetryDelayMs;
      event = WifiEvent::Disconnected;
    }

    if (attemptActive_) {
      if (now - attemptStartedAt_ < radio_config::wifiAttemptTimeoutMs) {
        return event;
      }
      Serial.printf("WiFi connection timed out for SSID entry %u\n",
                    static_cast<unsigned>(attemptIndex_ + 1));
      WiFi.disconnect(false, false);
      attemptActive_ = false;
      ++attemptIndex_;
      if (attemptIndex_ >= credentials_.size()) {
        attemptIndex_ = 0;
        nextAttemptAt_ = now + radio_config::wifiCycleRetryDelayMs;
        Serial.println("All configured WiFi networks failed; retrying later");
      } else {
        nextAttemptAt_ = now + radio_config::wifiRetryDelayMs;
      }
      return event;
    }

    if (now - lastFileLoadAt_ >= radio_config::wifiFileReloadDelayMs
        && credentials_.empty()) {
      if (!sdReady) {
        sdReady = SD.begin(radio_config::sdCs, SPI, 1000000);
        if (!sdReady) {
          lastFileLoadAt_ = now;
          Serial.println("SD card is still unavailable; will retry WiFi configuration later");
          return event;
        }
      }
      loadCredentialsFile(sdReady);
      attemptIndex_ = 0;
      nextAttemptAt_ = now + radio_config::wifiRetryDelayMs;
    }
    if (credentials_.empty() || static_cast<int32_t>(now - nextAttemptAt_) < 0) {
      return event;
    }

    const WifiCredential& credential = credentials_[attemptIndex_];
    Serial.printf("Connecting to configured WiFi network %u of %u\n",
                  static_cast<unsigned>(attemptIndex_ + 1),
                  static_cast<unsigned>(credentials_.size()));
    WiFi.disconnect(false, false);
    if (credential.password.length() == 0) {
      WiFi.begin(credential.ssid.c_str());
    } else {
      WiFi.begin(credential.ssid.c_str(), credential.password.c_str());
    }
    attemptStartedAt_ = now;
    attemptActive_ = true;
    return event;
}

const String& RadioWifiManager::configDisplayMessage() const {
    return configDisplayMessage_;
}
