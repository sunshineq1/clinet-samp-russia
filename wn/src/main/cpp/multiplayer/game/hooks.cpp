#include <GLES2/gl2.h>
#include "SaaArchive.h"
#include "CHandlingDefault.h"
#include "Enums/eCarNodes.h"
#include "../main.h"
#include "../vendor/armhook/patch.h"
#include "game.h"
#include "../net/netgame.h"
#include "../gui/gui.h"
#include "Textures/TextureDatabase.h"
#include "Textures/TextureDatabaseEntry.h"
#include "Textures/TextureDatabaseRuntime.h"
#include "Water/CWaterFX.h"
#include "FrameRateFixes.h"
#include "Scene.h"
#include "sprite2d.h"
#include "Entity/PlayerPedGta.h"
#include "Pools.h"
#include "java/jniutil.h"
#include "game/Models/ModelInfo.h"
#include "MatrixLink.h"
#include "MatrixLinkList.h"
#include "game/Collision/Collision.h"
#include "TxdStore.h"
#include "util/CUtil.h"
#include "Coronas.h"
#include "multitouch.h"
#include "Streaming.h"
#include "References.h"
#include "VisibilityPlugins.h"
#include "game/Animation/AnimManager.h"
#include "FileLoader.h"
#include "Renderer.h"
#include "CrossHair.h"
#include "World.h"

#include "IplStore.h"
#include "Core/QuadTreeNode.h"
// #include "MobileMenu.h"
#include "java/HUD.h"

extern UI* pUI;
extern CGame* pGame;
extern CNetGame *pNetGame;
extern MaterialTextGenerator* pMaterialTextGenerator;

uint8_t byteInternalPlayer = 0;
CPedGTA* dwCurPlayerActor = 0;
uint8_t byteCurPlayer = 0;

extern "C" uintptr_t get_lib() {
	return g_libGTASA;
}

PLAYERID FindPlayerIDFromGtaPtr(CEntityGTA* pEntity)
{
	if (pEntity == nullptr) return INVALID_PLAYER_ID;

	CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
	CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();

	PLAYERID PlayerID = pPlayerPool->FindRemotePlayerIDFromGtaPtr((CPedGTA*)pEntity);
	if (PlayerID != INVALID_PLAYER_ID) return PlayerID;

	VEHICLEID VehicleID = pVehiclePool->FindIDFromGtaPtr((CVehicleGTA*)pEntity);
	if (VehicleID != INVALID_VEHICLE_ID)
	{
		for (PLAYERID i = 0; i < MAX_PLAYERS; i++)
		{
			CRemotePlayer* pRemotePlayer = pPlayerPool->GetAt(i);
			if (pRemotePlayer && pRemotePlayer->CurrentVehicleID() == VehicleID) {
				return i;
			}
		}
	}

	return INVALID_PLAYER_ID;
}

PLAYERID FindActorIDFromGtaPtr(CPedGTA* pPed)
{
	if (pPed) {
		return pNetGame->GetActorPool()->FindIDFromGtaPtr(pPed);
	}

	return INVALID_PLAYER_ID;
}

void RenderEffects() {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x0059DA40 + 1 : 0x6C1D6C));
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005BE914 + 1 : 0x6E2FB4));

    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005A6BC8 + 1 : 0x6CA5D0));
    CVisibilityPlugins::RenderReallyDrawLastObjects();
    CCoronas::Render();

    // FIXME
    CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
    auto g_fx = *(uintptr_t *) (g_libGTASA + (VER_x32 ? 0x00820520 : 0xA062A8));
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x00363DF0 + 1 : 0x433F54), &g_fx, TheCamera.m_pRwCamera, false);

    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005CBBAC + 1 : 0x6F054C));
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x0059BF84 + 1 : 0x6C0268));
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005A1C38 + 1 : 0x6C552C));

    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005E3390 + 1 : 0x708DF0));

    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005C0B14 + 1 : 0x6E50CC));
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005B19D0 + 1 : 0x6D6068));
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005B5F78 + 1 : 0x6DA2B8));
}

#include "java/GUIManager.h"
#include "java/HUD.h"
void MainLoop();
#define ENABLE_CUSTOM_2DSTUFF false

#if !ENABLE_CUSTOM_2DSTUFF
void (*Render2dStuff)();
void Render2dStuff_hook() {
    Render2dStuff();

    if(pNetGame) {
        CTextDrawPool* pTextDrawPool = pNetGame->GetTextDrawPool();
        if(pTextDrawPool) {
            pTextDrawPool->Draw();
        }
    }

    if(!pUI->m_bCamEditGui) {
        if(pNetGame) {
            CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();
            if (pVehiclePool) {
                VEHICLEID closetVehicleID = pVehiclePool->FindNearestToLocalPlayerPed();
                if (closetVehicleID < MAX_VEHICLES && pVehiclePool->GetSlotState(closetVehicleID)) {
                    CVehicle* pVehicle = pVehiclePool->GetAt(closetVehicleID);
                    if (pVehicle && pVehicle->m_pVehicle->GetDistanceFromLocalPlayerPed() < 5.0f) {
                        CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
                        if (pPlayerPool && !pPlayerPool->GetLocalPlayer()->GetPlayerPed()->IsInVehicle() && !pPlayerPool->GetLocalPlayer()->GetPlayerPed()->IsAPassenger()) {
                            CHud::updateG(1);
                        } else CHud::updateG(0);
                    } else CHud::updateG(0);
                } else CHud::updateG(0);
            }
        }
    }

    if (pUI) {
        pUI->render();
    }

    return;
}
#else
void Render2dStuff() {
    if(CHook::CallFunction<bool>(g_libGTASA + (VER_x32 ? 0x001BB7F4 + 1 : 0x24EA90))) // emu_IsAltRenderTarget()
        CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x001BC20C + 1 : 0x24F5B8)); // emu_FlushAltRenderTarget()

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND, RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, RWRSTATE(rwRENDERSTATENARENDERSTATE));
    RwRenderStateSet(rwRENDERSTATECULLMODE, RWRSTATE(rwCULLMODECULLNONE));

    if (pUI) {
        pUI->render();
    }

    // Widgets
    if(!pUI->m_bCamEditGui) {
        if(pNetGame) {
            CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();
            if (pVehiclePool) {
                VEHICLEID closetVehicleID = pVehiclePool->FindNearestToLocalPlayerPed();
                if (closetVehicleID < MAX_VEHICLES && pVehiclePool->GetSlotState(closetVehicleID)) {
                    CVehicle* pVehicle = pVehiclePool->GetAt(closetVehicleID);
                    if (pVehicle && pVehicle->m_pVehicle->GetDistanceFromLocalPlayerPed() < 5.0f) {
                        CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
                        if (pPlayerPool && !pPlayerPool->GetLocalPlayer()->GetPlayerPed()->IsInVehicle() && !pPlayerPool->GetLocalPlayer()->GetPlayerPed()->IsAPassenger()) {
                            CHud::updateG(1);
                        } else CHud::updateG(0);
                    } else CHud::updateG(0);
                } else CHud::updateG(0);
            }
        }

        ((void (*)()) (g_libGTASA + (VER_x32 ? 0x00437B0C + 1 : 0x51CFF0)))(); // CHud::DrawRadar
        ((void (*)(bool)) (g_libGTASA + (VER_x32 ? 0x002B0BD8 + 1 : 0x36FB00)))(false); // CTouchInterface::DrawAll
    }

    if (pNetGame) {
        CTextDrawPool* pTextDrawPool = pNetGame->GetTextDrawPool();
        if(pTextDrawPool)
            pTextDrawPool->Draw();
    }
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x1C0750 + 1 : 0x252CE4), 1); // _Z12emu_GammaSeth

    ((void (*)(bool)) (g_libGTASA + (VER_x32 ? 0x0054BDD4 + 1 : 0x66B678)))(1u); // CMessages::Display
    ((void (*)(bool)) (g_libGTASA + (VER_x32 ? 0x005A9120 + 1 : 0x6CCEA0)))(1u); // CFont::RenderFontBuffer
}
#endif

#include "GPS.h"
uintptr (*CRadar__SetCoordBlip)(eBlipType blipType, CVector pos, uint32 nColour, eBlipDisplay r5, char *name);
uintptr CRadar__SetCoordBlip_hook(eBlipType blipType, CVector pos, uint32 nColour, eBlipDisplay r5, char *name)
{
	if(pNetGame && !strncmp(name, "CODEWAY", 7))
	{
        pos.z += 500;
        pos.z = CWorld::FindGroundZFor3DCoord(pos);
        pos.z += 1.f;

		if(pNetGame->GetGameState() != GAMESTATE_CONNECTED) return 0;

        GPS::Set(&pos);

		RakNet::BitStream bsSend;
        bsSend.Write(pos.x);
        bsSend.Write(pos.y);
        bsSend.Write(pos.z);
		pNetGame->GetRakClient()->RPC(&RPC_MapMarker, &bsSend, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, nullptr);

        return CRadar__SetCoordBlip(blipType, pos, nColour, r5, name);
    }

	return CRadar__SetCoordBlip(blipType, pos, nColour, r5, name);
}

#include "Mobile/MobileMenu/MobileMenu.h"
void (*CRadar__ClearBlip)(int32_t index);
void CRadar__ClearBlip_hook(int32_t index)
{
    if ((uint16_t) index > 249) {
        Log("Invalid blip ID");
        return;
    }
    if (CMobileMenu::Get().waypoint_blip == index) {
         GPS::Set(NULL);
    }

    CRadar__ClearBlip(index);
}

uint8_t bGZ = 0;
void CRadar__DrawRadarGangOverlay(uint8_t v1)
{
    bGZ = v1;
    if (pNetGame)
	{
		CGangZonePool *pGangZonePool = pNetGame->GetGangZonePool();
		if (pGangZonePool) {
			pGangZonePool->Draw(v1);
		}
	}

    GPS::DoPathDraw();
}

typedef struct {
    CVector     vecPosObject;
    CQuaternion m_qRotation;
    int32       wModelIndex;
    union {
        struct { // CFileObjectInstanceType
            uint32 m_nAreaCode : 8;
            uint32 m_bRedundantStream : 1;
            uint32 m_bDontStream : 1; // Merely assumed, no countercheck possible.
            uint32 m_bUnderwater : 1;
            uint32 m_bTunnel : 1;
            uint32 m_bTunnelTransition : 1;
            uint32 m_nReserved : 19;
        };
        uint32 m_nInstanceType;
    };
    int32 m_nLodInstanceIndex; // -1 - without LOD model
} stLoadObjectInstance;
VALIDATE_SIZE(stLoadObjectInstance, (VER_x32 ? 0x28 : 0x28));

extern int iBuildingToRemoveCount;
extern std::list <stRemoveBuilding> BuildingToRemove;

int (*CFileLoader__LoadObjectInstance)(stLoadObjectInstance *thiz);
int CFileLoader__LoadObjectInstance_hook(stLoadObjectInstance *thiz)
{
    if(thiz)
    {
        if(iBuildingToRemoveCount >= 1)
        {
            for(auto &entry : BuildingToRemove)
            {
                float fDistance = GetDistance(entry.vecPos, thiz->vecPosObject);
                if(fDistance <= entry.fRange)
                {
                    if(entry.dwModel == -1 || thiz->wModelIndex == (uint16_t)entry.dwModel)
                    {
                        thiz->wModelIndex = 19300;
                        break;
                    }
                }
            }
        }
    }

    return CFileLoader__LoadObjectInstance(thiz);
}

void (*CObject_Render)(CObjectGta* thiz);
void CObject_Render_hook(CObjectGta* thiz)
{
	CObjectGta *object = thiz;
	if(pNetGame && object != 0)
	{
		CObject *pObject = pNetGame->GetObjectPool()->FindObjectFromGtaPtr(object);
		if(pObject && pObject->m_pEntity)
		{
			RwObject* rwObject = (RwObject*)pObject->m_pEntity->m_pRwObject;
			if(rwObject)
			{
				if(pObject->m_bHasMaterial || pObject->m_bHasMaterialText)
				{
					RwFrameForAllObjects((RwFrame*)rwObject->parent, (RwObject *(*)(RwObject *, void *))ObjectMaterialCallBack, pObject);
				}
				if(pObject->m_bHasMaterialText)
                {
                    RwFrameForAllObjects((RwFrame*)rwObject->parent, (RwObject *(*)(RwObject *, void *))ObjectMaterialTextCallBack, pObject);
                }
			}
		}
	}

    CObject_Render(object);
}

bool NotifyEnterVehicle(CVehicleGTA *_pVehicle)
{
	if(!pNetGame) {
		return false;
	}

	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	if(!pVehiclePool) {
		return false;
	}

	CVehicle *pVehicle = nullptr;
	VEHICLEID VehicleID = pVehiclePool->FindIDFromGtaPtr(_pVehicle);

	if(VehicleID <= 0 || VehicleID >= MAX_VEHICLES) {
		return false;
	}

	if(!pVehiclePool->GetSlotState(VehicleID)) {
		return false;
	}

	pVehicle = pVehiclePool->GetAt(VehicleID);
	if(!pVehicle) {
		return false;
	}

	CLocalPlayer *pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();

	if(pLocalPlayer) {
		// .. Log("Vehicle ID: %d", VehicleID);
		pLocalPlayer->SendEnterVehicleNotification(VehicleID, false);
	}

	return true;
}

int (*TaskEnterVehicle)(uintptr_t a1, uintptr_t a2);
int TaskEnterVehicleHook(uintptr_t a1, uintptr_t a2)
{
    if(!NotifyEnterVehicle((CVehicleGTA*)a1)) {
        return false;
    }

    // CTask::operator new
    uintptr_t pTask = ((uintptr_t (*)(void))(g_libGTASA + (VER_x32 ? 0x4D6A70:0x5D7414)))();

    // CTaskComplexEnterCarAsDriver::CTaskComplexEnterCarAsDriver
    ((void (__fastcall *)(uintptr_t, uintptr_t))(g_libGTASA + (VER_x32 ? 0x4F6FE0:0x6007E0)))(pTask, a1);

    // CTaskManager::SetTask
    ((int (__fastcall *)(uintptr_t, uintptr_t, int, int))(g_libGTASA + (VER_x32 ? 0x53397A:0x64E084)))(a2, pTask, 3, 0);

    return true;
}

