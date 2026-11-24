#pragma once
#include "game/Enums/eCustomHandlingType.h"

class CustomHandling {
public:
    static void Load(eCustomHandlingType type);
    static void ChangeName(eCustomHandlingType type);
};
