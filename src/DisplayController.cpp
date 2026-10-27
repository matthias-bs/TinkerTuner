///////////////////////////////////////////////////////////////////////////////////////////////////
// DisplayController.cpp
//
// Renders station, playback, Wi-Fi, logo, and volume information on the display.
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

#include "DisplayController.h"

#include <HTTPClient.h>
#include <M5Unified.h>
#include <SD.h>
#include <cstdio>

void DisplayController::begin() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setFont(&fonts::efontJA_24_b);
    M5.Lcd.setTextSize(1.4f);
    M5.Lcd.setTextDatum(textdatum_t::top_left);
    M5.Lcd.setTextWrap(false);
    setStationName("Waiting...");
    titleChangedAt_ = millis();
    drawTitle(true);
}

void DisplayController::setStationName(const String& name) {
    stationName_ = name;
    drawStationName(true);
}

void DisplayController::setTitle(const String& title) {
    displayedTitle_ = title;
    titleChangedAt_ = millis();
    drawTitle(true);
}

void DisplayController::setWifiConfigMessage(const String& message) {
    if (wifiConfigMessage_ == message) {
      return;
    }
    wifiConfigMessage_ = message;
    drawTitle(true);
}

void DisplayController::setLogoPaths(const String& localPath, const String& remoteUrl) {
    localLogoPath_ = localPath;
    remoteLogoUrl_ = remoteUrl;
    drawStationLogo();
    if (volumeFeedbackVisible_) {
      drawVolumeFeedback();
    }
}

void DisplayController::showVolumeFeedback(uint8_t volume, bool muted) {
    feedbackVolume_ = volume;
    feedbackMuted_ = muted;
    volumeFeedbackVisible_ = true;
    volumeFeedbackUntil_ = millis() + 1200;
    drawVolumeFeedback();
}

void DisplayController::refreshVolumeFeedback() {
    if (volumeFeedbackVisible_) {
      drawVolumeFeedback();
    }
}

void DisplayController::update() {
    drawStationName(false);
    drawTitle(false);
    if (volumeFeedbackVisible_ && !feedbackMuted_
        && static_cast<int32_t>(millis() - volumeFeedbackUntil_) >= 0) {
      volumeFeedbackVisible_ = false;
      drawStationLogo();
    }
}

void DisplayController::drawStationName(bool force) {
    const uint32_t now = millis();
    const int32_t screenWidth = M5.Lcd.width();
    const int32_t nameWidth = M5.Lcd.textWidth(stationName_);
    const bool scroll = nameWidth > screenWidth;
    if (!force && (!scroll || now - lastStationNameDrawAt_ < 40)) {
      return;
    }
    if (force) {
      stationNameChangedAt_ = now;
    }

    int32_t x = 0;
    if (scroll && now - stationNameChangedAt_ > 1000) {
      constexpr int32_t gap = 48;
      const uint32_t scrollLength = static_cast<uint32_t>(nameWidth + gap);
      const uint32_t offset = ((now - stationNameChangedAt_ - 1000) / 33) % scrollLength;
      x = -static_cast<int32_t>(offset);
    }

    M5.Lcd.fillRect(0, 0, screenWidth, 48, BLACK);
    M5.Lcd.setClipRect(0, 0, M5.Lcd.width(), 48);
    M5.Lcd.drawString(stationName_, x, 0);
    if (scroll) {
      constexpr int32_t gap = 48;
      const int32_t nextX = x + nameWidth + gap;
      if (nextX < screenWidth) {
        M5.Lcd.drawString(stationName_, nextX, 0);
      }
    }
    M5.Lcd.clearClipRect();
    lastStationNameDrawAt_ = now;
}

