///////////////////////////////////////////////////////////////////////////////////////////////////
// RadioController.h
//
// Declares the controller that coordinates the radio application components.
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

#include "DisplayController.h"
#include "InputController.h"
#include "RadioTypes.h"
#include "SettingsStore.h"
#include "WifiManager.h"

#include "Audio.h"
#include <vector>

class RadioController {
public:
    void begin();
    void update();

private:
    static void audioInfoCallback(Audio::msg_t message);
    static void inputActionCallback(void* context, InputAction action);

    void handleAudioInfo(Audio::msg_t message);
    void handleInputAction(InputAction action);
    void connectActiveStation();
    void selectStation(size_t index);
    void changeStation(int direction);
    void adjustVolume(int direction);
    void toggleMute();
    void toggleGesture();
    void updateStreamMetadata();

    static RadioController* instance_;

    Audio audio_;
    DisplayController display_;
    InputController input_;
    RadioWifiManager wifi_;
    SettingsStore settingsStore_;
    std::vector<Station> stations_;
    size_t currentStationIndex_ = 0;
    uint32_t savedStationIndex_ = 0;
    uint8_t savedVolume_ = 3;
    bool gestureEnabled_ = true;
    String activeStationName_ = "NDR 2";
    String activeStationUrl_ = "http://icecast.ndr.de/ndr/ndr2/hamburg/mp3/128/stream.mp3";
    String activeStationLogoPath_ = "/ndr2.png";
    String remoteStationLogoUrl_;
    portMUX_TYPE streamDataMux_ = portMUX_INITIALIZER_UNLOCKED;
    char pendingStation_[96] = {};
    char pendingTitle_[160] = {};
    char pendingLogo_[256] = {};
    bool stationPending_ = false;
    bool titlePending_ = false;
    bool logoPending_ = false;
    bool catalogStationActive_ = false;
    bool sdReady_ = false;
};
