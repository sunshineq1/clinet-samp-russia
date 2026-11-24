#include "main.h"
#include "game.h"
#include "vendor/armhook/patch.h"
#include "Streaming.h"
#include "CustomTune.h"

extern void readVehiclesAudioSettings();
#include "MemoryMgr.h"
#include "CFileMgr.h"
#include "game/Textures/TextureDatabaseRuntime.h"
#include "Scene.h"
#include "TxdStore.h"
#include "VisibilityPlugins.h"
#include "net/netgame.h"
#include "CrossHair.h"
#include "Pickups.h"
#include "game/Models/ModelInfo.h"
#include "game/Enums/eModelID.h"
#include "Mirrors.h"
#include "Water/CWaterFX.h"
#include "game/Collision/Collision.h"
#include "CRenderTarget.h"
#include "java/GUIManager.h"
#include "java/SnapShotsWrapper.h"
#include "GPS.h"
#include "Mobile/MobileMenu/MobileMenu.h"

#include "java/jniutil.h"
extern CJavaWrapper* pJavaWrapper;

void ApplySAMPPatchesInGame();
void InitScripting();

bool bUsedPlayerSlots[PLAYER_PED_SLOTS];
int  g_nUsedPlayerSlots = 0;

uint16_t *szGameTextMessage;

bool CGame::bIsGameExiting = false;

inline int FindFirstFreePlayerPedSlot()
{
    for (uint8_t x = 2; x < PLAYER_PED_SLOTS; ++x) {
        if (!bUsedPlayerSlots[x]) {
            Log("Found free slot: %d", x);
            return x;
        }
    }
    Log("No free slot found!");
    // Return -1 or an appropriate error code if no free slot is found
    return -1;
}

CGame::CGame()
{
    m_pGameCamera = new CCamera();
	m_pGamePlayer = nullptr;
	m_bCheckpointsEnabled = false;
	m_bRaceCheckpointsEnabled = false;
	m_dwRaceCheckpointHandle = 0;

	m_bClockEnabled = false;
	m_bInputEnable = true;

    m_iLocalMoney = 0;

	memset(bUsedPlayerSlots, 0, sizeof(bUsedPlayerSlots));
	g_nUsedPlayerSlots = 0;
	memset(m_bPreloadedVehicleModels, 0, sizeof(m_bPreloadedVehicleModels));
}

CGame::~CGame()
{

}

void CGame::FixSettings() {
    CMobileMenu::ms_MobileSettings[MS_Brightness].value = 90;
    CMobileMenu::ms_MobileSettings[MS_Resolution].value = 100;
    CMobileMenu::ms_MobileSettings[MS_CarReflections].value = 3;
    CMobileMenu::ms_MobileSettings[MS_DrawDistance].value = 75;
    CMobileMenu::ms_MobileSettings[MS_Shadows].value = 0;
    CMobileMenu::ms_MobileSettings[MS_Visuals].value = 1;
}

void ApplyGlobalPatches();
void InstallHooks();
void CGame::StartGame()
{
	Log("Starting game..");

    InstallHooks();
    ApplyGlobalPatches();

	GameAimSyncInit();
}

void InstallSAMPHooks();
void InstallWidgetHooks();

void CGame::Initialize()
{
	Log("CGame initializing..");

    ApplySAMPPatchesInGame();
	GameResetRadarColors();

    szGameTextMessage = new uint16_t[1076];
}

