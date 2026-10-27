///////////////////////////////////////////////////////////////////////////////////////////////////
// DisplayController.h
//
// Declares the display interface for station, playback, Wi-Fi, logo, and volume information.
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

class DisplayController {
public:
    void begin();
    void setStationName(const String& name);
    void setTitle(const String& title);
    void setWifiConfigMessage(const String& message);
    void setLogoPaths(const String& localPath, const String& remoteUrl);
    void showVolumeFeedback(uint8_t volume, bool muted);
    void showGestureFeedback(bool enabled);
    void refreshVolumeFeedback();
    void update();

private:
    void drawStationName(bool force);
    void drawTitle(bool force);
    bool drawLocalStationLogo(const char* path);
    void drawStationLogo();
    void drawFeedback();

    enum class FeedbackType {
        Volume,
        Gesture
    };

    String stationName_;
    String displayedTitle_ = "Waiting...";
    String wifiConfigMessage_;
    String localLogoPath_;
    String remoteLogoUrl_;
    uint32_t stationNameChangedAt_ = 0;
    uint32_t lastStationNameDrawAt_ = 0;
    uint32_t titleChangedAt_ = 0;
    uint32_t lastTitleDrawAt_ = 0;
    uint32_t volumeFeedbackUntil_ = 0;
    uint8_t feedbackVolume_ = 0;
    bool feedbackMuted_ = false;
    bool feedbackGestureEnabled_ = true;
    FeedbackType feedbackType_ = FeedbackType::Volume;
    bool volumeFeedbackVisible_ = false;
};
