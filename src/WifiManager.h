///////////////////////////////////////////////////////////////////////////////////////////////////
// WifiManager.h
//
// Declares Wi-Fi credential loading and network connection management.
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

#include "RadioTypes.h"
#include <vector>

enum class WifiEvent {
    None,
    Connected,
    Disconnected
};

class RadioWifiManager {
public:
    bool loadCredentials(bool sdReady);
    void begin();
    WifiEvent update(bool& sdReady);
    const String& configDisplayMessage() const;

private:
    bool loadCredentialsFile(bool sdReady);

    std::vector<WifiCredential> credentials_;
    size_t attemptIndex_ = 0;
    bool attemptActive_ = false;
    bool connected_ = false;
    uint32_t attemptStartedAt_ = 0;
    uint32_t nextAttemptAt_ = 0;
    uint32_t lastFileLoadAt_ = 0;
    String configDisplayMessage_;
};
