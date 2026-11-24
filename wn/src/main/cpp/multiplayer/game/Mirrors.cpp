#include <GLES2/gl2.h>

#include "Mirrors.h"
#include "../vendor/armhook/patch.h"

#include "game.h"
#include "net/netgame.h"
#include "Camera.h"
#include "Scene.h"
#include "TimeCycle.h"
#include "Timer.h"
#include "World.h"
#include "VisibilityPlugins.h"
#include "RQShader.h"
#include "Water/CWaterFX.h"
#include "CGraphicsSettings.h"
#include "Mobile/MobileMenu/MobileMenu.h"
#include "../graphics/CSkyBox.h"

void emu_SetRenderingSphere(float* direction, GLboolean with) {
    CHook::CallFunction<void>("_Z22emu_SetRenderingSpherePfh", direction, (uint8)(with ? 1 : 0));
}

void CMirrors::CreateBuffer()
{
    if (!pBuffer) {
        const RwInt32 depth = RwRasterGetDepth(RwCameraGetRaster(Scene.m_pRwCamera));
        const RwInt32 width = 512;
        const RwInt32 height = 256;

        pBuffer = RwRasterCreate(width, height, depth, rwRASTERTYPECAMERATEXTURE);
        if (pBuffer && !(pZBuffer = RwRasterCreate(width, height, depth, rwRASTERTYPEZBUFFER))) {
            RwRasterDestroy(pBuffer);
            pBuffer = nullptr;
        }
    }

    const int reflectionQuality = CMobileMenu::ms_MobileSettings[MS_CarReflections].value;
    const int waterQuality = CGraphicsSettings::Get().iWaterQuality;

    RwInt32 newReflectionSize = 0;
    if (reflectionQuality >= 2 || CWaterFX::ms_bEnabled) {
        newReflectionSize = (waterQuality >= WATER_HIGH)   ? 1024 :
                            (waterQuality <= WATER_LOW)    ? 256  : 512;
    }

    if (reflBuffer[0]) {
        const bool needResize = (newReflectionSize > 0) &&
                                (reflBuffer[0]->height != newReflectionSize);

        if (newReflectionSize == 0 || needResize) {
            RwRasterDestroy(reflBuffer[0]);
            RwRasterDestroy(reflBuffer[1]);
            reflBuffer[0] = nullptr;
            reflBuffer[1] = nullptr;
        }
    }

    if (newReflectionSize > 0 && !reflBuffer[0]) {
        const RwInt32 depth = RwRasterGetDepth(RwCameraGetRaster(Scene.m_pRwCamera));
        reflBuffer[0] = RwRasterCreate(newReflectionSize, newReflectionSize,
                                       depth, rwRASTERTYPECAMERATEXTURE);
        reflBuffer[1] = reflBuffer[0] ?
                        RwRasterCreate(newReflectionSize, newReflectionSize, depth, rwRASTERTYPEZBUFFER) :
                        nullptr;

        if (!reflBuffer[1]) {
            if (reflBuffer[0]) RwRasterDestroy(reflBuffer[0]);
            reflBuffer[0] = nullptr;
        }
    }
}