void CGame::SetMaxStats()
{
    CHook::CallFunction<void>("_ZN6CCheat18VehicleSkillsCheatEv");
    CHook::CallFunction<void>("_ZN6CCheat17WeaponSkillsCheatEv");

    // CStats::SetStatValue nop
    CHook::RET("_ZN6CStats12SetStatValueEtf");
}
// 0.3.7
void CGame::ToggleThePassingOfTime(bool bOnOff)
{
	if (bOnOff)
	{
		CHook::WriteMemory(g_libGTASA + (VER_x32 ? 0x3E33C8:0x4C20A4),
                           (uintptr_t)(VER_x32 ? "\xD0\xB5":"\xF7\x0F\x1C\xF8"), (VER_x32 ? 2:4));
		this->m_bClockEnabled = true;
	}
	else
	{
		CHook::RET("_ZN6CClock6UpdateEv");
		this->m_bClockEnabled = false;
	}
}
// 0.3.7
void CGame::EnableClock(bool bEnable)
{
	char byteClockData[] = { '%', '0', '2', 'd', ':', '%', '0', '2', 'd', 0 };
	CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x2BD618:0x73E1DA));

	if (bEnable)
	{
		ToggleThePassingOfTime(true);
		memcpy((void*)(g_libGTASA + (VER_x32 ? 0x2BD618:0x73E1DA)), byteClockData, 10);
	}
	else
	{
		ToggleThePassingOfTime(false);
		memset((void*)(g_libGTASA + (VER_x32 ? 0x2BD618:0x73E1DA)), 0, 10);
	}
}
// 0.3.7
void CGame::EnableZoneNames(bool bEnable)
{
	ScriptCommand(&enable_zone_names, bEnable);
}
// 0.3.7
void CGame::SetWorldTime(int iHour, int iMinute)
{
    *(uint8_t*)(g_libGTASA + (VER_x32 ? 0x00953143 : 0xBBBC1B)) = (uint8_t)iMinute;
    *(uint8_t*)(g_libGTASA + (VER_x32 ? 0x00953142 : 0xBBBC1A)) = (uint8_t)iHour;
    ScriptCommand(&set_current_time, iHour, iMinute);
}
// 0.3.7
void CGame::GetWorldTime(int *iHour, int *iMinute)
{
	*iMinute = *(uint8_t*)(g_libGTASA + (VER_x32 ? 0x00953143 : 0xBBBC1B));
	*iHour = *(uint8_t*)(g_libGTASA + (VER_x32 ? 0x00953142 : 0xBBBC1A));
}
// 0.3.7
void CGame::PreloadObjectsAnims()
{
	// keep the throwable weapon models loaded
	if(!IsModelLoaded(WEAPON_MODEL_TEARGAS)) RequestModel(WEAPON_MODEL_TEARGAS);
	if(!IsModelLoaded(WEAPON_MODEL_GRENADE)) RequestModel(WEAPON_MODEL_GRENADE);
	if(!IsModelLoaded(WEAPON_MODEL_MOLOTOV)) RequestModel(WEAPON_MODEL_MOLOTOV);

	// special action object
	if(!IsModelLoaded(330)) RequestModel(330);
	if(!IsModelLoaded(OBJECT_PARACHUTE)) RequestModel(OBJECT_PARACHUTE);
	if(!IsModelLoaded(OBJECT_CJ_CIGGY)) RequestModel(OBJECT_CJ_CIGGY);
	if(!IsModelLoaded(OBJECT_DYN_BEER_1)) RequestModel(OBJECT_DYN_BEER_1);
	if(!IsModelLoaded(OBJECT_CJ_BEER_B_2)) RequestModel(OBJECT_CJ_BEER_B_2);
	if(!IsModelLoaded(OBJECT_CJ_PINT_GLASS)) RequestModel(OBJECT_CJ_PINT_GLASS);
	if(!IsModelLoaded(18631)) RequestModel(18631);

	// special action anim
	if(IsAnimationLoaded("PARACHUTE") == 0) RequestAnimation("PARACHUTE");
	if(IsAnimationLoaded("PAULNMAC") == 0) RequestAnimation("PAULNMAC");
	if(IsAnimationLoaded("BAR") == 0) RequestAnimation("BAR");
	if(IsAnimationLoaded("SMOKING") == 0) RequestAnimation("SMOKING");
	if(IsAnimationLoaded("DANCING") == 0) RequestAnimation("DANCING");
	if(IsAnimationLoaded("GFUNK") == 0) RequestAnimation("GFUNK");
	if(IsAnimationLoaded("RUNNINGMAN") == 0) RequestAnimation("RUNNINGMAN");
	if(IsAnimationLoaded("STRIP") == 0) RequestAnimation("STRIP");
	if(IsAnimationLoaded("WOP") == 0) RequestAnimation("WOP");
}
// 0.3.7
void CGame::SetWorldWeather(int byteWeatherID)
{
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005CDF88 + 1 : 0x6F24E8), byteWeatherID);

    if(!m_bClockEnabled)
    {
        *(uint16_t*)(g_libGTASA + (VER_x32 ? 0x00A7D136 : 0xD216F2)) = byteWeatherID;
        *(uint16_t*)(g_libGTASA + (VER_x32 ? 0x00A7D134 : 0xD216F0)) = byteWeatherID;
    }
}
// 0.3.7
void CGame::DisplayHUD(bool bDisp)
{
    // todo: gui packet
    // return;

	if (bDisp)
	{
		*(uint8_t*)(g_libGTASA + (VER_x32 ? 0x819D88 : 0x9FF3A8)) = 1;
		*(uint8_t*)(g_libGTASA + (VER_x32 ? 0x991FD8 : 0xC20DFC)) = 0;
	}
	else
	{
		*(uint8_t*)(g_libGTASA + (VER_x32 ? 0x819D88 : 0x9FF3A8)) = 0;
		*(uint8_t*)(g_libGTASA + (VER_x32 ? 0x991FD8 : 0xC20DFC)) = 1;
	}
}
// 0.3.7
uint8_t CGame::GetActiveInterior()
{
	return CGame::currArea;
}

const char* CGame::GetDataDirectory()
{
	return ""; // StorageRootBuffer
}
// 0.3.7
void CGame::UpdateCheckpoints()
{
	if (m_bCheckpointsEnabled)
	{
		CPlayerPed* pPlayerPed = this->FindPlayerPed();
		if (pPlayerPed) 
		{
			ScriptCommand(&is_actor_near_point_3d, pPlayerPed->m_dwGTAId,
				m_vecCheckpointPos.x, m_vecCheckpointPos.y, m_vecCheckpointPos.z,
				m_vecCheckpointExtent.x, m_vecCheckpointExtent.y, m_vecCheckpointExtent.z, 1);

			if (!m_dwCheckpointMarker)
			{
				m_dwCheckpointMarker = CreateRadarMarkerIcon(0, m_vecCheckpointPos.x,
					m_vecCheckpointPos.y, m_vecCheckpointPos.z, 1005, 0);
			}
		}
	}
	else if (m_dwCheckpointMarker)
	{
		DisableMarker(m_dwCheckpointMarker);
		m_dwCheckpointMarker = 0;
	}

	if (m_bRaceCheckpointsEnabled)
	{
		CPlayerPed* pPlayerPed = this->FindPlayerPed();
		if (pPlayerPed)
		{
			if (!m_dwRaceCheckpointMarker)
			{
				m_dwRaceCheckpointMarker = CreateRadarMarkerIcon(0, m_vecRaceCheckpointPos.x,
					m_vecRaceCheckpointPos.y, m_vecRaceCheckpointPos.z, TranslateColorCodeToRGBA(1005), 0);
			}
		}
	}
	else if (m_dwRaceCheckpointMarker)
	{
		DisableMarker(m_dwRaceCheckpointMarker);
		DisableRaceCheckpoint();
		m_dwRaceCheckpointMarker = 0;
	}
}
// 0.3.7
uint8_t CGame::GetPedSlotsUsed()
{
	uint8_t count = 0;
	for (int i = 2; i < PLAYER_PED_SLOTS; i++)
	{
		if (bUsedPlayerSlots[i])
			count++;
	}

	return count;
}

