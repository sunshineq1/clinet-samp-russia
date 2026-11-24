#include <jni.h>
#include "game/Enums/eHandlingParam.h"
#include "game/CustomTune.h"
#include "game/CHandlingDefault.h"
#include "game/Models/ModelInfo.h"
#include "game/VehicleNeon.h"
#include <pthread.h>
#include <syscall.h>

#include "main.h"
#include "game/game.h"
#include "game/CGraphicsSettings.h"
#include "game/CExhaust.h"
#include "game/CVehicleHands.h"
#include "net/netgame.h"
#include "gui/gui.h"
#include "playertags.h"
#include "java/jniutil.h"
#include <dlfcn.h>
#include "StackTrace.h"

#include "voice_new/Plugin.h"

#include "vendor/armhook/patch.h"

#include "CSettings.h"
#include "crashlytics.h"

JavaVM* javaVM;
char* g_pszStorage = nullptr;

UI* pUI = nullptr;
CGame *pGame = nullptr;

CNetGame *pNetGame = nullptr;
CPlayerTags* pPlayerTags = nullptr;
CSnapShotHelper* pSnapShotHelper = nullptr;
CJavaWrapper* pJavaWrapper = nullptr;
CSettings* pSettings = nullptr;
MaterialTextGenerator* pMaterialTextGenerator = nullptr;

bool bDebug = false;
bool bGameInited = false;
bool bNetworkInited = false;

uintptr_t g_libGTASA = 0x00;
uintptr_t g_libSAMP = 0x00;

int g_maxFPS = FORCED_FPS;
void ApplyFPSPatch(uint8_t fps);

void ApplyGlobalPatches();
void ApplyPatches_level0();
void ApplyMultiTouchPatches();
void InstallGlobalHooks();
void InstallSpecialHooks();
void InitRenderWareFunctions();
void Log(const char* fmt, ...);

void ReadSettingFile() {
	pSettings = new CSettings();
	firebase::crashlytics::SetUserId(pSettings->Get().szNickName);
}

#include "java/GUIManager.h"
#include "java/SnapShotsWrapper.h"
#include "CServerManager.h"
static void ReportLoadProgress(int percent)
{
    static int lastSent = -1;

    if (percent > 99) percent = 99;
    if (percent == lastSent) return;

    lastSent = percent;

    char json[32];
    snprintf(json, sizeof(json), "{\"t\":0,\"s\":%d}", percent);

    GUIManager::LocalPacket(UI_ID_SELECTSERVER, 0, json);
}

// Движок сам считает долю загруженного в CLoadingScreen::m_PercentLoaded.
// Тянем её каждый кадр, пока идёт загрузка — это единственный настоящий источник.
void PollEngineLoadProgress()
{
    static bool bDone = false;
    if (bDone) return;

    const float percent = *(float*)(g_libGTASA + (VER_x32 ? 0x9920C0 : 0xC20F04));

    if (percent >= 1.0f) {
        bDone = true;
        return;
    }

    ReportLoadProgress((int)(percent * 90.0f));
}

void DoInitStuff()
{
	if (!bGameInited)
	{
		pPlayerTags = new CPlayerTags();
		pSnapShotHelper = new CSnapShotHelper();
		pMaterialTextGenerator = new MaterialTextGenerator();

		pUI->splashscreen()->setVisible(false);
		pUI->chat()->setVisible(true);
        pJavaWrapper->toggleCustomButtons(false);

		pGame->Initialize();
		pGame->SetMaxStats();
		pGame->ToggleThePassingOfTime(false);

		// .. toggle hud draw
        // *(uint8_t*)(g_libGTASA + (VER_x32 ? 0x819D88 : 0x9FF3A8)) = 0;

        ApplyFPSPatch(g_maxFPS);

        CGraphicsSettings::Initialise();
        CExhaust::Inject();
        CVehicleHands::InjectHooks();

		// voice
        // .. LogVoice("[dbg:samp:load] : module loading...");
        for (const auto &loadCallback: Samp::loadCallbacks) {
            if (loadCallback != nullptr) {
                loadCallback();
            }
        }
        Samp::loadStatus = true;
        // .. LogVoice("[dbg:samp:load] : module loaded");

		bGameInited = true;
	}

	if (!bNetworkInited)
	{
        ReportLoadProgress(75);

        pUI->loadTxd();

        // пустое подключение на петлю: движок и худ ждут живой pNetGame,
        // без него первый же кадр падает
        ReportLoadProgress(90);

        CServerInstance::initConnection(-1);

        CGame::FixSettings();

        ReportLoadProgress(99);
        GUIManager::LocalPacket(UI_ID_SELECTSERVER, 0, "{\"t\":1}");

        bNetworkInited = true;
	}
}

