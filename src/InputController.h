///////////////////////////////////////////////////////////////////////////////////////////////////
// InputController.h
//
// Declares the touch and gesture input interface for radio control actions.
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
#include <M5UnitUnified.h>
#include <M5UnitUnifiedGESTURE.h>

class InputController {
public:
    using ActionHandler = void (*)(void*, InputAction);

    void begin(ActionHandler handler, void* context);
    void configureTouch();
    void updateGesture();
    void updateTouch();

private:
    void executeGesture(m5::unit::paj7620u2::Gesture gesture);
    void emit(InputAction action);

    m5::unit::UnitUnified gestureUnits_;
    m5::unit::UnitGesture gestureSensor_;
    ActionHandler handler_ = nullptr;
    void* context_ = nullptr;
    bool gestureSensorAvailable_ = false;
    bool touchActive_ = false;
    int16_t touchStartX_ = 0;
    int16_t touchStartY_ = 0;
};