uint32_t CRadar__GetRadarTraceColor(uint32_t color, uint8_t bright, uint8_t friendly)
{
    return TranslateColorCodeToRGBA(color);
}
uint32_t CHudColours__GetIntColour(uintptr* thiz, uint8 colour_id)
{
    return TranslateColorCodeToRGBA(colour_id);
}

void (*AND_TouchEvent)(int type, int num, int posX, int posY);
void AND_TouchEvent_hook(int type, int num, int posX, int posY)
{
	// imgui
	// bool bRet = pUI->OnTouchEvent(type, num, posX, posY);

	if (pGame->IsGamePaused())
		return AND_TouchEvent(type, num, posX, posY);

	if (pUI != nullptr)
	{
		switch (type)
		{
			case 2: // push
				pUI->touchEvent(ImVec2(posX, posY), TouchType::push);
				break;

			case 3: // move
				pUI->touchEvent(ImVec2(posX, posY), TouchType::move);
				break;

			case 1: // pop
				pUI->touchEvent(ImVec2(posX, posY), TouchType::pop);
				break;
		}

        if (pUI->keyboard()->visible() || pUI->dialog()->visible()) {
            AND_TouchEvent(1, 0, 0, 0);
            return;
        }
        else
        {
            if (pNetGame && pNetGame->GetTextDrawPool())
            {
                if (!pNetGame->GetTextDrawPool()->onTouchEvent(type, num, posX, posY)) {
                    return AND_TouchEvent(1, 0, 0, 0);
                }
            }
        }
	}

	if (pGame->IsGameInputEnabled())
		AND_TouchEvent(type, num, posX, posY);
	else
		AND_TouchEvent(1, 0, 0, 0);
}

uint32_t (*CPed__GetWeaponSkill)(CPedGTA *thiz);
uint32_t CPed__GetWeaponSkill_hook(CPedGTA *thiz)
{
	bool bWeaponSkillStored = false;

	dwCurPlayerActor = thiz;
	byteInternalPlayer = CWorld::PlayerInFocus;
	byteCurPlayer = FindPlayerNumFromPedPtr(dwCurPlayerActor);

	if(dwCurPlayerActor && byteCurPlayer != 0 && CWorld::PlayerInFocus == 0)
	{
		GameStoreLocalPlayerSkills();
		GameSetRemotePlayerSkills(byteCurPlayer);
		bWeaponSkillStored = true;
	}

	// CPed::GetWeaponSkill
	uint32_t result = (( uint32_t (*)(CPedGTA *, uint32_t))(g_libGTASA+0x4A55E2+1))(thiz, thiz->m_aWeapons[thiz->m_nActiveWeaponSlot].dwType);

	if(bWeaponSkillStored)
	{
		GameSetLocalPlayerSkills();
		bWeaponSkillStored = false;
	}

	return result;
}

extern CPlayerPed* g_pCurrentFiredPed;
extern BULLET_DATA* g_pCurrentBulletData;

extern int g_iLagCompensationMode;

void SendBulletSync(CVector* vecOrigin, CVector* a2, CColPoint *colPoint, CEntityGTA** ppEntity)
{
    CMatrix mat1, mat2;

    static BULLET_DATA bulletData;
    memset(&bulletData, 0, sizeof(BULLET_DATA));

    bulletData.vecOrigin.x = vecOrigin->x;
    bulletData.vecOrigin.y = vecOrigin->y;
    bulletData.vecOrigin.z = vecOrigin->z;

    bulletData.vecPos.x = colPoint->m_vecPoint.x;
    bulletData.vecPos.y = colPoint->m_vecPoint.y;
    bulletData.vecPos.z = colPoint->m_vecPoint.z;

    if (ppEntity)
    {
        CEntityGTA* pEntity = *ppEntity;
        if (pEntity)
        {
            if (g_iLagCompensationMode != 0)
            {
                bulletData.vecOffset.x = colPoint->m_vecPoint.x - pEntity->m_matrix->m_pos.x;
                bulletData.vecOffset.y = colPoint->m_vecPoint.y - pEntity->m_matrix->m_pos.y;
                bulletData.vecOffset.z = colPoint->m_vecPoint.z - pEntity->m_matrix->m_pos.z;
            }
            else
            {
                memset(&mat1, 0, sizeof(CMatrix));
                memset(&mat2, 0, sizeof(CMatrix));
                // RwMatrixOrthoNormalize
                auto entMat = pEntity->GetMatrix().ToRwMatrix();
                RwMatrixOrthoNormalize(reinterpret_cast<RwMatrix *>(&mat2), &entMat);
                // RwMatrixInvert
                Invert(mat1, mat2);

                ProjectMatrix(&bulletData.vecOffset, &mat1, &colPoint->m_vecPoint);
            }

            bulletData.pEntity = pEntity;
        }
        else bulletData.vecOffset = 0;
    }

    pGame->FindPlayerPed()->ProcessBulletData(&bulletData);
}

extern bool g_customFire;
uint32_t(*CWeapon__FireInstantHit)(CWeapon* thiz, CPedGTA* pFiringEntity, CVector* vecOrigin, CVector* muzzlePosn, CEntityGTA* targetEntity,
								  CVector* target, CVector* originForDriveBy, bool arg6, bool muzzle);
uint32_t CWeapon__FireInstantHit_hook(CWeapon* thiz, CPedGTA* pFiringEntity, CVector* vecOrigin, CVector* muzzlePosn, CEntityGTA* targetEntity,
									 CVector* target, CVector* originForDriveBy, bool arg6, bool muzzle)
{
	if (pNetGame && pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()->m_pPed)		// CWeapon::Fire
    {
       	if(pFiringEntity != GamePool_FindPlayerPed())
			return muzzle;

		if(pNetGame)
		{
            pNetGame->GetPlayerPool()->ApplyCollisionChecking();
		}

		if(pGame)
		{
			CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
			if(pPlayerPed)
				pPlayerPed->FireInstant();
		}

		if(pNetGame)
		{
            pNetGame->GetPlayerPool()->ResetCollisionChecking();
		}

		return muzzle;
    }

    return CWeapon__FireInstantHit(thiz, pFiringEntity, vecOrigin, muzzlePosn, targetEntity,
                                  target, originForDriveBy, arg6, muzzle);
}

bool g_bForceWorldProcessLineOfSight = false;
uint32_t (*CWeapon__ProcessLineOfSight)(CVector *vecOrigin, CVector *vecEnd, CVector *vecPos, CPedGTA **ppEntity, CWeapon *pWeaponSlot, CPedGTA **ppEntity2, bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7);
uint32_t CWeapon__ProcessLineOfSight_hook(CVector *vecOrigin, CVector *vecEnd, CVector *vecPos, CPedGTA **ppEntity, CWeapon *pWeaponSlot, CPedGTA **ppEntity2, bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7)
{
    uintptr_t dwRetAddr = 0;
    GET_LR(dwRetAddr);

#if VER_x32
    if(dwRetAddr >= 0x005DC178 && dwRetAddr <= 0x005DD684)
		g_bForceWorldProcessLineOfSight = true;
#else
    if(dwRetAddr >= 0x701494 && dwRetAddr <= 0x702B18)
        g_bForceWorldProcessLineOfSight = true;
#endif

    return CWeapon__ProcessLineOfSight(vecOrigin, vecEnd, vecPos, ppEntity, pWeaponSlot, ppEntity2, b1, b2, b3, b4, b5, b6, b7);
}

uint32_t(*CWorld__ProcessLineOfSight)(CVector*, CVector*, CColPoint *colPoint, CEntityGTA**, bool, bool, bool, bool, bool, bool, bool, bool);
uint32_t CWorld__ProcessLineOfSight_hook(CVector* vecOrigin, CVector* vecEnd, CColPoint *colPoint, CEntityGTA** ppEntity,
										bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7, bool b8)
{
    uintptr_t dwRetAddr = 0;
    GET_LR(dwRetAddr);

    if(dwRetAddr == (VER_x32 ? 0x005dd0b0 + 1 : 0x70253C) || g_bForceWorldProcessLineOfSight)
    {
        g_bForceWorldProcessLineOfSight = false;
		//LOGI("CWorld_ProcessLineOfSight iLagCompensationMode: %d", g_iLagCompensationMode);
        static CVector vecPosPlusOffset;

		if (g_iLagCompensationMode != 2)
		{
			if (g_pCurrentFiredPed != pGame->FindPlayerPed())
			{
				if (g_pCurrentBulletData && g_pCurrentBulletData->pEntity)
				{
					if (*(uintptr_t*)(g_pCurrentBulletData->pEntity) != g_libGTASA+(VER_x32 ? 0x667D18:0x8300A0)) // CPlaceable
					{
                        if (g_iLagCompensationMode)
                        {
                            vecPosPlusOffset.x = g_pCurrentBulletData->pEntity->GetPosition().x + g_pCurrentBulletData->vecOffset.x;
                            vecPosPlusOffset.y = g_pCurrentBulletData->pEntity->GetPosition().y + g_pCurrentBulletData->vecOffset.y;
                            vecPosPlusOffset.z = g_pCurrentBulletData->pEntity->GetPosition().z + g_pCurrentBulletData->vecOffset.z;
                        }
                        else
                        {
                            if(g_pCurrentBulletData->pEntity->m_matrix && g_pCurrentBulletData->vecOffset != nullptr) {
                                if (g_pCurrentBulletData->pEntity->GetMatrix().m_up != nullptr &&
                                    g_pCurrentBulletData->pEntity->GetMatrix().m_forward != nullptr &&
                                    g_pCurrentBulletData->pEntity->GetMatrix().m_right != nullptr &&
                                    g_pCurrentBulletData->pEntity->GetMatrix().m_pos != nullptr) {
                                    ProjectMatrix((CVector *) &vecPosPlusOffset,
                                                  &g_pCurrentBulletData->pEntity->GetMatrix(),
                                                  &g_pCurrentBulletData->vecOffset);
                                }
                            }
                        }

                        vecEnd->x = vecPosPlusOffset.x - vecOrigin->x + vecPosPlusOffset.x;
                        vecEnd->y = vecPosPlusOffset.y - vecOrigin->y + vecPosPlusOffset.y;
                        vecEnd->z = vecPosPlusOffset.z - vecOrigin->z + vecPosPlusOffset.z;
					}
				}
			}
		}

		uint32_t result = CWorld__ProcessLineOfSight(vecOrigin, vecEnd, colPoint, ppEntity, b1, b2, b3, b4, b5, b6, b7, b8);

		if (g_iLagCompensationMode == 2)
		{
			if (g_pCurrentFiredPed == pGame->FindPlayerPed()) {
				SendBulletSync(vecOrigin, vecEnd, colPoint, ppEntity);
			}
			return result;
		}

		if (g_pCurrentFiredPed)
		{
			if (g_pCurrentFiredPed != pGame->FindPlayerPed())
			{
				if (g_pCurrentBulletData)
				{
					if (g_pCurrentBulletData->pEntity == nullptr)
					{
                        CPedGTA* pLocalPed = GamePool_FindPlayerPed();
						if (*ppEntity == GamePool_FindPlayerPed() ||
                                pLocalPed->IsInVehicle() && *ppEntity == pLocalPed->pVehicle)
						{
							result = 0;
							*ppEntity = nullptr;
                            colPoint->m_vecPoint.x = 0.0f;
                            colPoint->m_vecPoint.y = 0.0f;
                            colPoint->m_vecPoint.z = 0.0f;
							return result;
						}
					}
				}
			}
			else {
				SendBulletSync(vecOrigin, vecEnd, colPoint, ppEntity);
			}
		}

		return result;
	}

	return CWorld__ProcessLineOfSight(vecOrigin, vecEnd, colPoint, ppEntity, b1, b2, b3, b4, b5, b6, b7, b8);
}

uint32_t(*CWeapon__FireSniper)(CWeapon* thiz, CPedGTA* pFiringEntity, CEntityGTA* victim, CVector* target);
uint32_t CWeapon__FireSniper_hook(CWeapon* thiz, CPedGTA* pFiringEntity, CEntityGTA* victim, CVector* target)
{
	if (pFiringEntity == GamePool_FindPlayerPed())
	{
		if (pGame)
		{
			CPlayerPed* pPlayerPed = pGame->FindPlayerPed();
			if (pPlayerPed) {
				pPlayerPed->FireInstant();
			}
		}
	}

	return true;
}

bool(*CBulletInfo_AddBullet)(CEntityGTA* creator, int weaponType, CVector pos, CVector velocity);
bool CBulletInfo_AddBullet_hook(CEntityGTA* creator, int weaponType, CVector pos, CVector velocity)
{
	velocity.x *= 50.0f;
	velocity.y *= 50.0f;
	velocity.z *= 50.0f;

	CBulletInfo_AddBullet(creator, weaponType, pos, velocity);

	// CBulletInfo::Update
	CHook::CallFunction<void>("_ZN11CBulletInfo6UpdateEv");
	return true;
}

#pragma pack(push, 1)
struct CPedDamageResponseCalculator
{
    CPedGTA* m_pDamager;
	float m_fDamageFactor;
	int m_pedPieceType;
	int m_weaponType;
};
#pragma pack(pop)

