//
// Created by weikton on 13.05.2023.
//

#include "World.h"
#include "../vendor/armhook/patch.h"
#include "main.h"
#include "game/game.h"
#include "net/netgame.h"

//CRepeatSector CWorld::ms_aRepeatSectors[MAX_REPEAT_SECTORS_Y][MAX_REPEAT_SECTORS_X];
//CPtrListDoubleLink CWorld::ms_listMovingEntityPtrs;

CSector* GetSector(int32 x, int32 y) {
    static CSector(&ms_aSectors)[MAX_SECTORS_Y][MAX_SECTORS_X] = *(CSector(*)[MAX_SECTORS_Y][MAX_SECTORS_X])(g_libGTASA + (VER_x32 ? 0x96B9F4 : 0xBDCB20));

    const auto x1 = std::clamp<int32>(x, 0, MAX_SECTORS_X - 1);
    const auto y1 = std::clamp<int32>(y, 0, MAX_SECTORS_Y - 1);

    return &ms_aSectors[y1][x1];
}

CRepeatSector* GetRepeatSector(int32 x, int32 y) {
    static CRepeatSector(&ms_aRepeatSectors)[MAX_REPEAT_SECTORS_Y][MAX_REPEAT_SECTORS_X] = *(CRepeatSector(*)[MAX_REPEAT_SECTORS_Y][MAX_REPEAT_SECTORS_X])(g_libGTASA + (VER_x32 ? 0x987BF4 : 0xC14F20));

    return &ms_aRepeatSectors[y % MAX_REPEAT_SECTORS_Y][x % MAX_REPEAT_SECTORS_X];
}

void CWorld::InjectHooks() {
    CHook::Write(g_libGTASA + (VER_x32 ? 0x676698 : 0x84ADA0), &ms_nCurrentScanCode);
    CHook::InstallPLT(g_libGTASA + (VER_x32 ? 0x675C58 : 0x849A20), &ProcessPedsAfterPreRender);
}

bool CWorld::ProcessLineOfSight(const CVector* origin, const CVector* target, CColPoint* outColPoint, CEntityGTA** outEntity, bool buildings, bool vehicles, bool peds, bool objects, bool dummies, bool doSeeThroughCheck, bool doCameraIgnoreCheck, bool doShootThroughCheck) {
   // assert(!origin.HasNanOrInf() && !target.HasNanOrInf()); // We're getting random nan/inf's from somewhere, so let's try to root cause it...
    return CHook::CallFunction<bool>(g_libGTASA + (VER_x32 ? 0x424B94 + 1 : 0x508C7C), origin, target, outColPoint, outEntity, buildings, vehicles, peds, objects, dummies, doSeeThroughCheck, doCameraIgnoreCheck, doShootThroughCheck);
}

float CWorld::FindGroundZForCoord(float x, float y) {
    return CHook::CallFunction<float>(g_libGTASA + (VER_x32 ? 0x0042A7C4 + 1 : 0x50F3A0), x, y);
}
float CWorld::FindGroundZFor3DCoord(CVector coord, bool* outResult, CEntityGTA** outEntity) {
    return CHook::CallFunction<float>("_ZN6CWorld21FindGroundZFor3DCoordEfffPbPP7CEntity", coord.x, coord.y, coord.z, outResult, outEntity);
}

bool CWorld::GetIsLineOfSightClear(const CVector& origin, const CVector& target, bool buildings, bool vehicles, bool peds, bool objects, bool dummies, bool doSeeThroughCheck, bool doCameraIgnoreCheck) {
    return CHook::CallFunction<bool>(g_libGTASA + (VER_x32 ? 0x00423468 + 1 : 0x5075A4), &origin, &target, buildings, vehicles, peds, objects, dummies, doSeeThroughCheck, doCameraIgnoreCheck);
}

void CWorld::Add(CEntityGTA *entity) {
    if(entity)
        ((void(*)(CEntityGTA*))(g_libGTASA + (VER_x32 ? 0x00423418 + 1 : 0x507518)))(entity);
}

void CWorld::Remove(CEntityGTA *entity) {
    if(entity)
        ((void (*)(CEntityGTA*))(g_libGTASA + (VER_x32 ? 0x0042330C + 1 : 0x5073A0))) (entity);
}

extern CNetGame *pNetGame;
void CWorld::ProcessPedsAfterPreRender() {
    if (CTimer::bSkipProcessThisFrame)
        return;

    if (pNetGame->GetPlayerPool()->GetLocalPlayer())
    {
        CPlayerPed* pPlayerPed = pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed();
        if (pPlayerPed)
        {
            pPlayerPed->ProcessSpecialAction(pPlayerPed->iSpecialAction);
            pPlayerPed->ProcessAttach();
            pPlayerPed->m_pPed->GetIntelligence()->ProcessAfterPreRender();
        }
    }

    for (PLAYERID playerId = 0; playerId < MAX_PLAYERS; playerId++)
    {
        CRemotePlayer* pRemotePlayer = pNetGame->GetPlayerPool()->GetAt(playerId);
        if (pRemotePlayer)
        {
            CPlayerPed* pPlayerPed = pRemotePlayer->GetPlayerPed();
            if (pPlayerPed && !pPlayerPed->m_pPed->m_bRemoveFromWorld)
            {
                pPlayerPed->ProcessSpecialAction(pPlayerPed->iSpecialAction);
                pPlayerPed->ProcessAttach();
                pPlayerPed->m_pPed->GetIntelligence()->ProcessAfterPreRender();
            }
        }
    }
}

uint16 CWorld::GetCurrentScanCode() {
    return CWorld::ms_nCurrentScanCode;
}

void CWorld::IncrementCurrentScanCode() {
    if (ms_nCurrentScanCode >= 65535u) {
        ClearScanCodes();
        ms_nCurrentScanCode = 1;
    } else {
        ms_nCurrentScanCode++;
    }
}

void CWorld::ClearScanCodes() {
    const auto ProcessList = [](const CPtrList& list) {
        for (CPtrNode* node = list.GetNode(); node; node = node->GetNext()) {
            static_cast<CEntityGTA*>(node->m_item)->m_nScanCode = 0;
        }
    };

    for (auto y = 0; y < MAX_SECTORS_Y; y++) {
        for (auto x = 0; x < MAX_SECTORS_X; x++) {
            const auto& sector = *GetSector(x, y);
            ProcessList(sector.m_buildings);
            ProcessList(sector.m_dummies);
        }
    }

    for (auto y = 0; y < MAX_REPEAT_SECTORS_Y; y++) {
        for (auto x = 0; x < MAX_REPEAT_SECTORS_X; x++) {
            for (const auto& list : GetRepeatSector(x, y)->m_lists) {
                ProcessList(list);
            }
        }
    }
}