void CGame::PlaySound(int iSound, float fX, float fY, float fZ)
{
	ScriptCommand(&play_sound, fX, fY, fZ, iSound);
}
// 0.3.7
void CGame::RefreshStreamingAt(float x, float y)
{
	ScriptCommand(&refresh_streaming_at, x, y);
}
// 0.3.7
void CGame::DisableTrainTraffic()
{
	ScriptCommand(&enable_train_traffic, 0);
}
// 0.3.7
void CGame::UpdateGlobalTimer(uint32_t dwTimer)
{
	if (!m_bClockEnabled)
	{
		*(uint32_t*)(g_libGTASA + 0x96B4D8) = dwTimer & 0x3FFFFFFF;
	}
}
// 0.3.7
void CGame::SetGravity(float fGravity)
{
#if VER_x32
    CHook::UnFuck(g_libGTASA + (VER_2_1 ? 0x003FE810 : 0x3A0B64));
    *(float*)(g_libGTASA + (VER_2_1 ? 0x003FE810 : 0x3A0B64)) = fGravity;
#endif
}

bool CGame::IsGamePaused()
{
	return *(uint8_t*)(g_libGTASA + (VER_x32 ? 0x96B514 : 0xBDC594));
}

bool CGame::IsGameLoaded()
{
	return true;
}

void CGame::DrawGangZone(float fPos[], uint32_t dwColor, uint32_t dwUnk)
{
	// CRadar::DrawAreaOnRadar
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x00443C60 + 1 : 0x528EC4), fPos, &dwColor, dwUnk);
}
// 0.3.7
uint32_t CGame::CreatePickup(int iModel, int iType, float x, float y, float z, int *pdwIndex)
{
    uintptr hnd;

    auto dwModelArray = CModelInfo::ms_modelInfoPtrs;
    if(dwModelArray[iModel] == nullptr)
        iModel = 18631; // вопросик

    ScriptCommand(&create_pickup, iModel, iType, x, y, z, &hnd);

    int lol = 32 * (uint16_t)hnd;
    if(lol) lol /= 32;
    if(pdwIndex) *pdwIndex = lol;

    return hnd;
}
// 0.3.7
bool CGame::IsModelLoaded(int iModel)
{
	return CStreaming::IsModelLoaded(iModel);
}

// 0.3.7
void CGame::RequestModel(uint16_t iModelId, uint8_t iLoadingStream) 
{
	// ScriptCommand(&request_model, iModelId);
    return CStreaming::RequestModel(iModelId);
}

// 0.3.7
void CGame::RemoveModel(int iModel, bool bFromStreaming)
{
    return CStreaming::RemoveModel(iModel);
}

CObject* CGame::NewObject(int iModel, CVector vecPos, CVector vecRot, float fDrawDistance)
{
	CObject *pObjectNew = new CObject(iModel, vecPos, vecRot, fDrawDistance, 0);
	return pObjectNew;
}
// 0.3.7 (�� ����������� ������ bIsNPC)
CPlayerPed* CGame::NewPlayer(int iSkin, float fX, float fY, float fZ, float fRotation, bool unk, bool bIsNPC)
{
	int iPedSlot = FindFirstFreePlayerPedSlot();
	if (iPedSlot < 0) return nullptr;

	auto bytePedSlot = (uint8_t)iPedSlot;
	auto pPed = new CPlayerPed(bytePedSlot, iSkin, fX, fY, fZ, fRotation);
	if (pPed && pPed->m_pPed) {
		bUsedPlayerSlots[bytePedSlot] = true;
		g_nUsedPlayerSlots++;
		return pPed;
	}

	if (pPed) {
		delete pPed;
	}

	return nullptr;
}
// 0.3.7
bool CGame::RemovePlayer(CPlayerPed* pPed)
{
	if (!pPed) return false;

	const uint8_t bytePedSlot = pPed->m_bytePlayerNumber;

	delete pPed;

	if (bytePedSlot < PLAYER_PED_SLOTS && bUsedPlayerSlots[bytePedSlot]) {
		bUsedPlayerSlots[bytePedSlot] = false;
		if (g_nUsedPlayerSlots > 0) {
			g_nUsedPlayerSlots--;
		}
	}

	return true;
}

void CGame::DisableMarker(uint32_t dwMarker) {
    ScriptCommand(&disable_marker, dwMarker);
}

#include "Radar.h"
uintptr CGame::CreateRadarMarkerIcon(uint8_t byteType, float fPosX, float fPosY, float fPosZ, uint32_t dwColor, uint8_t byteStyle)
{
    auto pos = CVector(fPosX, fPosY, fPosZ);
    uintptr dwMarkerID = 0;
    bool bNeedMarker = false;

    if(byteType == 40)
        byteType = 31;

    if (byteStyle == 1) {
        ScriptCommand(&create_marker_icon, fPosX, fPosY, fPosZ, byteType, &dwMarkerID);
    } else if (byteStyle == 2) {
        ScriptCommand(&create_radar_marker_icon, fPosX, fPosY, fPosZ, byteType, &dwMarkerID);
    } else if (byteStyle == 3) {
        ScriptCommand(&create_icon_marker_sphere, fPosX, fPosY, fPosZ, byteType, &dwMarkerID);
    } else {
        dwMarkerID = CRadar::SetCoordBlip(BLIP_COORD, pos, BLIP_DISPLAY_BOTH);
        CRadar::ChangeBlipScale(dwMarkerID, 3);
        CRadar::ChangeBlipColour(dwMarkerID, TranslateColorCodeToRGBA(dwColor));
        CRadar::SetBlipSprite(dwMarkerID, byteType);
        bNeedMarker = true;
    }

    if (!bNeedMarker && byteStyle == 0) {
        if (dwColor >= 1004) {
            ScriptCommand(&show_on_radar, dwMarkerID, 3);
        } else {
            ScriptCommand(&show_on_radar, dwMarkerID, 2);
        }
    }

    return dwMarkerID;
}

