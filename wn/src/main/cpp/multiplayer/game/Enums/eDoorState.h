//
// Created by weikton on 02.02.25.
//

#pragma once
#include <cstdint>

enum eDoorState : uint8_t {
    DOOR_NOTHING,
    DOOR_HIT_MAX_END,
    DOOR_HIT_MIN_END,
    DOOR_POP_OPEN,
    DOOR_SLAM_SHUT
};