extern void RenderScene();
void CMirrors::RenderReflections()
{
    const auto carReflections = CMobileMenu::ms_MobileSettings[MS_CarReflections].value;
    if (carReflections < 1 && !CWaterFX::ms_bEnabled)
        return;

    CreateBuffer();

    if (!FindPlayerPed(-1) || !reflBuffer[0]) {
        return;
    }

    static uint32 nLastUpdate = 0;
    const uint32 now = CTimer::m_snTimeInMillisecondsNonClipped;
    if (now - nLastUpdate < ms_nReflectionIntervalMs) {
        return;
    }
    nLastUpdate = now;

    float reflectionRadius = 40.0f;
    if (carReflections >= 3) {
        reflectionRadius = 120.0f;
    } else if (carReflections == 2) {
        reflectionRadius = 80.0f;
    }

    CHook::CallFunction<void>("_ZN7CCamera14SetToSphereMapEf", &CCamera::Get(), reflectionRadius);

    const auto originalMirrorType = TypeOfMirror;
    TypeOfMirror = MIRROR_TYPE_SPHERE_MAP;

    const auto originalFrameBuffer = RwCameraGetRaster(Scene.m_pRwCamera);
    const auto originalZBuffer = RwCameraGetZRaster(Scene.m_pRwCamera);
    const float originalFarPlane = Scene.m_pRwCamera->farPlane;
    const float originalFogPlane = Scene.m_pRwCamera->fogPlane;

    RwCameraSetRaster(Scene.m_pRwCamera, reflBuffer[0]);
    RwCameraSetZRaster(Scene.m_pRwCamera, reflBuffer[1]);

    emu_SetRenderingSphere(nullptr, 1);
    RwRGBA skyColor(
            std::max<uint8_t>(CTimeCycle::m_CurrentColours.m_nSkyTopRed, 64u),
            std::max<uint8_t>(CTimeCycle::m_CurrentColours.m_nSkyTopGreen, 64u),
            std::max<uint8_t>(CTimeCycle::m_CurrentColours.m_nSkyTopBlue, 64u),
            255
    );

    RwCameraClear(Scene.m_pRwCamera, &skyColor, rwCAMERACLEARZ | rwCAMERACLEARIMAGE);
    if (RsCameraBeginUpdate(Scene.m_pRwCamera)) {
        Scene.m_pRwCamera->farPlane = reflectionRadius;
        Scene.m_pRwCamera->fogPlane = reflectionRadius * 0.75f;

        bRenderingReflection = true;
        DefinedState();

        if (CSkyBox::ms_bEnabled) {
            CSkyBox::Process(true);
        }

        if (carReflections >= 3 || CWaterFX::ms_bEnabled) {
            CHook::CallFunction<void>("_ZN9CRenderer23ConstructReflectionListEv");
            RenderScene();
        }

        bRenderingReflection = false;
        RwCameraEndUpdate(Scene.m_pRwCamera);

        emu_SetRenderingSphere(nullptr, 0);
        RwCameraSetRaster(Scene.m_pRwCamera, originalFrameBuffer);
        RwCameraSetZRaster(Scene.m_pRwCamera, originalZBuffer);
        TypeOfMirror = originalMirrorType;

        CHook::CallFunction<void>("_ZN7CCamera14SetToSphereMapEf", &CCamera::Get(), 0.0f);
        Scene.m_pRwCamera->farPlane = originalFarPlane;
        Scene.m_pRwCamera->fogPlane = originalFogPlane;
    } else {
        emu_SetRenderingSphere(nullptr, 0);
        CHook::CallFunction<void>("_ZN7CCamera14SetToSphereMapEf", &CCamera::Get(), 0.0f);

        RwCameraSetRaster(Scene.m_pRwCamera, originalFrameBuffer);
        RwCameraSetZRaster(Scene.m_pRwCamera, originalZBuffer);
        Scene.m_pRwCamera->farPlane = originalFarPlane;
        Scene.m_pRwCamera->fogPlane = originalFogPlane;
        TypeOfMirror = originalMirrorType;
    }
}

void CMirrors::BeforeMainRender() {

}

static void (*emu_SetEnvMap)(void *texture, float coefficient, int mode);
static void emu_SetEnvMap_hook(void *texture, float coefficient, int mode)
{
    emu_SetEnvMap(texture, coefficient * RQShader::g_fEnvMapBoost, mode);
}

void CMirrors::InjectHooks() {
    CHook::InlineHook("_Z13emu_SetEnvMapPvfi", &emu_SetEnvMap_hook, &emu_SetEnvMap);

    CHook::Write(g_libGTASA + (VER_x32 ? 0x679504 : 0x850A28), &reflBuffer);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x678B40 : 0x84F6B0), &pBuffer);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x6784DC : 0x84E9E0), &pZBuffer);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x677EB8 : 0x84DDA0), &TypeOfMirror);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x6763E8 : 0x84A848), &bRenderingReflection);

    CHook::Redirect("_ZN8CMirrors12CreateBufferEv", &CreateBuffer);
    CHook::Redirect("_ZN8CMirrors17RenderReflectionsEv", &RenderReflections);
    CHook::Redirect("_ZN8CMirrors16BeforeMainRenderEv", &BeforeMainRender);
}