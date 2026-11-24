//
// Created by weikton on 02.02.25.
//

#include "CDamageManager.h"

eDoorStatus CDamageManager::GetDoorStatus(eDoors nDoorIdx) const {
    switch (nDoorIdx) {
        case eDoors::DOOR_BONNET:
        case eDoors::DOOR_BOOT:
        case eDoors::DOOR_LEFT_FRONT:
        case eDoors::DOOR_RIGHT_FRONT:
        case eDoors::DOOR_LEFT_REAR:
        case eDoors::DOOR_RIGHT_REAR:
            return m_aDoorsStatus[(unsigned)nDoorIdx];
        default:
            return eDoorStatus::DAMSTATE_NOTPRESENT;
    }
}

// 0x6C21C0
void CDamageManager::SetDoorStatus(eDoors door, eDoorStatus status) {
    switch (door) {
        case eDoors::DOOR_BONNET:
        case eDoors::DOOR_BOOT:
        case eDoors::DOOR_LEFT_FRONT:
        case eDoors::DOOR_RIGHT_FRONT:
        case eDoors::DOOR_LEFT_REAR:
        case eDoors::DOOR_RIGHT_REAR:
            m_aDoorsStatus[(unsigned)door] = status;
            break;
    }
}

eCarWheelStatus CDamageManager::GetWheelStatus(eCarWheel wheel) const {
    return m_anWheelsStatus[(unsigned)wheel];
}

void CDamageManager::SetWheelStatus(eCarWheel wheel, eCarWheelStatus status) {
    m_anWheelsStatus[(unsigned)wheel] = status;
}

ePanelDamageState CDamageManager::GetPanelStatus(ePanels panel) const {
    return (ePanelDamageState)((m_nPanelsStatus >> (4 * (unsigned)panel)) & 0xF);
}

void CDamageManager::SetPanelStatus(ePanels panel, ePanelDamageState status) {
    m_nPanelsStatus = (status << (4 * (unsigned)panel)) | m_nPanelsStatus & ~(15 << (4 * (unsigned)panel));
}

eLightsState_Old CDamageManager::GetLightStatus(eLights light) const {
    return (eLightsState_Old)((m_nLightsStatus >> (2 * (unsigned)light)) & 3);
}

void CDamageManager::SetLightStatus(eLights light, eLightsState_Old status) {
    m_nLightsStatus = ((unsigned)status << (2 * (unsigned)light)) | m_nLightsStatus & ~(3 << (2 * (unsigned)light));
}
