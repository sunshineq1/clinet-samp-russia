#include "CGraphicsSettings.h"
#include "Renderer.h"
#include "TimeCycle.h"
#include "Draw.h"
#include "constants.h"
#include "Streaming.h"
#include "Water/CWaterFX.h"
#include "Mobile/MobileMenu/MobileMenu.h"
#include "../graphics/CSkyBox.h"
#include "RQShader.h"
#include "Models/ModelInfo.h"
#include "VisibilityPlugins.h"
#include "../vendor/armhook/patch.h"

#include <algorithm>
#include <cstdio>

static const char* kConfigName = "wn_graphics.cfg";

static constexpr float kFogStartLow = 25.0f;
static constexpr float kFogEndLow   = 150.0f;

const char* CGraphicsSettings::GetQualityLabel(int32 value)
{
    switch (value) {
        case 0:  return "Выключено";
        case 1:  return "Низкое";
        case 2:  return "Стандартное";
        default: return "Высокое";
    }
}

const char* CGraphicsSettings::GetDrawDistanceLabel(int32 value)
{
    if (value <= 25)  return "Низкая";
    if (value <= 60)  return "Стандартная";
    if (value <= 85)  return "Высокая";
    return "Максимальная";
}

void CGraphicsSettings::Initialise()
{
    Load();
    Apply();
}

static constexpr float kLowVehicleLodScale = 0.35f;

void CGraphicsSettings::ApplyVehicleDrawDistance(bool bLowered)
{
    CVisibilityPlugins::ms_fVehicleLodScale = bLowered ? kLowVehicleLodScale : 1.0f;
}

void CGraphicsSettings::Apply()
{
    auto &s = ms_Settings;

    s.iWaterQuality   = std::clamp(s.iWaterQuality, 0, 3);
    s.iCarReflections = std::clamp(s.iCarReflections, 0, 3);
    s.iDrawDistance   = std::clamp(s.iDrawDistance, 0, 100);

    CWaterFX::ms_bEnabled       = (s.iWaterQuality > WATER_OFF);
    CWaterFX::ms_bUseRefraction = (s.iWaterQuality >= WATER_HIGH);

    CMobileMenu::ms_MobileSettings[MS_CarReflections].value = s.iCarReflections;

    CMobileMenu::ms_MobileSettings[MS_DrawDistance].value = s.iDrawDistance;
    CRenderer::UpdateLodDistScale(s.iDrawDistance);

    CMobileMenu::ms_MobileSettings[MS_Visuals].value = s.bOptimizedRender ? 2 : 3;

    CSkyBox::ms_bEnabled = s.bEnhancedSky;
    CSkyBox::SetAutoTimeOfDay(true);

    ApplyVehicleDrawDistance(s.bLowVehicleDrawDist);

    CMobileMenu::ms_MobileSettings[MS_Resolution].value = s.bAntialiasing ? 100 : 80;

    const float fogScale = 1.0f + (float)s.iDrawDistance / 50.0f;
    RQShader::g_fFogStart = kFogStartLow * fogScale;
    RQShader::g_fFogEnd   = kFogEndLow   * fogScale;
}

void CGraphicsSettings::Load()
{
    if (!g_pszStorage) {
        return;
    }

    char path[512];
    snprintf(path, sizeof(path), "%s%s", g_pszStorage, kConfigName);

    FILE *f = fopen(path, "rb");
    if (!f) {
        return;
    }

    stGraphicsSettings loaded{};
    if (fread(&loaded, sizeof(loaded), 1, f) == 1) {
        ms_Settings = loaded;
    }

    fclose(f);
}

void CGraphicsSettings::Save()
{
    if (!g_pszStorage) {
        return;
    }

    char path[512];
    snprintf(path, sizeof(path), "%s%s", g_pszStorage, kConfigName);

    FILE *f = fopen(path, "wb");
    if (!f) {
        Log("CGraphicsSettings: не записать %s", path);
        return;
    }

    fwrite(&ms_Settings, sizeof(ms_Settings), 1, f);
    fclose(f);
}