#include "game/CustomHandling.h"
#include "game/RQShader.h"
#include "game/World.h"
// Профили прошивок повторяют серверные из brtuning.inc: нужны, чтобы проверить
// поведение машины локально, пока мод не пересобран.
static uint8_t BuildLocalFlash(int flash, CVehicle::SHandlingEdit* out, CVehicle* pVeh)
{
    if (!out || flash < 0 || flash > 3) {
        return 0;
    }

    uint8_t count = 0;

    auto add = [&](uint8_t param, uint8_t mode, float value) {
        if (count < CVehicle::kMaxHandlingEdits) {
            out[count++] = { param, mode, value };
        }
    };

    if (flash == 0) {
        add(HANDLING_PARAM_ACCELERATION,        HANDLING_MODE_SCALE, 0.95f);
        add(HANDLING_PARAM_TRACTION_MULTIPLIER, HANDLING_MODE_SCALE, 1.05f);
        add(HANDLING_PARAM_TRACTION_LOSS,       HANDLING_MODE_SCALE, 0.90f);
        add(HANDLING_PARAM_BRAKE_DECELERATION,  HANDLING_MODE_SCALE, 1.05f);
        add(HANDLING_PARAM_SUSPENSION_LOWER,    HANDLING_MODE_SCALE, 1.10f);
        return count;
    }

    if (flash == 1) {
        add(HANDLING_PARAM_MAX_SPEED,           HANDLING_MODE_SCALE, 1.10f);
        add(HANDLING_PARAM_ACCELERATION,        HANDLING_MODE_SCALE, 1.15f);
        add(HANDLING_PARAM_BRAKE_DECELERATION,  HANDLING_MODE_SCALE, 1.15f);
        add(HANDLING_PARAM_TRACTION_MULTIPLIER, HANDLING_MODE_SCALE, 1.05f);
        add(HANDLING_PARAM_SUSPENSION_LOWER,    HANDLING_MODE_SCALE, 0.92f);
        return count;
    }

    if (flash == 3) {
        add(HANDLING_PARAM_MAX_SPEED,           HANDLING_MODE_SCALE, 1.22f);
        add(HANDLING_PARAM_ACCELERATION,        HANDLING_MODE_SCALE, 1.32f);
        add(HANDLING_PARAM_BRAKE_DECELERATION,  HANDLING_MODE_SCALE, 1.25f);
        add(HANDLING_PARAM_TRACTION_MULTIPLIER, HANDLING_MODE_SCALE, 1.14f);
        add(HANDLING_PARAM_TRACTION_LOSS,       HANDLING_MODE_SCALE, 1.08f);
        add(HANDLING_PARAM_STEERING_LOCK,       HANDLING_MODE_SCALE, 1.05f);
        add(HANDLING_PARAM_ENGINE_INERTIA,      HANDLING_MODE_SCALE, 0.85f);
        add(HANDLING_PARAM_SUSPENSION_LOWER,    HANDLING_MODE_SCALE, 0.88f);
        return count;
    }

    // Дрифт задан целиком: передний привод, руль 65, четыре передачи.
    // Массу не трогаем, а масса вращения и сила подвески идут долей от неё.
    add(HANDLING_PARAM_DRIVE_TYPE,            HANDLING_MODE_ABSOLUTE,   2.0f);
    add(HANDLING_PARAM_TURN_MASS,             HANDLING_MODE_MASS_SCALE, 1.33333f);
    add(HANDLING_PARAM_DRAG_MULT,             HANDLING_MODE_ABSOLUTE,   2.0f);
    add(HANDLING_PARAM_CENTRE_OF_MASS_Z,      HANDLING_MODE_ABSOLUTE,  -0.1f);
    add(HANDLING_PARAM_TRACTION_MULTIPLIER,   HANDLING_MODE_ABSOLUTE,   0.75f);
    add(HANDLING_PARAM_TRACTION_LOSS,         HANDLING_MODE_ABSOLUTE,   0.75f);
    add(HANDLING_PARAM_TRACTION_BIAS,         HANDLING_MODE_ABSOLUTE,   0.6f);
    add(HANDLING_PARAM_NUMBER_OF_GEARS,       HANDLING_MODE_ABSOLUTE,   4.0f);
    add(HANDLING_PARAM_MAX_SPEED,             HANDLING_MODE_ABSOLUTE,   130.0f);
    add(HANDLING_PARAM_ACCELERATION,          HANDLING_MODE_ABSOLUTE,   34.0f);
    add(HANDLING_PARAM_ENGINE_INERTIA,        HANDLING_MODE_ABSOLUTE,   50.0f);
    add(HANDLING_PARAM_BRAKE_DECELERATION,    HANDLING_MODE_ABSOLUTE,   8.0f);
    add(HANDLING_PARAM_BRAKE_BIAS,            HANDLING_MODE_ABSOLUTE,   0.52f);
    add(HANDLING_PARAM_STEERING_LOCK,         HANDLING_MODE_ABSOLUTE,   65.0f);
    add(HANDLING_PARAM_SUSPENSION_FORCE,      HANDLING_MODE_MASS_SCALE, 0.00066667f);
    add(HANDLING_PARAM_SUSPENSION_DAMPING,    HANDLING_MODE_ABSOLUTE,   0.8f);
    add(HANDLING_PARAM_SUSPENSION_HIGH_DAMP,  HANDLING_MODE_ABSOLUTE,   4.0f);
    add(HANDLING_PARAM_SUSPENSION_UPPER,      HANDLING_MODE_ABSOLUTE,   0.2f);
    add(HANDLING_PARAM_SUSPENSION_LOWER,      HANDLING_MODE_ABSOLUTE,  -0.1f);
    add(HANDLING_PARAM_SUSPENSION_BIAS,       HANDLING_MODE_ABSOLUTE,   0.4f);
    add(HANDLING_PARAM_SUSPENSION_ANTIDIVE,   HANDLING_MODE_ABSOLUTE,   0.2f);
    add(HANDLING_PARAM_COLLISION_DAMAGE,      HANDLING_MODE_ABSOLUTE,   0.0f);

    return count;
}

