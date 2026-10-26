///////////////////////////////////////////////////////////////////////////////////////////////////
// RadioConfig.h
//
// Defines hardware pin assignments and shared radio configuration constants.
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

#ifndef USE_MAX98357A_PORTS_BC
#define USE_MAX98357A_PORTS_BC 0
#endif

namespace radio_config {

static constexpr int sdCs = 4;
static constexpr int sdMosi = 23;
static constexpr int sdMiso = 38;
static constexpr int sdSck = 18;

#if USE_MAX98357A_PORTS_BC
static constexpr int i2sDout = 14;
static constexpr int i2sBclk = 26;
static constexpr int i2sLrc = 13;
#else
static constexpr int i2sDout = 2;
static constexpr int i2sBclk = 12;
static constexpr int i2sLrc = 0;
#endif

static constexpr char fallbackStationName[] = "NDR 2";
static constexpr char fallbackStationUrl[] = "http://icecast.ndr.de/ndr/ndr2/hamburg/mp3/128/stream.mp3";
static constexpr char fallbackStationLogoPath[] = "/ndr2.png";
static constexpr char stationPlaylistPath[] = "/stations.m3u";
static constexpr char wifiCredentialsPath[] = "/wifi-credentials.txt";
static constexpr char preferencesNamespace[] = "m5radio";

static constexpr uint8_t defaultVolume = 3;
static constexpr size_t maxStations = 20;
static constexpr size_t maxWifiCredentials = 3;
static constexpr uint32_t preferencesSaveDelayMs = 1000;
static constexpr uint32_t wifiAttemptTimeoutMs = 15000;
static constexpr uint32_t wifiRetryDelayMs = 1000;
static constexpr uint32_t wifiCycleRetryDelayMs = 10000;
static constexpr uint32_t wifiFileReloadDelayMs = 30000;

}