bool ComputeDamageResponse(CPedDamageResponseCalculator* calculator, CPedGTA* pPed)
{
    CPedGTA* pGamePed = GamePool_FindPlayerPed();
	bool isLocalPed = false;

	if (!pNetGame) return false;

    CPedGTA* pDamager = calculator->m_pDamager;
	if (pDamager != pGamePed && IsValidGamePed(pGamePed)) /* CCivilianPed */
		return true;

	if (pPed == pGamePed) {
		isLocalPed = true;
	}
	else if (pDamager != pGamePed) {
		return false;
	}

	CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
	CLocalPlayer* pLocalPlayer = pPlayerPool->GetLocalPlayer();
	PLAYERID PlayerID;

	if (isLocalPed)
	{
		PlayerID = FindPlayerIDFromGtaPtr(pDamager);
		pLocalPlayer->SendTakeDamageEvent(PlayerID,
										  calculator->m_fDamageFactor,
										  calculator->m_weaponType,
										  calculator->m_pedPieceType);
	}
	else
	{
		PlayerID = FindPlayerIDFromGtaPtr(pPed);
		if (PlayerID != INVALID_PLAYER_ID)
		{
			pLocalPlayer->SendGiveDamageEvent(PlayerID,
											  calculator->m_fDamageFactor,
											  calculator->m_weaponType,
											  calculator->m_pedPieceType);
			if (pPlayerPool->GetAt(PlayerID)->IsNPC())
				return true;
		}
		else
		{
			PLAYERID ActorID = FindActorIDFromGtaPtr(pPed);
			if (ActorID != INVALID_PLAYER_ID) {
				pLocalPlayer->SendGiveDamageEvent(ActorID,
												  calculator->m_fDamageFactor,
												  calculator->m_weaponType,
												  calculator->m_pedPieceType);
				return true;
			}
		}
	}

	if (!pNetGame->m_pNetSet->bFriendlyFire)
		return false;

	uint8_t byteTeam = pPlayerPool->GetLocalPlayer()->m_byteTeam;
	if (byteTeam == NO_TEAM ||
		PlayerID == INVALID_PLAYER_ID ||
		pPlayerPool->GetAt(PlayerID)->m_byteTeam != byteTeam) {
		return false;
	}

	return true;
}

void (*CPedDamageResponseCalculator__ComputeDamageResponse)(CPedDamageResponseCalculator* thiz, CPedGTA* pPed, uintptr_t* a3, uint32_t a4);
void CPedDamageResponseCalculator__ComputeDamageResponse_hook(CPedDamageResponseCalculator* thiz, CPedGTA* pPed, uintptr_t *a3, uint32_t a4)
{
	if (thiz == nullptr || pPed == nullptr || a3 == nullptr) return;

    if (ComputeDamageResponse(thiz, pPed))
        return;

	CPedDamageResponseCalculator__ComputeDamageResponse(thiz, pPed, a3, a4);
}

#include "CFPSFix.h"
#include "ES2VertexBuffer.h"
#include "RQ_Commands.h"
#include "Pickups.h"
#include "TimeCycle.h"
#include "game/Pipelines/CustomCar/CustomCarEnvMapPipeline.h"
#include "game/Pipelines/CustomBuilding/CustomBuildingDNPipeline.h"
#include "RealTimeShadowManager.h"
#include "game/Widgets/WidgetGta.h"

CFPSFix g_fps;
void (*ANDRunThread)(void* a1);
void ANDRunThread_hook(void* a1) {
	g_fps.PushThread(gettid());
	ANDRunThread(a1);
}

static constexpr float ar43 = 4.0f/3.0f;
float *ms_fAspectRatio;
void (*DrawCrosshair)(uintptr_t* thiz);
void DrawCrosshair_hook(uintptr_t* thiz)
{
	float save1 = CCamera::m_f3rdPersonCHairMultX;
	CCamera::m_f3rdPersonCHairMultX = 0.530f - (*ms_fAspectRatio - ar43) * 0.01125f;

	float save2 = CCamera::m_f3rdPersonCHairMultY;
	CCamera::m_f3rdPersonCHairMultY = 0.400f + (*ms_fAspectRatio - ar43) * 0.03600f;

	DrawCrosshair(thiz);

	CCamera::m_f3rdPersonCHairMultX = save1;
	CCamera::m_f3rdPersonCHairMultY = save2;
}

CVector& (*FindPlayerSpeed)(int a1);
CVector& FindPlayerSpeed_hook(int a1)
{
	uintptr_t dwRetAddr = 0;
	__asm__ volatile ("mov %0, lr":"=r" (dwRetAddr));
	dwRetAddr -= g_libGTASA;

	if(dwRetAddr == 0x43E1F6 + 1)
	{
		if(pNetGame)
		{
			CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
			if(pPlayerPed &&
			   pPlayerPed->IsInVehicle() &&
			   pPlayerPed->IsAPassenger())
			{
				CVector vec = CVector(-1.0f);
				return vec;
			}
		}
	}

	return FindPlayerSpeed(a1);
}

int iLastTouchedWidgetId = -1;
int iLastReleasedWidgetId = -1;

int (*CTouchInterface__IsReleased)(int iWidgetId, int iUnk, int iEnableWidget);
int CTouchInterface__IsReleased_hook(int iWidgetId, int iUnk, int iEnableWidget)
{
	uintptr_t dwRetAddr = 0;
	__asm__ volatile ("mov %0, lr" : "=r" (dwRetAddr));
	dwRetAddr -= g_libGTASA;

	int iReleased = CTouchInterface__IsReleased(iWidgetId, iUnk, iEnableWidget);
	if(iReleased && iEnableWidget)
	{
		iLastReleasedWidgetId = iWidgetId;
	}

	return iReleased;
}

int (*CTextureDatabaseRuntime__GetEntry)(uintptr_t thiz, const char* a2, bool* a3);
int CTextureDatabaseRuntime__GetEntry_hook(uintptr_t thiz, const char* a2, bool* a3)
{
	if (!thiz)
	{
		return -1;
	}
	return CTextureDatabaseRuntime__GetEntry(thiz, a2, a3);
}

int (*CCustomRoadsignMgr_RenderRoadsignAtomic)(int a1, int a2);
int CCustomRoadsignMgr_RenderRoadsignAtomic_hook(int a1, int a2)
{
	if ( a1 )
		return CCustomRoadsignMgr_RenderRoadsignAtomic(a1, a2);
}

void (*CCamera__Process)(uintptr_t thiz);
void CCamera__Process_hook(uintptr_t thiz)
{
	CCamera__Process(thiz);
}

extern CJavaWrapper* pJavaWrapper;
void (*MainMenuScreen__OnExit)();
void MainMenuScreen__OnExit_hook()
{
	pGame->bIsGameExiting = true;
	pNetGame->GetRakClient()->Disconnect(0);
	pJavaWrapper->exitGame();
}

void (*rxOpenGLDefaultAllInOneRenderCB)(RwResEntry* repEntry, void *object, RwUInt8 type, RwUInt32 flags);
void rxOpenGLDefaultAllInOneRenderCB_hook(RwResEntry* repEntry, void *object, RwUInt8 type, RwUInt32 flags) {
	if(!repEntry || !flags)
		return;

	return rxOpenGLDefaultAllInOneRenderCB(repEntry, object, type, flags);
}

void (*CCustomBuildingDNPipeline__CustomPipeRenderCB)(RwResEntry* repEntry, void *object, RwUInt8 type, RwUInt32 flags);
void CCustomBuildingDNPipeline__CustomPipeRenderCB_hook(RwResEntry* repEntry, void *object, RwUInt8 type, RwUInt32 flags) {
    if (!repEntry || !flags) {
        return;
    }

	return CCustomBuildingDNPipeline__CustomPipeRenderCB(repEntry, object, type, flags);
}

RpAtomic* (*CCustomBuildingRenderer__AtomicSetup)(RpAtomic *pAtomic);
RpAtomic* CCustomBuildingRenderer__AtomicSetup_hook(RpAtomic *pAtomic) {
    RxPipeline* pipeline = pAtomic->pipeline;
    if (pipeline && pipeline->pluginId == 0x116) {
        return pAtomic;
    }

	return CCustomBuildingRenderer__AtomicSetup(pAtomic);
}

float float_4DD9E8;
float ms_fTimeStep;
float fMagic = 50.0f / 30.0f;

void (*CTaskSimpleUseGun__SetMoveAnim)(uintptr_t *thiz, uintptr_t *a2);
void CTaskSimpleUseGun__SetMoveAnim_hook(uintptr_t *thiz, uintptr_t *a2) {
	ms_fTimeStep = *(float*)(g_libGTASA + 0x96B500);
	float_4DD9E8 = *(float*)(g_libGTASA + 0x4DD9E8);
	float_4DD9E8 = (fMagic) * (0.1f / ms_fTimeStep);
	CTaskSimpleUseGun__SetMoveAnim(thiz, a2);
}

int (*CAnimManager_UncompressAnimation)(int result);
int CAnimManager_UncompressAnimation_hook(int result) {
	if (result)
		return CAnimManager_UncompressAnimation(result);

	return 0;
}

void ReadSettingFile();
void ApplyFPSPatch(uint8_t fps);

void (*NvUtilInit)();
void NvUtilInit_hook()
{
#ifdef BR_DEBUG
    Log("NvUtilInit");
#endif

    NvUtilInit();

    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x6D687C : 0x8B46A8));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x6D687C : 0x8B46A8)), "/storage/emulated/0/Weik/");

    g_pszStorage = (char*)(g_libGTASA + (VER_x32 ? 0x6D687C : 0x8B46A8)); // StorageRootBuffer
    LOGI("Storage: %s", g_pszStorage);

    CSaaArchive::MountAll();

    ReadSettingFile();
}

struct stFile
{
    int isFileExist;
    FILE *f;
};

char lastFile[123];

// Прозрачный слой поверх .saa: если файла нет на диске, он ищется в архивах.
// Диск идёт первым намеренно — так одиночный файл всегда перебивает архив.
int (*OS_FileOpen_orig)(int area, void** out, const char* name, int access);

// В water.dat нового кеша 332 четырёхугольника, а игра держит m_aQuads[301]
// и m_aVertices[1021] — по четыре вершины на запись, то есть не больше 255.
// Лишние затирали память, и падало в AddWaterLevelVertex. Вода читается через
// CFileMgr::OpenFile, поэтому обрезать файл нечем — ограничиваем добавление.
static constexpr int32_t kMaxWaterQuads = 255;

struct CRenParRaw {
    float m_Z;
    float m_BigWaves;
    float m_SmallWaves;
    int8_t m_FlowX;
    int8_t m_FlowY;
    int8_t m_pad[2];
};

static_assert(sizeof(CRenParRaw) == 0x10, "CRenPar 16 байт");

static int32_t g_waterQuads = 0;

void (*CWaterLevel__AddWaterLevelQuad_orig)(int32_t, int32_t, CRenParRaw,
                                            int32_t, int32_t, CRenParRaw,
                                            int32_t, int32_t, CRenParRaw,
                                            int32_t, int32_t, CRenParRaw, uint32_t);

void CWaterLevel__AddWaterLevelQuad_hook(int32_t x1, int32_t y1, CRenParRaw p1,
                                         int32_t x2, int32_t y2, CRenParRaw p2,
                                         int32_t x3, int32_t y3, CRenParRaw p3,
                                         int32_t x4, int32_t y4, CRenParRaw p4, uint32_t flags)
{
    if (++g_waterQuads > kMaxWaterQuads) {
        if (g_waterQuads == kMaxWaterQuads + 1) {
            Log("water: сверх ёмкости, дальше четырёхугольники не добавляются");
        }

        return;
    }

    CWaterLevel__AddWaterLevelQuad_orig(x1, y1, p1, x2, y2, p2,
                                        x3, y3, p3, x4, y4, p4, flags);
}

void (*CWaterLevel__WaterLevelInitialise_orig)();

void CWaterLevel__WaterLevelInitialise_hook()
{
    g_waterQuads = 0;

    CWaterLevel__WaterLevelInitialise_orig();

    Log("water: принято %d четырёхугольников", g_waterQuads < kMaxWaterQuads ? g_waterQuads : kMaxWaterQuads);
}

// Диск важнее архива: одиночный файл всегда перебивает запись внутри.
// Полагаться на код возврата нельзя — для несуществующих образов
// OS_FileOpen отдаёт успех и хендл без открытого файла, поэтому наличие
// проверяется отдельно.
static bool ExistsInStorage(const char* name)
{
    if (!name || !*name) {
        return false;
    }

    if (name[0] == '/') {
        return access(name, R_OK) == 0;
    }

    if (!g_pszStorage) {
        return false;
    }

    char full[512];
    snprintf(full, sizeof(full), "%s%s", g_pszStorage, name);

    return access(full, R_OK) == 0;
}

int OS_FileOpen_hook(int area, void** out, const char* name, int access_)
{
    const char* archive = nullptr;
    uint32_t offset = 0, size = 0;

    if (name && !ExistsInStorage(name) && CSaaArchive::Find(name, archive, offset, size)) {
        const int inner = OS_FileOpen_orig(area, out, archive, access_);

        if (inner == 0 && out && *out) {
            // Игра сама умеет окно внутри файла: кладёт размер и базовое
            // смещение прямо в хендл
            CHook::CallFunction<void>("_Z16OS_FileSubHandlePvii", *out, (int)offset, (int)size);
            CSaaArchive::NoteWindow(*out, size);

            return 0;
        }

        Log("SaaArchive: %s найден в %s, но сам архив не открылся", name, archive);
    }

    return OS_FileOpen_orig(area, out, name, access_);
}

// Чтение, позиционирование и tell границы окна соблюдают, а OS_FileSize для
// файлов через stdio — нет: она делает fseek/ftell и отдаёт длину всего
// архива. Отсюда malloc на сотни мегабайт и падение на разборе texdb.
int (*OS_FileSize_orig)(void* file);

int OS_FileSize_hook(void* file)
{
    uint32_t size = 0;

    if (CSaaArchive::WindowSize(file, size)) {
        return (int)size;
    }

    return OS_FileSize_orig(file);
}

int (*OS_FileClose_orig)(void* file);

int OS_FileClose_hook(void* file)
{
    CSaaArchive::ForgetWindow(file);

    return OS_FileClose_orig(file);
}

