#include "../main.h"
#include "Models/ModelInfo.h"
#include "../game/game.h"
#include "../vendor/armhook/patch.h"
#include "Mobile/MobileMenu/MobileMenu.h"
#include "Timer.h"
#include "../CSettings.h"
extern CSettings* pSettings;

VehicleAudioPropertiesStruct VehicleAudioProperties[20000];
#include "game.h"
#include "World.h"
#include "net/netgame.h"

extern CGame* pGame;

void fixUVAnim(bool enable) {
#if VER_x32
    CHook::WriteMemory(g_libGTASA + 0x6BD1E4, (uintptr_t)"\x01", 1);
#else
    CHook::Write(g_libGTASA + 0x89AA88, &enable);

    CHook::NOP(g_libGTASA + 0x25EE0C, 1);
    CHook::NOP(g_libGTASA + 0x25F5EC, 1);
    CHook::NOP(g_libGTASA + 0x25EDC8, 1);
#endif
}

void ApplyFPSPatch(uint8_t fps)
{
    fps = FORCED_FPS;

    CTimer::SetTargetFPS((float)fps);

#if VER_x32
    // GameState.cpp:366 movs r0, #0x1e  /  movne r0, #0x14
    CHook::WriteMemory(g_libGTASA + 0x5E49E0, (uintptr_t)&fps, 1);
    CHook::WriteMemory(g_libGTASA + 0x5E49E6, (uintptr_t)&fps, 1);
    // GameState.cpp:304 movs r1, #0x1e
    CHook::WriteMemory(g_libGTASA + 0x5E492E, (uintptr_t)&fps, 1);

    CGame::PostToMainThread([=] {
        CHook::UnFuck(g_libGTASA + 0x009FC8FC);
        RsGlobal->maxFPS = fps;
    });
#else
    CHook::NOP(g_libGTASA + 0x70A474, 1);
    CHook::NOP(g_libGTASA + 0x70A398, 1);

    CHook::NOP(g_libGTASA + 0x70A38C, 1);
    CHook::NOP(g_libGTASA + 0x70A43C, 1);
    CHook::NOP(g_libGTASA + 0x70A458, 1);

    CGame::PostToMainThread([=] {
        auto RsGlobal = (RsGlobalType*)(g_libGTASA + 0xC9B320);
        CHook::UnFuck(g_libGTASA + 0xC9B320);

        RsGlobal->maxFPS = fps;
    });
#endif

#ifdef BR_DEBUG
    Log("FPS in game: %d", fps);
#endif
}

/*
#if VER_x32
    uint8_t fps = 0x78;

    CHook::WriteMemory(g_libGTASA + 0x5E49E0, (uintptr_t)&fps, 1);
	CHook::WriteMemory(g_libGTASA + 0x5E49E2, (uintptr_t)&fps, 1);
#else
    CHook::WriteMemory(g_libGTASA + 0x70A38C, "\x89\x0F\x80\x52", 4);
    CHook::WriteMemory(g_libGTASA + 0x70A43C, "\x89\x0F\x80\x52", 4);
    CHook::WriteMemory(g_libGTASA + 0x70A458, "\x89\x0F\x80\x52", 4);
#endif
 */

void DisableAutoAim()
{
    CHook::RET("_ZN10CPlayerPed22FindWeaponLockOnTargetEv"); // CPed::FindWeaponLockOnTarget
    CHook::RET("_ZN10CPlayerPed26FindNextWeaponLockOnTargetEP7CEntityb"); // CPed::FindNextWeaponLockOnTarget
    CHook::RET("_ZN4CPed21SetWeaponLockOnTargetEP7CEntity"); // CPed::SetWeaponLockOnTarget
}

