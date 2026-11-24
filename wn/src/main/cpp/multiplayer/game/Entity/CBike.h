//
// Created by weikton on 02.02.25.
//

#pragma once
#include "../main.h"
#include "CPhysical.h"
#include "game/Enums/tWheelState.h"
#include "game/Enums/eBikeNodes.h"
#include "game/Enums/eVehicleType.h"
#include "CVehicleGTA.h"

enum class eSkidmarkType : uint32;

class CBike : public CVehicleGTA {
    static constexpr auto NUM_SUSP_LINES = 4;
public:
    RwFrame *m_aBikeNodes[10];
    bool m_bLeanMatrix;
    CMatrix m_LeanMatrix;
    union {
        struct {
            uint8 bShouldNotChangeColour : 1;
            uint8 bPanelsAreThougher : 1;
            uint8 bWaterTight : 1; // Ignores water
            uint8 bGettingPickedUp : 1;
            uint8 bOnSideStand : 1; // Is Standing
            uint8 bPlayerBoost : 1;
            uint8 bEngineOnFire : 1;
            uint8 bWheelieForCamera : 1;
        } bikeFlags;
        uint8 nBikeFlags;
    };
    CVector m_vecAveGroundNormal;
    CVector m_vecGroundRight;
    CVector m_vecOldSpeedForPlayback;
    uintptr_t *pBikeHandling;
    CRideAnimData RideAnimData;
    uint8 m_nWheelStatus[2];
    CColPoint m_aWheelColPoints[4];
    float m_aWheelRatios[4];
    float m_aRatioHistory[4];
    float m_aWheelCounts[4];
    float fBrakeCount;
    eSkidmarkType aWheelSkidmarkType[2];
    bool bWheelBloody[2];
    bool bMoreSkidMarks[2];
    float m_aWheelPitchAngles[2];
    float m_aWheelAngularVelocity[2];
    float m_aWheelSuspensionHeights[2];
    float m_aWheelOrigHeights[2];
    float m_fSuspensionLength[4];
    float m_fLineLength[4];
    float m_fHeightAboveRoad;
    float m_fExtraTractionMult;
    float m_fSwingArmLength;
    float m_fForkYOffset;
    float m_fForkZOffset;
    float m_fSteerAngleTan;
    uint16_t nBrakesOn;
    float m_fTyreTemp;
    float m_fBrakingSlide;
    uint8 m_nFixLeftHand;
    uint8 m_nFixRightHand;
    uint8 m_nTestPedCollision;
    float fPrevSpeed;
    float m_BlowUpTimer;
    CPhysical *m_aGroundPhysicalPtrs[4];
    CVector m_aGroundOffsets[4];
    CEntityGTA *pEntityThatSetUsOnFire;
    uint8_t nNoOfContactWheels;
    uint8_t m_nDriveWheelsOnGround;
    uint8_t m_nDriveWheelsOnGroundLastFrame;
    float m_fGasPedalAudioRevs;
    tWheelState m_aWheelState[2];

public:
    static constexpr auto Type = VEHICLE_TYPE_BIKE;
};

VALIDATE_SIZE(CBike, (VER_x32 ? 0x828 : 0xA18));