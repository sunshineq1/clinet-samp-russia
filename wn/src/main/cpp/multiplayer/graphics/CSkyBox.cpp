//
// Created by weikton on 22.07.2025.
//
#include "CSkyBox.h"
#include "game/Streaming.h"
#include "main.h"

#include "net/netgame.h"
extern CNetGame* pNetGame;
#include "game/game.h"
extern CGame* pGame;

#include "game/Models/ModelInfo.h"
#include "game/Clock.h"
#include "game/Camera.h"
#include "game/TimeCycle.h"
#include "game/Timer.h"
#include "game/Scene.h"

#include "app/app_light.h"
#include "vendor/armhook/patch.h"

#include <algorithm>
#include <cmath>

static constexpr uint32 kChunkClump = 0x10;

RpClump* CSkyBox::LoadClumpFile(const char *fileName)
{
    if (!fileName || !g_pszStorage) {
        return nullptr;
    }

    char path[512];
    char relPath[512];
    snprintf(path, sizeof(path), "%sloader/%s.dff", g_pszStorage, fileName);
    snprintf(relPath, sizeof(relPath), "loader/%s.dff", fileName);

    if (!fileExists(path)) {
        return nullptr;
    }

    RwStream *stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, relPath);
    if (!stream) {
        Log("CSkyBox: не открылся поток %s", path);
        return nullptr;
    }

    RpClump *clump = nullptr;
    if (RwStreamFindChunk(stream, kChunkClump, nullptr, nullptr)) {
        clump = RpClumpStreamRead(stream);
    }

    RwStreamClose(stream, nullptr);

    if (clump) {
        Log("CSkyBox: купол из файла %s", relPath);
    }

    return clump;
}

RpClump* CSkyBox::LoadClumpModel(int32_t modelId)
{
    if (modelId <= 0 || modelId >= CModelInfo::NUM_MODEL_INFOS) {
        return nullptr;
    }

    auto info = CModelInfo::GetModelInfo(modelId);
    if (!info) {
        Log("CSkyBox: модели %d нет в таблице", modelId);
        return nullptr;
    }

    if (!CStreaming::IsModelLoaded(modelId)) {
        if (!CStreaming::GetInfo(modelId).HasCdPosnAndSize()) {
            Log("CSkyBox: модель %d не лежит ни в одном архиве", modelId);
            return nullptr;
        }

        CStreaming::RequestModel(modelId, STREAMING_GAME_REQUIRED);
        CStreaming::LoadAllRequestedModels(false);
    }

    if (!CStreaming::IsModelLoaded(modelId) || !info->m_pRwObject) {
        Log("CSkyBox: модель %d не догрузилась", modelId);
        return nullptr;
    }

    auto clump = (RpClump*)info->m_pRwObject;
    if (clump->object.type != rpCLUMP) {
        Log("CSkyBox: модель %d не кламп", modelId);
        return nullptr;
    }

    return clump;
}

static void AverageHorizonColour(RwImage *image, CRGBA &out)
{
    if (!image || !image->cpPixels || image->width <= 0 || image->height <= 0) {
        return;
    }

    const int bytesPerPixel = image->depth / 8;
    if (bytesPerPixel < 3) {
        return;
    }

    const int firstRow = image->height / 2;
    uint32 sumR = 0, sumG = 0, sumB = 0, count = 0;

    for (int y = firstRow; y < image->height; y += 2) {
        const uint8 *row = image->cpPixels + (size_t)y * image->stride;
        for (int x = 0; x < image->width; x += 4) {
            const uint8 *px = row + (size_t)x * bytesPerPixel;
            sumR += px[0];
            sumG += px[1];
            sumB += px[2];
            count++;
        }
    }

    if (!count) {
        return;
    }

    out.r = (uint8)(sumR / count);
    out.g = (uint8)(sumG / count);
    out.b = (uint8)(sumB / count);
    out.a = 255;
}

