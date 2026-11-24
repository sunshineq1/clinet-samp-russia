#pragma once

#include "common.h"

struct CPedGTA;

class CVehicleHands
{
public:
    static inline bool  ms_bEnabled = true;
    static inline float ms_fReachLimit = 0.995f;

    // Запасной хват по ободу руля, когда у модели нет дамми под вторую руку
    static inline float ms_fDefaultWheelRadius = 0.17f;
    static inline float ms_fGripRadiusScale = 0.85f;

    static void InjectHooks();
    static void Apply(CPedGTA* pPed);
};
