#include "../main.h"
#include "game.h"
#include "../net/netgame.h"
#include "vehicle.h"
#include "Streaming.h"
#include "../game/Entity/Automobile.h"
#include "game/Entity/CBike.h"
#include "game/Entity/CTrain.h"
#include "game/Entity/CPlane.h"

extern CGame* pGame;
extern CNetGame* pNetGame;

bool bInProcessDetachTrailer = false;

CVehicle::CVehicle(int iType, float fX, float fY, float fZ, float fRotation, bool bPreloaded, bool bSiren)
{
	RwMatrix matEnt;
	static CVehicleGTA* pCreatedTrain = nullptr;

	m_pVehicle = nullptr;
	m_dwGTAId = 0;
	m_pTrailer = nullptr;

	if ((iType != TRAIN_PASSENGER_LOCO) &&
		(iType != TRAIN_FREIGHT_LOCO) &&
		(iType != TRAIN_PASSENGER) &&
		(iType != TRAIN_FREIGHT) &&
		(iType != TRAIN_TRAM)) {

        if (!CStreaming::TryLoadModel(iType))
            throw std::runtime_error("Model not loaded");

		ScriptCommand(&create_car, iType, fX, fY, fZ, &m_dwGTAId);
		ScriptCommand(&set_car_z_angle, m_dwGTAId, fRotation);
		ScriptCommand(&car_gas_tank_explosion, m_dwGTAId, 0);
		ScriptCommand(&set_car_hydraulics, m_dwGTAId, 0);
		ScriptCommand(&toggle_car_tires_vulnerable, m_dwGTAId, 0);

		m_pVehicle = GamePool_Vehicle_GetAt(m_dwGTAId);

		if (m_pVehicle) 
		{
            m_pVehicle->m_pVehicleMP = this;

			m_pVehicle->m_nDoorLock = (eCarLock)0;
			m_bIsLocked = false;

			m_pVehicle->GetMatrix(&matEnt);
			matEnt.pos.x = fX;
			matEnt.pos.y = fY;
			matEnt.pos.z = fZ;

			if( GetVehicleSubtype() != VEHICLE_SUBTYPE_BIKE &&
				GetVehicleSubtype() != VEHICLE_SUBTYPE_PUSHBIKE)
				matEnt.pos.z += 0.25f;

			m_pVehicle->SetMatrix((CMatrix&)matEnt);
			m_bPreloaded = bPreloaded;
		}
	}
	else if ((iType == TRAIN_PASSENGER_LOCO) ||
		(iType == TRAIN_FREIGHT_LOCO) ||
		(iType == TRAIN_TRAM))
	{
		if (iType == TRAIN_PASSENGER_LOCO) iType = 5;
		else if (iType == TRAIN_FREIGHT_LOCO) iType = 3;
		else if (iType == TRAIN_TRAM) iType = 9;

		uint32_t dwDirection = 0;
		if (fRotation > 180.0f) {
			dwDirection = 1;
		}

        if (!CStreaming::TryLoadModel(TRAIN_PASSENGER_LOCO))
            throw std::runtime_error("Model not loaded");

        if (!CStreaming::TryLoadModel(TRAIN_PASSENGER))
            throw std::runtime_error("Model not loaded");

        if (!CStreaming::TryLoadModel(TRAIN_FREIGHT_LOCO))
            throw std::runtime_error("Model not loaded");

        if (!CStreaming::TryLoadModel(TRAIN_FREIGHT))
            throw std::runtime_error("Model not loaded");
        if (!CStreaming::TryLoadModel(TRAIN_TRAM))
            throw std::runtime_error("Model not loaded");

		ScriptCommand(&create_train, iType, fX, fY, fZ, dwDirection, &m_dwGTAId);
		m_pVehicle = GamePool_Vehicle_GetAt(m_dwGTAId);

		pCreatedTrain = m_pVehicle;

		GamePrepareTrain(m_pVehicle);
	}
	else if ((iType == TRAIN_PASSENGER) ||
		(iType == TRAIN_FREIGHT))
	{
		if (!pCreatedTrain)
		{
			m_pVehicle = nullptr;
			return;
		}

		m_dwGTAId = GamePool_Vehicle_GetIndex(m_pVehicle);
		pCreatedTrain = m_pVehicle;
	}

	m_bIsInvulnerable = false;
	m_byteObjectiveVehicle = 0;
	m_bSpecialMarkerEnabled = false;
	m_dwMarkerID = 0;
	m_iEngineState = -1;
	m_bDoorsLocked = false;
	m_bHaveColor = false;
	m_bHasBeenDriven = false;
	m_dwTimeSinceLastDriven = GetTickCount();

	memset(m_szPlateText, 0, sizeof(m_szPlateText));
}

CVehicle::~CVehicle()
{
	m_pVehicle = GamePool_Vehicle_GetAt(m_dwGTAId);

	ReleaseSuspensionLines();
	RemoveAllCustomComponents();
	ReleaseHandling();

	m_pVinylTex = nullptr;

	if (m_pVehicle) {
        auto modelId = m_pVehicle->m_nModelIndex;
		if(m_dwMarkerID)
		{
			pGame->DisableMarker(m_dwMarkerID);
			m_dwMarkerID = 0;
		}

        m_pVehicle->m_pVehicleMP = nullptr;

		RemoveEveryoneFromVehicle();

		if(SirenEnabled()) EnableSiren(false);

		if (m_pTrailer) {
			DetachTrailer();
			m_pTrailer = nullptr;
		}

		int iModel = m_pVehicle->m_nModelIndex;
		if (iModel == 538 || iModel == 537)
		{
			ScriptCommand(&destroy_train, m_dwGTAId);
		}
		else
		{
			ScriptCommand(&destroy_car, m_dwGTAId);
		}

        if(pPlateTexture) {
            RwTextureDestroy(pPlateTexture);
            pPlateTexture = nullptr;
        }
        CStreaming::RemoveModelIfNoRefs(modelId);
	}
}

eTurnState CVehicle::GetTurnLightsState() const
{
    return m_iTurnState;
}

void CVehicle::SetTurnLightsState(eTurnState state)
{
    if (m_iTurnState != state) {
        m_nTurnStateTime = CTimer::m_snTimeInMilliseconds;
    }
    m_iTurnState = state;
}

bool CVehicle::IsTurnBlinkOn() const
{
    return ((CTimer::m_snTimeInMilliseconds - m_nTurnStateTime) / 500) % 2 == 0;
}

eTurnState CVehicle::GetTurnLightsBlinkState() const
{
    return IsTurnBlinkOn() ? m_iTurnState : eTurnState::TURN_OFF;
}

void CVehicle::RenderTurnLights()
{
    if (IsTurnBlinkOn())
	{
        if (m_iTurnState == eTurnState::TURN_RIGHT)
		{
            DrawRightTurnLight();
        }
		else if (m_iTurnState == eTurnState::TURN_LEFT)
		{
            DrawLeftTurnLight();
        }
		else if (m_iTurnState == eTurnState::TURN_ALL)
		{
            DrawLeftTurnLight();
            DrawRightTurnLight();
        }
		else
		{
            m_iTurnState = eTurnState::TURN_OFF;
        }
    }
}

void CVehicle::DrawRightTurnLight()
{
    DrawTurnlight(false);
}

void CVehicle::DrawLeftTurnLight()
{
    DrawTurnlight(true);
}

#include "Coronas.h"
#include "Models/ModelInfo.h"
#include "Models/ClumpModelInfo.h"
#include "Collision/ColModel.h"
#include "MemoryMgr.h"
#include "Streaming.h"
#include "CustomTune.h"
#include "CHandlingDefault.h"
#include "Enums/eHandlingParam.h"
#include "VisibilityPlugins.h"
#include "Shadows.h"
#include "util/CUtil.h"

