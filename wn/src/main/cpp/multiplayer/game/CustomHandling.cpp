#include "CustomHandling.h"
#include "net/netgame.h"
#include "net/vehiclepool.h"
#include "game/vehicle.h"
#include "main.h"

extern CNetGame *pNetGame;

#include "game/common.h"
#include "util/CUtil.h"

#include "vendor/armhook/patch.h"

#include "java/GUIManager.h"
#include "java/jniutil.h"

void CustomHandling::Load(eCustomHandlingType type) {
    Log(OBFUSCATE("Load custom handling..."));

    ChangeName(type);

    auto mod_HandlingManager = g_libGTASA + (VER_x32 ? 0xA066B8 : 0xCA6844);
    CHook::CallFunction<void>("_ZN16cHandlingDataMgr16LoadHandlingDataEv", mod_HandlingManager);

    if (!pNetGame) {
        return;
    }

    CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();
    if (!pVehiclePool) {
        return;
    }

    int32_t restored = 0;

    for (VEHICLEID i = 0; i < MAX_VEHICLES; i++) {
        if (!pVehiclePool->GetSlotState(i)) {
            continue;
        }

        CVehicle* pVehicle = pVehiclePool->GetAt(i);
        if (!pVehicle) {
            continue;
        }

        pVehicle->RebuildHandling();
        pVehicle->RebuildSuspensionLines();
        restored++;
    }

    Log("CustomHandling: таблица перечитана, правки возвращены на %d машин", restored);
}

void CustomHandling::ChangeName(eCustomHandlingType type) {
    CHook::UnFuck(g_libGTASA + (VER_x32? 0x473F74 : 0x759AE2));
    CHook::UnFuck(g_libGTASA + (VER_x32? 0x6B06A8 : 0x88D088));

    switch (type) {
        case HANDLING_DEFAULT: {
            strcpy((char*)(g_libGTASA + (VER_x32? 0x473F74 : 0x759AE2)), OBFUSCATE("HANDLING.CFG"));
            strcpy((char*)(g_libGTASA + (VER_x32? 0x6B06A8 : 0x88D088)), OBFUSCATE("HANDLING.CFG"));
            break;
        }
        case HANDLING_COMFORT: {
            strcpy((char*)(g_libGTASA + (VER_x32? 0x473F74 : 0x759AE2)), OBFUSCATE("HANDLING-COMFORT.CFG"));
            strcpy((char*)(g_libGTASA + (VER_x32? 0x6B06A8 : 0x88D088)), OBFUSCATE("HANDLING-COMFORT.CFG"));
            break;
        }
        case HANDLING_DRIFT: {
            strcpy((char*)(g_libGTASA + (VER_x32? 0x473F74 : 0x759AE2)), OBFUSCATE("HANDLING-DRIFT.CFG"));
            strcpy((char*)(g_libGTASA + (VER_x32? 0x6B06A8 : 0x88D088)), OBFUSCATE("HANDLING-DRIFT.CFG"));
            break;
        }
        case HANDLING_SPORT: {
            strcpy((char*)(g_libGTASA + (VER_x32? 0x473F74 : 0x759AE2)), OBFUSCATE("HANDLING-SPORT.CFG"));
            strcpy((char*)(g_libGTASA + (VER_x32? 0x6B06A8 : 0x88D088)), OBFUSCATE("HANDLING-SPORT.CFG"));
            break;
        }
        case HANDLING_SPORTPLUS: {
            strcpy((char*)(g_libGTASA + (VER_x32? 0x473F74 : 0x759AE2)), OBFUSCATE("HANDLING-PLUS.CFG"));
            strcpy((char*)(g_libGTASA + (VER_x32? 0x6B06A8 : 0x88D088)), OBFUSCATE("HANDLING-PLUS.CFG"));
            break;
        }
    }
}