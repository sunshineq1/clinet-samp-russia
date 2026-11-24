//
// Created by weikton on 02.02.25.
//

#pragma once
#include <stdint.h>

enum eCarWheelStatus : uint8_t  {
    WHEEL_STATUS_OK = 0,
    WHEEL_STATUS_BURST,
    WHEEL_STATUS_MISSING,
    WHEEL_STATUS_3,
    WHEEL_STATUS_4
};