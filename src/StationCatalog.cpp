///////////////////////////////////////////////////////////////////////////////////////////////////
// StationCatalog.cpp
//
// Loads station and logo metadata from the SD-card playlist.
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

#include "StationCatalog.h"

#include "RadioConfig.h"
#include <SD.h>

namespace {

String normalizeLogoPath(const String& value) {
    String path = value;
    path.trim();
    if (!path.length()) {
      return "";
    }
    if (path.startsWith("http://") || path.startsWith("https://") || path.indexOf(':') >= 0) {
      Serial.printf("Ignoring non-local station logo path: %s\n", path.c_str());
      return "";
    }
    if (!path.startsWith("/")) {
      path = "/" + path;
    }
    int segmentStart = 1;
    while (segmentStart <= path.length()) {
      int segmentEnd = path.indexOf('/', segmentStart);
      if (segmentEnd < 0) {
        segmentEnd = path.length();
      }
      if (path.substring(segmentStart, segmentEnd) == "..") {
        Serial.printf("Ignoring invalid station logo path: %s\n", path.c_str());
        return "";
      }
      segmentStart = segmentEnd + 1;
    }
    return path;
}

String parseLogoPath(const String& extinfMetadata) {
    const int attributeStart = extinfMetadata.indexOf("tvg-logo=");
    if (attributeStart < 0) {
      return "";
    }
    int valueStart = attributeStart + 9;
    if (valueStart >= extinfMetadata.length()) {
      return "";
    }
    const char quote = extinfMetadata.charAt(valueStart);
    if (quote != '"' && quote != '\'') {
      Serial.println("Ignoring tvg-logo without quoted value");
      return "";
    }
    ++valueStart;
    const int valueEnd = extinfMetadata.indexOf(quote, valueStart);
    if (valueEnd < 0) {
      Serial.println("Ignoring unterminated tvg-logo value");
      return "";
    }
    return normalizeLogoPath(extinfMetadata.substring(valueStart, valueEnd));
}

}

bool station_catalog::loadFromSd(bool sdReady, std::vector<Station>& stations) {
    stations.clear();
    if (!sdReady) {
      Serial.println("Cannot load station playlist: SD card is unavailable");
      return false;
    }
    File playlist = SD.open(radio_config::stationPlaylistPath, FILE_READ);
    if (!playlist) {
      Serial.printf("Station playlist not found: %s\n", radio_config::stationPlaylistPath);
      return false;
    }

    String pendingName;
    String pendingLogoPath;
    bool hasPendingEntry = false;
    while (playlist.available()) {
      String line = playlist.readStringUntil('\n');
      line.trim();
      if (!line.length()) {
        continue;
      }
      if (line.startsWith("#EXTINF:")) {
        const int comma = line.indexOf(',');
        if (comma < 0) {
          Serial.println("Ignoring malformed EXTINF entry without station name");
          hasPendingEntry = false;
          continue;
        }
        String metadata = line.substring(8, comma);
        pendingName = line.substring(comma + 1);
        pendingName.trim();
        pendingLogoPath = parseLogoPath(metadata);
        hasPendingEntry = pendingName.length() > 0;
        if (!hasPendingEntry) {
          Serial.println("Ignoring EXTINF entry with an empty station name");
        }
        continue;
      }
      if (line.startsWith("#")) {
        continue;
      }
      if (!hasPendingEntry) {
        Serial.printf("Ignoring playlist URL without a valid EXTINF: %s\n", line.c_str());
        continue;
      }
      if (!line.startsWith("http://") && !line.startsWith("https://")) {
        Serial.printf("Ignoring unsupported station URL: %s\n", line.c_str());
        hasPendingEntry = false;
        continue;
      }
      if (stations.size() >= radio_config::maxStations) {
        Serial.printf("Station playlist limit reached (%u); remaining entries ignored\n",
                      static_cast<unsigned>(radio_config::maxStations));
        break;
      }
      Station station;
      station.name = pendingName;
      station.url = line;
      station.logoPath = pendingLogoPath;
      stations.push_back(station);
      hasPendingEntry = false;
    }
    playlist.close();
    if (hasPendingEntry) {
      Serial.printf("Ignoring incomplete playlist entry: %s\n", pendingName.c_str());
    }
    if (stations.empty()) {
      Serial.println("Station playlist contains no valid stations");
      return false;
    }
    Serial.printf("Loaded %u station(s) from %s\n",
                  static_cast<unsigned>(stations.size()), radio_config::stationPlaylistPath);
    return true;
}