RwTexture* CSkyBox::LoadPngFile(const char *fileName)
{
    if (!fileName || !g_pszStorage) {
        return nullptr;
    }

    char path[512];
    char relPath[512];
    snprintf(path, sizeof(path), "%sloader/%s.png", g_pszStorage, fileName);
    snprintf(relPath, sizeof(relPath), "loader/%s.png", fileName);

    if (!fileExists(path)) {
        Log("CSkyBox: нет текстуры %s", path);
        return nullptr;
    }

    RwImage *image = RtPNGImageRead(relPath);
    if (!image) {
        Log("CSkyBox: не прочитался png %s", path);
        return nullptr;
    }

    RwInt32 width = 0, height = 0, depth = 0, format = 0;
    RwImageFindRasterFormat(image, rwRASTERTYPETEXTURE, &width, &height, &depth, &format);

    RwRaster *raster = RwRasterCreate(width, height, depth, format);
    if (!raster) {
        RwImageDestroy(image);
        return nullptr;
    }

    RwRasterSetFromImage(raster, image);

    if (ms_pPendingAverage) {
        AverageHorizonColour(image, *ms_pPendingAverage);
        ms_pPendingAverage = nullptr;
    }

    RwImageDestroy(image);

    RwTexture *texture = RwTextureCreate(raster);
    if (!texture) {
        RwRasterDestroy(raster);
        return nullptr;
    }

    return texture;
}

void CSkyBox::EnsureAssets()
{
    if (m_bAssetsChecked) {
        return;
    }
    m_bAssetsChecked = true;

    for (int i = 0; i < SKY_TYPE_COUNT; i++) {
        ms_pPendingAverage = &m_HorizonColour[i];
        m_pTex[i] = LoadPngFile(SKY_TEXTURES[i]);
        ms_pPendingAverage = nullptr;

        char blurName[128];
        snprintf(blurName, sizeof(blurName), "%s_blur", SKY_TEXTURES[i]);
        m_pTexBlur[i] = LoadPngFile(blurName);
    }

    for (int i = 0; i < SKY_TYPE_COUNT && !m_pClump[0]; i++) {
        m_pClump[0] = LoadClumpFile(SKY_MODELS[i]);
        m_bClumpFromFile[0] = m_pClump[0] != nullptr;

        if (!m_pClump[0]) {
            m_pClump[0] = LoadClumpModel(SKY_MODEL_IDS[i]);
        }
    }

    for (int i = 0; i < SKY_TYPE_COUNT && !m_pClump[1]; i++) {
        m_pClump[1] = LoadClumpFile(SKY_MODELS_BLUR[i]);
        m_bClumpFromFile[1] = m_pClump[1] != nullptr;

        if (!m_pClump[1]) {
            m_pClump[1] = LoadClumpModel(SKY_MODEL_IDS_BLUR[i]);
        }
    }

    if (!m_pClump[0]) {
        Log("CSkyBox: купол не загружен, скайбокс отключён");
    }
}

bool CSkyBox::GetFogColour(CRGBA &out)
{
    if (!ms_bEnabled || !m_bAssetsChecked) {
        return false;
    }

    const int current = std::clamp(m_iSkyType, 0, SKY_TYPE_COUNT - 1);
    const int next    = (current + 1) % SKY_TYPE_COUNT;

    const CRGBA &a = m_HorizonColour[current];
    const CRGBA &b = m_HorizonColour[next];

    if (a.a == 0 && b.a == 0) {
        return false;
    }

    const float k = std::clamp(m_fSkyMixCoef, 0.0f, 1.0f);

    out.r = (uint8)(a.r + (b.r - a.r) * k);
    out.g = (uint8)(a.g + (b.g - a.g) * k);
    out.b = (uint8)(a.b + (b.b - a.b) * k);
    out.a = 255;
    return true;
}

void CSkyBox::Shutdown()
{
    for (int i = 0; i < 2; i++) {
        if (m_pClump[i] && m_bClumpFromFile[i]) {
            RpClumpDestroy(m_pClump[i]);
        }

        m_pClump[i] = nullptr;
        m_bClumpFromFile[i] = false;
    }

    m_fClumpRadius[0] = 0.0f;
    m_fClumpRadius[1] = 0.0f;

    for (int i = 0; i < SKY_TYPE_COUNT; i++) {
        if (m_pTex[i]) {
            RwTextureDestroy(m_pTex[i]);
            m_pTex[i] = nullptr;
        }
        if (m_pTexBlur[i]) {
            RwTextureDestroy(m_pTexBlur[i]);
            m_pTexBlur[i] = nullptr;
        }
    }

    m_bAssetsChecked = false;
}