void CVehicle::DrawTurnlightPool(const CVector& vecObjOffset, bool bRear, int32 id)
{
    static RwTexture* pTurnLightTex = nullptr;
    static bool bTextureChecked = false;

    if (!bTextureChecked) {
        bTextureChecked = true;
        pTurnLightTex = CUtil::LoadTextureFromDB("txd", "headlight1");
    }

    if (!pTurnLightTex || !m_pVehicle) {
        return;
    }

    CMatrix& mat = m_pVehicle->GetMatrix();

    // Передние пятна ложились слишком близко к бамперу и наезжали на кузов,
    // поэтому вперёд отодвигаем дальше, чем назад.
    constexpr float kAwayFront = 0.78f;
    constexpr float kAwayRear  = 0.48f;
    constexpr float kAwaySide  = 0.17f;

    CVector vecLightPos = vecObjOffset;
    vecLightPos.y += bRear ? -kAwayRear : kAwayFront;
    vecLightPos.x += (vecLightPos.x < 0.0f) ? -kAwaySide : kAwaySide;

    CVector vecWorld;
    ProjectMatrix(&vecWorld, &mat, &vecLightPos);

    const CVector& vecForward = mat.GetForward();
    float fwdX = vecForward.x;
    float fwdY = vecForward.y;

    const float fwdLen = sqrtf(fwdX * fwdX + fwdY * fwdY);
    if (fwdLen < 0.001f) {
        return;
    }
    fwdX /= fwdLen;
    fwdY /= fwdLen;

    if (bRear) {
        fwdX = -fwdX;
        fwdY = -fwdY;
    }

    constexpr float kPoolSize = 0.60f;

    const float frontX = fwdX * kPoolSize;
    const float frontY = fwdY * kPoolSize;
    const float sideX  = fwdY * kPoolSize * 0.7f;
    const float sideY  = -fwdX * kPoolSize * 0.7f;

    vecWorld.z += 2.0f;

    CShadows::StoreCarLightShadow(
            m_pVehicle, id, pTurnLightTex, &vecWorld,
            frontX, frontY,
            sideX, sideY,
            150, 150, 0,
            7.0f, true);
}

void CVehicle::DrawTurnlight(bool leftSide)
{
    auto pModelInfoStart = CModelInfo::GetVehicleModelInfo(m_pVehicle->m_nModelIndex);
    CVector* m_avDummyPos = pModelInfoStart->m_pVehicleStruct->m_avDummyPos;

    CVector vecFront;
    auto frontOffsetX = m_avDummyPos[0].x + 0.1f;
    vecFront.x = leftSide ? -frontOffsetX : frontOffsetX;
    vecFront.y = m_avDummyPos[0].y;
    vecFront.z = m_avDummyPos[0].z;

    CVector vecRear;
    auto rearOffsetX = m_avDummyPos[1].x + 0.1f;
    vecRear.x = leftSide ? -rearOffsetX : rearOffsetX;
    vecRear.y = m_avDummyPos[1].y;
    vecRear.z = m_avDummyPos[1].z;

    CRGBA turnSignalColor(255, 180, 0, 255);
    CCoronas::RegisterCorona(
            m_dwGTAId + reinterpret_cast<std::uintptr_t>(m_avDummyPos) + leftSide + 0,
            m_pVehicle,
            turnSignalColor.r, turnSignalColor.g, turnSignalColor.b, turnSignalColor.a,
            &vecFront,
            0.3f,
            40.0f,
            eCoronaType::CORONATYPE_SHINYSTAR,
            eCoronaFlareType::FLARETYPE_NONE,
            false,
            false,
            0,
            0.f,
            false,
            0.5,
            0,
            50.0f,
            false,
            true
    );
    CCoronas::RegisterCorona(
            m_dwGTAId + reinterpret_cast<std::uintptr_t>(m_avDummyPos) + leftSide + 2,
            m_pVehicle,
            turnSignalColor.r, turnSignalColor.g, turnSignalColor.b, turnSignalColor.a,
            &vecRear,
            0.3f,
            40.0f,
            eCoronaType::CORONATYPE_SHINYSTAR,
            eCoronaFlareType::FLARETYPE_NONE,
            false,
            false,
            0,
            0.f,
            false,
            0.5,
            0,
            50.0f,
            false,
            true
    );

    DrawTurnlightPool(vecFront, false, (int32)(m_dwGTAId * 4 + leftSide));
    DrawTurnlightPool(vecRear,  true,  (int32)(m_dwGTAId * 4 + leftSide + 2));
}

// 0.3.7
bool CVehicle::IsRCVehicle()
{
	if (m_pVehicle)
	{
		if (GamePool_Vehicle_GetAt(m_dwGTAId))
		{
			int iModel = m_pVehicle->m_nModelIndex;
			if (iModel == 441
				|| iModel == 464
				|| iModel == 465
				|| iModel == 594
				|| iModel == 501
				|| iModel == 564) {
				return true;
			}
		}
	}

	return false;
}
// 0.3.7
float CVehicle::GetHealth()
{
	if (m_pVehicle) {
		return m_pVehicle->fHealth;
	}

	return 0.0f;
}
// 0.3.7
void CVehicle::SetHealth(float fHealth)
{
	if (m_pVehicle) {
		m_pVehicle->fHealth = fHealth;
	}
}
// 0.3.7
int CVehicle::GetVehicleSubtype()
{
	if (m_pVehicle) {
		return ::GetVehicleSubtype(m_pVehicle);
	}

	return 0;
}
// 0.3.7
void CVehicle::AddComponent(int iComponentID)
{
	if (!m_pVehicle || !GamePool_Vehicle_GetAt(m_dwGTAId)) return;
	if (GetVehicleSubtype() != VEHICLE_SUBTYPE_CAR) return;

    if(!CStreaming::GetInfo(iComponentID).IsLoaded()) {
        //CStreaming::RequestModel(iComponentID, STREAMING_GAME_REQUIRED | STREAMING_KEEP_IN_MEMORY);
        CHook::CallFunction<void>("_ZN10CStreaming21RequestVehicleUpgradeEii", iComponentID, 10);
        CStreaming::LoadAllRequestedModels(false);
        uint32 count = 0;
        while (!CStreaming::GetInfo(iComponentID).IsLoaded()) {
            count++;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (count > 30) {
                //pUI->chat()->addDebugMessage("{ff0000} Error loading model %d", iComponentID);
                return;
            }
        }
    }

	uint32_t dwRet;
	ScriptCommand(&add_car_component, m_dwGTAId, iComponentID, &dwRet);
}
// 0.3.7
void CVehicle::SetPaintJob(uint8_t bytePaintJobID)
{
    if(!GamePool_Vehicle_GetAt(m_dwGTAId) || bytePaintJobID < 0)
        return;

    ScriptCommand(&change_car_skin, m_dwGTAId, bytePaintJobID);
}
// 0.3.7
void CVehicle::SetColor(uint8_t byteColor1, uint8_t byteColor2)
{
	if (m_pVehicle && GamePool_Vehicle_GetAt(m_dwGTAId))
	{
		m_pVehicle->m_nPrimaryColor = byteColor1;
		m_pVehicle->m_nSecondaryColor = byteColor2;
	}

	m_byteColor1 = byteColor1;
	m_byteColor2 = byteColor2;
	m_bHaveColor = true;
}
// 0.3.7
void CVehicle::DetachTrailer()
{
	bInProcessDetachTrailer = true;
	if (m_pTrailer)
	{
		if (m_dwGTAId && GamePool_Vehicle_GetAt(m_dwGTAId))
		{
			if (m_pTrailer->m_pVehicle)
				ScriptCommand(&detach_trailer_from_cab, m_pTrailer->m_dwGTAId, m_dwGTAId);
		}
	}
}
// 0.3.7
void CVehicle::SetTrailer(CVehicle* pTrailer)
{
	m_pTrailer = pTrailer;
}
// 0.3.7
void CVehicle::LinkToInterior(int iInterior)
{
	if(m_pVehicle)
        m_pVehicle->SetInterior(iInterior);
}

void CVehicle::SetDamageStatus(uint32_t dwPanelDamageStatus, uint32_t dwDoorDamageStatus, uint8_t byteLightDamageStatus)
{

}