// 0.3.7
bool CGame::IsAnimationLoaded(const char* szAnimLib)
{
	return ScriptCommand(&is_animation_loaded, szAnimLib);
}
// 0.3.7
void CGame::RequestAnimation(const char* szAnimLib)
{
	ScriptCommand(&request_animation, szAnimLib);
}

// CWorld::FindGroundZForCoord
float CGame::FindGroundZForCoord(float fX, float fY, float fZ)
{
    return CHook::CallFunction<float>(g_libGTASA + (VER_x32 ? 0x0042A7C4 + 1 : 0x50F3A0), fX, fY);
}

// 0.3.7
void CGame::DisableAutoAim() {
}
void CGame::EnabledAutoAim() {
}

// 0.3.7
CVehicle* CGame::NewVehicle(int iVehicleType, float fX, float fY, float fZ, float fRotation, bool bAddSiren)
{
	bool bPreloaded = false;
	if (m_bPreloadedVehicleModels[iVehicleType - 400] == true) {
		bPreloaded = true;
	}

	CVehicle* pNewVehicle = new CVehicle(iVehicleType, fX, fY, fZ, fRotation, bPreloaded, bAddSiren);

	return pNewVehicle;
}

// 0.3.7
void CGame::SetCheckpointInformation(CVector* vecPos, CVector* vecSize)
{
	m_vecCheckpointPos.x = vecPos->x;
	m_vecCheckpointPos.y = vecPos->y;
	m_vecCheckpointPos.z = vecPos->z;

	m_vecCheckpointExtent.x = vecSize->x;
	m_vecCheckpointExtent.y = vecSize->y;
	m_vecCheckpointExtent.z = vecSize->z;
	
	if (m_dwCheckpointMarker)
	{
		DisableMarker(m_dwCheckpointMarker);
		m_dwCheckpointMarker = 0;

		m_dwCheckpointMarker = CreateRadarMarkerIcon(0,
			m_vecCheckpointPos.x,
			m_vecCheckpointPos.y,
			m_vecCheckpointPos.z,
			1005, 0);
	}
}
// 0.3.7
void CGame::SetRaceCheckpointInformation(uint8_t byteType, CVector* vecPos, CVector* vecNextPos, float fRadius)
{
	m_vecRaceCheckpointPos.x = vecPos->x;
	m_vecRaceCheckpointPos.y = vecPos->y;
	m_vecRaceCheckpointPos.z = vecPos->z;

	m_vecRaceCheckpointNextPos.x = vecNextPos->x;
	m_vecRaceCheckpointNextPos.y = vecNextPos->y;
	m_vecRaceCheckpointNextPos.z = vecNextPos->z;

	m_byteRaceType = byteType;
	m_fRaceCheckpointRadius = fRadius;

	if (m_dwRaceCheckpointMarker)
	{
		DisableMarker(m_dwRaceCheckpointMarker);
		
		m_dwRaceCheckpointMarker = CreateRadarMarkerIcon(0,
			m_vecRaceCheckpointPos.x,
			m_vecRaceCheckpointPos.y,
			m_vecRaceCheckpointPos.z,
			1005,
			0);
	}

	MakeRaceCheckpoint();
}
// 0.3.7
void CGame::MakeRaceCheckpoint()
{
	DisableRaceCheckpoint();

	ScriptCommand(&create_racing_checkpoint, (int)m_byteRaceType,
		m_vecRaceCheckpointPos.x, m_vecRaceCheckpointPos.y, m_vecRaceCheckpointPos.z,
		m_vecRaceCheckpointNextPos.x, m_vecRaceCheckpointNextPos.y, m_vecRaceCheckpointNextPos.z,
		m_fRaceCheckpointRadius, &m_dwRaceCheckpointHandle);

    EnableGameInput(true);
    FindPlayerPed()->TogglePlayerControllable(true);

	m_bRaceCheckpointsEnabled = true;
}
// 0.3.7
void CGame::DisableRaceCheckpoint()
{
	if (m_dwRaceCheckpointHandle)
	{
		ScriptCommand(&destroy_racing_checkpoint, m_dwRaceCheckpointHandle);
		m_dwRaceCheckpointHandle = 0;
	}

	m_bRaceCheckpointsEnabled = false;
}

void CGame::SetWantedLevel(uint8_t level)
{
	//CHook::WriteMemory(g_libGTASA+0x2BDF6E, (uintptr_t)&level, 1);
}

