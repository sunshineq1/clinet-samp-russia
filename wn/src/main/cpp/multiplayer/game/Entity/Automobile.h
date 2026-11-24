//
// Created by weikton on 16.09.2023.
//

#pragma once

#include "CVehicleGTA.h"
#include "CDamageManager.h"
#include "game/Enums/eCarNodes.h"
#include "CDoor.h"
#include "game/Enums/tWheelState.h"
#include "CBouncingPanel.h"

enum class eSkidmarkType : uint32;
class CAutomobile : public CVehicleGTA {
public:
    CDamageManager m_nDamageManager;
    CDoor Door[6];
    RwFrame *m_aCarNodes[eCarNodes::CAR_NUM_NODES];
    CBouncingPanel BouncingPanels[3];
    CDoor ChassisDoor;
    CColPoint m_aWheelColPoints[4];
    float m_aWheelRatios[4];
    float m_aRatioHistory[4];
    float m_aWheelCounts[4];
    float fBrakeCount;
    float m_fEngineRevs;
    float m_fEngineForce;
    eSkidmarkType aWheelSkidmarkType[4];
    bool bWheelBloody[4];
    bool bMoreSkidMarks[4];
    float m_aWheelPitchAngles[4];
    float m_aWheelSuspensionHeights[4];
    float m_aWheelAngularVelocity[4];
    float m_aWheelLongitudinalSlip[4];
    struct {
        bool bTaxiLight : 1; // AKA `bTaxiLightOn`
        bool bShouldNotChangeColour : 1; // AKA `ucNPCFlagPad2`
        bool bWaterTight : 1; // AKA `bIgnoreWater`
        bool bDoesNotGetDamagedUpsideDown : 1; // AKA `bDontDamageOnRoof`
        bool bCanBeVisiblyDamaged : 1; // AKA `bTakePanelDamage`
        bool bTankExplodesCars : 1; // AKA `ucTaxiUnkn6`
        bool bIsBoggedDownInSand : 1; // AKA `bLostTraction`
        bool bIsMonsterTruck : 1; // AKA `bSoftSuspension`
    } m_nAutomobileFlags;
    uint16_t nBrakesOn;
    uint16_t m_nSuspensionHydraulics;
    uint16_t m_nOldSuspensionHydraulics;
    uint32_t m_nBusDoorTimer;
    uint32_t m_nBusDoorStart;
    float m_fSuspensionLength[4];
    float m_fLineLength[4];
    float m_fHeightAboveRoad;
    float m_fRearHeightAboveRoad;
    float m_fExtraTractionMult;
    float m_fTyreTemp;
    float ZRot;
    float ZRotSpeed;
    float fPrevSpeed;
    CVector m_vecOldMoveSpeed;
    CVector m_vecOldTurnSpeed;
    float DoorRotation[6];
    float m_BlowUpTimer;
    CPhysical *m_aGroundPhysicalPtrs[4];
    CVector m_aGroundOffsets[4];
    CEntityGTA *pEntityThatSetUsOnFire;
    float m_fTransformPosition;
    int16 m_nTransformState;
    float m_fBasePositions[4];
    float LeftDoorOpenForDriveBys;
    float RightDoorOpenForDriveBys;
    float GunOrientation;
    float GunElevation;
    float HeliRequestedOrientation;
    float PropRotate;
    float CumulativeDamage;
    uint8_t nNoOfContactWheels;
    uint8_t m_nDriveWheelsOnGround;
    uint8_t m_nDriveWheelsOnGroundLastFrame;
    float m_fGasPedalAudioRevs;
    tWheelState m_aWheelState[4];
    uintptr_t *m_pFxSysNitro1;
    uintptr_t *m_pFxSysNitro2;
    uint8_t m_HarvesterTimer;
    uint8_t m_statusWhenEngineBlown;
    float m_heliDustRatio;
public:
    static constexpr auto Type = VEHICLE_TYPE_AUTOMOBILE;
};

VALIDATE_SIZE(CAutomobile, (VER_x32 ? 0x99C : 0xBC8));