void CVehicle::SetTireDamageStatus(uint8_t byteTireDamageStatus)
{

}
// 0.3.7
void CVehicle::RemoveEveryoneFromVehicle()
{
	if (!m_pVehicle) return;
	if (!GamePool_Vehicle_GetAt(m_dwGTAId)) return;

    if (!m_pVehicle) return;
    if(!m_dwGTAId)return;
    if (!GamePool_Vehicle_GetAt(m_dwGTAId)) return;

    if (m_pVehicle->pDriver)
    {
        m_pVehicle->pDriver->RemoveFromVehicle();
    }

    for (int i = 0; i < 7; i++)
    {
        if (m_pVehicle->m_apPassengers[i] != nullptr)
        {
            m_pVehicle->m_apPassengers[i]->RemoveFromVehicle();
        }
    }
}
// 0.3.7
CVehicle* CVehicle::GetTrailer()
{
	if (m_pVehicle)
	{
        CVehicleGTA* pTrailer = m_pVehicle->m_pTrailer;
		if (pTrailer)
		{
			if (pNetGame)
			{
				CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();
				if (pVehiclePool) {
					VEHICLEID TrailerID = pVehiclePool->FindIDFromGtaPtr(pTrailer);
					if (TrailerID != INVALID_VEHICLE_ID)
					{
						return pVehiclePool->GetAt(TrailerID);
					}
				}
			}
		}
	}

	return nullptr;
}
// 0.3.7
CVehicle* CVehicle::GetTractor()
{
	if (!m_pVehicle) return nullptr;

	int iSubType = ::GetVehicleSubtype(m_pVehicle);
	if (iSubType == VEHICLE_SUBTYPE_BIKE
		|| iSubType == VEHICLE_SUBTYPE_BOAT
		|| iSubType == VEHICLE_SUBTYPE_TRAIN
		|| iSubType == VEHICLE_SUBTYPE_PUSHBIKE)
	{
		return nullptr;
	}

    CVehicleGTA* pTractor = m_pVehicle->m_pTowingVehicle;
	if (pTractor)
	{
		if (pNetGame)
		{
			CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();
			if (pVehiclePool)
			{
				VEHICLEID TrailerID = pVehiclePool->FindIDFromGtaPtr(pTractor);
				if (TrailerID != INVALID_VEHICLE_ID)
				{
					return pVehiclePool->GetAt(TrailerID);
				}
			}
		}
	}

	return nullptr;
}
// 0.3.7
float CVehicle::GetTrainSpeed()
{
	if (m_pVehicle && m_pVehicle->GetType() == VEHICLE_TYPE_TRAIN) {
		return ((CTrain*)m_pVehicle)->m_fTrainSpeed;
	}
	
	return 0.0f;
}
// 0.3.7 (dword -> word)
uint16_t CVehicle::GetHydraThrusters()
{
	if (m_pVehicle) {
		return ((CAutomobile*)m_pVehicle)->m_nSuspensionHydraulics;
	}

	return 0.0f;
}

bool CVehicle::IsLandingGearNotUp()
{
	if(m_pVehicle && m_pVehicle->GetType() == VEHICLE_TYPE_PLANE)
		return ((CPlane*)m_pVehicle)->m_fLGearAngle != 0.0f;

	return 0;
}

float CVehicle::GetBikeLean()
{
	if (m_pVehicle && GamePool_Vehicle_GetAt(m_dwGTAId) && m_pVehicle->GetType() == VEHICLE_TYPE_BIKE)
		return ((CBike*)m_pVehicle)->m_LeanMatrix.GetPosition().z;

	return 0.0f;
}
// 0.3.7
bool CVehicle::IsATrainPart()
{
	if (m_pVehicle)
	{
		int iModelIndex = m_pVehicle->m_nModelIndex;
		if (iModelIndex == 538 || iModelIndex == 570 || iModelIndex == 537 ||
			iModelIndex == 569 || iModelIndex == 449) {
			return true;
		}
	}

	return false;
}
// 0.3.7
bool CVehicle::VerifyInstance()
{
	return GamePool_Vehicle_GetAt(m_dwGTAId) != nullptr;
}
// 0.3.7
void CVehicle::SetDoorState(int state)
{
	if (state)
	{
		m_pVehicle->m_nDoorLock = (eCarLock)2;
		m_bDoorsLocked = true;
	}
	else
	{
		m_pVehicle->m_nDoorLock =(eCarLock) 0;
		m_bDoorsLocked = false;
	}
}

void CVehicle::SetComponentOpenState(int iDoor, int iComponent, float fDoorOpenRatio)
{
	if (GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR)
	{
		ScriptCommand(&set_car_door_rotation_to, m_dwGTAId, iDoor, iComponent, fDoorOpenRatio);
	
	}
}

void CVehicle::OpenWindow(uint8_t component)
{
	if (m_pVehicle) {

	}
}

void CVehicle::CloseWindow(uint8_t component)
{
	if (m_pVehicle) {

	}
}
// 0.3.7
void CVehicle::AttachTrailer()
{
	if (m_pTrailer) {
		ScriptCommand(&put_trailer_on_cab, m_pTrailer->m_dwGTAId, m_dwGTAId);
	}
}
// 0.3.7
void CVehicle::RemoveComponent(int iComponentID)
{
	if (!m_pVehicle || !GamePool_Vehicle_GetAt(m_dwGTAId))
		return;

	ScriptCommand(&remove_component, m_dwGTAId, iComponentID);
}
// 0.3.7
void CVehicle::SetZAngle(float fAngle)
{
	if (GamePool_Vehicle_GetAt(m_dwGTAId)) {
		ScriptCommand(&set_car_z_angle, m_dwGTAId, fAngle);
	}
}
// 0.3.7
bool CVehicle::IsDriverLocalPlayer()
{
	if (m_pVehicle && m_pVehicle->pDriver == GamePool_FindPlayerPed())
		return true;

	return false;
}
// 0.3.7
void CVehicle::SetInvulnerable(bool bInv)
{
	if (m_pVehicle && GamePool_Vehicle_GetAt(m_dwGTAId))
	{
		if (bInv)
		{
			ScriptCommand(&set_car_immunities, m_dwGTAId, 1, 1, 1, 1, 1);
			ScriptCommand(&toggle_car_tires_vulnerable, m_dwGTAId, 0);
			m_bIsInvulnerable = true;
		}
		else
		{
			ScriptCommand(&set_car_immunities, m_dwGTAId, 0, 0, 0, 0, 0);
			ScriptCommand(&toggle_car_tires_vulnerable, m_dwGTAId, 1);
			m_bIsInvulnerable = false;
		}
	}
}
// 0.3.7
bool CVehicle::HasSunk()
{
	if (m_pVehicle) {
		return ScriptCommand(&has_car_sunk, m_dwGTAId);
	}

	return false;
}
// 0.3.7
bool CVehicle::HasADriver()
{
	if (m_pVehicle && GamePool_Vehicle_GetAt(m_dwGTAId))
	{
		if (m_pVehicle->pDriver)
		{
			if (m_pVehicle->pDriver->IsInVehicle() &&
				m_pVehicle->pDriver->m_nPedType == (ePedType)0)
				return true;
		}
	}

	return false;
}

void CVehicle::ApplyEngineState(int iState)
{
	if (!m_pVehicle) return;

	m_iEngineState = iState;

    m_pVehicle->m_nVehicleFlags.bEngineOn = iState;
}

