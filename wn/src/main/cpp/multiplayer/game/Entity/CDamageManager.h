//
// Created by weikton on 02.02.25.
//
#pragma once
#include "../main.h"
#include "game/Enums/eCarWheelStatus.h"
#include "game/Enums/eCarWheel.h"
#include "game/Enums/ePanelDamageState.h"
#include "game/Enums/eLightState.h"
#include "game/Enums/ePanels.h"
#include "game/Enums/eDoors.h"
#include "game/Enums/eLights.h"

class CDamageManager // sizeof=0x18
{
public:
    float fWheelDamageEffect;
    uint8 m_nEngineStatus;
    eCarWheelStatus m_anWheelsStatus[eCarWheel::MAX_CARWHEELS];
    eDoorStatus m_aDoorsStatus[eDoors::MAX_DOORS];
    uint8 pad;
    union {
        /* Great job R*, really could've just used an array here... Ends up to be the same size... */

        uint32_t m_nLightsStatus;
        struct {
            uint32 leftFront : 2;
            uint32 rightFront : 2;
            uint32 rightRear : 2;
            uint32 leftRear : 2;
        } m_lightStates;
    };
    union {
        uint32_t m_nPanelsStatus; // Index with ePanels
        struct {
            uint32 frontLeft : 4;
            uint32 frontRight : 4;
            uint32 rearLeft : 4;
            uint32 rearRight : 4;
            uint32 windscreen : 4;
            uint32 frontBumper : 4;
            uint32 rearBumper : 4;
        } m_panelStates;
        struct {
            /* unknown, bitfields of 2 */
        } m_aeroplanePanelStates;
    };

public:
    eDoorStatus GetDoorStatus(eDoors door) const;
    void SetDoorStatus(eDoors door, eDoorStatus status);

    eCarWheelStatus GetWheelStatus(eCarWheel wheel) const;
    void SetWheelStatus(eCarWheel wheel, eCarWheelStatus status);

    ePanelDamageState GetPanelStatus(ePanels panel) const;
    void SetPanelStatus(ePanels panel, ePanelDamageState status);

    eLightsState_Old GetLightStatus(eLights light) const;
    void SetLightStatus(eLights light, eLightsState_Old status);
};

VALIDATE_SIZE(CDamageManager, 0x18);