void CSkyBox::SetSkyType(int type)
{
    if (type < 0 || type >= SKY_TYPE_COUNT) {
        type = 0;
    }
    m_iSkyType = type;
}

void CSkyBox::SetMixCoef(float coef)
{
    m_fSkyMixCoef = std::clamp(coef, 0.0f, 1.0f);
}

void CSkyBox::SetRotSpeed(float speed)
{
    m_fRotSpeed = speed;
}

bool CSkyBox::IsNeedRender()
{
    return m_bNeedRender;
}

void CSkyBox::UpdateFromGameClock()
{
    const int minutesOfDay = CClock::GetGameClockHours() * 60 + CClock::GetGameClockMinutes();

    static constexpr int kBounds[SKY_TYPE_COUNT] = { 5 * 60, 9 * 60, 18 * 60, 22 * 60 };

    int type = SKY_NIGHT;
    for (int i = SKY_TYPE_COUNT - 1; i >= 0; --i) {
        if (minutesOfDay >= kBounds[i]) {
            type = i;
            break;
        }
    }

    const int from = kBounds[type];
    const int to   = kBounds[(type + 1) % SKY_TYPE_COUNT];

    int span = to - from;
    if (span <= 0) {
        span += 24 * 60;
    }

    int passed = minutesOfDay - from;
    if (passed < 0) {
        passed += 24 * 60;
    }

    m_iSkyType = type;

    const float t = (float)passed / (float)span;
    m_fSkyMixCoef = (t <= 0.5f) ? 0.0f : std::clamp((t - 0.5f) / 0.5f, 0.0f, 1.0f);
}

struct SkyTextureApply {
    RwTexture *texture;
    uint8      alpha;
};

RwObject* CSkyBox::ApplyTextureCallback(RwObject *object, void *data)
{
    auto *atomic = (RpAtomic *)object;
    auto *apply = (SkyTextureApply *)data;

    if (!atomic || !apply || !apply->texture) {
        return object;
    }

    RpGeometry *geometry = atomic->geometry;
    if (!geometry) {
        return object;
    }

    const int numMats = std::min(geometry->matList.numMaterials, 16);
    for (int i = 0; i < numMats; i++) {
        RpMaterial *material = geometry->matList.materials[i];
        if (!material) {
            continue;
        }

        material->texture = apply->texture;
        material->color.alpha = apply->alpha;
    }

    return object;
}

static RwObject* CollectRadiusCallback(RwObject *object, void *data)
{
    auto *atomic = (RpAtomic *)object;
    auto *maxRadius = (float *)data;

    if (atomic && atomic->boundingSphere.radius > *maxRadius) {
        *maxRadius = atomic->boundingSphere.radius;
    }

    return object;
}

float CSkyBox::GetClumpRadius(RpClump *clump)
{
    if (!clump) {
        return 0.0f;
    }

    const int slot = (clump == m_pClump[1]) ? 1 : 0;
    if (m_fClumpRadius[slot] > 0.0f) {
        return m_fClumpRadius[slot];
    }

    float radius = 0.0f;
    RpClumpForAllAtomics(clump, (RpAtomicCallBack)CollectRadiusCallback, &radius);
    m_fClumpRadius[slot] = radius;

    return radius;
}

void CSkyBox::RenderDome(RpClump *clump, RwTexture *texture, uint8 alpha)
{
    if (!clump || !texture) {
        return;
    }

    RwFrame *frame = RpClumpGetFrame(clump);
    if (!frame) {
        return;
    }

    RwMatrix *matrix = RwFrameGetMatrix(frame);
    if (!matrix) {
        return;
    }

    float scale = m_fObjSize;
    if (m_bReflectionPass) {
        scale = m_fObjSize * 0.1f;

        const float radius    = GetClumpRadius(clump);
        const float reference = GetClumpRadius(m_pClump[0]);
        if (radius > 0.0001f && reference > 0.0001f) {
            scale *= reference / radius;
        }
    }

    RwCamera *camera = Scene.m_pRwCamera;
    const float savedFarPlane = camera ? camera->farPlane : 0.0f;

    const bool bAdjustFarPlane = !m_bReflectionPass && camera && savedFarPlane < 3000.0f;
    if (bAdjustFarPlane) {
        RwCameraSetFarClipPlane(camera, 3000.0f);
    }

    const float c = cosf(m_fRotation) * scale;
    const float s = sinf(m_fRotation) * scale;

    matrix->right = RwV3d{  c,  s, 0.0f };
    matrix->up    = RwV3d{ -s,  c, 0.0f };
    matrix->at    = RwV3d{ 0.0f, 0.0f, scale };
    matrix->pos   = CCamera::Get().GetPosition();
    matrix->flags = 0;

    RwFrameUpdateObjects(frame);

    SkyTextureApply apply{ texture, alpha };
    RpClumpForAllAtomics(clump, (RpAtomicCallBack)ApplyTextureCallback, &apply);

    RpClumpRender(clump);

    if (bAdjustFarPlane && savedFarPlane > 0.0f) {
        RwCameraSetFarClipPlane(camera, savedFarPlane);
    }
}