void CVehicle::ApplyLightState(int iState)
{
	if (!m_pVehicle)
        return;

	m_bIsLightOn = static_cast<eLightsState>(iState);
    m_pVehicle->m_nVehicleFlags.bLightsOn = iState;
}
// 0.3.7
void CVehicle::ProcessMarkers()
{
	if(!m_pVehicle || !GamePool_Vehicle_GetAt(m_dwGTAId))
		return;

	if(m_byteObjectiveVehicle)
	{
		if(!m_bSpecialMarkerEnabled)
		{
			if(m_dwMarkerID)
			{
				pGame->DisableMarker(m_dwMarkerID);
				m_dwMarkerID = 0;
			}

			ScriptCommand(&tie_marker_to_car, m_dwGTAId, 1, 3, &m_dwMarkerID);
			ScriptCommand(&set_marker_color, m_dwMarkerID, 1006);
			ScriptCommand(&show_on_radar, m_dwMarkerID, 3);

            CHook::CallFunction<void>("_ZN6CRadar16ChangeBlipColourEij", m_dwMarkerID, TranslateColorCodeToRGBA(1006));
			m_bSpecialMarkerEnabled = true;
		}

		return;
	}

	if(m_byteObjectiveVehicle && m_bSpecialMarkerEnabled)
	{
		if(m_dwMarkerID)
		{
			pGame->DisableMarker(m_dwMarkerID);
			m_dwMarkerID = 0;

			m_bSpecialMarkerEnabled = false;
		}
	}

	if(m_pVehicle->GetDistanceFromLocalPlayerPed() < 200.0f && GetTractor() == NULL &&
	   !IsOccupied())
	{
		if(!m_dwMarkerID)
		{
			ScriptCommand(&tie_marker_to_car, m_dwGTAId, 1, 2, &m_dwMarkerID);
			ScriptCommand(&set_marker_color, m_dwMarkerID, 1004);
            CHook::CallFunction<void>("_ZN6CRadar16ChangeBlipColourEij", m_dwMarkerID, TranslateColorCodeToRGBA(1004));
		}
	}

	else if(IsOccupied() || GetTractor() != NULL ||
            m_pVehicle->GetDistanceFromLocalPlayerPed() >= 200.0f)
	{
		if(m_dwMarkerID)
		{
			pGame->DisableMarker(m_dwMarkerID);
			m_dwMarkerID = 0;
		}
	}
}
// 0.3.7
bool CVehicle::IsOccupied()
{
	if (m_pVehicle)
	{
		if (m_pVehicle &&
			(m_pVehicle->m_apPassengers[0] ||
			m_pVehicle->m_apPassengers[1] ||
			m_pVehicle->m_apPassengers[2] ||
			m_pVehicle->m_apPassengers[3] ||
			m_pVehicle->m_apPassengers[4] ||
			m_pVehicle->m_apPassengers[5] ||
			m_pVehicle->m_apPassengers[6])) {
			return true;
		}
	}

	return false;
}

bool CVehicle::IsATowTruck()
{
	if(!m_pVehicle) return false;
	return m_pVehicle->m_nModelIndex == 525;
}

bool CVehicle::IsATrailer()
{
	if(!m_pVehicle) return false;

  	int nModel = m_pVehicle->m_nModelIndex;
	return (nModel == 435 ||
		nModel == 450 || 
		nModel == 584 || 
		nModel == 591 || 
		nModel == 606 || 
		nModel == 607 || 
		nModel == 608 || 
		nModel == 610 || 
		nModel == 611
	);
}

void CVehicle::UpdateDamageStatus(uint32_t dwPanelDamage, uint32_t dwDoorDamage, uint8_t byteLightDamage, uint8_t byteWheelDamage)
{
    if(m_pVehicle)
    {
        if(GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR)
        {
            Log("UpdateDamageStatus 1");
            if(!dwPanelDamage && !dwDoorDamage && !byteLightDamage)
            {
                if(GetPanelDamageStatus() || GetDoorDamageStatus() || GetLightDamageStatus())
                {
                    // CAutoMobile::Fix
                    Log("UpdateDamageStatus 2");
                    (( void (*)(CVehicleGTA*))(g_libGTASA+(VER_x32 ? 0x55D5C0+1:0x67DF0C)))(m_pVehicle);
                    return;
                }
            }
            Log("UpdateDamageStatus 3");

            SetDoorDamageStatus(dwDoorDamage);
            Log("UpdateDamageStatus 3 1");
            SetPanelDamageStatus(dwPanelDamage);
            Log("UpdateDamageStatus 3 2");
            SetLightDamageStatus(byteLightDamage);

            Log("UpdateDamageStatus 4");
            // CAutomobile::SetupDamageAfterLoad
            (( void (*)(CVehicleGTA*))(g_libGTASA+(VER_x32 ? 0x55D886+1:0x67E368)))(m_pVehicle);
            Log("UpdateDamageStatus 5");
        }
    }
}

void CVehicle::SetPanelDamageStatus(uint32_t dwPanelStatus)
{
    if(m_pVehicle && GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR)
        ((CAutomobile*)m_pVehicle)->m_nDamageManager.m_nPanelsStatus = dwPanelStatus;
}

uint32_t CVehicle::GetPanelDamageStatus()
{
	if(m_pVehicle && GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR)
		return ((CAutomobile*)m_pVehicle)->m_nDamageManager.m_nPanelsStatus;

	return 0;
}

void CVehicle::SetDoorDamageStatus(uint32_t dwDoorStatus)
{
	if(m_pVehicle && GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR)
	{
        ((CAutomobile*)m_pVehicle)->m_nDamageManager.m_aDoorsStatus[0] = static_cast<eDoorStatus>(dwDoorStatus & 7);
        ((CAutomobile*)m_pVehicle)->m_nDamageManager.m_aDoorsStatus[1] = static_cast<eDoorStatus>((dwDoorStatus >> 8) & 7);
        ((CAutomobile*)m_pVehicle)->m_nDamageManager.m_aDoorsStatus[2] = static_cast<eDoorStatus>((dwDoorStatus >> 16) & 7);
        ((CAutomobile*)m_pVehicle)->m_nDamageManager.m_aDoorsStatus[3] = static_cast<eDoorStatus>((dwDoorStatus >> 24) & 7);
	}
}

uint32_t CVehicle::GetDoorDamageStatus()
{
	if(m_pVehicle && GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR)
	{
		uint32_t dwDoorStatus = (((CAutomobile*)m_pVehicle)->m_nDamageManager.m_aDoorsStatus[0]
                | (((CAutomobile*)m_pVehicle)->m_nDamageManager.m_aDoorsStatus[1] << 8)
                | (((CAutomobile*)m_pVehicle)->m_nDamageManager.m_aDoorsStatus[2] << 16)
                | (((CAutomobile*)m_pVehicle)->m_nDamageManager.m_aDoorsStatus[3] << 24));
		return dwDoorStatus;
	}

	return 0;
}

void CVehicle::SetLightDamageStatus(uint8_t byteLightStatus)
{
	if(m_pVehicle && GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR)
        ((CAutomobile*)m_pVehicle)->m_nDamageManager.m_nLightsStatus = (uint32_t)byteLightStatus;
}

uint8_t CVehicle::GetLightDamageStatus()
{
	if(m_pVehicle && GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR)
		return (uint8_t)((CAutomobile*)m_pVehicle)->m_nDamageManager.m_nLightsStatus;

	return 0;
}

void CVehicle::SetWheelPoppedStatus(uint8_t byteWheelStatus)
{
	if(m_pVehicle)
	{
		if(GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR)
		{
            ((CAutomobile*)m_pVehicle)->m_nDamageManager.m_anWheelsStatus[3] = static_cast<eCarWheelStatus>(byteWheelStatus & 1);
            ((CAutomobile*)m_pVehicle)->m_nDamageManager.m_anWheelsStatus[2] = static_cast<eCarWheelStatus>((byteWheelStatus >> 1) & 1);
            ((CAutomobile*)m_pVehicle)->m_nDamageManager.m_anWheelsStatus[1] = static_cast<eCarWheelStatus>((byteWheelStatus >> 2) & 1);
            ((CAutomobile*)m_pVehicle)->m_nDamageManager.m_anWheelsStatus[0] = static_cast<eCarWheelStatus>((byteWheelStatus >> 3) & 1);
		}
		else if(GetVehicleSubtype() == VEHICLE_SUBTYPE_BIKE)
		{
            ((CBike*)m_pVehicle)->m_nWheelStatus[1] = byteWheelStatus & 1;
            ((CBike*)m_pVehicle)->m_nWheelStatus[0] = (byteWheelStatus >> 1) & 1;
		}
	}
}

uint8_t CVehicle::GetWheelPoppedStatus()
{
	/*if(m_pVehicle)
	{
		if(GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR)
		{
			uint8_t byteWheelStatus = (m_pVehicle->byteWheelStatus[3] | (m_pVehicle->byteWheelStatus[2] << 1) | (m_pVehicle->byteWheelStatus[1] << 2) | (m_pVehicle->byteWheelStatus[0] << 3));
			return byteWheelStatus;
		}
		else if(GetVehicleSubtype() == VEHICLE_SUBTYPE_BIKE)
		{
			uint8_t byteWheelStatus = (m_pVehicle->byteBikeWheelStatus[1] | (m_pVehicle->byteBikeWheelStatus[0] << 1));
			return byteWheelStatus;
		}
	}*/

	return 0;
}

bool CVehicle::SirenEnabled()
{
	if(!m_pVehicle)
		return m_pVehicle->m_nVehicleFlags.bSirenOrAlarm;

	return false;
}