// Часть текстовых данных в кеше blackrussia.online лежит зашифрованной:
// оба weapon.dat и все пять handling-*.cfg. Шифр — гамма восьмибитного
// сдвигового регистра, полином x^8+x^4+x^3+x^2+1 (маска 0x71), затравка 0x80.
// Ключ восстановлен по паре «открытый текст — шифр» для weapon.dat из старого
// кеша: файлы одного размера, и их сумма оказалась последовательностью
// состояний регистра.
namespace {

constexpr uint8_t kCryptTaps = 0x71;
constexpr uint8_t kCryptSeed = 0x80;

uint8_t CryptNext(uint8_t state) {
    uint8_t v = (uint8_t)(state & kCryptTaps);

    v ^= (uint8_t)(v >> 4);
    v ^= (uint8_t)(v >> 2);
    v ^= (uint8_t)(v >> 1);

    return (uint8_t)((state >> 1) | ((v & 1) << 7));
}

void CryptApply(uint8_t* data, size_t size) {
    uint8_t state = kCryptSeed;

    for (size_t i = 0; i < size; i++) {
        data[i] ^= state;
        state = CryptNext(state);
    }
}

double PrintableRatio(const uint8_t* data, size_t size) {
    if (!size) {
        return 0.0;
    }

    size_t good = 0;

    for (size_t i = 0; i < size; i++) {
        const uint8_t c = data[i];

        if ((c >= 32 && c < 127) || c == 9 || c == 10 || c == 13) {
            good++;
        }
    }

    return (double)good / (double)size;
}

bool NameMayBeCrypted(const char* path) {
    const size_t len = path ? strlen(path) : 0;

    if (len < 4) {
        return false;
    }

    return strcasecmp(path + len - 4, ".cfg") == 0
        || strcasecmp(path + len - 4, ".dat") == 0;
}

// Отдаёт поток с расшифрованным содержимым либо nullptr, если файл открытый
FILE* OpenDecrypted(FILE* f, const char* path) {
    constexpr long kMaxSize = 4 * 1024 * 1024;

    if (!NameMayBeCrypted(path)) {
        return nullptr;
    }

    fseek(f, 0, SEEK_END);
    const long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0 || size > kMaxSize) {
        return nullptr;
    }

    auto* data = (uint8_t*)malloc((size_t)size);
    if (!data) {
        return nullptr;
    }

    if (fread(data, 1, (size_t)size, f) != (size_t)size) {
        free(data);
        return nullptr;
    }

    fseek(f, 0, SEEK_SET);

    const size_t probe = (size_t)(size < 512 ? size : 512);

    if (PrintableRatio(data, probe) > 0.5) {
        free(data);
        return nullptr;
    }

    CryptApply(data, (size_t)size);

    if (PrintableRatio(data, probe) < 0.9) {
        free(data);
        return nullptr;
    }

    FILE* mem = fmemopen(data, (size_t)size, "rb");
    if (!mem) {
        free(data);
        return nullptr;
    }

    Log("Расшифрован %s (%ld байт)", path, size);

    return mem;
}

} // namespace

stFile* (* NvFOpen_orig)(const char*, const char*, bool, bool);
stFile* NvFOpen(const char* r0, const char* r1, int r2, int r3)
{
    strcpy(lastFile, r1);

    static char path[255]{};
    memset(path, 0, sizeof(path));

    sprintf(path, "%s%s", g_pszStorage, r1);

    // ----------------------------
    if(!strncmp(r1+12, "mainV1.scm", 10))
    {
        sprintf(path, "%sSAMP/main.scm", g_pszStorage);
        Log("Loading %s", path);
    }
    // ----------------------------
    if(!strncmp(r1+12, "SCRIPTV1.IMG", 12))
    {
        sprintf(path, "%sSAMP/script.img", g_pszStorage);
        Log("Loading script.img..");
    }
    // ----------------------------
    if(!strncmp(r1, "DATA/PEDS.IDE", 13))
    {
        sprintf(path, "%sSAMP/peds.ide", g_pszStorage);
        Log("Loading peds.ide..");
    }
    // ----------------------------
    if(!strncmp(r1, "DATA/VEHICLES.IDE", 17))
    {
        sprintf(path, "%sSAMP/vehicles.ide", g_pszStorage);
        Log("Loading vehicles.ide..");
    }

    if (!strncmp(r1, "DATA/GTA.DAT", 12))
    {
        sprintf(path, "%sSAMP/gta.dat", g_pszStorage);
        Log("Loading gta.dat..");
    }

    if (!strncmp(r1, "DATA/HANDLING.CFG", 17))
    {
        sprintf(path, "%sSAMP/handling.cfg", g_pszStorage);
        Log("Loading handling.cfg..");
    }
    if (!strncmp(r1, "DATA/HANDLING-COMFORT.CFG", strlen("DATA/HANDLING-COMFORT.CFG")))
    {
        sprintf(path, "%sSAMP/handling-comfort.cfg", g_pszStorage);
        Log("Loading handling-comfort.cfg..");
    }
    if (!strncmp(r1, "DATA/HANDLING-DRIFT.CFG", strlen("DATA/HANDLING-DRIFT.CFG")))
    {
        sprintf(path, "%sSAMP/handling-drift.cfg", g_pszStorage);
        Log("Loading handling-drift.cfg..");
    }
    if (!strncmp(r1, "DATA/HANDLING-SPORT.CFG", strlen("DATA/HANDLING-SPORT.CFG")))
    {
        sprintf(path, "%sSAMP/handling-sport.cfg", g_pszStorage);
        Log("Loading handling-sport.cfg..");
    }
    if (!strncmp(r1, "DATA/HANDLING-PLUS.CFG", strlen("DATA/HANDLING-PLUS.CFG")))
    {
        sprintf(path, "%sSAMP/handling-plus.cfg", g_pszStorage);
        Log("Loading handling-plus.cfg..");
    }

    if (!strncmp(r1, "DATA/WEAPON.DAT", 15))
    {
        sprintf(path, "%sSAMP/weapon.dat", g_pszStorage);
        Log("Loading weapon.dat..");
    }

    if (!strncmp(r1, "DATA/FONTS.DAT", 15))
    {
        sprintf(path, "%sdata/fonts.dat", g_pszStorage);
        Log("Loading fonts.dat..");
    }

    if (!strncmp(r1, "DATA/PEDSTATS.DAT", 15))
    {
        sprintf(path, "%sdata/pedstats.dat", g_pszStorage);
        Log("Loading pedstats.dat..");
    }

    if (!strncmp(r1, "DATA/TIMECYC.DAT", 15))
    {
        // У нового кеша свой цикл времени в SAMP, у старого только стоковый в data
        sprintf(path, "%sSAMP/TIMECYC.DAT", g_pszStorage);

        if (FILE* probe = fopen(path, "rb")) {
            fclose(probe);
        } else {
            sprintf(path, "%sdata/timecyc.dat", g_pszStorage);
        }

        Log("Loading %s", path);
    }

    if (!strncmp(r1, "DATA/POPCYCLE.DAT", 15))
    {
        sprintf(path, "%sdata/popcycle.dat", g_pszStorage);
        Log("Loading popcycle.dat..");
    }

    /*if (strstr(r1, "TEXT/") || strstr(r1, "TEXTURES/"))
    {
        Log("Load TEXT/ and TEXTURES/ from APK..");
        return NvFOpen_orig(r0, r1, r2, r3);
    }*/

    if (!strncmp(r1, "MINFO.BIN", 9) || !strncmp(r1, "CINFO.BIN", 9)) {
        Log("Try to load .BIN files, skip..");
        return nullptr;
    }

#if VER_x32
    auto *st = (stFile*)malloc(8);
#else
    auto *st = (stFile*)malloc(0x10);
#endif
    st->isFileExist = false;

    Log("Open: %s", path);
    FILE *f  = fopen(path, "rb");

    if(f)
    {
        if (FILE* dec = OpenDecrypted(f, path)) {
            fclose(f);
            f = dec;
        }

        st->isFileExist = true;
        st->f = f;
        return st;
    }
    else
    {
        Log("Error: file not found (%s)", path);
        free(st);
        return nullptr;
    }
}

bool g_bPlaySAMP = false;
void MainMenu_OnStartSAMP()
{
    if(g_bPlaySAMP)
        return;

    pGame->StartGame();

    // StartGameScreen::OnNewGameCheck()
    (( void (*)())(g_libGTASA + (VER_x32 ? 0x002A7270 + 1 : 0x365EA0)))();

    g_bPlaySAMP = true;
}

unsigned int (*MainMenuScreen__Update)(uintptr_t thiz, float a2);
unsigned int MainMenuScreen__Update_hook(uintptr_t thiz, float a2)
{
    unsigned int ret = MainMenuScreen__Update(thiz, a2);
    MainMenu_OnStartSAMP();
    return ret;
}

void (*StartGameScreen__OnNewGameCheck)();
void StartGameScreen__OnNewGameCheck_hook()
{
    if(g_bPlaySAMP)
        return;

    StartGameScreen__OnNewGameCheck();
}

void (*CTaskSimpleUseGun__RemoveStanceAnims)(uintptr* thiz, void* ped, float a3);
void CTaskSimpleUseGun__RemoveStanceAnims_hook(uintptr* thiz, void* ped, float a3)
{
    if(!thiz)
        return;

    uintptr* m_pAnim = (uintptr*)(thiz + 0x2c);
    if(m_pAnim) {
        if (!((uintptr *)(m_pAnim + 0x14)))
            return;
    }
    CTaskSimpleUseGun__RemoveStanceAnims(thiz, ped, a3);
}

int (*CCollision__ProcessVerticalLine)(float *a1, float *a2, int a3, int a4, int *a5, int a6, int a7, int a8);
int CCollision__ProcessVerticalLine_hook(float *a1, float *a2, int a3, int a4, int *a5, int a6, int a7, int a8)
{
    int result;

    if (a3)
        result = CCollision__ProcessVerticalLine(a1, a2, a3, a4, a5, a6, a7, a8);
    else
        result = 0;
    return result;
}

int(*CUpsideDownCarCheck__IsCarUpsideDown)(int, int);
int CUpsideDownCarCheck__IsCarUpsideDown_hook(int a1, int a2)
{
    return 0;
}

int (*CTaskSimpleGetUp__ProcessPed)(uintptr_t* thiz, CPedGTA* ped);
int CTaskSimpleGetUp__ProcessPed_hook(uintptr_t* thiz, CPedGTA* ped)
{
    if(!ped)
        return 0;
    int res = 0;

    try {
        res = CTaskSimpleGetUp__ProcessPed(thiz, ped);
    }
    catch(...) {
        return 0;
    }

    return res;
}

uint32 getmip() {
    return 1;
}

void RQCommand_rqSetAlphaTest(uint8_t **qData) {
    *qData += 8; // next command
}

int GetInputType() {
    return 0;
}

long long (*CAnimBlendNode__FindKeyFrame)(CAnimBlendNode *thiz, float fCurrentTime);
long long CAnimBlendNode__FindKeyFrame_hook(CAnimBlendNode *thiz, float fCurrentTime)
{
	if (thiz->m_pAnimSequence)
	{
		return CAnimBlendNode__FindKeyFrame(thiz, fCurrentTime);
	}
	else return 0;
}

RwFrame* CClumpModelInfo_GetFrameFromId_Post(RwFrame* pFrameResult, RpClump* clump, int32 modelId) {
    if (pFrameResult)
        return pFrameResult;

    uintptr_t calledFrom = 0;
    GET_LR(calledFrom);

    if (calledFrom == (VER_x32 ? 0x0058C61C : 0x6AFEE4)                // CVehicle::SetWindowOpenFlag
        || calledFrom == (VER_x32 ? 0x0058C644 : 0x6AFF18)             // CVehicle::ClearWindowOpenFlag
        || calledFrom == (VER_x32 ? 0x003885EC : 0x45FC40)             // CVehicleModelInfo::GetOriginalCompPosition
        || calledFrom == (VER_x32 ? 0x00387A28 : 0x45ECD0))            // CVehicleModelInfo::CreateInstance
        return nullptr;

    for (uint i = 2; i < 40; i++) {
        RwFrame* pNewFrameResult;
        pNewFrameResult = CHook::CallFunction<RwFrame*>("_ZN15CClumpModelInfo14GetFrameFromIdEP7RpClumpi", clump, i);
        if (pNewFrameResult) {
            return pNewFrameResult;
        }
    }
    return nullptr;
}

RwFrame* (*CClumpModelInfo_GetFrameFromId)(RpClump*, int32);
RwFrame* CClumpModelInfo_GetFrameFromId_hook(RpClump* clump, int32 modelId) {
    return CClumpModelInfo_GetFrameFromId_Post(CClumpModelInfo_GetFrameFromId(clump, modelId), clump, modelId);
}

void (*FxEmitterBP_c__Render)(uintptr_t* a1, int a2, int a3, float a4, char a5);
void FxEmitterBP_c__Render_hook(uintptr_t* a1, int a2, int a3, float a4, char a5)
{
    if(!a1 || !a2) return;
    uintptr_t* temp = *((uintptr_t**)a1 + 3);
    if (!temp)
    {
        return;
    }
    FxEmitterBP_c__Render(a1, a2, a3, a4, a5);
}

bool (*RwResourcesFreeResEntry)(void* entry);
bool RwResourcesFreeResEntry_hook(void* entry)
{
    bool result;
    if (entry) result = RwResourcesFreeResEntry(entry);
    else result = false;
    return result;
}

int(*mpg123_param)(void* mh, int key, long val, int ZERO, double fval);
int mpg123_param_hook(void* mh, int key, long val, int ZERO, double fval)
{
    // 0x2000 = MPG123_SKIP_ID3V2
    // 0x200  = MPG123_FUZZY
    // 0x100  = MPG123_SEEKBUFFER
    // 0x40   = MPG123_GAPLESS
    return mpg123_param(mh, key, val | (0x2000 | 0x200 | 0x100 | 0x40), ZERO, fval);
}

CPlayerInfoGta* (* GetPlayerInfoForThisPlayerPed)(CPedGTA* ped);
CPlayerInfoGta* GetPlayerInfoForThisPlayerPed_hook(CPedGTA* ped)
{
    uintptr_t dwRetAddr = 0;
    GET_LR(dwRetAddr);

    if (dwRetAddr == (VER_x32 ? 0x4E45CC:0x00000000005E7FE0)
        || dwRetAddr == (VER_x32 ? 0x4E4FE8:0x00000000005E8C58)) {
        LOGI("GetPlayerInfoForThisPlayerPed called from 0x%X", dwRetAddr);
        return reinterpret_cast<CPlayerInfoGta*>(GamePool_FindPlayerPed());
    }

    return GetPlayerInfoForThisPlayerPed(ped);
}