void ApplySAMPPatchesInGame()
{
#ifdef BR_DEBUG
    Log("Installing patches (ingame)..");
#endif

    // CTheZones::ZonesVisited[100]
    memset((void*)(g_libGTASA + (VER_x32 ? 0x0098D252 : 0xC1BF92)), 1, 100);
    // CTheZones::ZonesRevealed
    *(uint32_t*)(g_libGTASA + (VER_x32 ? 0x0098D2B8 : 0xC1BFF8)) = 100;

    // CPlayerPed::CPlayerPed task fix
#if VER_x32
    CHook::WriteMemory(g_libGTASA + 0x004C36E2, (uintptr_t)"\xE0", 1);
#else
    CHook::WriteMemory(g_libGTASA + 0x5C0BC4, (uintptr_t)"\x34\x00\x80\x52", 4);
#endif

    // radar draw blips
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x0043FE5A : 0x52522C), 2);
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x004409AE : 0x525E14), 2);

    // no vehicle audio processing
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x00553E96 : 0x674610), 2);
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x00561AC2 : 0x682C1C), 2);
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x0056BED4 : 0x68DD0C), 2);

    // disable in-game radio
    CHook::RET("_ZN20CAERadioTrackManager7ServiceEi");

    // disable place name hud
    CHook::RET("_ZN10CPlaceName7ProcessEv");
    // disable veh name hud
    CHook::RET("_ZN4CHud15DrawVehicleNameEv");

    CHook::NOP(g_libGTASA + (VER_x32 ? 0x2ABA08 : 0x36A6E8), 2);
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x2ABA14 : 0x36A6F8), 2);
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x2AB4A6 : 0x36A190), 2);
}

int32_t CWorld__FindPlayerSlotWithPedPointer(CPedGTA* pPlayersPed)
{
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        if(CWorld::Players[i].m_pPed == pPlayersPed)
            return i;
    }
    return -1;
}

bool IsLockOnMode()
{
    return 1;
}

bool IsFreeAimMode()
{
    return 0;
}

void ApplyPatches_level0()
{
#ifdef BR_DEBUG
    Log("ApplyPatches_level0");
#endif

    CHook::Write(g_libGTASA + (VER_x32 ? 0x006783C0 : 0x84E7A8), &CWorld::Players);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x00679B5C : 0x8516D8), &CWorld::PlayerInFocus);

    CHook::Redirect("_ZN6CWorld28FindPlayerSlotWithPedPointerEPv", &CWorld__FindPlayerSlotWithPedPointer);

    // fix aplha raster, but it lag's
#if VER_x32
    CHook::WriteMemory(g_libGTASA + 0x001AE8DE, (uintptr_t)"\x01\x22", 2);
#else
    CHook::WriteMemory(g_libGTASA + 0x23FDE0, (uintptr_t)"\x22\x00\x80\x52", 4);
