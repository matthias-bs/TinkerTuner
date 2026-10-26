///////////////////////////////////////////////////////////////////////////////////////////////////
// RadioController.cpp
//
// Coordinates audio playback, stations, input, display, Wi-Fi, and settings.
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

#include "RadioController.h"

#include "RadioConfig.h"
#include "StationCatalog.h"
#include <M5Unified.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <cstring>

RadioController* RadioController::instance_ = nullptr;

void RadioController::begin() {
    instance_ = this;
    Audio::audio_info_callback = audioInfoCallback;

    auto cfg = M5.config();
    cfg.internal_spk = true;
    cfg.serial_baudrate = 115200;
    M5.begin(cfg);
    input_.begin(inputActionCallback, this);

    const PersistentSettings settings = settingsStore_.load();
    savedVolume_ = settings.volume;
    savedStationIndex_ = settings.stationIndex;

    M5.Speaker.end();
    switch (M5.Power.getType()) {
      case m5::Power_Class::pmic_axp192:
        M5.Power.Axp192.setGPIO2(true);
        break;
      case m5::Power_Class::pmic_axp2101:
        M5.Power.Axp2101.setALDO3(3300);
        break;
      default:
        Serial.printf("Unsupported PMIC for Core2 speaker: %d\n", M5.Power.getType());
        break;
    }

    display_.begin();

    pinMode(radio_config::sdCs, OUTPUT);
    digitalWrite(radio_config::sdCs, HIGH);
    SPI.begin(radio_config::sdSck, radio_config::sdMiso, radio_config::sdMosi);
    SPI.setFrequency(1000000);
    sdReady_ = SD.begin(radio_config::sdCs, SPI, 1000000);
    if (!sdReady_) {
      Serial.println("SD card initialization failed");
    }
    wifi_.loadCredentials(sdReady_);
    const bool hasStationPlaylist = station_catalog::loadFromSd(sdReady_, stations_);
    if (hasStationPlaylist) {
      if (savedStationIndex_ < stations_.size()) {
        currentStationIndex_ = savedStationIndex_;
      } else {
        currentStationIndex_ = 0;
        if (savedStationIndex_ != 0) {
          Serial.printf("Stored station index %lu is out of range; using station 0\n",
                        static_cast<unsigned long>(savedStationIndex_));
        }
      }
      activeStationName_ = stations_[currentStationIndex_].name;
      activeStationUrl_ = stations_[currentStationIndex_].url;
      activeStationLogoPath_ = stations_[currentStationIndex_].logoPath;
      portENTER_CRITICAL(&streamDataMux_);
      catalogStationActive_ = true;
      portEXIT_CRITICAL(&streamDataMux_);
    } else {
      activeStationName_ = radio_config::fallbackStationName;
      activeStationUrl_ = radio_config::fallbackStationUrl;
      activeStationLogoPath_ = radio_config::fallbackStationLogoPath;
      portENTER_CRITICAL(&streamDataMux_);
      catalogStationActive_ = false;
      portEXIT_CRITICAL(&streamDataMux_);
      Serial.printf("Using fallback station: %s\n", radio_config::fallbackStationName);
    }

    input_.configureTouch();
    display_.setWifiConfigMessage(wifi_.configDisplayMessage());
    display_.setStationName(activeStationName_);
    display_.setLogoPaths(activeStationLogoPath_, remoteStationLogoUrl_);

    if (!audio_.setPinout(radio_config::i2sBclk, radio_config::i2sLrc, radio_config::i2sDout)) {
      Serial.println("Audio I2S pin initialization failed");
    }
    audio_.setVolume(savedVolume_);

    wifi_.begin();
}

void RadioController::update() {
    vTaskDelay(1);
    M5.update();
    input_.updateGesture();

    const WifiEvent wifiEvent = wifi_.update(sdReady_);
    if (wifiEvent == WifiEvent::Connected) {
      connectActiveStation();
    } else if (wifiEvent == WifiEvent::Disconnected) {
      audio_.stopSong();
    }
    display_.setWifiConfigMessage(wifi_.configDisplayMessage());

    audio_.loop();
    input_.updateTouch();
    updateStreamMetadata();
    display_.update();
    settingsStore_.saveIfDue(audio_.getVolume(),
                             static_cast<size_t>(currentStationIndex_),
                             !stations_.empty());

    if (Serial.available()) {
      audio_.stopSong();
      String url = Serial.readString();
      url.trim();
      if (url.length() > 5) {
        audio_.connecttohost(url.c_str());
      }
      log_i("free heap=%i", ESP.getFreeHeap());
    }
}

void RadioController::audioInfoCallback(Audio::msg_t message) {
    if (instance_) {
      instance_->handleAudioInfo(message);
    }
}

void RadioController::inputActionCallback(void* context, InputAction action) {
    static_cast<RadioController*>(context)->handleInputAction(action);
}

