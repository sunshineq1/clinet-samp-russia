//
// Created by weikton on 02.02.25.
//

#pragma once
#include <cstdint>

enum tWheelState : int32_t {
    WHEEL_STATE_NORMAL,   // standing still or rolling normally
    WHEEL_STATE_SPINNING, // rotating but not moving
    WHEEL_STATE_SKIDDING,
    WHEEL_STATE_FIXED,    // not rotating
};