bool IsCustomCommands(const char* str)
{
    /*if(strstr(str, "/jopa"))
	{
        CFirstPersonCamera::m_bEnabled = !CFirstPersonCamera::m_bEnabled;
		return true;
	}*/
    if(strstr(str, "/cameditgui"))
	{
        GUIManager::ClearUIScreen();
		return true;
	}
    if(strstr(str, "/dl") || strstr(str, "/dlinfo"))
	{
        pUI->m_bObjInfo = !pUI->m_bObjInfo;
		return true;
	}
    if(strstr(str, "/client"))
	{
        if (pJavaWrapper) {
            pJavaWrapper->ShowGraphicSettings();
        }
		return true;
	}
    if(strstr(str, "/chat"))
	{
        GUIManager::ToggleUIChat();
		return true;
	}
    if (strstr(str, "/fpslimit "))
	{
		ApplyFPSPatch(g_maxFPS);
        pUI->chat()->addDebugMessage("{ffff00}|{ffffff} FrameRate: клиент работает на %d кадрах, предел не меняется", g_maxFPS);
		return true;
	}
    if (strstr(str, "/tune"))
    {
        CVehicleGTA* pGtaVeh = FindPlayerVehicle(-1, false);
        CVehicle* pVeh = pGtaVeh ? pNetGame->GetVehiclePool()->GetAt(
                pNetGame->GetVehiclePool()->FindIDFromGtaPtr(pGtaVeh)) : nullptr;

        if (!pVeh)
        {
            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: вы не в машине");
            return true;
        }

        if (strstr(str, "/tunehelp"))
        {
            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} /tunepart [tuneId] - деталь кузова из customtune.dat");
            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} /tunecomp [селектор] [tuneId] - то же вручную, -1 снять");
            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} /tuneparts - список деталей для этой модели");
            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} /tunewheel [клир] [клзад] [вылп] [вылз] [разп] [разз] [шир] [ширз] [радп] [радз]");
            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} /tuneflash [0 комфорт, 1 спорт, 2 дрифт, 3 спорт+, -1 снять]");
            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} /tuneneon [слот 0-2] [r] [g] [b] [a]");
            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} /tuneneonp - размеры и яркость, /tuneneontex - имена текстур");
            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} /tunestrobe [0-4]");
            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} /tunevinyl [имя] - например remapbody8, empty снять");
            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} /tunecolor [r] [g] [b], /tunewcolor - диски");
            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} /tuneinfo - что стоит, /tunereset - снять всё");
            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} /tunedat - перечитать детали, /tunehandreload - перечитать хендлинг");
            return true;
        }

        // Перечитать handling.cfg вручную: снимок наполняется только при разборе
        if (strstr(str, "/tunehandreload"))
        {
            CustomHandling::Load(eCustomHandlingType::HANDLING_DEFAULT);

            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: снимок заводского хендлинга — %d записей",
                CHandlingDefault::Count());
            return true;
        }

        if (strstr(str, "/tunedat"))
        {
            const bool ok = CCustomTune::Load();
            const int model = pVeh->m_pVehicle->m_nModelIndex;

            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} customtune.dat: %s", ok ? "загружен" : "НЕ НАЙДЕН");
            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} путь: %s",
                CCustomTune::LastPath()[0] ? CCustomTune::LastPath() : "ни один вариант не открылся");
            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} деталей всего %d, для модели %d их %d",
                CCustomTune::Count(), model, CCustomTune::CountForModel(model));
            return true;
        }

        if (strstr(str, "/tuneinfo"))
        {
            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Модель %d, деталей в customtune.dat %d, для этой модели %d",
                pVeh->m_pVehicle->m_nModelIndex,
                CCustomTune::Count(),
                CCustomTune::CountForModel(pVeh->m_pVehicle->m_nModelIndex));

            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Колёса: радиус %.2f/%.2f ширина %.2f/%.2f развал %.2f/%.2f",
                pVeh->m_fWheelScaleFront, pVeh->m_fWheelScaleRear,
                pVeh->m_fWheelWidthFront, pVeh->m_fWheelWidthBack,
                pVeh->m_fWheelAngleFront, pVeh->m_fWheelAngleBack);

            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Снимок заводского хендлинга: %d записей",
                CHandlingDefault::Count());

            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Хендлинг: правок %d, своя копия %s, линии %s",
                (int)pVeh->m_handlingEditCount,
                pVeh->m_pCustomHandling ? "есть" : "нет",
                pVeh->bHasSuspensionLines ? "свои" : "общие");

            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Неон %s, мигалка %d, деталей навешано %d",
                pVeh->HasNeon() ? "есть" : "нет",
                (int)pVeh->m_iStrobeType,
                (int)pVeh->m_customComponents.size());
            return true;
        }

        if (strstr(str, "/tuneparts"))
        {
            const int model = pVeh->m_pVehicle->m_nModelIndex;
            int shown = 0;

            for (int tuneId = 0; tuneId < 1024 && shown < 24; tuneId++)
            {
                const SCustomTunePart* part = CCustomTune::Find(tuneId);
                if (!part || part->vehicleModel != model) {
                    continue;
                }

                const bool bHasModel = part->componentModel >= 0
                    && part->componentModel < CModelInfo::NUM_MODEL_INFOS
                    && CModelInfo::GetModelInfo(part->componentModel) != nullptr;

                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} %d - тип %d, на %s, модель %d %s",
                    part->tuneId, part->type, part->dummy, part->componentModel,
                    bHasModel ? "" : "{ff5533}(модели нет)");
                shown++;
            }

            if (!shown) {
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: для модели %d деталей нет", model);
            }

            return true;
        }

        if (strstr(str, "/tunereset"))
        {
            pVeh->RemoveAllCustomComponents();
            pVeh->ApplyHandlingEdits(nullptr, 0);
            pVeh->SetStrobeType(0);

            for (uint8_t slot = 0; slot < 3; slot++) {
                pVeh->SetNeonColor(slot, CRGBA(0, 0, 0, 0));
            }

            float zero[CVehicle::kWheelParamCount] = { 0.0f };
            pVeh->SetWheelParameters(zero);

            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: всё снято");
            return true;
        }

        if (strstr(str, "/tunecomp"))
        {
            int selector = -1, tuneId = -1;

            if (sscanf(str, "%*s%d%d", &selector, &tuneId) < 2)
            {
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: /tunecomp [селектор] [tuneId]");
                return true;
            }

            if (!pVeh->SetCustomComponent(selector, tuneId))
            {
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: не встала — %s",
                    CVehicle::ms_szLastComponentError);
                return true;
            }

            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: селектор %d деталь %d, навешано %d",
                selector, tuneId, (int)pVeh->m_customComponents.size());
            return true;
        }

        // Селектор для проверки не важен, берём его из самого файла
        if (strstr(str, "/tunepart"))
        {
            int tuneId = -1;

            if (sscanf(str, "%*s%d", &tuneId) < 1)
            {
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: /tunepart [tuneId], список - /tuneparts");
                return true;
            }

            const SCustomTunePart* part = CCustomTune::Find(tuneId);
            if (!part)
            {
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: детали %d нет в customtune.dat", tuneId);
                return true;
            }

            if (part->vehicleModel != pVeh->m_pVehicle->m_nModelIndex)
            {
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: деталь %d от модели %d, а вы на %d",
                    tuneId, part->vehicleModel, pVeh->m_pVehicle->m_nModelIndex);
                return true;
            }

            if (!pVeh->SetCustomComponent(part->type, tuneId))
            {
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: не встала — %s",
                    CVehicle::ms_szLastComponentError);
                return true;
            }

            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: %d на %s, модель %d, гасим %s",
                tuneId, part->dummy, part->componentModel,
                part->hideAtomic[0] ? part->hideAtomic : "ничего");
            return true;
        }

        if (strstr(str, "/tunewheel"))
        {
            float values[CVehicle::kWheelParamCount] = { 0.0f };

            const int read = sscanf(str, "%*s%f%f%f%f%f%f%f%f%f%f",
                &values[0], &values[1], &values[2], &values[3], &values[4],
                &values[5], &values[6], &values[7], &values[8], &values[9]);

            if (read < 1)
            {
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: /tunewheel [клир] [клзад] [вылп] [вылз] [разп] [разз] [шир] [ширз] [радп] [радз]");
                return true;
            }

            pVeh->SetWheelParameters(values);

            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: колёса приняты, полей %d", read);
            return true;
        }

        if (strstr(str, "/tuneflash"))
        {
            int flash = -1;
            sscanf(str, "%*s%d", &flash);

            CVehicle::SHandlingEdit edits[CVehicle::kMaxHandlingEdits];
            const uint8_t count = BuildLocalFlash(flash, edits, pVeh);

            if (!CHandlingDefault::EnsureReady())
            {
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: снимок заводского хендлинга пуст — перехват не поймал разбор handling.cfg");
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} попробуйте /tunehandreload, потом снова прошивку");
                return true;
            }

            pVeh->ApplyHandlingEdits(edits, count);

            if (!pVeh->m_pCustomHandling)
            {
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: своя копия хендлинга не создалась");
                return true;
            }

            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: прошивка %d, правок %d, привод %c, руль %.0f",
                flash, (int)count,
                pVeh->m_pCustomHandling->m_transmissionData.m_nDriveType,
                pVeh->m_pCustomHandling->m_fSteeringLock);
            return true;
        }

        if (strstr(str, "/tuneneontex"))
        {
            char name[32] = { 0 }, db[16] = { 0 };

            const int read = sscanf(str, "%*s%31s%15s", name, db);
            if (read < 1)
            {
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Neon: /tuneneontex [текстура] [архив]");
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} сейчас %s из %s",
                    CVehicleNeon::ms_szTexture, CVehicleNeon::ms_szTextureDb);
                return true;
            }

            strncpy(CVehicleNeon::ms_szTexture, name, sizeof(CVehicleNeon::ms_szTexture) - 1);

            if (read >= 2) {
                strncpy(CVehicleNeon::ms_szTextureDb, db, sizeof(CVehicleNeon::ms_szTextureDb) - 1);
            }

            CVehicleNeon::ResetTexture();

            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Neon: текстура %s из %s",
                CVehicleNeon::ms_szTexture, CVehicleNeon::ms_szTextureDb);
            return true;
        }

        if (strstr(str, "/tuneneonp"))
        {
            float ul = 0, uw = 0, sl = 0, sw = 0, so = 0, br = 0;

            const int read = sscanf(str, "%*s%f%f%f%f%f%f", &ul, &uw, &sl, &sw, &so, &br);
            if (read < 6)
            {
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Neon: /tuneneonp [дл центр] [шир центр] [дл борт] [шир борт] [сдвиг] [яркость]");
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} сейчас %.2f %.2f %.2f %.2f %.2f %.2f",
                    CVehicleNeon::ms_fUnderLength, CVehicleNeon::ms_fUnderWidth,
                    CVehicleNeon::ms_fSideLength, CVehicleNeon::ms_fSideWidth,
                    CVehicleNeon::ms_fSideOffset, CVehicleNeon::ms_fBrightness);
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} высота над низом кузова %.2f — /tuneneonz",
                    CVehicleNeon::ms_fGroundOffset);
                return true;
            }

            CVehicleNeon::ms_fUnderLength = ul;
            CVehicleNeon::ms_fUnderWidth  = uw;
            CVehicleNeon::ms_fSideLength  = sl;
            CVehicleNeon::ms_fSideWidth   = sw;
            CVehicleNeon::ms_fSideOffset  = so;
            CVehicleNeon::ms_fBrightness  = br;

            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Neon: принято");
            return true;
        }

        if (strstr(str, "/tuneneonz"))
        {
            float z = 0.0f;

            if (sscanf(str, "%*s%f", &z) != 1)
            {
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Neon: /tuneneonz [высота], сейчас %.2f",
                    CVehicleNeon::ms_fGroundOffset);
                return true;
            }

            CVehicleNeon::ms_fGroundOffset = z;

            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Neon: высота %.2f", z);
            return true;
        }

        if (strstr(str, "/tuneneon"))
        {
            int slot = 0, r = 0, g = 0, b = 0, a = 255;

            if (sscanf(str, "%*s%d%d%d%d%d", &slot, &r, &g, &b, &a) < 4)
            {
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: /tuneneon [слот 0-2] [r] [g] [b] [a]");
                return true;
            }

            pVeh->SetNeonColor((uint8_t)slot, CRGBA(
                (uint8_t)std::clamp(r, 0, 255), (uint8_t)std::clamp(g, 0, 255),
                (uint8_t)std::clamp(b, 0, 255), (uint8_t)std::clamp(a, 0, 255)));

            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: неон %d = %d %d %d %d", slot, r, g, b, a);
            return true;
        }

        if (strstr(str, "/tunestrobe"))
        {
            int type = 0;
            sscanf(str, "%*s%d", &type);

            pVeh->SetStrobeType((uint8_t)std::clamp(type, 0, 4));

            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: мигалка %d", type);
            return true;
        }

        if (strstr(str, "/tunevinyl"))
        {
            char name[32] = { 0 };

            if (sscanf(str, "%*s%31s", name) != 1)
            {
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: /tunevinyl [имя], empty чтобы снять");
                return true;
            }

            pVeh->setVinyl(name);

            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: винил %s %s", name,
                pVeh->m_pVinylTex ? "найден" : "НЕ найден");
            return true;
        }

        if (strstr(str, "/tunewcolor"))
        {
            int r = 0, g = 0, b = 0;

            if (sscanf(str, "%*s%d%d%d", &r, &g, &b) < 3)
            {
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: /tunewcolor [r] [g] [b]");
                return true;
            }

            pVeh->wheelColor = CRGBA((uint8_t)r, (uint8_t)g, (uint8_t)b, 255);

            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: диски %d %d %d", r, g, b);
            return true;
        }

        if (strstr(str, "/tunecolor"))
        {
            int r = 0, g = 0, b = 0;

            if (sscanf(str, "%*s%d%d%d", &r, &g, &b) < 3)
            {
                pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: /tunecolor [r] [g] [b]");
                return true;
            }

            pVeh->mainColor = CRGBA((uint8_t)r, (uint8_t)g, (uint8_t)b, 255);

            pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: кузов %d %d %d", r, g, b);
            return true;
        }

        pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Tune: наберите /tunehelp");
        return true;
    }

    if (strstr(str, "/localtoner"))
	{
		CVehicleGTA *pGtaVeh = FindPlayerVehicle(-1, false);
		CVehicle *pVeh = pGtaVeh ? pNetGame->GetVehiclePool()->GetAt(
				pNetGame->GetVehiclePool()->FindIDFromGtaPtr(pGtaVeh)) : nullptr;

		if (!pVeh)
		{
			pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Toner: вы не в машине");
			return true;
		}

		int r = 0, g = 0, b = 0, a = 255;
		const int read = sscanf(str, "%*s%d%d%d%d", &r, &g, &b, &a);
		if (read < 3)
		{
			pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Toner: /localtoner [r] [g] [b] [a] — сейчас %d %d %d %d",
				pVeh->tonerColor.r, pVeh->tonerColor.g, pVeh->tonerColor.b, pVeh->tonerColor.a);
			return true;
		}

		pVeh->tonerColor = CRGBA(
			(uint8_t)std::clamp(r, 0, 255), (uint8_t)std::clamp(g, 0, 255),
			(uint8_t)std::clamp(b, 0, 255), (uint8_t)std::clamp(a, 0, 255));

		pVeh->bTonerColorSet = true;

		pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Toner: %d %d %d %d",
			pVeh->tonerColor.r, pVeh->tonerColor.g, pVeh->tonerColor.b, pVeh->tonerColor.a);
		return true;
	}
    if (strstr(str, "/dsave "))
	{
		char name[64] = { 0 };

		if (sscanf(str, "%*s%63s", name) != 1)
		{
			pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Save: /save [name]");
			return true;
		}

		CLocalPlayer* pLocalPlayer = pNetGame ? pNetGame->GetPlayerPool()->GetLocalPlayer() : nullptr;
		CPlayerPed* pPed = pLocalPlayer ? pLocalPlayer->GetPlayerPed() : nullptr;

		if (!pPed || !pPed->m_pPed)
		{
			pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Save: player not inited");
			return true;
		}

		char path[0xFF];
		sprintf(path, "%sSAMP/savedposition.txt", g_pszStorage);

		FILE* fileOut = fopen(path, "a");

		if (!fileOut)
		{
			pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Save: failed to open savedposition.txt");
			return true;
		}

		float fZAngle = 0.0f;
		CVehicleGTA* pGtaVeh = pPed->IsInVehicle() ? pPed->m_pPed->pVehicle : nullptr;

		if (pGtaVeh)
		{
			const uintptr_t dwVehicleId = GamePool_Vehicle_GetIndex(pGtaVeh);
			const CVector pos = pGtaVeh->GetPosition();

			ScriptCommand(&get_car_z_angle, dwVehicleId, &fZAngle);

			fprintf(fileOut, "%s = %.3f, %.3f, %.3f, %.3f\n", name, pos.x, pos.y, pos.z, fZAngle);

			pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Save: saved pos in veh");
		}
		else
		{
			const CVector pos = pPed->m_pPed->GetPosition();

			ScriptCommand(&get_actor_z_angle, pPed->m_dwGTAId, &fZAngle);

			fprintf(fileOut, "%s = %.3f, %.3f, %.3f, %.3f\n", name, pos.x, pos.y, pos.z, fZAngle);

			pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Save: saved pos from ped");
		}

		fclose(fileOut);
		return true;
	}
    if(strstr(str, "/online") || strstr(str, "/tab"))
	{
		GUIManager::Create(UI_ID_PLAYERS_LIST);
		return true;
	}
    if (strstr(str, "/fog"))
	{
		float start = 0.0f, end = 0.0f;
		if (sscanf(str, "%*s%f%f", &start, &end) != 2)
		{
			RQShader::g_fFogStart = 0.0f;
			RQShader::g_fFogEnd   = 0.0f;
			pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Fog: свои значения сброшены, /fog [начало] [конец]");
			return true;
		}

		RQShader::g_fFogStart = start;
		RQShader::g_fFogEnd   = end;
		pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Fog: %.0f -> %.0f", start, end);
		return true;
	}
    if (strstr(str, "/refl"))
	{
		float percent = 0.0f;
		if (sscanf(str, "%*s%f", &percent) != 1)
		{
			pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Refl: поправка по Y %+.0f%% — /refl [0-100], 50 без поправки",
				RQShader::g_fReflCenterY - 50.0f);
			return true;
		}

		RQShader::g_fReflCenterY = std::clamp(percent, 0.0f, RQShader::REFL_CENTER_MAX);
		pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Refl: поправка по Y %+.0f%%",
			RQShader::g_fReflCenterY - 50.0f);
		return true;
	}
    if (strstr(str, "/fp"))
	{
		char param[32]{};
		float value = 0.0f;

		if (sscanf(str, "%*s%31s%f", param, &value) != 2)
		{
			CFirstPersonCamera::m_bEnabled = !CFirstPersonCamera::m_bEnabled;
			pUI->chat()->addDebugMessage("{ffff00}|{ffffff} FirstPerson: %s — /fp [fov|near|x|y|z] [значение]",
				CFirstPersonCamera::m_bEnabled ? "включено" : "выключено");
			return true;
		}

		if (!strcmp(param, "fov")) {
			CFirstPersonCamera::m_fFov = std::clamp(value, 0.0f, 120.0f);
		} else if (!strcmp(param, "near")) {
			CFirstPersonCamera::m_fNearOnFoot = std::clamp(value, 0.02f, 1.0f);
		} else if (!strcmp(param, "x")) {
			CFirstPersonCamera::m_vecOffsetOnFoot.x = value;
		} else if (!strcmp(param, "y")) {
			CFirstPersonCamera::m_vecOffsetOnFoot.y = value;
		} else if (!strcmp(param, "z")) {
			CFirstPersonCamera::m_vecOffsetOnFoot.z = value;
		} else {
			pUI->chat()->addDebugMessage("{ffff00}|{ffffff} FirstPerson: /fp [fov|near|x|y|z] [значение]");
			return true;
		}

		pUI->chat()->addDebugMessage("{ffff00}|{ffffff} FirstPerson: FOV %.0f, near %.2f, смещение %.2f %.2f %.2f",
			CFirstPersonCamera::m_fFov, CFirstPersonCamera::m_fNearOnFoot,
			CFirstPersonCamera::m_vecOffsetOnFoot.x, CFirstPersonCamera::m_vecOffsetOnFoot.y,
			CFirstPersonCamera::m_vecOffsetOnFoot.z);
		return true;
	}
    if (strstr(str, "/carrefl"))
	{
		float boost = 0.0f;
		if (sscanf(str, "%*s%f", &boost) != 1)
		{
			pUI->chat()->addDebugMessage("{ffff00}|{ffffff} CarRefl: сила отражения %.1f — /carrefl [0.5-6]",
				RQShader::g_fEnvMapBoost);
			return true;
		}

		RQShader::g_fEnvMapBoost = std::clamp(boost, 0.5f, 6.0f);
		pUI->chat()->addDebugMessage("{ffff00}|{ffffff} CarRefl: сила отражения %.1f", RQShader::g_fEnvMapBoost);
		return true;
	}
    if (strstr(str, "/draw "))
	{
		int dr = 0;
		if (sscanf(str, "%*s%d", &dr) == -1)
		{
			pUI->chat()->addDebugMessage("{ffff00}|{ffffff} Note: use /draw [0-1-2-3] to update");
			return true;
		}

		RQShader::g_nearFogDistanceControl = dr;
        pUI->chat()->addDebugMessage("{ffff00}|{ffffff} new draw distance -> %d", dr);
		return true;
	}

    return false;
}

