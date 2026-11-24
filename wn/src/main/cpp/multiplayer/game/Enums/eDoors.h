//
// Created by weikton on 02.02.25.
//

#pragma once
#include <cstdint>

enum eDoors : uint8_t  {
    DOOR_BONNET = 0,
    DOOR_BOOT,
    DOOR_LEFT_FRONT,
    DOOR_RIGHT_FRONT,
    DOOR_LEFT_REAR,
    DOOR_RIGHT_REAR,

    MAX_DOORS
};