#endif

    /* fixes a weird ass glitches with the timer value clamping
#if VER_x32
    CHook::Write(g_libGTASA + 0x420E40, 0.001f);
#else
    CHook::Write32(g_libGTASA + 0x5045F4, 0xD00010AB);
    CHook::Write32(g_libGTASA + 0x504600, 0xBD41E161);
#endif*/

    CHook::RET("_ZN6CTrain10InitTrainsEv"); // CTrain::InitTrains
    CHook::RET("_ZN6CTrain18CreateMissionTrainE7CVectorbjPPS_S2_iib"); // CTrain::CreateMissionTrain

    CHook::RET("_ZN8CClothes4InitEv"); // CClothes::Init()
    CHook::RET("_ZN8CClothes13RebuildPlayerEP10CPlayerPedb"); // CClothes::RebuildPlayer

    CHook::RET("_ZNK35CPedGroupDefaultTaskAllocatorRandom20AllocateDefaultTasksEP9CPedGroupP4CPed"); // AllocateDefaultTasks
    CHook::RET("_ZN6CGlass4InitEv"); // CGlass::Init
    CHook::RET("_ZN8CGarages17Init_AfterRestartEv"); // CGarages::Init_AfterRestart
    CHook::RET("_ZN6CGangs10InitialiseEv"); // CGangs::Initialise
    CHook::RET("_ZN5CHeli9InitHelisEv"); // CHeli::InitHelis(void)
    CHook::RET("_ZN11CFileLoader10LoadPickupEPKc"); // CFileLoader::LoadPickup

    CHook::RET("_ZN10CSkidmarks6UpdateEv"); // CSkidmarks::Update
    CHook::RET("_ZN10CSkidmarks6RenderEv"); // CSkidmarks::Render

    CHook::RET("_ZN14SurfaceInfos_c16CreatesWheelDustEj"); // SurfaceInfos_c::CreatesWheelDust
    CHook::RET("_ZN14SurfaceInfos_c17CreatesWheelSprayEj"); // SurfaceInfos_c::CreatesWheelSpray

    CHook::RET("_ZN4Fx_c12AddWheelDustEP8CVehicle7CVectorhf"); // Fx_c::AddWheelDust
    CHook::RET("_ZN4Fx_c13AddWheelSprayEP8CVehicle7CVectorhhf"); // Fx_c::AddWheelSpray

    /*// .. fix skin vertices a lot, and it caused caching in RenderQueue
#if VER_x32
    CHook::WriteMemory(g_libGTASA + 0x1D16E4, (uintptr_t)"\x4F\xF0\x80\x70", 4);
    CHook::WriteMemory(g_libGTASA + 0x1D16EE, (uintptr_t)"\x4F\xF0\x80\x76", 4);
#else
    CHook::WriteMemory(g_libGTASA + 0x266FC8, (uintptr_t)"\x15\x80\xA0\x52", 4);
#endif

    // .. vertex weight
#if !VER_x32
    CHook::Write32(g_libGTASA + 0x25C278, MOVBits::Create(1, 27, false));
    CHook::NOP(g_libGTASA + 0x25C28C, 1);
    CHook::Write32(g_libGTASA + 0x25C290, MOVBits::Create(1, 27, false));
#endif

#if !VER_x32
    CHook::Write32(g_libGTASA + 0x4DB614, 0xD0001BAB);
    CHook::Write32(g_libGTASA + 0x4DB618, 0xF947996B);
#endif

#if !VER_x32
    CHook::NOP(g_libGTASA + 0x6E9508, 2);

    CHook::Write32(g_libGTASA + 0x6E8F60, 0x52808009);

    CHook::Write32(g_libGTASA + 0x6E8EB4, 0x52810000);
    CHook::Write32(g_libGTASA + 0x6E8EB8, 0x52808001);
    CHook::Write32(g_libGTASA + 0x6E8ED0, 0x52810000);
    CHook::Write32(g_libGTASA + 0x6E8ED4, 0x52808001);

    CHook::Write32(g_libGTASA + 0x4D8764, 0x5400004E); // m -> 3
#endif*/
}

void InstallCustomPatches()
{
	// OSWrapper
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x60A484 : 0x7371E0));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x60A484 : 0x7371E0)), ("WeiktonWrapper?"));

    // NVEvent
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x60A5B5 : 0x73740C));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x60A5B5 : 0x73740C)), ("WeiktonNVEvent?"));
}