#define SPCALL_BUTTON_ALT 0
#define SPCALL_BUTTON_TAB 1
#define SPCALL_BUTTON_Y 2
#define SPCALL_BUTTON_N 3
#define SPCALL_BUTTON_F 4
#define SPCALL_HUD_FIST 5

bool bWeaponClicked;
extern bool bNeedEnterVehicle;

extern "C" {
	JNIEXPORT void JNICALL Java_com_wn_blackrussia_game_Multiplayer_initializeClient(JNIEnv *pEnv, jobject thiz, jint maxFrameRate)
	{
		pJavaWrapper = new CJavaWrapper(pEnv, thiz);
        (void)maxFrameRate;
        g_maxFPS = FORCED_FPS;

        GUIManager::Create(UI_ID_HUD);
        pJavaWrapper->loadingScreenCopyright("reBR 2026-2027. Powered by Weikton. All rights reserved.");
	}

	JNIEXPORT jintArray JNICALL Java_com_wn_blackrussia_game_Multiplayer_getGraphicsSettings(JNIEnv *pEnv, jobject thiz)
	{
		auto &s = CGraphicsSettings::Get();

		jint values[7] = {
			s.iWaterQuality,
			s.iCarReflections,
			s.iDrawDistance,
			s.bLowVehicleDrawDist ? 1 : 0,
			s.bOptimizedRender ? 1 : 0,
			s.bAntialiasing ? 1 : 0,
			s.bEnhancedSky ? 1 : 0,
		};

		jintArray result = pEnv->NewIntArray(7);
		if (result) {
			pEnv->SetIntArrayRegion(result, 0, 7, values);
		}
		return result;
	}

	JNIEXPORT void JNICALL Java_com_wn_blackrussia_game_Multiplayer_applyGraphicsSettings(
			JNIEnv *pEnv, jobject thiz,
			jint water, jint reflections, jint drawDistance,
			jboolean lowVehicleDist, jboolean optimizedRender,
			jboolean antialiasing, jboolean enhancedSky)
	{
		auto &s = CGraphicsSettings::Get();

		s.iWaterQuality       = water;
		s.iCarReflections     = reflections;
		s.iDrawDistance       = drawDistance;
		s.bLowVehicleDrawDist = lowVehicleDist;
		s.bOptimizedRender    = optimizedRender;
		s.bAntialiasing       = antialiasing;
		s.bEnhancedSky        = enhancedSky;

		CGame::PostToMainThread([] {
			CGraphicsSettings::Apply();
			CGraphicsSettings::Save();
		});
	}
	JNIEXPORT void JNICALL Java_com_wn_blackrussia_game_Multiplayer_onInputEnd(JNIEnv *pEnv, jobject thiz, jbyteArray str)
	{
		if(pUI)
		{
			pUI->keyboard()->sendForGB(pEnv, thiz, str);
		}
	}
    JNIEXPORT void JNICALL Java_com_wn_blackrussia_game_Multiplayer_displayAccessState(JNIEnv *pEnv, jobject thiz, jboolean state)
	{
        if(pGame) pGame->EnableGameInput(state);
    }
	JNIEXPORT void JNICALL Java_com_wn_blackrussia_game_Multiplayer_onEventBackPressed(JNIEnv *pEnv, jobject thiz)
    {
		if(pSettings && pUI && pJavaWrapper)
        {
			if(pSettings->Get().iAndroidKeyboard)
            {
                if(pUI->keyboard() && pUI->keyboard()->visible())
                {
                    pJavaWrapper->HideKeyboard();
                }
            }
		}
	}
	JNIEXPORT void JNICALL Java_com_wn_blackrussia_game_Multiplayer_sendDialogResponse(JNIEnv* pEnv, jobject thiz, jint i3, jint i, jint i2, jbyteArray str)
	{
		jboolean isCopy = true;

		jbyte* pMsg = pEnv->GetByteArrayElements(str, &isCopy);
		jsize length = pEnv->GetArrayLength(str);

		std::string szStr((char*)pMsg, length);
		
		if(pNetGame) {
			pNetGame->SendDialogResponse(i, i3, i2, (char*)szStr.c_str());
            /*if(!pNetGame->GetTextDrawPool()->GetState()) {
                GUIManager::TempToggle(UI_ID_HUD, true);
            }*/
            CJavaWrapper::GUIDialogShow = false;
		}

		pEnv->ReleaseByteArrayElements(str, pMsg, JNI_ABORT);
	}
    JNIEXPORT void JNICALL Java_com_wn_blackrussia_game_Multiplayer_sendCommand(JNIEnv* pEnv, jobject thiz, jbyteArray str)
	{
		jboolean isCopy = true;

		jbyte* pMsg = pEnv->GetByteArrayElements(str, &isCopy);
		jsize length = pEnv->GetArrayLength(str);

		std::string szStr((char*)pMsg, length);

		if(pNetGame) {
			pNetGame->SendChatCommand((char*)szStr.c_str());
		}

		pEnv->ReleaseByteArrayElements(str, pMsg, JNI_ABORT);
	}
    JNIEXPORT void JNICALL Java_com_wn_blackrussia_game_Multiplayer_sendServer(JNIEnv *pEnv, jobject thiz, jint id)
	{
        CGame::PostToMainThread([id] {
            CServerInstance::initConnection((int)id);
        });
    }
    JNIEXPORT jboolean JNICALL Java_com_wn_blackrussia_game_Multiplayer_isPlayerDriving(JNIEnv *pEnv, jobject thiz)
	{
        if (!pGame || !pNetGame) return JNI_FALSE;

        auto pPlayerPool = pNetGame->GetPlayerPool();
        if (!pPlayerPool || !pPlayerPool->GetLocalPlayer()) return JNI_FALSE;

        auto pPed = pPlayerPool->GetLocalPlayer()->GetPlayerPed();
        if (!pPed) return JNI_FALSE;

        return (pPed->IsInVehicle() && !pPed->IsAPassenger()) ? JNI_TRUE : JNI_FALSE;
    }
    JNIEXPORT void JNICALL Java_com_wn_blackrussia_game_Multiplayer_togglePlayer(JNIEnv *pEnv, jobject thiz, jint toggle)
	{
        CGame::PostToMainThread([toggle] {
            if(pGame)
                pGame->FindPlayerPed()->TogglePlayerControllable((bool) toggle);
        });
    }
    JNIEXPORT void JNICALL Java_com_wn_blackrussia_game_Multiplayer_toggleDrawing2dStuff(JNIEnv *pEnv, jobject thiz, jboolean isHide)
	{
        CGame::PostToMainThread([isHide] {
            GUIManager::TempToggle(UI_ID_HUD, !isHide);

            if (pUI) pUI->setInterfaceHidden(isHide, isHide);
        });
    }
    JNIEXPORT void JNICALL Java_com_wn_blackrussia_game_Multiplayer_sendG(JNIEnv* pEnv, jobject thiz)
	{
        CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();
        CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
        if (pVehiclePool)
        {
            VEHICLEID ClosetVehicleID = pVehiclePool->FindNearestToLocalPlayerPed();
            if (ClosetVehicleID < MAX_VEHICLES && pVehiclePool->GetSlotState(ClosetVehicleID))
            {
                CVehicle* pVehicle = pVehiclePool->GetAt(ClosetVehicleID);
                if (pVehicle)
                {
                    CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
                    if (pPlayerPool)
                    {
                        CLocalPlayer* pLocalPlayer;
                        pPlayerPed->EnterVehicle(pVehicle->m_dwGTAId, true);
                        pLocalPlayer->SendEnterVehicleNotification(ClosetVehicleID, true);
                        bNeedEnterVehicle = true;
                    }
                }
            }
        }
	}
    JNIEXPORT void JNICALL Java_com_wn_blackrussia_game_Multiplayer_specialCall(JNIEnv *pEnv, jobject thiz, jint id)
	{
		switch(id)
        {
            case SPCALL_BUTTON_ALT:
            {
                CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
                if (pPlayerPool)
                {
                    CLocalPlayer* pLocalPlayer;
                    if (!pPlayerPool->GetLocalPlayer()->GetPlayerPed()->IsInVehicle() && !pPlayerPool->GetLocalPlayer()->GetPlayerPed()->IsAPassenger())
                        LocalPlayerKeys.bKeys[ePadKeys::KEY_WALK] = true;
                    else
                        LocalPlayerKeys.bKeys[ePadKeys::KEY_FIRE] = true;
                }

                break;
            }
            case SPCALL_BUTTON_TAB:
            {
                if(pUI->playertablist()->visible())
                    pUI->playertablist()->hide();
                else
                    pUI->playertablist()->show();

                break;
            }
            case SPCALL_BUTTON_Y: LocalPlayerKeys.bKeys[ePadKeys::KEY_YES] = true; break;
            case SPCALL_BUTTON_N: LocalPlayerKeys.bKeys[ePadKeys::KEY_NO] = true; break;
            case SPCALL_BUTTON_F: LocalPlayerKeys.bKeys[ePadKeys::KEY_SECONDARY_ATTACK] = true; break;
            case SPCALL_HUD_FIST: {
                if(pGame->FindPlayerPed()) {
                    if(!bWeaponClicked)
                        bWeaponClicked = true;
                    else
                        bWeaponClicked = false;
                }

                break;
            }
        }
	}
}

