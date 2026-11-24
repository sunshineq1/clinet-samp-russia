//
// Created by weikton on 02.02.25.
//

#pragma once
#include "../main.h"
#include "CVehicleGTA.h"
#include "game/Enums/eTrainNodes.h"

class CTrain : public CVehicleGTA {
public:
    int16 m_nNodeIndex;
    uint8_t pading0[2];
    float m_fTrainSpeed; // 1.0 - train derails
    float m_fCurrentRailDistance;
    float m_fLength;
    float m_fTrainGas;   // gas pedal pressed: 255.0, moving forward: 0.0f, moving back: -255.0
    float m_fTrainBrake; // 255.0 - braking
    union {
        struct {
            uint16 b01: 1; // initialised with 1
            uint16 bStoppedAtStation: 1;
            uint16 bPassengersCanEnterAndLeave: 1;
            uint16 bIsFrontCarriage: 1;
            uint16 bIsLastCarriage: 1;
            uint16 bMissionTrain: 1;
            uint16 bClockwiseDirection: 1;
            uint16 bStopsAtStations: 1;

            uint16 bNotOnARailRoad: 1;
            uint16 bForceSlowDown: 1;
            uint16 bIsStreakModel: 1;
        } trainFlags;
        uint16 m_nTrainFlags;
    };
    uint32 m_nTimeWhenStoppedAtStation;
    int8 m_nTrackId;
    uint32 m_nTimeWhenCreated;
    int16 field_5C8;                    // initialized with 0, not referenced
    uint8 m_nPassengersGenerationState; // see eTrainPassengersGenerationState
    uint8 m_nNumPassengersToLeave
    : 4;  // 0 to 4
    uint8 m_nNumPassengersToEnter
    : 4;  // 0 to 4
    CPedGTA *m_pTemporaryPassenger;        // we tell peds to enter train and then delete them
    CTrain *m_pPrevCarriage;
    CTrain *m_pNextCarriage;
    CDoor m_aDoors[6];
    RwFrame *m_aTrainNodes[TRAIN_NUM_NODES];

    static uint32 &GenTrain_Track;
    static uint32 &GenTrain_TrainConfig;
    static uint8 &GenTrain_Direction;
    static uint32 &GenTrain_GenerationNode;
    static uint32 &GenTrain_Status;
    static bool &bDisableRandomTrains;
    static CVector aStationCoors[6];

    static constexpr auto Type = VEHICLE_TYPE_TRAIN;
};

VALIDATE_SIZE(CTrain, (VER_x32 ? 0x6C0:0x8B8));