void ApplyGlobalPatches()
{
#ifdef BR_DEBUG
    Log("Installing patches..");
#endif

    CHook::RET("_ZN17CVehicleModelInfo17SetCarCustomPlateEv"); // default plate
	CHook::RET("_Z16SaveGameForPause10eSaveTypesPc");

#if VER_x32
	CHook::WriteMemory(g_libGTASA + 0x00442120, (uintptr_t)"\x2C\xE0", 2);
	CHook::WriteMemory(g_libGTASA + 0x0044217C, (uintptr_t)"\x30\x46", 2);
#else
	CHook::WriteMemory(g_libGTASA + 0x52737C, (uintptr_t)"\x1E\x00\x00\x14", 4);
	CHook::WriteMemory(g_libGTASA + 0x5273F4, (uintptr_t)"\xE1\x03\x14\x2A", 4);

    // crash legend
    CHook::NOP(g_libGTASA + 0x36A690, 1);
#endif

    CHook::RET("_ZN12CAudioEngine16StartLoadingTuneEv");

    // DefaultPCSaveFileName
    char* DefaultPCSaveFileName = (char*)(g_libGTASA + (VER_x32 ? 0x006B012C : 0x88CB08));
    memcpy(DefaultPCSaveFileName, "GTASAMP", 8);

#if VER_x32
    CHook::NOP(g_libGTASA + 0x003F61B6, 2);	// CCoronas::RenderSunReflection crash
    CHook::NOP(g_libGTASA + 0x00584884, 2);
    CHook::NOP(g_libGTASA + 0x00584850, 2);
#else
    CHook::NOP(g_libGTASA + 0x004D8700, 1);  // CCoronas::RenderSunReflection crash
    CHook::NOP(g_libGTASA + 0x006A852C, 1);
    CHook::NOP(g_libGTASA + 0x006A84E0, 1);
#endif

    CHook::RET("_ZN17CVehicleRecording4LoadEP8RwStreamii"); // CVehicleRecording::Load

    CHook::RET("_ZN18CMotionBlurStreaks6UpdateEv");
    CHook::RET("_ZN7CCamera16RenderMotionBlurEv");

    CHook::RET("_ZN11CPlayerInfo17FindObjectToStealEP4CPed"); // Crash
    CHook::RET("_ZN26CAEGlobalWeaponAudioEntity21ServiceAmbientGunFireEv");	// CAEGlobalWeaponAudioEntity::ServiceAmbientGunFire
    CHook::RET("_ZN30CWidgetRegionSteeringSelection4DrawEv"); // CWidgetRegionSteeringSelection::Draw
    CHook::RET("_ZN23CTaskSimplePlayerOnFoot18PlayIdleAnimationsEP10CPlayerPed");	// CTaskSimplePlayerOnFoot::PlayIdleAnimations
    CHook::RET("_ZN13CCarEnterExit17SetPedInCarDirectEP4CPedP8CVehicleib");	// CCarEnterExit::SetPedInCarDirect
    CHook::RET("_ZN6CRadar10DrawLegendEiii"); // CRadar::DrawLgegend
    CHook::RET("_ZN6CRadar19AddBlipToLegendListEhi"); // CRadar::AddBlipToLegendList

    CHook::RET("_ZN11CAutomobile35CustomCarPlate_BeforeRenderingStartEP17CVehicleModelInfo"); // CAutomobile::CustomCarPlate_BeforeRenderingStart
    CHook::RET("_ZN11CAutomobile33CustomCarPlate_AfterRenderingStopEP17CVehicleModelInfo"); // CAutomobile::CustomCarPlate_AfterRenderingStop
    CHook::RET("_ZN7CCamera8CamShakeEffff"); // CCamera::CamShake
    CHook::RET("_ZN7CEntity23PreRenderForGlassWindowEv"); // CEntity::PreRenderForGlassWindow
    CHook::RET("_ZN8CMirrors16RenderReflBufferEb"); // CMirrors::RenderReflBuffer
    CHook::RET("_ZN4CHud23DrawBustedWastedMessageEv");
    CHook::RET("_ZN4CHud14SetHelpMessageEPKcPtbbbj"); // CHud::SetHelpMessage
    CHook::RET("_ZN4CHud24SetHelpMessageStatUpdateEhtff"); // CHud::SetHelpMessageStatUpdate
    CHook::RET("_ZN6CCheat16ProcessCheatMenuEv"); // CCheat::ProcessCheatMenu
    CHook::RET("_ZN6CCheat13ProcessCheatsEv"); // CCheat::ProcessCheats
    CHook::RET("_ZN6CCheat16AddToCheatStringEc"); // CCheat::AddToCheatString
    CHook::RET("_ZN6CCheat12WeaponCheat1Ev"); // CCheat::WeaponCheat1
    CHook::RET("_ZN6CCheat12WeaponCheat2Ev"); // CCheat::WeaponCheat2
    CHook::RET("_ZN6CCheat12WeaponCheat3Ev"); // CCheat::WeaponCheat3
    CHook::RET("_ZN6CCheat12WeaponCheat4Ev"); // CCheat::WeaponCheat4
    CHook::RET("_ZN8CGarages14TriggerMessageEPcsts"); // CGarages::TriggerMessage

    CHook::RET("_ZN11CPopulation6AddPedE8ePedTypejRK7CVectorb"); // CPopulation::AddPed
    CHook::RET("_ZN6CPlane27DoPlaneGenerationAndRemovalEv"); // CPlane::DoPlaneGenerationAndRemoval

    CHook::RET("_ZN10CEntryExit19GenerateAmbientPedsERK7CVector"); // CEntryExit::GenerateAmbientPeds
    CHook::RET("_ZN8CCarCtrl31GenerateOneEmergencyServicesCarEj7CVector"); // CCarCtrl::GenerateOneEmergencyServicesCar
    CHook::RET("_ZN11CPopulation17AddPedAtAttractorEiP9C2dEffect7CVectorP7CEntityi"); // CPopulation::AddPedAtAttractor crash. wtf stuff?

    CHook::RET("_ZN7CDarkel26RegisterCarBlownUpByPlayerEP8CVehiclei"); // CDarkel__RegisterCarBlownUpByPlayer_hook
    CHook::RET("_ZN7CDarkel25ResetModelsKilledByPlayerEi"); // CDarkel__ResetModelsKilledByPlayer_hook
    CHook::RET("_ZN7CDarkel25QueryModelsKilledByPlayerEii"); // CDarkel__QueryModelsKilledByPlayer_hook
    CHook::RET("_ZN7CDarkel27FindTotalPedsKilledByPlayerEi"); // CDarkel__FindTotalPedsKilledByPlayer_hook
    CHook::RET("_ZN7CDarkel20RegisterKillByPlayerEPK4CPed11eWeaponTypebi"); // CDarkel__RegisterKillByPlayer_hook

    fixUVAnim(true);

    // .. unlock img archives size
#if VER_x32
    const uint8_t limit = (uint8_t)(TOTAL_IMG_ARCHIVES - 1);
    CHook::WriteMemory(g_libGTASA + 0x46BDE8, (uintptr_t)&limit, 1);
    CHook::WriteMemory(g_libGTASA + 0x46BDF8, (uintptr_t)&limit, 1);
#else
    CHook::NOP(g_libGTASA + 0x55768C, 1);
    CHook::NOP(g_libGTASA + 0x557690, 1);
#endif

#if VER_x32
    CHook::NOP(g_libGTASA + (VER_2_1 ? 0x0040BF26 : 0x3AC8B2), 2); 	// CMessages::AddBigMessage from CPlayerInfo::KillPlayer
    CHook::NOP(g_libGTASA + 0x004C5902, 2);  // CCamera::ClearPlayerWeaponMode from CPedSamp::ClearWeaponTarget
    CHook::NOP(g_libGTASA + (VER_2_1 ? 0x003F395E : 0x39840A), 2);	// CStreaming::Shutdown from CGame::Shutdown
    CHook::WriteMemory(g_libGTASA + 0x003F4138, "\x03", 1); // RE3: Fix R* optimization that prevents peds to spawn
#else
    CHook::NOP(g_libGTASA + 0x5C3258, 1);  // CCamera::ClearPlayerWeaponMode from CPlayerPed::ClearWeaponTarget
    CHook::WriteMemory(g_libGTASA + 0x266FC8, "\xF5\x03\x08\x32", 4); //  RenderQueue::RenderQueue
    CHook::WriteMemory(g_libGTASA + 0x4D644C, "\x1F\x0D\x00\x71", 4); // RE3: Fix R* optimization that prevents peds to spawn
#endif

    CHook::RET("_ZN10CPlayerPed14AnnoyPlayerPedEb"); // CPedSamp::AnnoyPlayerPed
    CHook::RET("_ZN11CPopulation15AddToPopulationEffff");	// CPopulation::AddToPopulation

    CHook::RET("_ZN23CAEPedSpeechAudioEntity11AddSayEventEisjfhhh"); // CPed::Say

    CHook::RET("_ZN8CMirrors16BeforeMainRenderEv"); // CMirrors::BeforeMainRender(void)

    CHook::RET("_ZN8CCarCtrl18GenerateRandomCarsEv"); // CCarCtrl::GenerateRandomCarsEv
}

