///////////////////////////////////////////////////////////////////////////////////////////////////
// InputController.cpp
//
// Converts touch and gesture sensor input into radio control actions.
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

#include "InputController.h"

#include <M5Unified.h>
#include <wiring/m5_unit_unified_wiring.hpp>
#include <cstdlib>

void InputController::begin(ActionHandler handler, void* context) {
    handler_ = handler;
    context_ = context;
    auto gestureConfig = gestureSensor_.config();
    gestureConfig.vflip = true;
    gestureSensor_.config(gestureConfig);
    if (!m5::unit::wiring::addI2C(gestureUnits_, gestureSensor_) || !gestureUnits_.begin()) {
      Serial.println("M5Unit-GESTURE initialization failed on Port A; touch controls remain available");
      return;
    }
    gestureSensorAvailable_ = true;
    Serial.println("M5Unit-GESTURE initialized on Port A");
}

void InputController::configureTouch() {
    M5.Touch.setFlickThresh(12);
}

void InputController::updateGesture() {
    if (!gestureSensorAvailable_ || !gestureEnabled_) {
      return;
    }
    gestureUnits_.update();
    if (gestureSensor_.updated()) {
      const auto gesture = gestureSensor_.gesture();
      if (gesture != m5::unit::paj7620u2::Gesture::None) {
        executeGesture(gesture);
      }
    }
}

void InputController::updateTouch() {
    if (gestureSensorAvailable_ && M5.BtnC.wasPressed()) {
      buttonCActive_ = true;
      emit(InputAction::ToggleGesture);
      return;
    }
    if (buttonCActive_) {
      if (!M5.BtnC.isPressed() && M5.Touch.getCount() == 0) {
        buttonCActive_ = false;
      }
      return;
    }
    if (M5.Touch.getCount() == 0) {
      return;
    }

    const auto& touch = M5.Touch.getDetail(0);
    if (touch.wasPressed()) {
      touchStartX_ = touch.x;
      touchStartY_ = touch.y;
      touchActive_ = true;
    }
    if (!touch.wasReleased() || !touchActive_) {
      return;
    }
    touchActive_ = false;

    if (touch.wasClicked()) {
      emit(InputAction::ToggleMute);
      return;
    }

    const int deltaX = touch.x - touchStartX_;
    const int deltaY = touch.y - touchStartY_;
    const int absX = std::abs(deltaX);
    const int absY = std::abs(deltaY);
    constexpr int minimumSwipeDistance = 48;
    if (absX >= minimumSwipeDistance && absX >= absY * 1.4f) {
      emit(deltaX < 0 ? InputAction::NextStation : InputAction::PreviousStation);
    } else if (absY >= minimumSwipeDistance && absY >= absX * 1.4f) {
      emit(deltaY < 0 ? InputAction::VolumeUp : InputAction::VolumeDown);
    }
}

bool InputController::gestureSensorAvailable() const {
    return gestureSensorAvailable_;
}

bool InputController::gestureEnabled() const {
    return gestureEnabled_;
}

void InputController::setGestureEnabled(bool enabled) {
    gestureEnabled_ = enabled;
}

void InputController::executeGesture(m5::unit::paj7620u2::Gesture gesture) {
    if (gestureSensor_.config().vflip) {
      if (gesture == m5::unit::paj7620u2::Gesture::Up) {
        gesture = m5::unit::paj7620u2::Gesture::Down;
      } else if (gesture == m5::unit::paj7620u2::Gesture::Down) {
        gesture = m5::unit::paj7620u2::Gesture::Up;
      }
    }

    switch (gesture) {
      case m5::unit::paj7620u2::Gesture::Left:
        emit(InputAction::NextStation);
        break;
      case m5::unit::paj7620u2::Gesture::Right:
        emit(InputAction::PreviousStation);
        break;
      case m5::unit::paj7620u2::Gesture::Up:
        emit(InputAction::VolumeUp);
        break;
      case m5::unit::paj7620u2::Gesture::Down:
        emit(InputAction::VolumeDown);
        break;
      case m5::unit::paj7620u2::Gesture::Forward:
        emit(InputAction::ToggleMute);
        break;
      default:
        break;
    }
}

void InputController::emit(InputAction action) {
    if (handler_) {
      handler_(context_, action);
    }
}