void MainLoop()
{
	if (pGame->bIsGameExiting) 
		return;

	PollEngineLoadProgress();

	DoInitStuff();

	if (pNetGame) {
		pNetGame->Process();

        if(!pUI->m_bCamEditGui/* && !CJavaWrapper::GUIDialogShow
        */&& pNetGame->GetGameState() == GAMESTATE_CONNECTED && GUIManager::drawHud) {
            pGame->DisplayHUD(false); // .. piece of sh*t, WN delete it in near future.
            *(uint8_t*)(g_libGTASA + (VER_x32 ? 0x991FD8 : 0xC20DFC)) = 0;
        } else {
            *(uint8_t*)(g_libGTASA + (VER_x32 ? 0x991FD8 : 0xC20DFC)) = 1;
        }
	}
}

bool fileExists(const std::string& path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

void InitGui()
{
	Plugin::OnPluginLoad();
	Plugin::OnSampLoad();

	// todo: rework
	std::string font_path = string_format("%sfonts/%s", g_pszStorage, FONT_NAME);
    if (fileExists(font_path)) {
        Log("Loading %s...", FONT_NAME);
    } else {
        Log("Replace %s to \"SAMP/fonts/\"...", FONT_NAME);
        font_path = string_format("%sSAMP/fonts/%s", g_pszStorage, FONT_NAME);
    }

	pUI = new UI(ImVec2(RsGlobal->maximumWidth, RsGlobal->maximumHeight), font_path.c_str());
	pUI->initialize();
	pUI->performLayout();
}

#include "game/multitouch.h"
#include "armhook/patch.h"
#include "util/CUtil.h"

struct sigaction act_old;
struct sigaction act1_old;
struct sigaction act2_old;
struct sigaction act3_old;

extern int g_iLastProcessedSkinCollision, g_iLastProcessedEntityCollision, g_iLastRenderedObject;
extern uintptr_t g_dwLastRetAddrCrash;
void handler(int signum, siginfo_t *info, void* contextPtr)
{
	ucontext* context = (ucontext_t*)contextPtr;

	if (act_old.sa_sigaction)
	{
		act_old.sa_sigaction(signum, info, contextPtr);
	}

	if(info->si_signo == SIGSEGV)
	{
		Log("SIGSEGV | Fault address: 0x%x", info->si_addr);

		PRINT_CRASH_STATES(context);

		CStackTrace::printBacktrace();
	}

	return;
}

void handler1(int signum, siginfo_t *info, void* contextPtr)
{
	ucontext* context = (ucontext_t*)contextPtr;

	if (act1_old.sa_sigaction)
	{
		act1_old.sa_sigaction(signum, info, contextPtr);
	}

	if(info->si_signo == SIGABRT)
	{
		Log("SIGABRT | Fault address: 0x%x", info->si_addr);

		PRINT_CRASH_STATES(context);

		CStackTrace::printBacktrace();
	}

	return;
}

void handler2(int signum, siginfo_t *info, void* contextPtr)
{
	ucontext* context = (ucontext_t*)contextPtr;

	if (act2_old.sa_sigaction)
	{
		act2_old.sa_sigaction(signum, info, contextPtr);
	}

	if(info->si_signo == SIGFPE)
	{
		Log("SIGFPE | Fault address: 0x%x", info->si_addr);

		PRINT_CRASH_STATES(context);

		CStackTrace::printBacktrace();
	}

	return;
}

void handler3(int signum, siginfo_t *info, void* contextPtr)
{
	ucontext* context = (ucontext_t*)contextPtr;

	if (act3_old.sa_sigaction)
	{
		act3_old.sa_sigaction(signum, info, contextPtr);
	}

	if(info->si_signo == SIGBUS)
	{
		Log("SIGBUS | Fault address: 0x%x", info->si_addr);

		PRINT_CRASH_STATES(context);

		CStackTrace::printBacktrace();
	}

	return;
}

jclass LinkJavaClass(jclass localObj) {
    auto env = CJavaWrapper::GetEnv();
    auto globalRef = (jclass)env->NewGlobalRef(localObj);
    env->DeleteLocalRef(localObj);
    return globalRef;
}

int androidSdkVer = 0;
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	javaVM = vm;
	LOGI("t.me/weikton");

	g_libGTASA = CUtil::FindLib("libGame.so");
	if (g_libGTASA == 0x00) {
		LOGE("libGame.so address was not found! ");
		return JNI_VERSION_1_6;
	}

	g_libSAMP = CUtil::FindLib("libmultiplayer.so");
	if (g_libSAMP == 0x00) {
		LOGE("libmultiplayer.so address was not found! ");
		return JNI_VERSION_1_6;
	}

	firebase::crashlytics::Initialize();

	uintptr_t libgtasa = CUtil::FindLib("libGame.so");
	uintptr_t libsamp = CUtil::FindLib("libmultiplayer.so");
	uintptr_t libc = CUtil::FindLib("libc.so");

	// Log("libGame.so: 0x%x", libgtasa);
	// Log("libsamp.so: 0x%x", libsamp);
	// Log("libc.so: 0x%x", libc);

	char str[100];

	sprintf(str, "0x%x", libgtasa);
	firebase::crashlytics::SetCustomKey("libGame.so", str);
	
	sprintf(str, "0x%x", libsamp);
	firebase::crashlytics::SetCustomKey("libsamp.so", str);

	sprintf(str, "0x%x", libc);
	firebase::crashlytics::SetCustomKey("libc.so", str);

    char sdk_ver_str[92];
    if(__system_property_get("ro.build.version.sdk", sdk_ver_str)) {
        androidSdkVer = atoi(sdk_ver_str);
    }

    JNIEnv* tempEnv = nullptr;
    (void)vm->GetEnv((void**)&tempEnv, JNI_VERSION_1_6);
    GUIManager::clazz = LinkJavaClass(tempEnv->FindClass(OBFUSCATE("com/wn/blackrussia/game/GUIManager")));
    SnapShotsWrapper::clazz = LinkJavaClass(tempEnv->FindClass(OBFUSCATE("com/wn/blackrussia/game/ui/util/EntitySnaps")));

	CHook::InitHookStuff();
	InstallSpecialHooks();
	ApplyPatches_level0();
    InitRenderWareFunctions();
    MultiTouch::initialize();

	pGame = new CGame();

	struct sigaction act;
	act.sa_sigaction = handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &act, &act_old);

	struct sigaction act1;
	act1.sa_sigaction = handler1;
	sigemptyset(&act1.sa_mask);
	act1.sa_flags = SA_SIGINFO;
	sigaction(SIGABRT, &act1, &act1_old);

	struct sigaction act2;
	act2.sa_sigaction = handler2;
	sigemptyset(&act2.sa_mask);
	act2.sa_flags = SA_SIGINFO;
	sigaction(SIGFPE, &act2, &act2_old);

	struct sigaction act3;
	act3.sa_sigaction = handler3;
	sigemptyset(&act3.sa_mask);
	act3.sa_flags = SA_SIGINFO;
	sigaction(SIGBUS, &act3, &act3_old);
		
	return JNI_VERSION_1_6;
}

