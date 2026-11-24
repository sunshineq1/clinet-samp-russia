//
// Created by weikton on 02.02.25.
//

#pragma once
#include <cstdint>

enum ePanels : uint8_t {
    FRONT_LEFT_PANEL = 0,
    FRONT_RIGHT_PANEL,
    REAR_LEFT_PANEL,
    REAR_RIGHT_PANEL,
    WINDSCREEN_PANEL,
    FRONT_BUMPER,
    REAR_BUMPER,

    MAX_PANELS
};