void RadioController::handleAudioInfo(Audio::msg_t message) {
    Serial.printf("%s: %s\n", message.s ? message.s : "audio",
                  message.msg ? message.msg : "");
    if (!message.msg) {
      return;
    }

    portENTER_CRITICAL(&streamDataMux_);
    switch (message.e) {
      case Audio::evt_name:
        if (!catalogStationActive_) {
          snprintf(pendingStation_, sizeof(pendingStation_), "%s", message.msg);
          stationPending_ = true;
        }
        break;
      case Audio::evt_streamtitle:
      case Audio::evt_id3data:
        snprintf(pendingTitle_, sizeof(pendingTitle_), "%s", message.msg);
        titlePending_ = true;
        break;
      case Audio::evt_icylogo:
        snprintf(pendingLogo_, sizeof(pendingLogo_), "%s", message.msg);
        logoPending_ = true;
        break;
      default:
        break;
    }
    portEXIT_CRITICAL(&streamDataMux_);
}

void RadioController::handleInputAction(InputAction action) {
    switch (action) {
      case InputAction::NextStation:
        changeStation(1);
        break;
      case InputAction::PreviousStation:
        changeStation(-1);
        break;
      case InputAction::VolumeUp:
        adjustVolume(1);
        break;
      case InputAction::VolumeDown:
        adjustVolume(-1);
        break;
      case InputAction::ToggleMute:
        toggleMute();
        break;
    }
}

void RadioController::connectActiveStation() {
    audio_.stopSong();
    if (!audio_.connecttohost(activeStationUrl_.c_str())) {
      Serial.printf("Could not connect to station: %s\n", activeStationUrl_.c_str());
    }
}

void RadioController::selectStation(size_t index) {
    if (stations_.empty()) {
      return;
    }
    currentStationIndex_ = index % stations_.size();
    settingsStore_.markChanged();
    const Station& station = stations_[currentStationIndex_];
    activeStationName_ = station.name;
    activeStationUrl_ = station.url;
    activeStationLogoPath_ = station.logoPath;
    remoteStationLogoUrl_ = "";
    portENTER_CRITICAL(&streamDataMux_);
    catalogStationActive_ = true;
    portEXIT_CRITICAL(&streamDataMux_);
    portENTER_CRITICAL(&streamDataMux_);
    stationPending_ = false;
    titlePending_ = false;
    logoPending_ = false;
    portEXIT_CRITICAL(&streamDataMux_);

    display_.setStationName(activeStationName_);
    display_.setTitle("Waiting...");
    display_.setLogoPaths(activeStationLogoPath_, remoteStationLogoUrl_);
    display_.refreshVolumeFeedback();
    connectActiveStation();
}

void RadioController::changeStation(int direction) {
    if (stations_.empty()) {
      return;
    }
    const size_t count = stations_.size();
    const size_t nextIndex = direction > 0
        ? (currentStationIndex_ + 1) % count
        : (currentStationIndex_ + count - 1) % count;
    selectStation(nextIndex);
}

void RadioController::adjustVolume(int direction) {
    const int currentVolume = audio_.getVolume();
    const int newVolume = constrain(currentVolume + direction, 0, 21);
    if (newVolume == currentVolume) {
      return;
    }
    audio_.setVolume(static_cast<uint8_t>(newVolume));
    settingsStore_.markChanged();
    display_.showVolumeFeedback(audio_.getVolume(), audio_.getMute());
}

void RadioController::toggleMute() {
    audio_.setMute(!audio_.getMute());
    display_.showVolumeFeedback(audio_.getVolume(), audio_.getMute());
}

void RadioController::updateStreamMetadata() {
    char station[sizeof(pendingStation_)];
    char title[sizeof(pendingTitle_)];
    char logo[sizeof(pendingLogo_)];
    bool updateStation;
    bool updateTitle;
    bool updateLogo;
    portENTER_CRITICAL(&streamDataMux_);
    updateStation = stationPending_;
    updateTitle = titlePending_;
    updateLogo = logoPending_;
    if (updateStation) {
      memcpy(station, pendingStation_, sizeof(station));
      stationPending_ = false;
    }
    if (updateTitle) {
      memcpy(title, pendingTitle_, sizeof(title));
      titlePending_ = false;
    }
    if (updateLogo) {
      memcpy(logo, pendingLogo_, sizeof(logo));
      logoPending_ = false;
    }
    portEXIT_CRITICAL(&streamDataMux_);

    if (updateStation) {
      activeStationName_ = station;
      display_.setStationName(activeStationName_);
    }
    if (updateTitle) {
      display_.setTitle(title);
    }
    if (updateLogo) {
      remoteStationLogoUrl_ = logo;
      display_.setLogoPaths(activeStationLogoPath_, remoteStationLogoUrl_);
    }
}