// Заводская таблица игры лежит статически, индекс считается от модели 400,
// запись 36 байт. Наполняем её напрямую — тогда весь звук ниже по течению
// работает без единого хука.
static VehicleAudioPropertiesStruct* GetGameAudioTable() {
    return (VehicleAudioPropertiesStruct*)(g_libGTASA + (VER_x32 ? 0x6145E8 : 0x74C928));
}

static int32_t FindVehicleModelByName(const char* name) {
    for (int32_t id = 400; id <= 611; id++) {
        auto info = CModelInfo::GetModelInfo(id);

        if (info && strcasecmp(info->m_modelName, name) == 0) {
            return id;
        }
    }

    return -1;
}

void readVehiclesAudioSettings() {
    if (!g_pszStorage) {
        return;
    }

    char path[256];
    snprintf(path, sizeof(path), "%sSAMP/vehicleAudioSettings.cfg", g_pszStorage);

    FILE* file = fopen(path, "r");
    if (!file) {
        Log("vehicleAudioSettings: не открылся %s", path);
        return;
    }

    auto table = GetGameAudioTable();

    char line[512];
    int32_t applied = 0, skipped = 0;

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == ';' || line[0] == '#' || line[0] == '|' ||
            line[0] == '\r' || line[0] == '\n') {
            continue;
        }

        char name[64]{};
        int soundType = 0, engineOn = 0, engineOff = 0, field4 = 0;
        float field5 = 0.0f, field6 = 0.0f, hornHigh = 0.0f, field16 = 0.0f;
        int hornTon = 0, doorSound = 0, radioNum = 0, radioType = 0, field14 = 0;

        // Порядок колонок описан в шапке самого файла: имя и тринадцать значений
        const int read = sscanf(line, "%63s %d %d %d %d %f %f %d %f %d %d %d %d %f",
                                name, &soundType, &engineOn, &engineOff, &field4,
                                &field5, &field6, &hornTon, &hornHigh, &doorSound,
                                &radioNum, &radioType, &field14, &field16);

        if (read < 14) {
            skipped++;
            continue;
        }

        const int32_t modelId = FindVehicleModelByName(name);
        if (modelId < 400) {
            skipped++;
            continue;
        }

        VehicleAudioPropertiesStruct entry{};

        entry.VehicleType    = (int16_t)soundType;
        entry.EngineOnSound  = (int16_t)engineOn;
        entry.EngineOffSound = (int16_t)engineOff;
        entry.field_4        = (int16_t)field4;
        entry.field_5        = field5;
        entry.field_6        = field6;
        entry.HornTon        = (char)hornTon;
        entry.HornHigh       = hornHigh;
        entry.DoorSound      = (char)doorSound;
        entry.RadioNum       = (char)radioNum;
        entry.RadioType      = (char)radioType;
        entry.field_14       = (char)field14;
        entry.field_16       = field16;

        VehicleAudioProperties[modelId - 400] = entry;
        table[modelId - 400] = entry;

        applied++;
    }

    fclose(file);

    Log("vehicleAudioSettings: применено %d, пропущено %d", applied, skipped);
}