static CVehicle* g_pCurrentPreRenderVehicle = nullptr;
static int g_iLastProcessedWheelNode = -1;

// Оригинал сам ставит масштаб и поворот каждому колесу. Мы перехватываем ровно
// эти два вызова и подменяем аргументы: так развал, ширина и радиус попадают в
// ту же матрицу, которую игра тут же применяет.
void (*CMatrix__Rotate)(CMatrix* thiz, float x, float y, float z);
void CMatrix__Rotate_hook(CMatrix* thiz, float x, float y, float z)
{
    CMatrix__Rotate(thiz, x, y, z);

    if (!g_pCurrentPreRenderVehicle || g_iLastProcessedWheelNode == -1) {
        return;
    }

    switch (g_iLastProcessedWheelNode) {
        case CAR_WHEEL_LF:
            thiz->RotateY(g_pCurrentPreRenderVehicle->m_fWheelAngleFront);
            break;
        case CAR_WHEEL_RF:
            thiz->RotateY(-g_pCurrentPreRenderVehicle->m_fWheelAngleFront);
            break;
        case CAR_WHEEL_RB:
            thiz->RotateY(-g_pCurrentPreRenderVehicle->m_fWheelAngleBack);
            break;
        case CAR_WHEEL_LB:
            thiz->RotateY(g_pCurrentPreRenderVehicle->m_fWheelAngleBack);
            break;
        default:
            break;
    }
}

void (*CMatrix__SetScale)(CMatrix* thiz, float x, float y, float z);
void CMatrix__SetScale_hook(CMatrix* thiz, float x, float y, float z)
{
    auto vehicleMP = g_pCurrentPreRenderVehicle;
    const int node = g_iLastProcessedWheelNode;

    if (vehicleMP && node != -1) {
        if (node == CAR_WHEEL_LF || node == CAR_WHEEL_RF) {
            const float factor = vehicleMP->m_fWheelScaleFront - vehicleMP->m_fDefaultWheelSizeFront;

            y = factor + vehicleMP->m_pVehicle->m_wheelScale;
            z = factor + vehicleMP->m_pVehicle->m_wheelScale;

            if (vehicleMP->m_fWheelWidthFront != 0.0f) {
                x = vehicleMP->m_fWheelWidthFront;
            }
        }

        if (node == CAR_WHEEL_LB || node == CAR_WHEEL_RB || node == CAR_WHEEL_LM || node == CAR_WHEEL_RM) {
            const float factor = vehicleMP->m_fWheelScaleRear - vehicleMP->m_fDefaultWheelSizeRear;

            y = factor + vehicleMP->m_pVehicle->m_wheelScale;
            z = factor + vehicleMP->m_pVehicle->m_wheelScale;

            if (vehicleMP->m_fWheelWidthBack != 0.0f) {
                x = vehicleMP->m_fWheelWidthBack;
            }
        }
    }

    CMatrix__SetScale(thiz, x, y, z);
}

void (*CAutomobile__UpdateWheelMatrix)(CAutomobile* thiz, int nodeIndex, int flags);
void CAutomobile__UpdateWheelMatrix_hook(CAutomobile* thiz, int nodeIndex, int flags)
{
    if (g_pCurrentPreRenderVehicle) {
        g_iLastProcessedWheelNode = nodeIndex;
    }

    CAutomobile__UpdateWheelMatrix(thiz, nodeIndex, flags);

    g_iLastProcessedWheelNode = -1;
}

void (*CAutomobile__PreRender)(CAutomobile* thiz);
void CAutomobile__PreRender_hook(CAutomobile* thiz)
{
	if(!thiz->IsSubAutomobile()) {
		CAutomobile__PreRender(thiz);
		return;
	}

    auto vehicleMP = thiz->m_pVehicleMP;
    if (vehicleMP) {
        g_pCurrentPreRenderVehicle = vehicleMP;

        if (vehicleMP->HasNeon()) {
            vehicleMP->neon.Render(vehicleMP->m_pVehicle);
        }

        vehicleMP->ProcessWheelsOffset();
        vehicleMP->RenderTurnLights();
    }

	CAutomobile__PreRender(thiz);

	g_pCurrentPreRenderVehicle = nullptr;
	g_iLastProcessedWheelNode = -1;
}

// Подвеска считается по общим на модель линиям из CColModel, поэтому на время
// расчёта столкновений машине подставляются её собственные, под её радиус колеса
int32_t (*CAutomobile__ProcessEntityCollision)(CAutomobile* thiz, CEntityGTA* pEntity, CColPoint* pColPoints);
int32_t CAutomobile__ProcessEntityCollision_hook(CAutomobile* thiz, CEntityGTA* pEntity, CColPoint* pColPoints)
{
    CCollisionData* pColData = nullptr;
    CColLine* pGlobalLines = nullptr;

    auto vehicleMP = thiz->m_pVehicleMP;
    if (vehicleMP && vehicleMP->bHasSuspensionLines && vehicleMP->m_pSuspensionLines
        && vehicleMP->GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR) {

        pColData = GetCollisionDataFromModel(thiz->m_nModelIndex);
        if (pColData && pColData->m_pLines
            && vehicleMP->m_nSuspensionLinesCopied >= pColData->m_nNumLines) {
            pGlobalLines = pColData->m_pLines;
            pColData->m_pLines = vehicleMP->m_pSuspensionLines;
        }
    }

    auto result = CAutomobile__ProcessEntityCollision(thiz, pEntity, pColPoints);

    if (pGlobalLines) {
        pColData->m_pLines = pGlobalLines;
    }

    return result;
}

// Снимок заводского хендлинга снимается до перевода в игровые единицы
void (*cHandlingDataMgr__ConvertDataToGameUnits)(void* thiz, tHandlingData* handling);
void cHandlingDataMgr__ConvertDataToGameUnits_hook(void* thiz, tHandlingData* handling)
{
    CHandlingDefault::ms_pManager = thiz;

    if (!CHandlingDefault::ms_bConvertingOwn) {
        CHandlingDefault::Store(handling);
    }

    cHandlingDataMgr__ConvertDataToGameUnits(thiz, handling);
}

#if VER_x32
int (*emu_ArraysGetID)(unsigned int a1);
int emu_ArraysGetID_hook(unsigned int a1)
{
    if(!a1 || !*(uint32_t*)(a1+(VER_x32 ? 36:48))) return 0;
   return *(uint32_t*)(a1+36);
}
#else
long (*emu_ArraysGetID)(unsigned long a1);
long emu_ArraysGetID_hook(unsigned long a1)
{
    if(!a1 || !*(uint64_t*)(a1+(VER_x32 ? 36:48))) return 0;
    return *(uint64_t*)(a1+48);
}
#endif

#include "../graphics/CSkyBox.h"
#include "CGraphicsSettings.h"
#include "Mirrors.h"
void (*CClouds__Render)();
void CClouds__Render_hook() {
    CWaterFX::Update(CTimer::GetTimeStepInSeconds());

    if (CSkyBox::ms_bEnabled) {
        CSkyBox::Process(false);
    } else {
        CClouds__Render();
    }

    CWaterFX::CaptureFogMap();
}

extern PAD_KEYS LocalPlayerKeys;
void (*CCam__Process)(CCam* thiz);
void CCam__Process_hook(CCam* thiz)
{
    if (CFirstPersonCamera::IsSuspended()) {
        CCam__Process(thiz);
        return;
    }

    CVehicleGTA* veh = nullptr;
    CVector vecSpeed;
    float& CAR_FOV_START_SPEED = *(float*)(g_libGTASA + (VER_x32 ? 0x006A9FD0 : 0x8855D4));
    const float fSavedFovStart = CAR_FOV_START_SPEED;

    if (pNetGame && (thiz->m_nMode == MODE_CAM_ON_A_STRING || thiz->m_nMode == MODE_1STPERSON))
    {
        auto pLocal = pNetGame->GetPlayerPool()->GetLocalPlayer();
        auto ped = pLocal ? pLocal->GetPlayerPed()->m_pPed : nullptr;

        if (ped && ped->IsInVehicle() && ped->pVehicle)
        {
            veh = ped->pVehicle;
            vecSpeed = veh->m_vecMoveSpeed;
            veh->m_vecMoveSpeed *= 6.0f;
            CAR_FOV_START_SPEED = 0.4f;
        }
    }

    CCam__Process(thiz);

    if (veh)
    {
        veh->m_vecMoveSpeed = vecSpeed;
        CAR_FOV_START_SPEED = fSavedFovStart;
    }

    auto pLocalPlayer = pNetGame ? pNetGame->GetPlayerPool()->GetLocalPlayer() : nullptr;
    CPlayerPed* pPed = pLocalPlayer ? pLocalPlayer->GetPlayerPed() : nullptr;
    if (!pPed || !pPed->m_pPed) {
        return;
    }

    CCamera::Get().m_uiTransitionDuration = 0xFFFFFFFF;
    CCamera::Get().m_uiTransitionDurationTargetCoors = 0xFFFFFFFF;
    CCamera::Get().m_bJust_Switched = false;

    if (pPed->m_pPed->IsInVehicle() && pPed->m_pPed->pVehicle)
    {
        if (thiz->m_nMode == MODE_CAM_ON_A_STRING || thiz->m_nMode == MODE_1STPERSON) {
            CFirstPersonCamera::ProcessCameraInVeh(thiz, pPed, pPed->m_pPed->pVehicle);
        }
    }
    else if (thiz->m_nMode != MODE_1STPERSON &&
             thiz->m_nMode != MODE_SNIPER &&
             thiz->m_nMode != MODE_ROCKETLAUNCHER &&
             thiz->m_nMode != MODE_M16_1STPERSON &&
             thiz->m_nMode != MODE_CAMERA)
    {
        CFirstPersonCamera::ProcessCameraOnFoot(thiz, pPed);
    }
}

bool (*CCamera__Using1stPersonWeaponMode)(CCamera *thiz);
bool CCamera__Using1stPersonWeaponMode_hook(CCamera *thiz)
{
    if (CFirstPersonCamera::m_bEnabled) {
        return true;
    }

    return CCamera__Using1stPersonWeaponMode(thiz);
}

int g_iCameraTypeSwitch = 0;

bool (*CPad__CycleCameraModeDownJustDown)(void*);
bool CPad__CycleCameraModeDownJustDown_hook(void* thiz) {
	CPedGTA* pPed = GamePool_FindPlayerPed();
	if (!pPed) {
		return 0;
	}

	if (pPed->bInVehicle)
	{
		if (g_iCameraTypeSwitch == 1) {
			CFirstPersonCamera::m_bEnabled = true;
			return false;
		} else if (g_iCameraTypeSwitch >= 7) {
			g_iCameraTypeSwitch = 0;
            CFirstPersonCamera::m_bEnabled = false;
			return true;
		} else {
            CFirstPersonCamera::m_bEnabled = false;
		}
	} else {
        if(g_iCameraTypeSwitch = 2) {
            CFirstPersonCamera::m_bEnabled = !CFirstPersonCamera::m_bEnabled;
        }
    }

	return false;
}

void (*CDraw__SetFOV)(float thiz, float a2);
void CDraw__SetFOV_hook(float thiz, float a2)
{
	float tmp = (float)((float)((float)(*(float *)&*(float *)(g_libGTASA + (VER_x32 ? 0x00A26A90 : 0xCC7F00)) - 1.3333) * 11.0) / 0.44444) + thiz;

    if(CFirstPersonCamera::m_bEnabled)
        tmp = 110.0;
    else if(tmp > 100)
        tmp = 100.0;

	*(float *)(g_libGTASA + (VER_x32 ? 0x006B1CB8 : 0x88E6BC)) = tmp;
}

int g_iLastRenderedObject;
void (*CEntity_Render)(CEntityGTA* pEntity);
void CEntity_Render_hook(CEntityGTA* pEntity)
{
    g_iLastRenderedObject = pEntity->GetModelId();

    CEntity_Render(pEntity);
}

int (*CRadar__DrawRadarSprite)(unsigned int iconID, float x, float y, unsigned int alpha);
int CRadar__DrawRadarSprite_hook(unsigned int iconID, float x, float y, unsigned int alpha) {
	if(iconID == 4)
		return 0;

	return CRadar__DrawRadarSprite(iconID, x, y, alpha);
}
static bool onceRadar = false;

#include "Widgets/TouchInterface.h"
#include "java/GUIManager.h"
#include "java/jniutil.h"
#include "CustomRadar.h"
void(*CHud__DrawRadar)();
void CHud__DrawRadar_hook() {
	float* thiz = (float*) CTouchInterface::m_pWidgets[WIDGET_RADAR];
	if (thiz && !onceRadar) {
#if VER_x32
        thiz[3] = CJavaWrapper::radarPos.x;
        thiz[4] = CJavaWrapper::radarPos.y;

		thiz[5] = CJavaWrapper::radarSize.x;
		thiz[6] = CJavaWrapper::radarSize.x;
#else
        thiz[6] = CJavaWrapper::radarPos.x;
        thiz[7] = CJavaWrapper::radarPos.y;

        thiz[8] = CJavaWrapper::radarSize.x;
		thiz[9] = CJavaWrapper::radarSize.x;
#endif
        onceRadar = true;
	}

    if(onceRadar && !pUI->m_bCamEditGui /*&& !CJavaWrapper::GUIDialogShow */&& pNetGame->GetGameState() == GAMESTATE_CONNECTED
    && GUIManager::drawHud && !pNetGame->GetTextDrawPool()->GetState()) {
        CustomRadar::Render();
    }

    if(pUI->m_bCamEditGui) {
        return;
    }

	CHud__DrawRadar();
}

#include "Widgets/TouchInterface.h"
#include "game/Widgets/WidgetRegion.h"
#include "game/Widgets/WidgetButton.h"
#include "Shadows.h"
#include "Weather.h"
#include "Clock.h"
#include "RQShader.h"
#include "RenderBuffer.h"
#include "Mobile/MobileMenu/MobileMenu.h"
#include "Draw.h"
#include "PathFind.h"
#include "Mirrors.h"

