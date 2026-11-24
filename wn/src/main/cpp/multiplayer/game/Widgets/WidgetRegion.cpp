//
// Created by weikton on 01.05.2023.
//

#include "WidgetRegion.h"
#include "patch.h"
#include "WidgetButton.h"
#include "../game.h"
#include "net/netgame.h"

extern CNetGame *pNetGame;
extern CGame* pGame;

void (*CWidgetRegionSprint_Draw)(uintptr_t thiz);
void CWidgetRegionSprint_Draw_hook(uintptr_t thiz) {
    if(pGame->FindPlayerPed() && pGame->FindPlayerPed()->IsInVehicle()) return;

    CWidgetRegionSprint_Draw(thiz);
}

void CWidgetRegion::InjectHooks()
{
    CHook::InlineHook("_ZN19CWidgetRegionSprint4DrawEv", &CWidgetRegionSprint_Draw_hook,
                      &CWidgetRegionSprint_Draw);
}