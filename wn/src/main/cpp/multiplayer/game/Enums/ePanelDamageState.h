//
// Created by weikton on 02.02.25.
//

#pragma once
#include <cstdint>

enum ePanelDamageState : uint8_t {
    DAMSTATE_OK             = 0, // Closed
    DAMSTATE_OPENED         = 1, // Open
    DAMSTATE_DAMAGED        = 2, // Closed
    DAMSTATE_OPENED_DAMAGED = 3, // Open
    DAMSTATE_NOTPRESENT     = 4  // Missing
};

using eDoorStatus = ePanelDamageState;