uint32_t GetTickCount()
{
    return CTimer::m_snTimeInMillisecondsNonClipped;
}	

void Log(const char *fmt, ...)
{
	static char buffer[512] {};

	memset(buffer, 0, sizeof(buffer));

	va_list arg;
	va_start(arg, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg);
	va_end(arg);

	firebase::crashlytics::Log(buffer);
	__android_log_write(ANDROID_LOG_INFO, "AXL", buffer);

	static FILE* flLog = nullptr;

	if(flLog == nullptr && g_pszStorage != nullptr)
	{
		sprintf(buffer, "%sreBRClientLog.txt", g_pszStorage);
		flLog = fopen(buffer, "ab");
	}

	if(flLog == nullptr) return;
	fprintf(flLog, "%s\n", buffer);
	fflush(flLog);
}

void ChatLog(const char* fmt, ...)
{
	char buffer[0xFF];
	static FILE* flLog = nullptr;
	const char* pszStorage = g_pszStorage;

	if (flLog == nullptr && pszStorage != nullptr)
	{
		sprintf(buffer, "%s/chat_log.txt", pszStorage);
		flLog = fopen(buffer, "a");
	}

	memset(buffer, 0, sizeof(buffer));

	va_list arg;
	va_start(arg, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg);
	va_end(arg);

	if (flLog == nullptr) return;
	fprintf(flLog, "%s\n", buffer);
	fflush(flLog);

	return;
}