void CVehicle::EnableSiren(bool bState)
{
	if(!m_pVehicle) return;

	m_pVehicle->m_nVehicleFlags.bSirenOrAlarm = (int)bState;
}

void CVehicle::UpdateColor()
{
	if(m_pVehicle)
	{
		if(m_bHaveColor)
		{
			if(!m_bHasBeenDriven)
			{
				if(m_pVehicle->m_nPrimaryColor != m_byteColor1 || m_pVehicle->m_nSecondaryColor != m_byteColor2)
				{
					m_pVehicle->m_nPrimaryColor = m_byteColor1;
					m_pVehicle->m_nSecondaryColor = m_byteColor2;
				}
			}
		}
	}
}

#include "CCustomPlateManager.h"
void CVehicle::setPlate(ePlateType type, std::string& szNumber, std::string& szRegion)
{

	if(pPlateTexture) {
		RwTextureDestroy(pPlateTexture);
		pPlateTexture = nullptr;
	}

    pPlateTexture = CCustomPlateManager::createTexture(type, szNumber, szRegion);
}

#include "Textures/TextureDatabaseRuntime.h"
void CVehicle::setVinyl(const char* remapName) {
    m_pVinylTex = nullptr;

    if (!remapName || !strcmp(remapName, "empty")) {
        return;
    }

	m_pVinylTex = TextureDatabaseRuntime::GetTexture(remapName);
}

void CVehicle::processHeadlightsColor(uint8_t& r, uint8_t& g, uint8_t& b) {
	if (GetVehicleSubtype() != VEHICLE_SUBTYPE_CAR) {
		return;
	}

    const CRGBA& color = (m_iStrobeType != STROBE_OFF) ? strobeColor : lightColor;

	r = color.r;
	g = color.g;
	b = color.b;
}
void CVehicle::processHeadlightsBeamColor(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
	if (GetVehicleSubtype() != VEHICLE_SUBTYPE_CAR) {
		return;
	}

    const CRGBA& color = (m_iStrobeType != STROBE_OFF) ? strobeColor : lightColor;

	r = color.r;
	g = color.g;
	b = color.b;
    a = color.a;
}

void CVehicle::SetNeonColor(uint8_t slot, const CRGBA& color)
{
    neon.SetColor(slot, color);
}

bool CVehicle::HasNeon() const
{
    return neon.IsSet();
}

eLightsState_Old CVehicle::GetLightState(eLights light)
{
    if (!m_pVehicle || GetVehicleSubtype() != VEHICLE_SUBTYPE_CAR) {
        return VEHICLE_LIGHT_OK;
    }

    return ((CAutomobile*)m_pVehicle)->m_nDamageManager.GetLightStatus(light);
}

void CVehicle::SetLightState(eLights light, eLightsState_Old state)
{
    if (m_pVehicle && GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR) {
        ((CAutomobile*)m_pVehicle)->m_nDamageManager.SetLightStatus(light, state);
    }
}

namespace
{
    constexpr uint32_t kStrobeStepMs = 40;

    // Кадр мигалки: состояние левой и правой фары
    struct SStrobeFrame
    {
        eLightsState_Old left;
        eLightsState_Old right;
    };

    constexpr eLightsState_Old kLightOn  = VEHICLE_LIGHT_OK;
    constexpr eLightsState_Old kLightOff = VEHICLE_LIGHT_SMASHED;

    constexpr SStrobeFrame kFramesPolice[] = {
            { kLightOn,  kLightOff }, { kLightOff, kLightOff }, { kLightOn,  kLightOff }, { kLightOff, kLightOff }, { kLightOn,  kLightOff }, { kLightOff, kLightOff },
            { kLightOff, kLightOn  }, { kLightOff, kLightOff }, { kLightOff, kLightOn  }, { kLightOff, kLightOff }, { kLightOff, kLightOn  }, { kLightOff, kLightOff }
    };

    constexpr SStrobeFrame kFramesAlternate[] = {
            { kLightOn,  kLightOff }, { kLightOff, kLightOn  }, { kLightOff, kLightOff }
    };

    constexpr SStrobeFrame kFramesTogether[] = {
            { kLightOn,  kLightOn  }, { kLightOff, kLightOff }, { kLightOn,  kLightOn  }, { kLightOff, kLightOff },
            { kLightOn,  kLightOn  }, { kLightOff, kLightOff }, { kLightOn,  kLightOn  }, { kLightOff, kLightOff }
    };

    constexpr SStrobeFrame kFramesDouble[] = {
            { kLightOn,  kLightOff }, { kLightOff, kLightOff }, { kLightOn,  kLightOff }, { kLightOff, kLightOff },
            { kLightOff, kLightOn  }, { kLightOff, kLightOff }, { kLightOff, kLightOn  }, { kLightOff, kLightOff }
    };

    struct SStrobePattern
    {
        const SStrobeFrame* frames;
        uint32_t            count;   // сколько шагов задают свет
        uint32_t            period;  // длина цикла вместе с паузой в конце
    };

    constexpr SStrobePattern kStrobePatterns[STROBE_TYPE_COUNT - 1] = {
            { kFramesPolice,    std::size(kFramesPolice),    15 },
            { kFramesAlternate, std::size(kFramesAlternate),  6 },
            { kFramesTogether,  std::size(kFramesTogether),  13 },
            { kFramesDouble,    std::size(kFramesDouble),    11 }
    };
}

void CVehicle::SetStrobeType(uint8_t type)
{
    if (!m_pVehicle || GetVehicleSubtype() != VEHICLE_SUBTYPE_CAR) {
        return;
    }

    if (type >= STROBE_TYPE_COUNT) {
        type = STROBE_OFF;
    }

    if (m_iStrobeType == STROBE_OFF && type != STROBE_OFF) {
        m_savedLightLeft  = GetLightState(LIGHT_FRONT_LEFT);
        m_savedLightRight = GetLightState(LIGHT_FRONT_RIGHT);
    }

    SetLightState(LIGHT_FRONT_LEFT,  m_savedLightLeft);
    SetLightState(LIGHT_FRONT_RIGHT, m_savedLightRight);

    m_iStrobeType      = (eStrobeType)type;
    m_dwStrobeStep     = 0;
    m_dwLastStrobeTime = 0;
    strobeColor        = lightColor;
}

void CVehicle::ProcessStrobes()
{
    if (m_iStrobeType == STROBE_OFF || !m_pVehicle || GetVehicleSubtype() != VEHICLE_SUBTYPE_CAR) {
        return;
    }

    const uint32_t dwNow = CTimer::m_snTimeInMillisecondsNonClipped;
    if (dwNow - m_dwLastStrobeTime < kStrobeStepMs) {
        return;
    }
    m_dwLastStrobeTime = dwNow;

    const SStrobePattern& pattern = kStrobePatterns[m_iStrobeType - STROBE_TYPE_1];

    if (++m_dwStrobeStep >= pattern.period) {
        m_dwStrobeStep = 0;
        return;
    }

    // Первый тип — милицейская сине-красная, остальные светят цветом фар
    if (m_iStrobeType == STROBE_TYPE_1) {
        const bool bFirstHalf = m_dwStrobeStep <= pattern.count / 2;
        strobeColor = bFirstHalf ? CRGBA(255, 0, 0, lightColor.a)
                                 : CRGBA(0, 0, 255, lightColor.a);
    } else {
        strobeColor = lightColor;
    }

    if (m_dwStrobeStep > pattern.count) {
        return;
    }

    const SStrobeFrame& frame = pattern.frames[m_dwStrobeStep - 1];
    SetLightState(LIGHT_FRONT_LEFT,  frame.left);
    SetLightState(LIGHT_FRONT_RIGHT, frame.right);
}

bool CVehicle::UpdateLastDrivenTime()
{
	if(m_pVehicle)
	{
		if(m_pVehicle->pDriver)
		{
			m_bHasBeenDriven = true;
			m_dwTimeSinceLastDriven = GetTickCount();
			return true;
		}
	}

	return false;
}
#include "Models/ClumpModelInfo.h"
#include "Plugins/NodeNamePlugin/NodeName.h"

