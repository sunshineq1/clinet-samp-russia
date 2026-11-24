//
// Created by weikton on 04.09.2023.
//

#include "WidgetButton.h"

#include "../game.h"
#include "net/netgame.h"
#include "patch.h"

extern CNetGame *pNetGame;
extern CGame* pGame;

void (*CWidgetButtonEnterCar_Draw)(uintptr_t thiz);
void CWidgetButtonEnterCar_Draw_hook(uintptr_t thiz) {
    if (pNetGame && pGame->FindPlayerPed()) {
        CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();
        if (pVehiclePool) {
            VEHICLEID vehicleId = pVehiclePool->FindNearestToLocalPlayerPed();
            if (vehicleId != INVALID_VEHICLE_ID) {
                CVehicle* pVehicle = pVehiclePool->GetAt(vehicleId);
                if (pVehicle) {
                    if(!pGame->FindPlayerPed()->IsInJetpackMode())
                    {
                        if ((pGame->FindPlayerPed()->IsCrouching() && pVehicle->m_pVehicle->GetDistanceFromLocalPlayerPed() > 10.0f) || pVehicle->m_pVehicle->GetDistanceFromLocalPlayerPed() > 10.0f) {
                            return;
                        }
                    }
                }
            }
        }
    }

    CWidgetButtonEnterCar_Draw(thiz);
}

void CWidgetButton::InjectHooks() {
    CHook::InlineHook("_ZN21CWidgetButtonEnterCar4DrawEv", &CWidgetButtonEnterCar_Draw_hook,
                      &CWidgetButtonEnterCar_Draw);
}