inline void* TryLoadAlphaFunc(const char* szlib) {
    void* lib = dlopen(szlib, RTLD_LAZY);
    if(!lib)
        return NULL;

    void* sym = dlsym(lib, "glAlphaFuncQCOM");
    if(!sym)
        sym = dlsym(lib, "glAlphaFunc");

    return sym;
}

void InjectHooks()
{
#ifdef BR_DEBUG
    Log("InjectHooks");
#endif

    // new 09.03.2025
    CIplStore::InjectHooks();
    CQuadTreeNode::InjectHooks();

    CHook::Write(g_libGTASA + (VER_x32 ? 0x678954 : 0x84F2D0), &Scene);

    CHook::Write(g_libGTASA + (VER_x32 ? 0x00679A3C : 0x851498), &CMobileMenu::ms_MobileSettings);

#if !VER_x32 // mb all.. wtf crash x64?
    CHook::RET("_ZN11CPlayerInfo14LoadPlayerSkinEv");
    CHook::RET("_ZN11CPopulation10InitialiseEv");
#endif

    CCustomCarEnvMapPipeline::InjectHooks();
    CCamera::InjectHooks();
    CReferences::InjectHooks();
    CModelInfo::injectHooks();
    CTimer::InjectHooks();
    // cTransmission::InjectHooks();
    CAnimBlendAssociation::InjectHooks();
    // cHandlingDataMgr::InjectHooks();
    CPools::InjectHooks();
    CVehicleGTA::InjectHooks();
    CMatrixLink::InjectHooks();
    CMatrixLinkList::InjectHooks();
    CStreaming::InjectHooks();
    CPlaceable::InjectHooks();
    CMatrix::InjectHooks();
    CCollision::InjectHooks();
    // CIdleCam::InjectHooks();
    CTouchInterface::InjectHooks();
    CWidgetGta::InjectHooks();
    CEntityGTA::InjectHooks();
    CPhysical::InjectHooks();
    CAnimManager::InjectHooks();
    // CCarEnterExit::InjectHooks();
    CPlayerPedGta::InjectHooks();
    CTaskManager::InjectHooks();
    // CPedIntelligence::InjectHooks();
    CWorld::InjectHooks();
    CGame::InjectHooks();
    ES2VertexBuffer::InjectHooks();
    // CRQ_Commands::InjectHooks();
    CTxdStore::InjectHooks();
    CVisibilityPlugins::InjectHooks();
    // CAdjustableHUD::InjectHooks();
    CWidgetRegion::InjectHooks();
    CWidgetButton::InjectHooks();
    // CClouds::InjectHooks();
    CWeather::InjectHooks();
    RenderBuffer::InjectHooks();
    RQShader::InjectHooks(); // new
    CWaterFX::InjectHooks();
    CFrameRateFixes::InjectHooks();
    CTimeCycle::InjectHooks();
    CCoronas::InjectHooks();
    CDraw::InjectHooks();
    CClock::InjectHooks();
    // CBirds::Init();
    CPathFind::InjectHooks();
    CVehicleModelInfo::InjectHooks();
    CSprite2d::InjectHooks();
    // CFileLoader::InjectHooks();
    CShadows::InjectHooks();
    CPickups::InjectHooks();
    CRenderer::InjectHooks();
    CStreamingInfo::InjectHooks();
    TextureDatabase::InjectHooks();
    TextureDatabaseEntry::InjectHooks();
    TextureDatabaseRuntime::InjectHooks();
    CCustomBuildingDNPipeline::InjectHooks();
    // CWidgetRadar::InjectHooks();
    CRealTimeShadowManager::InjectHooks();

    CMirrors::InjectHooks();
    CFirstPersonCamera::InjectHooks();
}

static std::vector<pthread_t> g_threads;
static std::mutex g_threadsMutex;

void addThreadToGlobalList(pthread_t thread, bool joinable) {
    if (joinable) {
        std::lock_guard<std::mutex> lock(g_threadsMutex);
        g_threads.push_back(thread);
    }
}

void cleanupAllThreads() {
    std::lock_guard<std::mutex> lock(g_threadsMutex);
    for (pthread_t thread : g_threads) {
        pthread_join(thread, nullptr);
    }
    g_threads.clear();
}

#include <pthread.h>
#include <unistd.h>
#include <cstring>
#include <sys/resource.h>
#include <cstdlib>

typedef pthread_t OSThreadHandle;
typedef void* (*OSThreadFunction)(void*);

typedef enum {
    THREAD_RENDER_QUEUE,
    THREAD_CD_STREAM,
    THREAD_MAIN,
    THREAD_BANK_LOADER,
    THREAD_STREAM_RADIO,
    THREAD_DEFAULT
} CustomThreadType;

typedef struct {
    const char* threadNamePattern;
    int priority;
    size_t stackSizeKB;
} ThreadConfig;

static const ThreadConfig threadConfigs[] = {
        [THREAD_RENDER_QUEUE]  = { "RenderQueue", -20, 512 },
        [THREAD_CD_STREAM]     = { "CdStream",    -20, 512 },
        [THREAD_MAIN]          = { "MainThread",  -15, 256 },
        [THREAD_BANK_LOADER]   = { "BankLoader",   -5, 128 },
        [THREAD_STREAM_RADIO]  = { "StreamThread",  0, 128 },
        [THREAD_DEFAULT]       = { NULL,            0, 256 }
};

static CustomThreadType getThreadType(const char* threadName) {
    if (!threadName) return THREAD_DEFAULT;

    for (int i = 0; i < sizeof(threadConfigs)/sizeof(ThreadConfig); i++) {
        if (threadConfigs[i].threadNamePattern &&
            strstr(threadName, threadConfigs[i].threadNamePattern)) {
            return (CustomThreadType)i;
        }
    }
    return THREAD_DEFAULT;
}

struct ThreadLaunchData {
    void* thread_struct;
    OSThreadFunction func;
    char thread_name[32];
};

OSThreadHandle (*orig_OS_ThreadLaunch)(
        OSThreadFunction function,
        void* functionData,
        unsigned int processorAffinity,
        const char* threadName,
        int joinable,
        uintptr_t priority);
OSThreadHandle custom_OS_ThreadLaunch(
        OSThreadFunction function,
        void* functionData,
        unsigned int processorAffinity,
        const char* threadName,
        int joinable,
        uintptr_t priority) {

    CustomThreadType threadType = getThreadType(threadName);
    const ThreadConfig* config = &threadConfigs[threadType];

#if VER_x32
    if (threadType == THREAD_MAIN) {
        return orig_OS_ThreadLaunch(function, functionData, processorAffinity, threadName, joinable, priority);
    }
#endif

    auto* threadData = (ThreadLaunchData*)malloc(sizeof(ThreadLaunchData));
    if (!threadData) return static_cast<OSThreadHandle>(NULL);

    threadData->func = function;
    threadData->thread_struct = functionData;

    if (threadName) {
        strncpy(threadData->thread_name, threadName, sizeof(threadData->thread_name)-1);
        threadData->thread_name[sizeof(threadData->thread_name)-1] = '\0';
    } else {
        strcpy(threadData->thread_name, "AppThread");
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    struct rlimit rlim{};
    getrlimit(RLIMIT_STACK, &rlim);

    size_t safe_stack = (rlim.rlim_cur == RLIM_INFINITY)
                        ? config->stackSizeKB * 1024
                        : std::min(config->stackSizeKB * 1024, (size_t)rlim.rlim_cur);

    struct sched_param param = {0};
    pthread_attr_getschedparam(&attr, &param);

    int priority_min = sched_get_priority_min(SCHED_OTHER);
    int priority_max = sched_get_priority_max(SCHED_OTHER);
    int range = priority_max - priority_min;

    switch (priority) {
        case 0:
            param.sched_priority = priority_min;
            break;
        case 1:
            param.sched_priority = priority_min + (2 * range) / 3;
            break;
        case 2:
            param.sched_priority = priority_min + (4 * range) / 5;
            break;
        case 3:
            param.sched_priority = priority_max;
            break;
        default:
            param.sched_priority = 0;
            break;
    }

    pthread_t thread;
    if (pthread_create(&thread, &attr, CJavaWrapper::NVThreadSpawnProc, threadData) != 0) {
        free(threadData);
        pthread_attr_destroy(&attr);
        return static_cast<OSThreadHandle>(NULL);
    }

    addThreadToGlobalList(thread, joinable);

    if (threadName) {
        pthread_setname_np(thread, threadName);
    }

    if (!joinable) {
        pthread_detach(thread);
    }

    pthread_attr_destroy(&attr);
    return thread;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wn_blackrussia_game_Multiplayer_jniDestroy(JNIEnv *env, jobject thiz) {
    cleanupAllThreads();
}

void RenderScene()
{
    const bool underWater = CWeather::UnderWaterness <= 0.0f;

    if (!CMirrors::TypeOfMirror) {
        CWaterFX::Update(CTimer::GetTimeStepInSeconds());
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RWRSTATE(NULL));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(FALSE));

    if (!CMirrors::TypeOfMirror) {
        CHook::CallFunction<void>("_ZN13CMovingThings19Render_BeforeCloudsEv"); // CMovingThings::Render_BeforeClouds();
        CHook::CallFunction<void>("_ZN7CClouds6RenderEv"); // CClouds::Render();
    }

    if (CSkyBox::ms_bEnabled) {
        CSkyBox::Process(CMirrors::TypeOfMirror != 0);
    } else {
        CClouds__Render();
    }

    if (!CMirrors::TypeOfMirror) {
        CWaterFX::CaptureFogMap();
    }

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESHADEMODE, RWRSTATE(rwSHADEMODEGOURAUD));

    if (CMirrors::TypeOfMirror != MIRROR_TYPE_SPHERE_MAP)
        CHook::CallFunction<void>("_ZN14CCarFXRenderer15PreRenderUpdateEv");

    CRenderer::RenderRoads();

    if (CMirrors::TypeOfMirror != MIRROR_TYPE_SPHERE_MAP)
        CCoronas::RenderReflections();

    CRenderer::RenderEverythingBarRoads();

    if (CMirrors::TypeOfMirror != MIRROR_TYPE_SPHERE_MAP) {
        CRenderer::RenderFadingInUnderwaterEntities();
        if (underWater) {
            RwRenderStateSet(rwRENDERSTATECULLMODE, RWRSTATE(rwCULLMODECULLNONE));
            CWaterFX::CaptureRefractionMap();
            CHook::CallFunction<void>("_ZN11CWaterLevel11RenderWaterEv"); // CWaterLevel::RenderWater();
            RwRenderStateSet(rwRENDERSTATECULLMODE, RWRSTATE(rwCULLMODECULLBACK));
        }
    }

    if (CMirrors::TypeOfMirror != MIRROR_TYPE_SPHERE_MAP || CMobileMenu::ms_MobileSettings[MS_CarReflections].value == 3)
        CRenderer::RenderFadingInEntities();

    if (CMirrors::TypeOfMirror != MIRROR_TYPE_SPHERE_MAP)
        CHook::CallFunction<void>("_ZN9CPlantMgr6RenderEv");

    RwRenderStateSet(rwRENDERSTATECULLMODE, RWRSTATE(rwCULLMODECULLNONE));
    if (!CMirrors::TypeOfMirror) {
        // CClouds::RenderBottomFromHeight();
        CWeather::RenderRainStreaks();

        if (CMobileMenu::ms_MobileSettings[MS_Visuals].value == 3)
            CCoronas::RenderSunReflection();
    }

    if (!underWater && CMirrors::TypeOfMirror != MIRROR_TYPE_SPHERE_MAP) {
        RwRenderStateSet(rwRENDERSTATECULLMODE, RWRSTATE(rwCULLMODECULLNONE));
        CWaterFX::CaptureRefractionMap();
        CHook::CallFunction<void>("_ZN11CWaterLevel11RenderWaterEv"); // CWaterLevel::RenderWater();
        RwRenderStateSet(rwRENDERSTATECULLMODE, RWRSTATE(rwCULLMODECULLBACK));
    }
}

void (*CTheCarGenerators__Process)();
void CTheCarGenerators__Process_hook()
{
    return;
	// return CTheCarGenerators__Process();
}

void (*CEntryExitManager__Update)();
void CEntryExitManager__Update_hook()
{
    return;
	// return CEntryExitManager__Update();
}

bool (*InteriorManager_c__Update)(void* thiz);
bool InteriorManager_c__Update_hook(void* thiz)
{
    return false;
	// return InteriorManager_c__Update(thiz);
}

extern int androidSdkVer;
void (*CCustomRoadsignMgr__Initialise)();
void CCustomRoadsignMgr__Initialise_hook()
{
    const int32 slot = CTxdStore::FindTxdSlot("particle");
    Log("CCustomRoadsignMgr: txd particle slot = %d", slot);

    if (slot < 0) {
        Log("CCustomRoadsignMgr: txd particle не найден, инициализация пропущена");
        return;
    }

    CTxdStore::PushCurrentTxd();
    CTxdStore::SetCurrentTxd(slot);

    RwTexture *probe = RwTextureRead("roadsignfont", nullptr);
    Log("CCustomRoadsignMgr: roadsignfont = %p", probe);

    CTxdStore::PopCurrentTxd();

    if (!probe) {
        Log("CCustomRoadsignMgr: нет текстуры roadsignfont в particle.txd, инициализация пропущена");
        return;
    }

    CCustomRoadsignMgr__Initialise();
}

void (*CVehicleModelInfo__LoadEnvironmentMaps)();
void CVehicleModelInfo__LoadEnvironmentMaps_hook()
{
    const int32 slot = CTxdStore::FindTxdSlot("particle");
    if (slot < 0) {
        Log("CVehicleModelInfo: txd particle не найден, envmap пропущен");
        return;
    }

    CTxdStore::PushCurrentTxd();
    CTxdStore::SetCurrentTxd(slot);
    RwTexture *probe = RwTextureRead("white", nullptr);
    CTxdStore::PopCurrentTxd();

    if (!probe) {
        Log("CVehicleModelInfo: нет текстуры white в particle.txd, envmap пропущен");
        return;
    }

    CVehicleModelInfo__LoadEnvironmentMaps();
}