void CVehicle::UpdateSteeringWheel()
{
    if (!m_pVehicle) {
        return;
    }

    RpClump *clump = (RpClump*)m_pVehicle->m_pRwObject;
    if (!clump || clump->object.type != rpCLUMP) {
        return;
    }

    RwFrame *outer = CClumpModelInfo::GetFrameFromName(clump, "br_steering");
    if (!outer) {
        return;
    }

    RwFrame *wheel = nullptr;
    for (RwFrame *child = outer->child; child; child = child->next) {
        const RwChar *name = GetFrameNodeName(child);
        if (name && !strcmp(name, "br_steering")) {
            wheel = child;
            break;
        }
    }

    if (!wheel) {
        return;
    }

    const float angle = m_pVehicle->m_fSteerAngle * ms_fSteeringWheelRatio;
    const float c = cosf(angle);
    const float s = sinf(angle);

    RwMatrix *matrix = RwFrameGetMatrix(wheel);
    if (!matrix) {
        return;
    }

    matrix->right = RwV3d{  c, 0.0f, s };
    matrix->up    = RwV3d{ 0.0f, 1.0f, 0.0f };
    matrix->at    = RwV3d{ -s, 0.0f, c };
    matrix->pos   = RwV3d{ 0.0f, 0.0f, 0.0f };
    matrix->flags = 0;

    RwFrameUpdateObjects(wheel);
}

bool CVehicle::GetHandPosition(bool bLeft, CVector &out)
{
    if (!m_pVehicle) {
        return false;
    }

    RpClump *clump = (RpClump*)m_pVehicle->m_pRwObject;
    if (!clump || clump->object.type != rpCLUMP) {
        return false;
    }

    RwFrame *hand = CClumpModelInfo::GetFrameFromName(clump, bLeft ? "LHandPos" : "RHandPos");
    if (!hand) {
        return false;
    }

    RwMatrix *ltm = RwFrameGetLTM(hand);
    if (!ltm) {
        return false;
    }

    out = CVector(ltm->pos.x, ltm->pos.y, ltm->pos.z);
    return true;
}

// ---------------------------------------------------------------------------
//  Кузовной тюнинг
// ---------------------------------------------------------------------------

namespace {

struct SAtomicList {
    RpAtomic** items;
    uint32_t   count;
    uint32_t   capacity;
};

RwObject* CollectAtomicCB(RwObject* object, void* data) {
    auto list = (SAtomicList*)data;

    if (object && list->count < list->capacity) {
        list->items[list->count++] = (RpAtomic*)object;
    }

    return object;
}

// Гашение и возврат стокового атомика по имени его фрейма. Игра держит две
// версии геометрии — целую и битую, поэтому включать можно только целую.
void SetFrameAtomicsVisible(RpClump* clump, const char* frameName, bool bVisible) {
    if (!clump || !frameName || !frameName[0]) {
        return;
    }

    RwFrame* frame = CClumpModelInfo::GetFrameFromName(clump, frameName);
    if (!frame) {
        return;
    }

    RpAtomic* storage[8]{};
    SAtomicList list{ storage, 0, 8 };

    RwFrameForAllObjects(frame, CollectAtomicCB, &list);

    uint32_t visible = 0;
    for (uint32_t i = 0; i < list.count; i++) {
        if (list.items[i]->object.object.flags & 0x04) {
            visible++;
        }
    }

    if (bVisible && visible == 0) {
        for (uint32_t i = 0; i < list.count; i++) {
            const int id = CVisibilityPlugins::GetAtomicId(list.items[i]);

            if (!(id & eAtomicComponentFlag::ATOMIC_IS_DAM_STATE)) {
                list.items[i]->object.object.flags |= 0x04;
            }
        }
        return;
    }

    if (!bVisible && visible > 0) {
        for (uint32_t i = 0; i < list.count; i++) {
            list.items[i]->object.object.flags &= ~0x05;
        }
    }
}

} // namespace

namespace {

void Fail(const char* reason)
{
    strncpy(CVehicle::ms_szLastComponentError, reason,
            sizeof(CVehicle::ms_szLastComponentError) - 1);

    Log("CustomTune: %s", reason);
}

} // namespace

bool CVehicle::SetCustomComponent(int32_t selector, int32_t tuneId) {
    ms_szLastComponentError[0] = 0;
    RemoveCustomComponent(selector);

    if (tuneId < 0 || !m_pVehicle) {
        return true;
    }

    const SCustomTunePart* part = CCustomTune::Find(tuneId);
    if (!part) {
        Fail("детали нет в customtune.dat");
        return false;
    }

    // Каждая деталь сделана под свою модель машины
    if (part->vehicleModel != m_pVehicle->m_nModelIndex) {
        Fail("деталь от другой модели машины");
        return false;
    }

    auto clump = (RpClump*)m_pVehicle->m_pRwObject;
    if (!clump || clump->object.type != rpCLUMP) {
        Fail("у машины нет клумпа");
        return false;
    }

    RwFrame* attach = CClumpModelInfo::GetFrameFromName(clump, part->dummy);
    if (!attach) {
        Fail("в модели нет фрейма для крепления");
        return false;
    }

    // Модели детали в сборке может не быть вовсе. Проверять это надо ДО запроса:
    // CStreaming::RequestModel сам лезет в CBaseModelInfo и падает на нулевом.
    if (part->componentModel < 0 || part->componentModel >= CModelInfo::NUM_MODEL_INFOS) {
        Fail("модель детали вне таблицы");
        return false;
    }

    auto info = CModelInfo::GetModelInfo(part->componentModel);
    if (!info) {
        Fail("модели детали нет в этой сборке");
        return false;
    }

    if (!CStreaming::IsModelLoaded(part->componentModel)) {
        if (!CStreaming::GetInfo(part->componentModel).HasCdPosnAndSize()) {
            Fail("модель детали не лежит ни в одном архиве");
            return false;
        }

        CStreaming::RequestModel(part->componentModel, STREAMING_GAME_REQUIRED);

        // Ровно один проход. Повторные вызовы блокируют главный поток, и на
        // модели, которая не отдаётся, игра просто зависает.
        CStreaming::LoadAllRequestedModels(false);
    }

    if (!CStreaming::IsModelLoaded(part->componentModel) || !info->m_pRwObject) {
        char reason[128];
        snprintf(reason, sizeof(reason),
                 "модель детали %d не догрузилась (в потоке %d, объект %d)",
                 part->componentModel,
                 (int)CStreaming::IsModelLoaded(part->componentModel),
                 info->m_pRwObject != nullptr);
        Fail(reason);
        return false;
    }

    RpAtomic* atomic = RpAtomicClone((RpAtomic*)info->m_pRwObject);
    if (!atomic) {
        Fail("не удалось скопировать геометрию");
        return false;
    }

    RwFrame* frame = RwFrameCreate();
    if (!frame) {
        RpAtomicDestroy(atomic);
        Fail("не удалось создать фрейм");
        return false;
    }

    RpAtomicSetFrame(atomic, frame);
    RpClumpAddAtomic(clump, atomic);
    RwFrameAddChild(attach, frame);
    RwFrameUpdateObjects(frame);

    SetFrameAtomicsVisible(clump, part->hideAtomic, false);

    SCustomComponent component;
    component.modelId = part->componentModel;
    component.pAtomic = atomic;
    component.pFrame = frame;
    strncpy(component.hidden, part->hideAtomic, sizeof(component.hidden) - 1);

    m_customComponents[selector] = component;

    return true;
}

void CVehicle::RemoveCustomComponent(int32_t selector) {
    auto it = m_customComponents.find(selector);
    if (it == m_customComponents.end()) {
        return;
    }

    SCustomComponent& component = it->second;

    if (m_pVehicle && m_pVehicle->m_pRwObject) {
        auto clump = (RpClump*)m_pVehicle->m_pRwObject;

        if (component.pAtomic) {
            RpClumpRemoveAtomic(clump, component.pAtomic);
        }

        SetFrameAtomicsVisible(clump, component.hidden, true);
    }

    if (component.pFrame) {
        RwFrameRemoveChild(component.pFrame);
    }

    if (component.pAtomic) {
        RpAtomicDestroy(component.pAtomic);
    }

    if (component.pFrame) {
        RwFrameDestroy(component.pFrame);
    }

    m_customComponents.erase(it);
}

void CVehicle::RemoveAllCustomComponents() {
    while (!m_customComponents.empty()) {
        RemoveCustomComponent(m_customComponents.begin()->first);
    }
}