void CSkyBox::Process(bool bReflectionPass)
{
    if (!bReflectionPass) {
        EnsureAssets();
    }

    if (!m_pClump[0] || !m_bAssetsChecked) {
        return;
    }

    m_bReflectionPass = bReflectionPass;

    if (!bReflectionPass) {
        if (m_bAutoTimeOfDay) {
            UpdateFromGameClock();
        }

        m_fRotation += m_fRotSpeed * CTimer::ms_fTimeStep;
        if (m_fRotation > 6.2831853f) {
            m_fRotation -= 6.2831853f;
        }

        DeActivateDirectional();
        SetFullAmbient();
    } else {
        DeActivateDirectional();
        SetFullAmbient();
    }

    m_bNeedRender = true;

    void *prevFog = nullptr, *prevZTest = nullptr, *prevZWrite = nullptr;
    void *prevCull = nullptr, *prevBlend = nullptr, *prevRaster = nullptr;
    void *prevSrcBlend = nullptr, *prevDestBlend = nullptr;
    RwRenderStateGet(rwRENDERSTATEFOGENABLE,         &prevFog);
    RwRenderStateGet(rwRENDERSTATEZTESTENABLE,       &prevZTest);
    RwRenderStateGet(rwRENDERSTATEZWRITEENABLE,      &prevZWrite);
    RwRenderStateGet(rwRENDERSTATECULLMODE,          &prevCull);
    RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &prevBlend);
    RwRenderStateGet(rwRENDERSTATETEXTURERASTER,     &prevRaster);
    RwRenderStateGet(rwRENDERSTATESRCBLEND,          &prevSrcBlend);
    RwRenderStateGet(rwRENDERSTATEDESTBLEND,         &prevDestBlend);

    RwRenderStateSet(rwRENDERSTATEFOGENABLE,         RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,       RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATECULLMODE,          RWRSTATE(rwCULLMODECULLNONE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND,          RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,         RWRSTATE(rwBLENDINVSRCALPHA));

    const int nextType = (m_iSkyType + 1) % SKY_TYPE_COUNT;

    RwTexture *nextTex = m_pTex[nextType];

    /*if (bReflectionPass) {
        static uint32 nLastLog = 0;
        const uint32 now = CTimer::m_snTimeInMillisecondsNonClipped;
        if (now - nLastLog > 3000) {
            nLastLog = now;
            Log("CSkyBox refl: %02d:%02d type %d mix %.2f | tex0 %p tex1 %p",
                CClock::GetGameClockHours(), CClock::GetGameClockMinutes(),
                m_iSkyType, m_fSkyMixCoef, m_pTex[m_iSkyType], nextTex);
        }
    }*/

    RenderDome(m_pClump[0], m_pTex[m_iSkyType], 255);

    if (m_pClump[1] && m_fSkyMixCoef > 0.001f) {
        RenderDome(m_pClump[1], nextTex, (uint8)(m_fSkyMixCoef * 255.0f));
    }

    RwRenderStateSet(rwRENDERSTATEDESTBLEND,         prevDestBlend);
    RwRenderStateSet(rwRENDERSTATESRCBLEND,          prevSrcBlend);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,     prevRaster);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, prevBlend);
    RwRenderStateSet(rwRENDERSTATECULLMODE,          prevCull);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      prevZWrite);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,       prevZTest);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,         prevFog);

    CHook::CallFunction<void>("_Z19ActivateDirectionalv");
    CHook::CallFunction<void>("_Z17SetAmbientColoursv");

    m_bNeedRender = false;
    m_bReflectionPass = false;
}
