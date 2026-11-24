//
// Created by weikton on 02.02.25.
//

#pragma once
#include <cstdint>
enum eLights : uint8_t {
    LIGHT_FRONT_LEFT  = 0,
    LIGHT_FRONT_RIGHT = 1,
    LIGHT_REAR_RIGHT  = 2,
    LIGHT_REAR_LEFT   = 3,

    MAX_LIGHTS
};