// ---------------------------------------------------------------------------
//  Геометрия колёс
// ---------------------------------------------------------------------------

CCollisionData* GetCollisionDataFromModel(int nModelIndex) {
    auto mi = CModelInfo::GetModelInfo(nModelIndex);
    if (!mi || !mi->m_pColModel) {
        return nullptr;
    }

    return mi->m_pColModel->m_pColData;
}

CColLine* GetSuspensionLinesFromModel(int nModelIndex, int& numWheels) {
    numWheels = 0;

    auto colData = GetCollisionDataFromModel(nModelIndex);
    if (!colData) {
        return nullptr;
    }

    numWheels = colData->m_nNumLines;
    return colData->m_pLines;
}

void CVehicle::CopyGlobalSuspensionLinesToPrivate() {
    if (!m_pVehicle) {
        return;
    }

    int numWheels = 0;
    const CColLine* lines = GetSuspensionLinesFromModel(m_pVehicle->m_nModelIndex, numWheels);
    if (!lines || numWheels <= 0) {
        bHasSuspensionLines = false;
        m_nSuspensionLinesCopied = 0;
        return;
    }

    // Модель могут подменить на лету, и линий станет больше, чем в буфере
    if (numWheels > m_nSuspensionLineCount) {
        if (m_pSuspensionLines) {
            CMemoryMgr::Free(m_pSuspensionLines);
        }

        m_pSuspensionLines = static_cast<CColLine*>(CMemoryMgr::Malloc(numWheels * sizeof(CColLine)));
        m_nSuspensionLineCount = m_pSuspensionLines ? numWheels : 0;
    }

    if (!m_pSuspensionLines) {
        bHasSuspensionLines = false;
        m_nSuspensionLinesCopied = 0;
        return;
    }

    memcpy(m_pSuspensionLines, lines, numWheels * sizeof(CColLine));

    m_nSuspensionLinesCopied = numWheels;
    bHasSuspensionLines = true;
}

void CVehicle::ReleaseSuspensionLines() {
    if (m_pSuspensionLines) {
        CMemoryMgr::Free(m_pSuspensionLines);
        m_pSuspensionLines = nullptr;
    }

    m_nSuspensionLineCount = 0;
    m_nSuspensionLinesCopied = 0;
    bHasSuspensionLines = false;
}

void CVehicle::RebuildSuspensionLines() {
    if (!m_pVehicle) {
        return;
    }

    if (GetVehicleSubtype() != VEHICLE_SUBTYPE_CAR) {
        return;
    }

    InitWheelFrames();

    // Снимок заводского радиуса делается вместе с фреймами колёс. Пока его нет,
    // трогать общий радиус модели нельзя — иначе он уедет в ноль у всех машин
    // этой модели. Пересчёт отложится до кадра, когда машина попадёт в стрим.
    if (!m_pWheelClump || m_fDefaultWheelSizeFront <= 0.0f) {
        m_bSuspensionRebuildPending = true;
        return;
    }

    auto mi = CModelInfo::GetVehicleModelInfo(m_pVehicle->m_nModelIndex);
    if (!mi || !mi->m_pColModel) {
        m_bSuspensionRebuildPending = true;
        return;
    }

    m_bSuspensionRebuildPending = false;

    // SetupSuspensionLines считает линии по радиусу колеса из modelinfo, а он
    // общий на всю модель, поэтому свой радиус ставится только на время расчёта
    mi->m_fWheelSizeFront = m_fWheelScaleFront;
    mi->m_fWheelSizeRear = m_fWheelScaleRear;

    CHook::CallFunction<void>("_ZN11CAutomobile20SetupSuspensionLinesEv", m_pVehicle);

    ((CAutomobile*)m_pVehicle)->m_aWheelSuspensionHeights[1] =
            m_fWheelScaleRear * 0.5f - ((CAutomobile*)m_pVehicle)->m_fRearHeightAboveRoad;
    ((CAutomobile*)m_pVehicle)->m_aWheelSuspensionHeights[3] =
            m_fWheelScaleRear * 0.5f - ((CAutomobile*)m_pVehicle)->m_fRearHeightAboveRoad;

    CopyGlobalSuspensionLinesToPrivate();

    mi->m_fWheelSizeFront = m_fDefaultWheelSizeFront;
    mi->m_fWheelSizeRear = m_fDefaultWheelSizeRear;

    CHook::CallFunction<void>("_ZN11CAutomobile17ProcessSuspensionEv", m_pVehicle);
}

void CVehicle::InitWheelFrames() {
    if (!m_pVehicle) {
        return;
    }

    auto clump = (RpClump*)m_pVehicle->m_pRwObject;
    if (!clump || clump->object.type != rpCLUMP) {
        return;
    }

    if (m_pWheelClump == clump) {
        return;
    }

    m_pWheelClump = nullptr;
    m_pWheelLF = m_pWheelRF = m_pWheelRB = m_pWheelLB = nullptr;

    auto mi = CModelInfo::GetVehicleModelInfo(m_pVehicle->m_nModelIndex);
    if (!mi) {
        return;
    }

    m_fDefaultWheelSizeFront = m_fWheelScaleFront = mi->m_fWheelSizeFront;
    m_fDefaultWheelSizeRear = m_fWheelScaleRear = mi->m_fWheelSizeRear;

    m_pWheelLF = CClumpModelInfo::GetFrameFromName(clump, "wheel_lf_dummy");
    m_pWheelRF = CClumpModelInfo::GetFrameFromName(clump, "wheel_rf_dummy");
    m_pWheelRB = CClumpModelInfo::GetFrameFromName(clump, "wheel_rb_dummy");
    m_pWheelLB = CClumpModelInfo::GetFrameFromName(clump, "wheel_lb_dummy");

    if (!m_pWheelLF || !m_pWheelRF || !m_pWheelRB || !m_pWheelLB) {
        return;
    }

    memcpy(&m_initialWheelMatrix[0], &m_pWheelLF->modelling, sizeof(RwMatrix));
    memcpy(&m_initialWheelMatrix[1], &m_pWheelRF->modelling, sizeof(RwMatrix));
    memcpy(&m_initialWheelMatrix[2], &m_pWheelRB->modelling, sizeof(RwMatrix));
    memcpy(&m_initialWheelMatrix[3], &m_pWheelLB->modelling, sizeof(RwMatrix));

    m_pWheelClump = clump;

    CopyGlobalSuspensionLinesToPrivate();
}

namespace {

// Вылет пишется прямо в позицию фрейма колеса. UpdateWheelMatrix снимает tx и
// ty до масштабирования и возвращает обратно, поэтому смещение переживает кадр.
void ApplyWheelOffset(RwFrame* frame, const RwMatrix* initial, bool bLeft, float value) {
    if (!frame) {
        return;
    }

    CVector offset;
    offset.x = bLeft ? value : -value;
    offset.y = 0.0f;
    offset.z = 0.0f;

    CVector out;
    ProjectMatrix(&out, (CMatrix*)initial, &offset);

    frame->modelling.pos = *(RwV3d*)&out;
}

} // namespace

void CVehicle::ProcessWheelsOffset() {
    InitWheelFrames();

    if (!m_pWheelClump) {
        return;
    }

    if (m_bSuspensionRebuildPending) {
        RebuildSuspensionLines();
    }

    if (GetTickCount() - m_uiLastWheelOffset <= 30) {
        return;
    }

    if (!m_bWheelOffsetDoneFront) {
        ApplyWheelOffset(m_pWheelLF, &m_initialWheelMatrix[0], true, m_fWheelOffsetFront);
        ApplyWheelOffset(m_pWheelRF, &m_initialWheelMatrix[1], false, m_fWheelOffsetFront);

        m_bWheelOffsetDoneFront = true;
    }

    if (!m_bWheelOffsetDoneBack) {
        ApplyWheelOffset(m_pWheelRB, &m_initialWheelMatrix[2], false, m_fWheelOffsetBack);
        ApplyWheelOffset(m_pWheelLB, &m_initialWheelMatrix[3], true, m_fWheelOffsetBack);

        m_bWheelOffsetDoneBack = true;
    }
}