void MyLog(const char* fmt, ...)
{
	char buffer[0xFF];
	static FILE* flLog = nullptr;
	const char* pszStorage = g_pszStorage;

	if (flLog == nullptr && pszStorage != nullptr)
	{
		sprintf(buffer, "%s/samp_log.txt", pszStorage);
		//LOGI("buffer: %s", buffer);
		flLog = fopen(buffer, "a");
	}

	memset(buffer, 0, sizeof(buffer));

	va_list arg;
	va_start(arg, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg);
	va_end(arg);

	if (flLog == nullptr) return;
	fprintf(flLog, "%s\n", buffer);
	fflush(flLog);

	return;
}

void MyLog2(const char* fmt, ...)
{
	char buffer[0xFF];
	static FILE* flLog = nullptr;
	const char* pszStorage = g_pszStorage;

	if (flLog == nullptr && pszStorage != nullptr)
	{
		sprintf(buffer, "%s/samp_log.txt", pszStorage);
		//LOGI("buffer: %s", buffer);
		flLog = fopen(buffer, "a");
	}

	memset(buffer, 0, sizeof(buffer));

	va_list arg;
	va_start(arg, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg);
	va_end(arg);

	if (pUI) pUI->chat()->addDebugMessage(buffer);

	if (flLog == nullptr) return;
	fprintf(flLog, "%s\n", buffer);
	fflush(flLog);
	return;
}

void LogVoice(const char* fmt, ...)
{
	char buffer[0xFF];
	static FILE* flLog = nullptr;
	const char* pszStorage = g_pszStorage;

	if (flLog == nullptr && pszStorage != nullptr)
	{
		sprintf(buffer, "%sSAMP/%s", pszStorage, SV::kLogFileName);
		flLog = fopen(buffer, "w");
	}

	memset(buffer, 0, sizeof(buffer));

	va_list arg;
	va_start(arg, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg);
	va_end(arg);

	__android_log_write(ANDROID_LOG_INFO, "AXL", buffer);

	if (flLog == nullptr) return;
	fprintf(flLog, "%s\n", buffer);
	fflush(flLog);

	return;
}