void DisplayController::drawTitle(bool force) {
    const uint32_t now = millis();
    const int32_t screenWidth = M5.Lcd.width();
    const int32_t titleY = 48;
    const int32_t titleHeight = M5.Lcd.height() / 2 - titleY;

    if (wifiConfigMessage_.length()) {
      if (!force && now - lastTitleDrawAt_ < 500) {
        return;
      }
      M5.Lcd.fillRect(0, titleY, screenWidth, titleHeight, BLACK);
      M5.Lcd.setClipRect(0, titleY, screenWidth, titleHeight);
      M5.Lcd.drawString(wifiConfigMessage_, 0, titleY);
      M5.Lcd.clearClipRect();
      lastTitleDrawAt_ = now;
      return;
    }

    const int32_t titleWidth = M5.Lcd.textWidth(displayedTitle_);
    const bool scroll = titleWidth > screenWidth;
    if (!force && (!scroll || now - lastTitleDrawAt_ < 40)) {
      return;
    }

    int32_t x = 0;
    if (scroll && now - titleChangedAt_ > 1000) {
      constexpr int32_t gap = 48;
      const uint32_t scrollLength = static_cast<uint32_t>(titleWidth + gap);
      const uint32_t offset = ((now - titleChangedAt_ - 1000) / 33) % scrollLength;
      x = -static_cast<int32_t>(offset);
    }

    M5.Lcd.fillRect(0, titleY, screenWidth, titleHeight, BLACK);
    M5.Lcd.setClipRect(0, titleY, screenWidth, titleHeight);
    M5.Lcd.drawString(displayedTitle_, x, titleY);
    if (scroll) {
      constexpr int32_t gap = 48;
      const int32_t nextX = x + titleWidth + gap;
      if (nextX < screenWidth) {
        M5.Lcd.drawString(displayedTitle_, nextX, titleY);
      }
    }
    M5.Lcd.clearClipRect();
    lastTitleDrawAt_ = now;
}

bool DisplayController::drawLocalStationLogo(const char* path) {
    const int logoY = M5.Lcd.height() / 2;
    const int logoHeight = M5.Lcd.height() - logoY;
    if (!SD.exists(path)) {
      Serial.printf("Station logo file not found: %s\n", path);
      return false;
    }
    File logoFile = SD.open(path, FILE_READ);
    if (!logoFile) {
      Serial.printf("Could not open station logo file: %s\n", path);
      return false;
    }
    uint8_t signature[8] = {};
    size_t signatureSize = logoFile.read(signature, sizeof(signature));
    logoFile.seek(0);

    bool displayed = false;
    if (signatureSize >= 3 && signature[0] == 0xFF && signature[1] == 0xD8 && signature[2] == 0xFF) {
      displayed = M5.Lcd.drawJpg(&logoFile, 0, logoY, M5.Lcd.width(), logoHeight,
                                 0, 0, 1.0f, 0.0f, datum_t::top_center);
    } else if (signatureSize == sizeof(signature)
               && signature[0] == 0x89 && signature[1] == 'P'
               && signature[2] == 'N' && signature[3] == 'G'
               && signature[4] == 0x0D && signature[5] == 0x0A
               && signature[6] == 0x1A && signature[7] == 0x0A) {
      displayed = M5.Lcd.drawPng(&logoFile, 0, logoY, M5.Lcd.width(), logoHeight,
                                 0, 0, 1.0f, 0.0f, datum_t::top_center);
    } else if (signatureSize >= 2 && signature[0] == 'B' && signature[1] == 'M') {
      displayed = M5.Lcd.drawBmp(&logoFile, 0, logoY, M5.Lcd.width(), logoHeight,
                                 0, 0, 1.0f, 0.0f, datum_t::top_center);
    }

    if (!displayed) {
      Serial.printf("Unsupported or invalid station logo image: %s\n", path);
    }
    logoFile.close();
    return displayed;
}

void DisplayController::drawStationLogo() {
    const int logoY = M5.Lcd.height() / 2;
    const int logoHeight = M5.Lcd.height() - logoY;
    M5.Lcd.fillRect(0, logoY, M5.Lcd.width(), logoHeight, BLACK);

    if (localLogoPath_.length()) {
      drawLocalStationLogo(localLogoPath_.c_str());
    } else if (remoteLogoUrl_.length()) {
      if (!M5.Lcd.drawJpgUrl(remoteLogoUrl_.c_str(), 0, logoY, M5.Lcd.width(), logoHeight,
                             0, 0, 1.0f, 0.0f, datum_t::top_center)) {
        Serial.printf("Could not display station logo URL: %s\n", remoteLogoUrl_.c_str());
      }
    }
}

void DisplayController::drawVolumeFeedback() {
    const int logoY = M5.Lcd.height() / 2;
    M5.Lcd.fillRect(0, logoY, M5.Lcd.width(), M5.Lcd.fontHeight() + 8, BLACK);
    char feedback[24];
    if (feedbackMuted_) {
      snprintf(feedback, sizeof(feedback), "Muted");
    } else {
      snprintf(feedback, sizeof(feedback), "Volume %u", feedbackVolume_);
    }
    M5.Lcd.drawString(feedback, 0, logoY);
}