void CGame::EnableStuntBonus(bool bEnable)
{
	//CHook::UnFuck(0x7BE2A8);
	//*(int*)(g_libGTASA+0x7BE2A8) = (int)bEnable;
}
// 0.3.7
void CGame::DisplayGameText(const char* szStr, int iTime, int iSize)
{
    ScriptCommand(&text_clear_all);
    CFont::AsciiToGxtChar(szStr, szGameTextMessage);

    // CMessages::AddBigMesssage
    (( void (*)(uint16_t*, int, int))(g_libGTASA + (VER_x32 ? 0x0054C62C + 1 : 0x66C150)))(szGameTextMessage, iTime, iSize);
}
// 0.3.7
void CGame::AddToLocalMoney(int iAmmount)
{
    ScriptCommand(&add_to_player_money, 0, iAmmount);
    m_iLocalMoney += iAmmount;

    std::string moneyStr = "{\"value\":" + std::to_string(m_iLocalMoney) + "}";
    GUIManager::LocalPacket(UI_ID_HUD, 2, moneyStr);
}
// 0.3.7
void CGame::ResetLocalMoney()
{
	int iMoney = GetLocalMoney();
	if (!iMoney) return;

	if (iMoney < 0)
		AddToLocalMoney(abs(iMoney));
	else
		AddToLocalMoney(-(iMoney));
}
// 0.3.7
int CGame::GetLocalMoney()
{
	return m_iLocalMoney;
}
// 0.3.7
void CGame::DisableEnterExits()
{
#if VER_x32
    uintptr_t addr = *(uintptr_t*)(g_libGTASA + (VER_2_1 ? 0x007A1E20 : 0x700120));
    int count = *(uint32_t*)(addr+8);

    addr = *(uintptr_t*)addr;

    for(int i=0; i<count; i++)
    {
        *(uint16_t*)(addr+0x30) = 0;
        addr += 0x3C;
    }
#endif
}

void CGame::ToggleCJWalk(bool bUseCJWalk)
{
    if(!bUseCJWalk)
        CHook::NOP(g_libGTASA + (VER_x32 ? 0x004C5F6A : 0x5C3970), (VER_x32 ? 2:4));
    else {
        CHook::WriteMemory(g_libGTASA + (VER_x32 ? 0x4C5F6A:0x5C3970), "\x68\x32\x06\xB9", 4);
    }
}

void CGame::InitialiseOnceBeforeRW() {
    CMemoryMgr::Init();
    CHook::CallFunction<void>("_ZN14MobileSettings10InitializeEv"); // впадлу реверсить т.к. меню надо вообще удалить
    CHook::CallFunction<void>("_ZN13CLocalisation10InitialiseEv");
    CFileMgr::Initialise();
    CdStreamInit(TOTAL_IMG_ARCHIVES); // mb use TOTAL_IMG_ARCHIVES?
    CHook::CallFunction<void>("_ZN4CPad10InitialiseEv");
}

void CameraSize(RwCamera* camera, RwRect* rect, RwReal viewWindow, RwReal aspectRatio) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005D32AC + 1 : 0x6F7F84), camera, rect, viewWindow, aspectRatio);
}

void CameraDestroy(RwCamera* camera) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005D33A4 + 1 : 0x6F80C0), camera);
}

void LightsCreate(RpWorld* world) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x0046FC08 + 1 : 0x55BDCC), world);
}

void InitGui();
bool bEnableWinter = false;
bool CGame::InitialiseRenderWare() {
#ifdef BR_DEBUG
    Log("InitialiseRenderWare...");
#endif

    CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));

    CTxdStore::Initialise();
    CVisibilityPlugins::Initialise();

    char texdbs_file[100+1];
    sprintf(texdbs_file, "%sloader/texdbs.cfg", g_pszStorage);
    auto db_names = TextureDatabaseRuntime::ReadTextureDBNamesFromCFG(texdbs_file);

    // В кеше blackrussia.online texdbs.cfg перечисляет только надстройку, а
    // основные базы опущены — без них не находятся даже шрифты. Дописываем их
    // спереди в том же порядке, в каком они шли в старом кеше.
    static const char *kCoreDatabases[] = {
        "samp", "mobile", "txd", "gta3", "gta_int", "player", "menu",
    };

    for (int i = (int)(sizeof(kCoreDatabases) / sizeof(*kCoreDatabases)) - 1; i >= 0; i--) {
        const char *core = kCoreDatabases[i];
        bool listed = false;

        for (const auto &db_name : db_names) {
            if (strcasecmp(db_name.c_str(), core) == 0) {
                listed = true;
                break;
            }
        }

        if (!listed) {
            db_names.insert(db_names.begin(), core);
            Log("texdb: %s нет в texdbs.cfg, добавлена принудительно", core);
        }
    }

    for (const auto &db_name : db_names) {
        // if (db_name.find("loader_winter") == 0)
        //    bEnableWinter = true;

        if (db_name.find("loader_") == 0) {
            TextureDatabaseRuntime *runtime = TextureDatabaseRuntime::Load(db_name.c_str(), false, TextureDatabaseFormat::DF_DXT);
            if (runtime) {
                TextureDatabaseRuntime::Register(runtime);
            } else {
                Log("texdb: база %s не загружена, в реестр не добавлена", db_name.c_str());
            }
        } else {
            if (!TextureDatabaseRuntime::Load(db_name.c_str(), false, TextureDatabaseFormat::DF_DXT))
                Log("texdb: база %s не загружена", db_name.c_str());
        }
    }

    TextureDatabaseRuntime::CompactRegistered();

    const auto camera = RwCameraCreate();
    if (!camera) {
        CameraDestroy(camera);
        return false;
    }

    const auto frame = RwFrameCreate();
    rwObjectHasFrameSetFrame(&camera->object.object, frame);
    camera->frameBuffer = RwRasterCreate(RsGlobal->maximumWidth, RsGlobal->maximumHeight, 0, rwRASTERTYPECAMERA);
    camera->zBuffer = RwRasterCreate(RsGlobal->maximumWidth, RsGlobal->maximumHeight, 0, rwRASTERTYPEZBUFFER);
    if (!camera->object.object.parent) {
        CameraDestroy(camera);
        return false;
    }
    Scene.m_pRwCamera = camera;
    TheCamera.Init();
    TheCamera.SetRwCamera(Scene.m_pRwCamera);
    RwCameraSetFarClipPlane(Scene.m_pRwCamera, 2000.0f);
    RwCameraSetNearClipPlane(Scene.m_pRwCamera, 0.9f);
    CameraSize(Scene.m_pRwCamera, nullptr, 0.7f, 4.0f / 3.0f);

    RwBBox bb;
    bb.sup = { 10'000.0f,  10'000.0f,  10'000.0f};
    bb.inf = {-10'000.0f, -10'000.0f, -10'000.0f};

    if (Scene.m_pRpWorld = RpWorldCreate(&bb); !Scene.m_pRpWorld) {
        CameraDestroy(Scene.m_pRwCamera);
        Scene.m_pRwCamera = nullptr;

        return false;
    }
    RpWorldAddCamera(Scene.m_pRpWorld, Scene.m_pRwCamera);
    LightsCreate(Scene.m_pRpWorld);
    CHook::CallFunction<void>("_Z15CreateDebugFontv");
    CFont::Initialise();
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x0046FF38 + 1 : 0x55C1C8)); // CHud::Initialise();
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005B1188 + 1 : 0x6D5970)); // CPlayerSkin::Initialise();
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005B28D4 + 1 : 0x6D6E30)); // CPostEffects::Initialise();
    CGame::m_pWorkingMatrix1 = RwMatrixCreate();
    CGame::m_pWorkingMatrix2 = RwMatrixCreate();

    InitGui();

    return true;
}

