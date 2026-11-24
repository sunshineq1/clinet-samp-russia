//
// Created by weikton on 02.02.25.
//

#pragma once
#include "../main.h"
#include "CVehicleGTA.h"
#include "Automobile.h"

class CPlane:public CAutomobile{
public:
    float m_fYawControl;
    float m_fPitchControl;
    float m_fRollControl;
    float m_fThrottleControl;
    float m_fScriptThrottleControl;
    float m_fPreviousRoll;
    uint32 m_nStallCounter;
    float m_TakeOffDirection;
    float m_LowestFlightHeight;
    float m_DesiredHeight;
    float m_MinHeightAboveTerrain;
    float m_FlightDirection;
    float m_FlightDirectionAvoidingTerrain;
    float m_OldTilt;
    uint32_t m_OnGroundTimer;
    float m_fEngineSpeed;
    float m_fPropellerAngle;
    float m_fLGearAngle;
    uint32 m_nDamageControlWaveCounter;
    uintptr_t **m_GunflashFxPtrs;
    uint8 m_FiringRateMultiplier;
    uint32 m_FireMissilePressedTime;
    CEntityGTA *m_pLastMissileTarget;
    uint32_t m_LastHSMissileLOSTime : 31;
    uint32_t m_bLastHSMissileLOS : 1;
    uintptr_t *m_fxSysNozzle[4];
    uintptr_t *m_fxSysFire;
    int32 m_fireTime;
    bool8 m_fxActive;

    static constexpr auto Type = VEHICLE_TYPE_PLANE;
};

VALIDATE_SIZE(CPlane, (VER_x32 ? 0xA18:0xC68));