void (*CCarFXRenderer__InitialiseDirtTexture)();
void CCarFXRenderer__InitialiseDirtTexture_hook()
{
    const int32 slot = CTxdStore::FindTxdSlot("particle");
    if (slot < 0) {
        Log("CCarFXRenderer: txd particle не найден, грязь пропущена");
        return;
    }

    CTxdStore::PushCurrentTxd();
    CTxdStore::SetCurrentTxd(slot);
    RwTexture *probe = RwTextureRead("particledirt", nullptr);
    CTxdStore::PopCurrentTxd();

    if (!probe) {
        Log("CCarFXRenderer: нет текстуры грязи в particle.txd, пропущено");
        return;
    }

    CCarFXRenderer__InitialiseDirtTexture();
}

void InstallSpecialHooks()
{
    InjectHooks();
    //CHook::Redirect("_ZN5CGame20InitialiseRenderWareEv", &CGame::InitialiseRenderWare);
    CHook::InstallPLT(g_libGTASA + (VER_x32 ? 0x6785FC : 0x84EC20), &StartGameScreen__OnNewGameCheck_hook, &StartGameScreen__OnNewGameCheck);

    CHook::InlineHook("_Z10NvUtilInitv", &NvUtilInit_hook, &NvUtilInit);

    CHook::RET("_ZN12CCutsceneMgr16LoadCutsceneDataEPKc"); // LoadCutsceneData
    CHook::RET("_ZN12CCutsceneMgr10InitialiseEv");			// CCutsceneMgr::Initialise

    CHook::InlineHook("_Z11OS_FileOpen14OSFileDataAreaPPvPKc16OSFileAccessType",
                      &OS_FileOpen_hook, &OS_FileOpen_orig);

    CHook::InlineHook("_Z11OS_FileSizePv", &OS_FileSize_hook, &OS_FileSize_orig);

    CHook::InlineHook("_ZN11CWaterLevel17AddWaterLevelQuadEii7CRenPariiS0_iiS0_iiS0_j",
                      &CWaterLevel__AddWaterLevelQuad_hook, &CWaterLevel__AddWaterLevelQuad_orig);
    CHook::InlineHook("_ZN11CWaterLevel20WaterLevelInitialiseEv",
                      &CWaterLevel__WaterLevelInitialise_hook, &CWaterLevel__WaterLevelInitialise_orig);
    CHook::InlineHook("_Z12OS_FileClosePv", &OS_FileClose_hook, &OS_FileClose_orig);

    CHook::InlineHook("_Z7NvFOpenPKcS0_bb", &NvFOpen, &NvFOpen_orig);

    // fps fix x64
#if !VER_x32
    CHook::InlineHook(g_libGTASA + 0x332040, &ANDRunThread_hook, &ANDRunThread);
#endif
    // fps fix x32
#if VER_x32
    CHook::InlineHook("_Z12ANDRunThreadPv", &ANDRunThread_hook, &ANDRunThread);
#endif

    CHook::InlineHook("_ZN14MainMenuScreen6UpdateEf", &MainMenuScreen__Update_hook, &MainMenuScreen__Update);

    CHook::RET("_ZN4CPed31RemoveWeaponWhenEnteringVehicleEi"); // CPed::RemoveWeaponWhenEnteringVehicle

    CHook::InlineHook("_ZN18CCustomRoadsignMgr10InitialiseEv",
                      &CCustomRoadsignMgr__Initialise_hook, &CCustomRoadsignMgr__Initialise);
    CHook::InlineHook("_ZN17CVehicleModelInfo19LoadEnvironmentMapsEv",
                      &CVehicleModelInfo__LoadEnvironmentMaps_hook, &CVehicleModelInfo__LoadEnvironmentMaps);
    CHook::InlineHook("_ZN14CCarFXRenderer21InitialiseDirtTextureEv",
                      &CCarFXRenderer__InitialiseDirtTexture_hook, &CCarFXRenderer__InitialiseDirtTexture);

    // texture crash
    CHook::InlineHook("_ZN22TextureDatabaseRuntime8GetEntryEPKcRb", &CTextureDatabaseRuntime__GetEntry_hook, &CTextureDatabaseRuntime__GetEntry);

    // 2.10 crashes
    CHook::InlineHook("_Z32_rxOpenGLDefaultAllInOneRenderCBP10RwResEntryPvhj", &rxOpenGLDefaultAllInOneRenderCB_hook, &rxOpenGLDefaultAllInOneRenderCB);
	CHook::InlineHook("_ZN25CCustomBuildingDNPipeline18CustomPipeRenderCBEP10RwResEntryPvhj", &CCustomBuildingDNPipeline__CustomPipeRenderCB_hook, &CCustomBuildingDNPipeline__CustomPipeRenderCB);
    CHook::InstallPLT(g_libGTASA + (VER_x32 ? 0x675164 : 0x848858), &CCustomBuildingRenderer__AtomicSetup_hook, &CCustomBuildingRenderer__AtomicSetup);

    CHook::InlineHook("_ZN10CPlayerPed29GetPlayerInfoForThisPlayerPedEv", &GetPlayerInfoForThisPlayerPed_hook, &GetPlayerInfoForThisPlayerPed);

    CHook::InlineHook("_Z15OS_ThreadLaunchPFjPvES_jPKci16OSThreadPriority", &custom_OS_ThreadLaunch, &orig_OS_ThreadLaunch);

    void* sym;
    if((sym = TryLoadAlphaFunc("libGLESv2.so")) || (sym = TryLoadAlphaFunc("libGLESv3.so")) ||
       (sym = TryLoadAlphaFunc("libGLESv2_mtk.so")) || (sym = TryLoadAlphaFunc("libGLES_mali.so")) ||
       (sym = TryLoadAlphaFunc("libGLES_android.so")) || (sym = TryLoadAlphaFunc("libGLES.so")) ||
       (sym = TryLoadAlphaFunc("libGLESv2_samsung.so")) || (sym = TryLoadAlphaFunc("libhwui.so"))) {
        *(void**)(g_libGTASA + (VER_x32? 0x6BCBF8 : 0x89A1B0)) = sym;
    }
}

CRect (*CEntity_GetBoundRect)(CEntityGTA *thiz);
CRect CEntity_GetBoundRect_hook(CEntityGTA *thiz)  {
    if (!thiz)
        return CRect(0, 0, 0, 0);

    if (!CModelInfo::ms_modelInfoPtrs[thiz->m_nModelIndex])
        return CRect(0, 0, 0, 0);

    CColModel* m_pColModel = CModelInfo::ms_modelInfoPtrs[thiz->m_nModelIndex]->m_pColModel;
    if (!m_pColModel) {
        CModelInfo::ms_modelInfoPtrs[thiz->m_nModelIndex]->m_pColModel = CModelInfo::ms_modelInfoPtrs[7]->m_pColModel;
        return CRect(0, 0, 0, 0);
    }
    if (!m_pColModel->m_boundBox.m_vecMin.IsZero() && !m_pColModel->m_boundBox.m_vecMax.IsZero()) {
        return CEntity_GetBoundRect(thiz);
    }

    return CRect(0, 0, 0, 0);
}

int (*CCamera__GetScreenFadeStatus)(CCamera *thiz);
int CCamera__GetScreenFadeStatus_hook(CCamera *thiz) {
    return 0;
}

// .. Need for winter radar textures
bool isMiniMapStream(uintptr_t callAddr) {
    if (callAddr == (VER_x32 ? 0x443054 : 0x528240)                // CRadar::RequestMapSection
        || callAddr == (VER_x32 ? 0x443120 : 0x528340))            // CRadar::StreamRadarSections
        return true;
    else
        return false;
}

extern bool bEnableWinter;
TextureDatabaseRuntime* (*TextureDatabaseRuntime__GetDatabase)(const char* name);
TextureDatabaseRuntime* TextureDatabaseRuntime__GetDatabase_hook(const char* name) {
    uintptr_t calledFrom = 0;
    GET_LR(calledFrom);

    // Log("TextureDatabaseRuntime__GetDatabase %s", name);
    if(bEnableWinter) {
        if(strstr(name, "gta_int") || strstr(name, "gta3")) {
            name = "loader_winter";
            // Log("TextureDatabaseRuntime__GetDatabase new %s", name);
        }
    }

    return TextureDatabaseRuntime__GetDatabase(name);
}

RwTexture* (*TextureDatabaseRuntime__GetTexture)(const char* name);
RwTexture* TextureDatabaseRuntime__GetTexture_hook(const char* name)
{
#if !VER_x32
    if (!(*(TDBArray<TextureDatabaseRuntime*>*)(g_libGTASA + 0x89AA08)).numEntries) {
        return nullptr;
    }

    for (int i = 0; i < (*(TDBArray<TextureDatabaseRuntime*>*)(g_libGTASA + 0x89AA08)).numEntries; i++) {
        if ((*(TDBArray<TextureDatabaseRuntime*>*)(g_libGTASA + 0x89AA08)).dataPtr == nullptr) {
            return nullptr;
        }
        if ((*(TDBArray<TextureDatabaseRuntime*>*)(g_libGTASA + 0x89AA08)).dataPtr[i] == nullptr) {
            if ((*(TDBArray<TextureDatabaseRuntime*>*)(g_libGTASA + 0x89AA08)).dataPtr[0] == nullptr) {
                return nullptr;
            }
            (*(TDBArray<TextureDatabaseRuntime*>*)(g_libGTASA + 0x89AA08)).dataPtr[i] = (*(TDBArray<TextureDatabaseRuntime*>*)(g_libGTASA + 0x89AA08)).dataPtr[0];
            RwTexture* ret = TextureDatabaseRuntime__GetTexture(name);
            (*(TDBArray<TextureDatabaseRuntime*>*)(g_libGTASA + 0x89AA08)).dataPtr[i] = nullptr;

            return ret;
        }
    }
#else
    // todo: x32 fix
    if (!(*(TDBArray<TextureDatabaseRuntime*>*)(g_libGTASA + 0x6BD174)).numEntries) {
        return nullptr;
    }

    for (int i = 0; i < (*(TDBArray<TextureDatabaseRuntime*>*)(g_libGTASA + 0x6BD174)).numEntries; i++) {
        if ((*(TDBArray<TextureDatabaseRuntime*>*)(g_libGTASA + 0x6BD174)).dataPtr == nullptr) {
            return nullptr;
        }
        if ((*(TDBArray<TextureDatabaseRuntime*>*)(g_libGTASA + 0x6BD174)).dataPtr[i] == nullptr) {
            if ((*(TDBArray<TextureDatabaseRuntime*>*)(g_libGTASA + 0x6BD174)).dataPtr[0] == nullptr) {
                return nullptr;
            }
            (*(TDBArray<TextureDatabaseRuntime*>*)(g_libGTASA + 0x6BD174)).dataPtr[i] = (*(TDBArray<TextureDatabaseRuntime*>*)(g_libGTASA + 0x6BD174)).dataPtr[0];
            RwTexture* ret = TextureDatabaseRuntime__GetTexture(name);
            (*(TDBArray<TextureDatabaseRuntime*>*)(g_libGTASA + 0x6BD174)).dataPtr[i] = nullptr;

            return ret;
        }
    }
#endif

    return TextureDatabaseRuntime__GetTexture(name);
}

void (*CSprite2d__Draw)(CSprite2d *thiz, const CRect *rectangle, CRGBA *rgba);
void CSprite2d__Draw_hook(CSprite2d *thiz, const CRect *rectangle, CRGBA *rgba) {
    return CSprite2d__Draw(thiz, rectangle, rgba);
}

CEntityGTA* (*CFileLoader__LoadObjectInstance2)(const char* line);
CEntityGTA* CFileLoader__LoadObjectInstance2_hook(const char* line) {
#ifdef BR_DEBUG
    Log("load IPL line: %s", line);
#endif
    return CFileLoader__LoadObjectInstance2(line);
}

void (*FxInfoMan__FXLeak)(uintptr_t thiz);
void FxInfoMan__FXLeak_hook(uintptr_t thiz) {
#if VER_x32
    RwTexture** texturePtr4 = (RwTexture**)(thiz - 44 + 24);
#else
    RwTexture** texturePtr4 = (RwTexture**)(thiz - 0x58 + 0x30);
#endif
    if(*texturePtr4)
    {
        RwTextureDestroy(*texturePtr4);
        *texturePtr4 = NULL;
    }
    FxInfoMan__FXLeak(thiz);
}

// Fuzzy search
int (*mpg123__param)(void* mh, int key, long val, int ZERO, double fval);
int mpg123__param_hook(void* mh, int key, long val, int ZERO, double fval) {
    // 0x2000 = MPG123_SKIP_ID3V2
    // 0x200  = MPG123_FUZZY
    // 0x100  = MPG123_SEEKBUFFER
    // 0x40   = MPG123_GAPLESS
    return mpg123_param(mh, key, val | (0x2000 | 0x200 | 0x100 | 0x40), ZERO, fval);
}

#include "TimeCycle.h"
void (*emu__DistanceFogSetup)(float minDistance, float maxDistance, float red, float green, float blue);
void emu__DistanceFogSetup_hook(float minDistance, float maxDistance, float red, float green, float blue)
{
    // todo: custom fog for graphics && skybox
    emu__DistanceFogSetup(0.25f * minDistance, 0.40f * maxDistance, 46, 57, 91);
}

bool (*CWeapon__Fire)(CEntityGTA* firingEntity, CVector* origin, CVector* muzzlePosn, CEntityGTA* targetEntity, CVector* target, CVector* originForDriveBy);
bool CWeapon__Fire_hook(CEntityGTA* firingEntity, CVector* origin, CVector* muzzlePosn, CEntityGTA* targetEntity, CVector* target, CVector* originForDriveBy) {
	auto res = CWeapon__Fire(firingEntity, origin, muzzlePosn, targetEntity, target, originForDriveBy);
    return res;
}

void (*CPed__SetCurrentWeapon)(CPedGTA* thiz, eWeaponType weaponType);
void CPed__SetCurrentWeapon_hook(CPedGTA* thiz, eWeaponType weaponType) {
	CPed__SetCurrentWeapon(thiz, weaponType);

	CHud::updateAmmo();
}