void CGame::PostToMainThread(std::function<void()> task)
{
    std::lock_guard<std::mutex> lock(mtx);
    tasks.push(std::move(task));
}

void CGame::ProcessMainThreadTasks()
{
    if (tasks.empty())
        return;

    std::function<void()> task;
    {
        std::lock_guard<std::mutex> lock(mtx);

        task = std::move(tasks.front());
        tasks.pop();
    }
    task();
}
extern CGame* pGame;
extern CNetGame* pNetGame;
extern UI *pUI;

void MainLoop();

#include "CustomRadar.h"
#include "Snow.h"
#include "CCustomPlateManager.h"

void (*CGame__Process)();
void CGame__Process_hook() {
    if(CGame::bIsGameExiting)
        return;

    MainLoop();
    if (pNetGame) {
        CObjectPool* pObjectPool = pNetGame->GetObjectPool();
        if (pObjectPool) {
            pObjectPool->Process();
            pObjectPool->ProcessMaterialText();
        }

        CTextDrawPool* pTextDrawPool = pNetGame->GetTextDrawPool();
        if (pTextDrawPool) {
            pTextDrawPool->SnapshotProcess();
        }
    }
    CGame::ProcessMainThreadTasks();

    // CStreaming::Update();
    // if (!CTimer::m_CodePause && !CTimer::m_UserPause)
    // {
    //     if (!CTimer::bSkipProcessThisFrame) {
    //         CPickups::Update();
    //     }
    // }

    static bool onceInit = false;
    if (!onceInit) {
        CRenderTarget::Initialise();
        CSnow::Initialise();
        CustomRadar::Init();
        CCustomPlateManager::Initialise();
        CCustomTune::Load();
        CCustomTune::SelfCheck();
        readVehiclesAudioSettings();
        CRenderTarget::Initialise();
        onceInit = true;
        return;
    }

    CGame__Process();
}

void CGame::Process() {
    if(bIsGameExiting)
        return;

    MainLoop();
    if (pNetGame)
    {
        CObjectPool* pObjectPool = pNetGame->GetObjectPool();
        if (pObjectPool) {
            pObjectPool->Process();
            pObjectPool->ProcessMaterialText();
        }

        CTextDrawPool* pTextDrawPool = pNetGame->GetTextDrawPool();
        if (pTextDrawPool) {
            pTextDrawPool->SnapshotProcess();
        }
    }

    ProcessMainThreadTasks();

    uint32_t CurrentTimeInCycles;
    uint32_t v1; // r4
    uint32_t v2; // r5
    uint32_t v3; // r5

    //FIXME
    ((void(*)())(g_libGTASA + (VER_x32 ? 0x003F8B50 + 1 : 0x4DB464)))(); // CPad::UpdatePads()
    ((void(*)())(g_libGTASA + (VER_x32 ? 0x002B03F8 + 1 : 0x36F374)))(); // CTouchInterface::Clear()
    ((void(*)())(g_libGTASA + (VER_x32 ? 0x0028C178 + 1 : 0x3467BC)))(); // CHID::Update()

//	CLoadMonitor::BeginFrame(&g_LoadMonitor);
    CurrentTimeInCycles = CTimer::GetCurrentTimeInCycles();
    v1 = CurrentTimeInCycles / CTimer::GetCyclesPerMillisecond();

    CStreaming::Update();

    v2 = CTimer::GetCurrentTimeInCycles();
    v3 = v2 / CTimer::GetCyclesPerMillisecond();

    //	CCutsceneMgr::Update();

    if ( !(CTimer::m_CodePause << 0x18) )
    {
        auto gMobileMenu = (uintptr_t *) (g_libGTASA + (VER_x32 ? 0x006E0074 : 0x8BE780));
        ((void(*)(uintptr_t*))(g_libGTASA + (VER_x32 ? 0x0029A730 + 1 : 0x356A7C)))(gMobileMenu); // MobileMenu::Update
    }

    // CTheZones::Update()

    // CCover::Update()

    CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));

