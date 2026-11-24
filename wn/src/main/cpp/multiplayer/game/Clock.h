/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

#include "common.h"

class CClock {
public:
    static inline uint16    ms_nGameClockSeconds;
    static inline uint8     ms_nGameClockMinutes;
    static inline uint8     ms_nGameClockHours;
    static inline uint8     ms_nGameClockDays;

    static inline bool gbFreezeTime;

public:
    static void InjectHooks();

    static void Initialise(uint32 millisecondsPerGameMinute);
    static void Update();

    static uint16 GetGameClockMinutesUntil(uint8 hours, uint8 minutes);
    static bool GetIsTimeInRange(uint8 from, uint8 to);
    static void NormaliseGameClock();
    static void OffsetClockByADay(uint32 timeDirection);
    static void SetGameClock(uint8 hours, uint8 minutes, uint8 day);
    static void StoreClock();
    static void RestoreClock();

    // inlined and may have not original names
    static uint16 GetGameClockSeconds()   { return ms_nGameClockSeconds; }
    static uint8  GetGameClockMinutes()   { return ms_nGameClockMinutes; }
    static uint8  GetGameClockHours()     { return ms_nGameClockHours; }
    static uint8  GetGameClockDays()      { return ms_nGameClockDays; }

    static float GetMinutesToday() { return float(ms_nGameClockMinutes) + 60.0f * (float)ms_nGameClockHours + (float)ms_nGameClockSeconds / 60.0f; }
};