float (*CEntity__GetDistanceFromCentreOfMassToBaseOfModel)(CEntityGTA *thiz);
float CEntity__GetDistanceFromCentreOfMassToBaseOfModel_hook(CEntityGTA *thiz) {
  if(thiz->m_nModelIndex == 10657) {
      return 0;
  }

  return CEntity__GetDistanceFromCentreOfMassToBaseOfModel(thiz);;
}

void (*CTouchInterface__DrawAll)(bool bAfterFade);
void CTouchInterface__DrawAll_hook(bool bAfterFade) {
    if(pUI->m_bCamEditGui)
        return;

    CTouchInterface__DrawAll(bAfterFade);
}

void (*FxInfoManager_c__Shutdown)(uintptr_t self);
void FxInfoManager_c__Shutdown_hook(uintptr_t self) {
#if VER_x32
    RwTexture** texture = (RwTexture**)(self - 44 + 24);
#else
    RwTexture** texture = (RwTexture**)(self - 0x58 + 0x30);
#endif

    if(*texture)
    {
        RwTextureDestroy(*texture);
        *texture = NULL;
    }

    FxInfoManager_c__Shutdown(self);
}

#include <EGL/egl.h>
#include <GLES2/gl2.h>
void SetUpGLHooks();
void InstallHooks()
{
    RQCapabilities* RQCaps = (RQCapabilities*)(g_libGTASA + (VER_x32 ? 0x6B8B9C : 0x896130));
    if(RQCaps->hasTextureCompressionPVRTCCap)
        CHook::Redirect("_ZNK14TextureListing11GetMipCountEv", &getmip);

#if ENABLE_CUSTOM_2DSTUFF
    CHook::Redirect("_Z13Render2dStuffv", &Render2dStuff);
#else
    CHook::InlineHook("_Z13Render2dStuffv", &Render2dStuff_hook, &Render2dStuff);
#endif

    CHook::InlineHook("_Z14AND_TouchEventiiii", &AND_TouchEvent_hook, &AND_TouchEvent);

    CHook::Redirect("_ZN11CHudColours12GetIntColourEh", &CHudColours__GetIntColour); // dangerous
    CHook::Redirect("_ZN6CRadar19GetRadarTraceColourEjhh", &CRadar__GetRadarTraceColor); // dangerous

    CHook::InlineHook("_ZN6CRadar12SetCoordBlipE9eBlipType7CVectorj12eBlipDisplayPc", &CRadar__SetCoordBlip_hook, &CRadar__SetCoordBlip);
    CHook::InlineHook("_ZN6CRadar9ClearBlipEi", &CRadar__ClearBlip_hook, &CRadar__ClearBlip);
	CHook::Redirect("_ZN6CRadar20DrawRadarGangOverlayEb", &CRadar__DrawRadarGangOverlay);

    CHook::Redirect("_Z10GetTexturePKc", &CUtil::GetTexture);

    CHook::InlineHook("_ZN14MainMenuScreen6OnExitEv", &MainMenuScreen__OnExit_hook, &MainMenuScreen__OnExit);

    CHook::InlineHook("_ZN17CTaskSimpleUseGun17RemoveStanceAnimsEP4CPedf", &CTaskSimpleUseGun__RemoveStanceAnims_hook, &CTaskSimpleUseGun__RemoveStanceAnims);

    // Bullet sync
    CHook::InlineHook("_ZN7CWeapon14FireInstantHitEP7CEntityP7CVectorS3_S1_S3_S3_bb", &CWeapon__FireInstantHit_hook, &CWeapon__FireInstantHit);
    CHook::InlineHook("_ZN7CWeapon10FireSniperEP4CPedP7CEntityP7CVector", &CWeapon__FireSniper_hook, &CWeapon__FireSniper);
    CHook::InlineHook("_ZN6CWorld18ProcessLineOfSightERK7CVectorS2_R9CColPointRP7CEntitybbbbbbbb", &CWorld__ProcessLineOfSight_hook, &CWorld__ProcessLineOfSight);
    CHook::InlineHook("_ZN28CPedDamageResponseCalculator21ComputeDamageResponseEP4CPedR18CPedDamageResponseb", &CPedDamageResponseCalculator__ComputeDamageResponse_hook, &CPedDamageResponseCalculator__ComputeDamageResponse);
    CHook::InlineHook("_ZN7CWeapon18ProcessLineOfSightERK7CVectorS2_R9CColPointRP7CEntity11eWeaponTypeS6_bbbbbbb", &CWeapon__ProcessLineOfSight_hook, &CWeapon__ProcessLineOfSight);
    CHook::InlineHook("_ZN11CBulletInfo9AddBulletEP7CEntity11eWeaponType7CVectorS3_", &CBulletInfo_AddBullet_hook, &CBulletInfo_AddBullet);

    CHook::InlineHook("_ZN11CFileLoader18LoadObjectInstanceEP19CFileObjectInstancePKc", &CFileLoader__LoadObjectInstance_hook, &CFileLoader__LoadObjectInstance);

    CHook::InlineHook("_ZN10CCollision19ProcessVerticalLineERK8CColLineRK7CMatrixR9CColModelR9CColPointRfbbP15CStoredCollPoly", &CCollision__ProcessVerticalLine_hook, &CCollision__ProcessVerticalLine);

    CHook::InlineHook("_ZN19CUpsideDownCarCheck15IsCarUpsideDownEPK8CVehicle", &CUpsideDownCarCheck__IsCarUpsideDown_hook, &CUpsideDownCarCheck__IsCarUpsideDown);

    CHook::InlineHook("_ZN16CTaskSimpleGetUp10ProcessPedEP4CPed", &CTaskSimpleGetUp__ProcessPed_hook, &CTaskSimpleGetUp__ProcessPed); // CTaskSimpleGetUp::ProcessPed
    // CHook::InlineHook("_ZN7CObject6RenderEv", &CObject_Render_hook, & CObject_Render);

    CHook::Redirect("_Z19PlayerIsEnteringCarv", &PlayerIsEnteringCar);
    if(*(uint8_t *)(g_libGTASA + (VER_x32 ? 0x6B8B9C:0x896135)))
    {
        CHook::Redirect("_ZNK14TextureListing11GetMipCountEv", &getmip);
    }

    auto glAlphaFuncQCOM = eglGetProcAddress("glAlphaFuncQCOM");
    if (!glAlphaFuncQCOM)
    {
        glAlphaFuncQCOM = eglGetProcAddress("glAlphaFunc");
        if(glAlphaFuncQCOM)
            *(uintptr_t *) (g_libGTASA + (VER_x32 ? 0x6BCBF8 : 0x89A1B0)) = reinterpret_cast<uintptr_t>(glAlphaFuncQCOM);
        else
            CHook::Redirect("_Z25RQ_Command_rqSetAlphaTestRPc", &RQCommand_rqSetAlphaTest);
    }

    CHook::Redirect("_ZN4CHID12GetInputTypeEv", &GetInputType);

#if VER_x32
    CHook::InlineHook("_ZN14CAnimBlendNode12FindKeyFrameEf", &CAnimBlendNode__FindKeyFrame_hook, &CAnimBlendNode__FindKeyFrame);
#endif

    // GetFrameFromID fix
	CHook::InlineHook("_ZN15CClumpModelInfo14GetFrameFromIdEP7RpClumpi", &CClumpModelInfo_GetFrameFromId_hook, &CClumpModelInfo_GetFrameFromId);

    CHook::InlineHook("_ZN13FxEmitterBP_c6RenderEP8RwCamerajfh", &FxEmitterBP_c__Render_hook, &FxEmitterBP_c__Render);
    CHook::InlineHook("_Z23RwResourcesFreeResEntryP10RwResEntry", &RwResourcesFreeResEntry_hook, &RwResourcesFreeResEntry);

    ms_fAspectRatio = (float*)(g_libGTASA+(VER_x32 ? 0xA26A90:0xCC7F00));
    CHook::InlineHook("_ZN4CHud14DrawCrossHairsEv", &DrawCrosshair_hook, &DrawCrosshair);

    // retexture
    CHook::InlineHook("_ZN7CEntity6RenderEv", &CEntity_Render_hook, &CEntity_Render);

    CHook::InlineHook("_ZN26CAEGlobalWeaponAudioEntity21ServiceAmbientGunFireEv", &TaskEnterVehicleHook, &TaskEnterVehicle);
#if VER_x32
    CHook::UnFuck(g_libGTASA + 0x4DD9E8);
    *(float*)(g_libGTASA + 0x4DD9E8) = 0.015f;
#else
    CHook::Write(g_libGTASA + 0x5DF790, 0x90000AA9);
    CHook::Write(g_libGTASA + 0x5DF794, 0xBD48D521);
#endif

    // CHook::InlineHook("_ZN5CDraw6SetFOVEfb", &CDraw__SetFOV_hook, &CDraw__SetFOV);

    CHook::InlineHook("_ZN12CAnimManager19UncompressAnimationEP19CAnimBlendHierarchy", &CAnimManager_UncompressAnimation_hook, &CAnimManager_UncompressAnimation);

    // N sprite in radar
    CHook::InlineHook("_ZN6CRadar15DrawRadarSpriteEtffh", &CRadar__DrawRadarSprite_hook, &CRadar__DrawRadarSprite);

    // skybox render
    CHook::InlineHook("_ZN7CClouds6RenderEv", &CClouds__Render_hook, &CClouds__Render);

    // sa:mp custom radar position
    CHook::InlineHook("_ZN4CHud9DrawRadarEv", &CHud__DrawRadar_hook, &CHud__DrawRadar);
    CHook::InlineHook("_ZN15CTouchInterface7DrawAllEb", &CTouchInterface__DrawAll_hook, &CTouchInterface__DrawAll);

    // custom veh render
    CHook::InlineHook("_ZN11CAutomobile9PreRenderEv", &CAutomobile__PreRender_hook, &CAutomobile__PreRender);
    CHook::InlineHook("_ZN11CAutomobile17UpdateWheelMatrixEii", &CAutomobile__UpdateWheelMatrix_hook, &CAutomobile__UpdateWheelMatrix);
    CHook::InlineHook("_ZN7CMatrix6RotateEfff", &CMatrix__Rotate_hook, &CMatrix__Rotate);
    CHook::InlineHook("_ZN7CMatrix8SetScaleEfff", &CMatrix__SetScale_hook, &CMatrix__SetScale);
    CHook::InlineHook("_ZN16cHandlingDataMgr22ConvertDataToGameUnitsEP13tHandlingData", &cHandlingDataMgr__ConvertDataToGameUnits_hook, &cHandlingDataMgr__ConvertDataToGameUnits);
    CHook::InlineHook("_ZN11CAutomobile22ProcessEntityCollisionEP7CEntityP9CColPoint", &CAutomobile__ProcessEntityCollision_hook, &CAutomobile__ProcessEntityCollision);

    CHook::InlineHook("_ZNK7CEntity12GetBoundRectEv", &CEntity_GetBoundRect_hook, &CEntity_GetBoundRect);
    CHook::InlineHook("_ZN7CCamera19GetScreenFadeStatusEv", &CCamera__GetScreenFadeStatus_hook, &CCamera__GetScreenFadeStatus);

    // CHook::InlineHook("_ZN22TextureDatabaseRuntime11GetDatabaseEPKc", &TextureDatabaseRuntime__GetDatabase_hook, &TextureDatabaseRuntime__GetDatabase);
    CHook::InlineHook("_ZN22TextureDatabaseRuntime10GetTextureEPKc", &TextureDatabaseRuntime__GetTexture_hook, &TextureDatabaseRuntime__GetTexture);

    // CHook::InlineHook("_ZN11CFileLoader18LoadObjectInstanceEPKc", &CFileLoader__LoadObjectInstance2_hook, &CFileLoader__LoadObjectInstance2);

    CHook::RET("_ZN14CLoadingScreen15DisplayPCScreenEv");

    // CHook::InlineHook("_Z20emu_DistanceFogSetupfffff", &emu__DistanceFogSetup_hook, &emu__DistanceFogSetup);

    // fx crash
    // CHook::InstallPLT(g_libGTASA + (VER_x32 ? 0x670FA8 : 0x841D00), &FxInfoManager_c__Shutdown_hook, &FxInfoManager_c__Shutdown);

    // hud
    // CHook::InlineHook("_ZN7CWeapon4FireEP7CEntityP7CVectorS3_S1_S3_S3_", &CWeapon__Fire_hook, &CWeapon__Fire);
    CHook::InlineHook("_ZN4CPed16SetCurrentWeaponE11eWeaponType", &CPed__SetCurrentWeapon_hook, &CPed__SetCurrentWeapon);

    // CHook::InlineHook("_ZN9CSprite2d4DrawERK5CRectRK5CRGBA", &CSprite2d__Draw_hook, &CSprite2d__Draw);

    // first person and veh speed cam fix
	CHook::InlineHook("_ZN4CCam7ProcessEv", &CCam__Process_hook, &CCam__Process);
	CHook::InlineHook("_ZN4CPad27CycleCameraModeDownJustDownEv", &CPad__CycleCameraModeDownJustDown_hook, &CPad__CycleCameraModeDownJustDown);
    CHook::InlineHook("_ZN5CDraw6SetFOVEfb", &CDraw__SetFOV_hook, &CDraw__SetFOV);
    // CHook::InlineHook("_ZN7CCamera24Using1stPersonWeaponModeEv", &CCamera__Using1stPersonWeaponMode_hook, &CCamera__Using1stPersonWeaponMode);

    // col fix
    // CHook::InlineHook("_ZN7CEntity40GetDistanceFromCentreOfMassToBaseOfModelEv", &CEntity__GetDistanceFromCentreOfMassToBaseOfModel_hook, &CEntity__GetDistanceFromCentreOfMassToBaseOfModel);

    // game logic
    // CHook::InlineHook("_ZN17CTheCarGenerators7ProcessEv", &CTheCarGenerators__Process_hook, &CTheCarGenerators__Process);
    // CHook::InlineHook("_ZN17CEntryExitManager6UpdateEv", &CEntryExitManager__Update_hook, &CEntryExitManager__Update);
    // CHook::InlineHook("_ZN17InteriorManager_c6UpdateEv", &InteriorManager_c__Update_hook, &InteriorManager_c__Update);

    HookCPad();
}