//	auto p_tx = (CSimpleTransform *)&TheCamera + 0x14 + 0x30;
//	if ( !TheCamera.m_pMat )
//		p_tx = *TheCamera + 0x4;

    //CAudioZones::Update(0, p_tx->m_translate);

    *(int32_t*)(g_libGTASA + (VER_x32 ? 0x00A7D22C: 0xD217F8)) = 0; // CWindModifiers::Number

    if ( !CTimer::m_CodePause && !CTimer::m_UserPause )
    {
        CSprite2d::SetRecipNearClip();
        ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005C89F8 + 1 : 0x6ECF00)))(); // CSprite2d::InitPerFrame();
        ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005A8A74 + 1 : 0x6CC898)))(); // CFont::InitPerFrame()
        // CCheat::DoCheats();
        // CClock::Update()

        ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005CC2E8 + 1 : 0x6F0BD8)))(); // CWeather::Update()
        ((void(*)())(g_libGTASA + (VER_x32 ? 0x0032AED8 + 1 : 0x3F3AD8)))(); // CTheScripts::Process()
        // CCollision::Update()

        // CPathFind::UpdateStreaming

        // CTrain::UpdateTrains();
        // CHeli::UpdatHelis
        // CDarkel::Update()
        ((void(*)())(g_libGTASA + (VER_x32 ? 0x005BE838 + 1 : 0x6E2F08)))(); // CSkidmarks::Update();
        ((void(*)())(g_libGTASA + (VER_x32 ? 0x005AB4C8 + 1 : 0x6D032C)))(); // CGlass::Update()
        // CWanted::UpdateEachFrame();
        // CCreepingFire::Update();
        // CSetPieces::Update();

        auto gFireManager = (uintptr_t *) (g_libGTASA + (VER_x32 ? 0x00958800 : 0xBC12D8));
        ((void (*)(uintptr_t *)) (g_libGTASA + (VER_x32 ? 0x003F1628 + 1 : 0x4D361C)))(gFireManager); // CFireManager::Update

        // FIXME: add if
        ((void(*)(bool))(g_libGTASA + (VER_x32 ? 0x004CC380 + 1 : 0x5CB5E0)))(false); // CPopulation::Update нужно (

        ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005DB8E8 + 1 : 0x700AF4)))(); // CWeapon::UpdateWeapons()
//		if ( !CCutsceneMgr::ms_running )
//			CTheCarGenerators::Process();
//		CCranes::UpdateCranes();
//		CClouds::Update();
        ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005A6720 + 1 : 0x6CA130)))(); // CMovingThings::Update();
        ((void(*)())(g_libGTASA + (VER_x32 ? 0x005CBB20 + 1 : 0x6F04CC)))(); // CWaterCannons::Update()
//		CUserDisplay::Process();
        ((void (*)()) (g_libGTASA + (VER_x32 ? 0x00427744 + 1 : 0x50BE40)))(); // CWorld::Process()

//		CLoadMonitor::EndFrame(&g_LoadMonitor);

        if ( !CTimer::bSkipProcessThisFrame )
        {
            CPickups::Update();
//			CCarCtrl::PruneVehiclesOfInterest();
//          CGarages::Update();
// 			CEntryExitManager::Update();
//          CStuntJumpManager::Update();
            ((void (*)()) (g_libGTASA + (VER_x32 ? 0x0059CFC0 + 1 : 0x6C13F0)))(); // CBirds::Update()
            ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005C03E4 + 1 : 0x6E4A7C)))(); // CSpecialFX::Update()
            // CRopes::Update();
        }
        ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005B28D8 + 1 : 0x6D6E34)))(); // CPostEffects::Update()
        ((void (*)()) (g_libGTASA + (VER_x32 ? 0x0041EF78 + 1 : 0x502ADC)))(); // CTimeCycle::Update() crash without
        // CPopCycle::Update()

        // CInterestingEvents::ScanForNearbyEntities

        ((void (*)(CCamera*)) (g_libGTASA + (VER_x32 ? 0x003DC7D0 + 1 : 0x4BAB78)))(&TheCamera); // CCamera::Process()

        // CCullZones::Update() менты не могут найти?
        // CGameLogic::Update() + CHook::CallFunction<void>(g_libGTASA+(VER_x32 ? 0x307D8C+1:0x3CD630));
        // CGangWars::Update();
        // CConversations::Update()
        // CPedToPlayerConversations::Update()
        // CBridge::Update()
#ifndef ENABLE_SKYBOX
        ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005A3E40 + 1 : 0x6C75E4)))(); // CCoronas::DoSunAndMoon()
#endif
        ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005A22C8 + 1 : 0x6C5BE0)))(); // CCoronas::Update()
        ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005BD370 + 1 : 0x6E1BC4)))(); // CShadows::UpdatePermanentShadows()

        // CPlantMgr::Update

        ((void (*)()) (g_libGTASA + (VER_x32 ? 0x002CA3A4 + 1 : 0x38B6DC)))(); // CCustomBuildingRenderer::Update()