void CVehicle::SetWheelParameters(const float* values) {
    if (!values || !m_pVehicle) {
        return;
    }

    if (GetVehicleSubtype() != VEHICLE_SUBTYPE_CAR) {
        return;
    }

    InitWheelFrames();

    m_fClearance = values[0];
    m_fClearanceRear = values[1];

    const float offsetFront = values[2] / 100.0f;
    const float offsetBack = values[3] / 100.0f;

    if (offsetFront != m_fWheelOffsetFront) {
        m_fWheelOffsetFront = offsetFront;
        m_bWheelOffsetDoneFront = false;
    }

    if (offsetBack != m_fWheelOffsetBack) {
        m_fWheelOffsetBack = offsetBack;
        m_bWheelOffsetDoneBack = false;
    }

    m_uiLastWheelOffset = GetTickCount();

    m_fWheelAngleFront = values[4] * 3.14159274f / 180.0f;
    m_fWheelAngleBack = values[5] * 3.14159274f / 180.0f;

    m_fWheelWidthFront = values[6] / 100.0f;
    m_fWheelWidthBack = values[7] / 100.0f;

    m_fWheelScaleFront = m_fDefaultWheelSizeFront + values[8] / 100.0f;
    m_fWheelScaleRear = m_fDefaultWheelSizeRear + values[9] / 100.0f;

    RebuildHandling();
    RebuildSuspensionLines();
}

// ---------------------------------------------------------------------------
//  Хендлинг
// ---------------------------------------------------------------------------

namespace {

float CombineHandling(float stock, uint8_t mode, float value, float mass) {
    switch (mode) {
        case HANDLING_MODE_ABSOLUTE:   return value;
        case HANDLING_MODE_MASS_SCALE: return mass * value;
        default:                       return stock * value;
    }
}

void ApplyHandlingEdit(tHandlingData* handling, const CVehicle::SHandlingEdit& edit, float mass) {
    switch (edit.param) {
        case HANDLING_PARAM_MAX_SPEED:
            handling->m_transmissionData.m_fMaxGearVelocity = CombineHandling(
                    handling->m_transmissionData.m_fMaxGearVelocity, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_ACCELERATION:
            handling->m_transmissionData.m_fEngineAcceleration = CombineHandling(
                    handling->m_transmissionData.m_fEngineAcceleration, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_DRIVE_TYPE: {
            static const char kDrive[] = { 'R', 'F', '4' };
            const int index = (int)edit.value - 1;

            if (index >= 0 && index < 3) {
                handling->m_transmissionData.m_nDriveType = kDrive[index];
            }
            break;
        }

        case HANDLING_PARAM_ENGINE_INERTIA:
            handling->m_transmissionData.m_fEngineInertia = CombineHandling(
                    handling->m_transmissionData.m_fEngineInertia, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_MASS:
            handling->m_fMass = CombineHandling(handling->m_fMass, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_TURN_MASS:
            handling->m_fTurnMass = CombineHandling(handling->m_fTurnMass, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_BRAKE_DECELERATION:
            handling->m_fBrakeDeceleration = CombineHandling(
                    handling->m_fBrakeDeceleration, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_TRACTION_MULTIPLIER:
            handling->m_fTractionMultiplier = CombineHandling(
                    handling->m_fTractionMultiplier, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_TRACTION_LOSS:
            handling->m_fTractionLoss = CombineHandling(
                    handling->m_fTractionLoss, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_TRACTION_BIAS:
            handling->m_fTractionBias = CombineHandling(
                    handling->m_fTractionBias, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_SUSPENSION_LOWER:
            handling->m_fSuspensionLowerLimit = CombineHandling(
                    handling->m_fSuspensionLowerLimit, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_SUSPENSION_BIAS:
            handling->m_fSuspensionBiasBetweenFrontAndRear = CombineHandling(
                    handling->m_fSuspensionBiasBetweenFrontAndRear, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_STEERING_LOCK:
            handling->m_fSteeringLock = CombineHandling(
                    handling->m_fSteeringLock, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_SUSPENSION_FORCE:
            handling->m_fSuspensionForceLevel = CombineHandling(
                    handling->m_fSuspensionForceLevel, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_SUSPENSION_DAMPING:
            handling->m_fSuspensionDampingLevel = CombineHandling(
                    handling->m_fSuspensionDampingLevel, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_SUSPENSION_HIGH_DAMP:
            handling->m_fSuspensionHighSpdComDamp = CombineHandling(
                    handling->m_fSuspensionHighSpdComDamp, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_SUSPENSION_UPPER:
            handling->m_fSuspensionUpperLimit = CombineHandling(
                    handling->m_fSuspensionUpperLimit, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_SUSPENSION_ANTIDIVE:
            handling->m_fSuspensionAntiDiveMultiplier = CombineHandling(
                    handling->m_fSuspensionAntiDiveMultiplier, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_BRAKE_BIAS:
            handling->m_fBrakeBias = CombineHandling(
                    handling->m_fBrakeBias, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_NUMBER_OF_GEARS:
            handling->m_transmissionData.m_nNumberOfGears = (uint8_t)edit.value;
            break;

        case HANDLING_PARAM_DRAG_MULT:
            handling->m_fDragMult = CombineHandling(
                    handling->m_fDragMult, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_CENTRE_OF_MASS_Z:
            handling->m_vecCentreOfMass.z = CombineHandling(
                    handling->m_vecCentreOfMass.z, edit.mode, edit.value, mass);
            break;

        case HANDLING_PARAM_COLLISION_DAMAGE:
            handling->m_fCollisionDamageMultiplier = CombineHandling(
                    handling->m_fCollisionDamageMultiplier, edit.mode, edit.value, mass);
            break;

        default:
            break;
    }
}

} // namespace

void CVehicle::ApplyHandlingEdits(const SHandlingEdit* edits, uint8_t count) {
    m_handlingEditCount = 0;

    if (edits) {
        for (uint8_t i = 0; i < count && i < kMaxHandlingEdits; i++) {
            m_handlingEdits[m_handlingEditCount++] = edits[i];
        }
    }

    RebuildHandling();
    RebuildSuspensionLines();
}

void CVehicle::ReleaseHandling() {
    if (!m_pCustomHandling) {
        return;
    }

    if (m_pVehicle && m_pStockHandling) {
        m_pVehicle->m_pHandlingData = m_pStockHandling;
    }

    delete m_pCustomHandling;
    m_pCustomHandling = nullptr;
}

void CVehicle::RebuildHandling() {
    if (!m_pVehicle || !CHandlingDefault::EnsureReady()) {
        return;
    }

    if (GetVehicleSubtype() != VEHICLE_SUBTYPE_CAR) {
        return;
    }

    auto mi = CModelInfo::GetVehicleModelInfo(m_pVehicle->m_nModelIndex);
    if (!mi) {
        return;
    }

    const tHandlingData* stock = CHandlingDefault::Get(mi->m_nHandlingId);
    if (!stock) {
        return;
    }

    if (!m_pCustomHandling) {
        m_pStockHandling = m_pVehicle->m_pHandlingData;
        m_pCustomHandling = new tHandlingData;
    }

    memcpy(m_pCustomHandling, stock, sizeof(tHandlingData));

    // Сила подвески и масса вращения задаются долей от массы машины, иначе на
    // тяжёлых кузовах колёса проваливаются сквозь землю.
    const float stockMass = stock->m_fMass;

    for (uint8_t i = 0; i < m_handlingEditCount; i++) {
        ApplyHandlingEdit(m_pCustomHandling, m_handlingEdits[i], stockMass);
    }

    // Клиренс опускает и поднимает машину, поэтому правит пределы подвески
    if (m_fClearance != 0.0f) {
        m_pCustomHandling->m_fSuspensionLowerLimit += m_fClearance / 100.0f;
        m_pCustomHandling->m_fSuspensionUpperLimit += m_fClearance / 100.0f;
    }

    if (m_fClearanceRear != 0.0f) {
        m_pCustomHandling->m_fSuspensionBiasBetweenFrontAndRear += m_fClearanceRear / 1000.0f;
    }

    CHandlingDefault::ConvertToGameUnits(m_pCustomHandling);

    m_pVehicle->m_pHandlingData = m_pCustomHandling;
    m_pVehicle->m_fMass = m_pCustomHandling->m_fMass;
    m_pVehicle->m_fTurnMass = m_pCustomHandling->m_fTurnMass;
}