//		if ( v6 <= 3 )
//			CCarCtrl::GenerateRandomCars();
//		CRoadBlocks::GenerateRoadBlocks();
//		CCarCtrl::RemoveDistantCars();
//		CCarCtrl::RemoveCarsIfThePoolGetsFull();
        auto temp = TheCamera.m_pRwCamera;

        auto g_fx = *(uintptr_t *) (g_libGTASA + (VER_x32 ? 0x00820520 : 0xA062A8));
        ((void (*)(uintptr_t*, RwCamera*, float )) (g_libGTASA + (VER_x32 ? 0x00363DE0 + 1 : 0x433F48)))(&g_fx, temp, CTimer::ms_fTimeStep / 50.0f); // Fx_c::Update

        auto g_breakMan = (uintptr_t *) (g_libGTASA + (VER_x32 ? 0x0099DD14 : 0xC31CF0));
        ((void (*)(uintptr_t*, float )) (g_libGTASA + (VER_x32 ? 0x0045267C + 1 : 0x53AFDC)))(g_breakMan, CTimer::ms_fTimeStep); // BreakManager_c::Update

        // InteriorManager_c::Update(&g_interiorMan);
        // ProcObjectMan_c::Update

        // WaterCreatureManager_c::Update

        ((void (*)()) (g_libGTASA + (VER_x32 ? 0x00596540 + 1 : 0x6BB3A8)))(); // CWaterLevel::PreRenderWater()

        // CSnow::Process();
    }

    static bool once = false;
    if (!once)
    {
        CSnow::Initialise();
        CustomRadar::Init();
        CCustomPlateManager::Initialise();
        CRenderTarget::Initialise();
        once = true;
        return;
    }

    SnapShotsWrapper::Process();
}

void CGame::InjectHooks()
{
    CHook::Redirect("_ZN5CGame22InitialiseOnceBeforeRWEv", &CGame::InitialiseOnceBeforeRW);
    CHook::Redirect("_ZN5CGame20InitialiseRenderWareEv", &CGame::InitialiseRenderWare);

    CHook::Redirect("_ZN5CGame7ProcessEv", &CGame::Process);
    // CHook::InlineHook("_ZN5CGame7ProcessEv", &CGame__Process_hook, &CGame__Process);

    CHook::Redirect("_ZN5CGame5Init2EPKc", &Init2);

    CHook::Write(g_libGTASA + (VER_x32 ? 0x00678C38 : 0x84F8A0), &CGame::currArea);

    CHook::Write(g_libGTASA + (VER_x32 ? 0x006796E8 : 0x850DF0), &CGame::m_pWorkingMatrix1);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x00677B38 : 0x84D6A0), &CGame::m_pWorkingMatrix2);
}

bool CGame::CanSeeOutSideFromCurrArea() {
    return currArea == AREA_CODE_NORMAL_WORLD;
}

#include "Draw.h"
#include "World.h"
#include "Animation/AnimManager.h"
bool CGame::Init2(const char *pDatFile)
{
    //std::ranges::for_each(CWorld::Players, [](auto& info) {
    //    info.Clear();
    //});
    CHook::CallFunction<void>("_ZN11CWaterLevel20WaterLevelInitialiseEv"); // CWaterLevel::WaterLevelInitialise();
    CDraw::SetFOV(120.0);
    CDraw::ms_fLODDistance = 500.0f;
    if (!CHook::CallFunction<bool>("_ZN18CCustomCarPlateMgr10InitialiseEv")) {
        Log("CCustomCarPlateMgr not init");
        return false;
    }
    CHook::CallFunction<void>("_ZN10CStreaming15LoadInitialPedsEv"); // CStreaming::LoadInitialPeds();
    CStreaming::LoadAllRequestedModels(false);
    {
        auto* mi = CModelInfo::GetModelInfo(MODEL_MALE01);
        const int32 animIdx = mi ? mi->GetAnimFileIndex() : -1;
        Log("Init2: MODEL_MALE01 loadState %d, rwObject %p, animFileIndex %d, animLoadState %d, cdPosn %u",
            (int)CStreaming::GetInfo(MODEL_MALE01).m_nLoadState,
            mi ? (void*)mi->m_pRwObject : nullptr,
            animIdx,
            animIdx != -1 ? (int)CStreaming::GetInfo(IFPToModelId(animIdx)).m_nLoadState : -1,
            CStreaming::GetInfo(MODEL_MALE01).GetCdPosn());
    }
    CAnimManager::LoadAnimFiles();
    CHook::CallFunction<void>("_ZN10CStreaming18LoadInitialWeaponsEv"); // CStreaming::LoadInitialWeapons();
    CStreaming::LoadAllRequestedModels(false);
    CHook::CallFunction<void>("_ZN8CPedType10InitialiseEv"); // CPed::Initialise();
    CRadar::Initialise();
    CRadar::LoadTextures();
    CHook::CallFunction<void>("_ZN7CWeapon17InitialiseWeaponsEv"); // CWeapon::InitialiseWeapons();
    CWorld::PlayerInFocus = 0;
    CHook::CallFunction<void>("_ZN8CCoronas4InitEv"); // CCoronas::Init();
    CHook::CallFunction<void>("_ZN8CShadows4InitEv"); // CShadows::Init();
    CHook::CallFunction<void>("_ZN14CWeaponEffects4InitEv"); // CWeaponEffects::Init();
    CHook::CallFunction<void>("_ZN10CSkidmarks4InitEv"); // CSkidmarks::Init();
    CHook::CallFunction<void>("_ZN6CGlass4InitEv"); // CGlass::Init();
    CHook::CallFunction<void>("_ZN11CTheScripts4InitEv");
    CHook::CallFunction<void>("_ZN6CClock10InitialiseEj", 0x3E8u); // CClock::Initialise(uint32 nMillisecondsPerGameMinute);
    CHook::CallFunction<void>("_ZN13CMovingThings4InitEv"); // CMovingThings::Init();

    CHook::CallFunction<void>("_ZN6CStats4InitEv");

    CHook::CallFunction<void>("_ZN7CClouds4InitEv"); // CClouds::Init();
    CHook::CallFunction<void>("_ZN10CSpecialFX4InitEv"); // CSpecialFX::Init();
    CHook::CallFunction<void>("_ZN13CWaterCannons4InitEv"); // CWaterCannons::Init();

    return true;
}