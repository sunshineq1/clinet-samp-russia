//
// Created by weikton on 05.08.2026.
//

#include "util/CUtil.h"
#include <GLES2/gl2.h>
#include <vector>

#include "CWaterFX.h"
#include "../Mirrors.h"
#include "../RQShader.h"
#include "../Scene.h"
#include "../Timer.h"
#include "../../vendor/armhook/patch.h"

extern uint32 GetRasterExtOffset();

static constexpr uint32 RQTEXTURE_TEXID_OFFSET = VER_x32 ? 0x28 : 0x30;
static constexpr uint32 ES2SHADER_PROGRAM_OFFSET = VER_x32 ? 0x3E8 : 0x3EC;
static constexpr uint32 ES2SHADER_FLAGS_OFFSET   = VER_x32 ? 0x3EC : 0x3F0;

static constexpr int TEXUNIT_ENVMAP     = 1;
static constexpr int TEXUNIT_REFRACTION = 3;
static constexpr int TEXUNIT_DUDV       = 4;
static constexpr int TEXUNIT_FOG        = 5;

static uintptr_t ES2TextureSelectAddr()
{
    return g_libGTASA + (VER_x32 ? 0x1D15DC + 1 : 0x266EA0);
}

static void *(*RQRenderTarget_Create)(uint32, uint32, RQTargetColorType, RQTargetDepthType) = nullptr;
static uint32 *g_pBoundTextures = nullptr;
static uint32 *g_pCurActiveTexture = nullptr;

static GLint (*gl_GetUniformLocation)(GLuint, const GLchar *) = nullptr;
static void  (*gl_Uniform1i)(GLint, GLint) = nullptr;
static void  (*gl_Uniform1fv)(GLint, GLsizei, const GLfloat *) = nullptr;
static void  (*gl_ActiveTexture)(GLenum) = nullptr;
static void  (*gl_BindTexture)(GLenum, GLuint) = nullptr;

template <typename T>
static void ResolveGlImport(T &fn, uintptr_t offset32, uintptr_t offset64)
{
    if (!fn) {
        fn = *(T *)(g_libGTASA + (VER_x32 ? offset32 : offset64));
    }
}

static void ResolveEngineGl()
{
    ResolveGlImport(gl_GetUniformLocation, 0x66FFBC, 0x8403B0);
    ResolveGlImport(gl_Uniform1i,          0x673DD8, 0x846858);
    ResolveGlImport(gl_Uniform1fv,         0x672F34, 0x845048);
    ResolveGlImport(gl_ActiveTexture,      0x670524, 0x840C10);
    ResolveGlImport(gl_BindTexture,        0x672E30, 0x844EA0);
}

static void ResolveEngineApi()
{
    ResolveEngineGl();

    if (!RQRenderTarget_Create) {
        SET_TO(RQRenderTarget_Create,
               CHook::getSym("_ZN14RQRenderTarget6CreateEjj17RQTargetColorType17RQTargetDepthType"));
    }
    if (!g_pBoundTextures) {
        g_pBoundTextures = (uint32 *)CHook::getSym("_ZN10ES2Texture13boundTexturesE");
    }
    if (!g_pCurActiveTexture) {
        g_pCurActiveTexture = (uint32 *)CHook::getSym("_ZN10ES2Texture16curActiveTextureE");
    }
}

uint32 CWaterFX::GetRasterTexID(RwRaster *raster)
{
    if (!raster) {
        return 0;
    }

    auto ext = *(uint8 **)((char *)raster + GetRasterExtOffset());
    if (!ext) {
        return 0;
    }

    return *(uint32 *)(ext + RQTEXTURE_TEXID_OFFSET);
}

uint32 CWaterFX::GetTargetTexID(void *target)
{
    if (!target) {
        return 0;
    }

    auto texture = *(uint8 **)target;
    if (!texture) {
        return 0;
    }

    return *(uint32 *)(texture + RQTEXTURE_TEXID_OFFSET);
}

bool CWaterFX::IsAvailable()
{
    return ms_bEnabled && ms_bInitialised && ms_pFogTarget;
}

void CWaterFX::EnsureTargets()
{
    if (!RsGlobal) {
        return;
    }

    const int width = (int)RsGlobal->maximumWidth;
    const int height = (int)RsGlobal->maximumHeight;

    if (width <= 0 || height <= 0) {
        return;
    }

    if (ms_pFogTarget && ms_targetWidth == width && ms_targetHeight == height) {
        return;
    }

    ResolveEngineApi();
    if (!RQRenderTarget_Create) {
        return;
    }

    ms_targetWidth = width;
    ms_targetHeight = height;

    ms_pFogTarget = RQRenderTarget_Create((uint32)width, (uint32)height, TCT_RGB_565, TDT_None);

    if (ms_bUseRefraction) {
        ms_pRefractionTarget = RQRenderTarget_Create((uint32)width, (uint32)height, TCT_RGB_565, TDT_None);
    }

}

void CWaterFX::EnsureDuDv()
{
    if (ms_dudvTexID || ms_bDuDvChecked) {
        return;
    }

    if (!ms_pDuDvTexture) {
        if (!g_pszStorage) {
            return;
        }

        char path[512];
        snprintf(path, sizeof(path), "%sloader/waterDUDV.png", g_pszStorage);

        if (!fileExists(path)) {
            static const CUtil::SNamedTexture kDuDv[] = {
                { "newtextures", "waterDUDV" },
                { "samp", "waterDUDV" },
            };

            ms_pDuDvTexture = CUtil::LoadTextureAny("CWaterFX", kDuDv);
            ms_bDuDvChecked = true;

            if (!ms_pDuDvTexture) {
                Log("CWaterFX: нет %s и нет текстуры waterDUDV, вода без искажения", path);
            }

            return;
        }

        RwImage *image = RtPNGImageRead("loader/waterDUDV.png");
        if (!image) {
            ms_bDuDvChecked = true;
            Log("CWaterFX: не прочитался %s", path);
            return;
        }

        RwInt32 width = 0, height = 0, depth = 0, format = 0;
        RwImageFindRasterFormat(image, rwRASTERTYPETEXTURE, &width, &height, &depth, &format);

        RwRaster *raster = RwRasterCreate(width, height, depth, format);
        if (!raster) {
            RwImageDestroy(image);
            ms_bDuDvChecked = true;
            Log("CWaterFX: не создался растр для waterDUDV");
            return;
        }

        RwRasterSetFromImage(raster, image);
        RwImageDestroy(image);

        ms_pDuDvTexture = RwTextureCreate(raster);
        if (!ms_pDuDvTexture) {
            RwRasterDestroy(raster);
            ms_bDuDvChecked = true;
            return;
        }

        ms_nDuDvWaitFrames = 0;
        Log("CWaterFX: waterDUDV прочитан (%dx%d), ждём texID", width, height);
    }

    RwRaster *raster = ms_pDuDvTexture->raster;
    if (!raster) {
        ms_bDuDvChecked = true;
        return;
    }

    auto ext = *(uint8 **)((char *)raster + GetRasterExtOffset());
    if (ext) {
        CHook::CallFunction<void>(ES2TextureSelectAddr(), ext, (uint32)TEXUNIT_DUDV);
    }

    ms_dudvTexID = GetRasterTexID(raster);

    if (!ms_dudvTexID) {
        if (++ms_nDuDvWaitFrames > kDuDvMaxWaitFrames) {
            ms_bDuDvChecked = true;
            Log("CWaterFX: waterDUDV не получил texID за %d кадров, вода без искажения", kDuDvMaxWaitFrames);
        }
        return;
    }

    ms_bDuDvChecked = true;

    const int width  = raster->width;
    const int height = raster->height;
    const int depth  = raster->depth;

    ResolveEngineApi();

    const uint32 previousUnit = g_pCurActiveTexture ? *g_pCurActiveTexture : 0;
    const uint32 previousTex  = g_pBoundTextures ? g_pBoundTextures[0] : 0;

    gl_ActiveTexture(GL_TEXTURE0);
    gl_BindTexture(GL_TEXTURE_2D, ms_dudvTexID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    gl_BindTexture(GL_TEXTURE_2D, previousTex);
    if (previousUnit != 0) {
        gl_ActiveTexture(GL_TEXTURE0 + previousUnit);
    }

    ApplyWaterTexture(true);

    Log("CWaterFX: waterDUDV готов, texID %u (%dx%d, depth %d), подставлен как текстура воды",
        ms_dudvTexID, width, height, depth);
}

void CWaterFX::ApplyWaterTexture(bool useDuDv)
{
    auto **ppWaterTex = (RwTexture **)(g_libGTASA + (VER_x32 ? 0xA1C45C : 0xCBD068));
    if (!ppWaterTex) {
        return;
    }

    if (useDuDv) {
        if (!ms_pDuDvTexture || *ppWaterTex == ms_pDuDvTexture) {
            return;
        }
        ms_pOriginalWaterTex = *ppWaterTex;
        *ppWaterTex = ms_pDuDvTexture;
        return;
    }

    if (ms_pOriginalWaterTex && *ppWaterTex == ms_pDuDvTexture) {
        *ppWaterTex = ms_pOriginalWaterTex;
    }
}

void CWaterFX::BindReflection()
{
    if (!ms_bEnabled || CMirrors::TypeOfMirror || CMirrors::bRenderingReflection) {
        return;
    }

    RwRaster *refl = CMirrors::reflBuffer[0];
    if (!refl) {
        return;
    }

    auto ext = *(uint8 **)((char *)refl + GetRasterExtOffset());
    if (!ext) {
        return;
    }

    CHook::CallFunction<void>(ES2TextureSelectAddr(), ext, (uint32)TEXUNIT_ENVMAP);

    static uint32 nLastLogTime = 0;
    static uint32 nBinds = 0;
    nBinds++;
    if (CTimer::m_snTimeInMillisecondsNonClipped - nLastLogTime > 5000) {
        nLastLogTime = CTimer::m_snTimeInMillisecondsNonClipped;
        nBinds = 0;
    }
}

static void (*emu_SetWater)(uint8 on) = nullptr;
static void emu_SetWater_hook(uint8 on)
{
    emu_SetWater(on);

    CWaterFX::ms_bDrawingWater = (on != 0);

    if (on) {
        CWaterFX::BindReflection();
    }
}

void CWaterFX::InjectHooks()
{
    CHook::InlineHook("_Z12emu_SetWaterh", &emu_SetWater_hook, &emu_SetWater);
}

void CWaterFX::Initialise()
{
    ms_fWaveness = 0.0f;
    ms_bInitialised = true;

    EnsureTargets();
    EnsureDuDv();
}

void CWaterFX::Shutdown()
{
    ms_pRefractionTarget = nullptr;
    ms_pFogTarget = nullptr;
    if (ms_pDuDvTexture) {
        RwTextureDestroy(ms_pDuDvTexture);
        ms_pDuDvTexture = nullptr;
    }
    ms_dudvTexID = 0;
    ms_bDuDvChecked = false;
    ms_targetWidth = 0;
    ms_targetHeight = 0;
    ms_bInitialised = false;
}

void CWaterFX::Update(float timeStep)
{
    if (!ms_bEnabled) {
        ApplyWaterTexture(false);
        return;
    }

    ApplyWaterTexture(true);

    if (!ms_bInitialised) {
        Initialise();
    } else {
        EnsureTargets();
        EnsureDuDv();
    }

    ms_fWaveness += timeStep * ms_fWaveSpeed;
    if (ms_fWaveness > 1.0f) {
        ms_fWaveness -= (float)(int)ms_fWaveness;
    }
}

void CWaterFX::CaptureInto(uint32 texID, int width, int height)
{
    if (!texID || width <= 0 || height <= 0) {
        return;
    }

    ResolveEngineApi();

    const uint32 previousUnit = g_pCurActiveTexture ? *g_pCurActiveTexture : 0;
    const uint32 previousTex  = g_pBoundTextures ? g_pBoundTextures[0] : 0;

    gl_ActiveTexture(GL_TEXTURE0);
    gl_BindTexture(GL_TEXTURE_2D, texID);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height);

    gl_BindTexture(GL_TEXTURE_2D, previousTex);

    if (previousUnit != 0) {
        gl_ActiveTexture(GL_TEXTURE0 + previousUnit);
    }
}

void CWaterFX::CaptureFogMap()
{
    if (!IsAvailable() || CMirrors::TypeOfMirror) {
        return;
    }

    CaptureInto(GetTargetTexID(ms_pFogTarget), ms_targetWidth, ms_targetHeight);
}

void CWaterFX::CaptureRefractionMap()
{
    if (!IsAvailable() || !ms_bUseRefraction || CMirrors::TypeOfMirror) {
        return;
    }

    CaptureInto(GetTargetTexID(ms_pRefractionTarget), ms_targetWidth, ms_targetHeight);
}

struct WaterUniformCache
{
    uint32 program;
    GLint  dudv;
    GLint  fog;
    GLint  waveness;
    GLint  refraction;
};

static WaterUniformCache g_uniformCache[64];
static int g_uniformCacheCount = 0;

static const WaterUniformCache *LookupUniforms(uint32 program)
{
    for (int i = 0; i < g_uniformCacheCount; ++i) {
        if (g_uniformCache[i].program == program) {
            return &g_uniformCache[i];
        }
    }

    if (g_uniformCacheCount >= (int)ARRAY_SIZE(g_uniformCache)) {
        return nullptr;
    }

    WaterUniformCache &entry = g_uniformCache[g_uniformCacheCount++];
    entry.program = program;
    entry.dudv = gl_GetUniformLocation(program, "DuDvMap");
    entry.fog = gl_GetUniformLocation(program, "FogMap");
    entry.waveness = gl_GetUniformLocation(program, "Waveness");
    entry.refraction = gl_GetUniformLocation(program, "RefractionMap");

    return &entry;
}

void CWaterFX::OnShaderSelected(void *shader)
{
    if (!shader) {
        return;
    }

    if (ms_bDrawingWater) {
        BindReflection();
    }

    if (!IsAvailable()) {
        return;
    }

    const uint32 program = *(uint32 *)((char *)shader + ES2SHADER_PROGRAM_OFFSET);
    if (!program) {
        return;
    }

    ResolveEngineApi();

    if (!ms_dudvTexID) {
        EnsureDuDv();
    }

    const WaterUniformCache *cache = LookupUniforms(program);
    if (!cache) {
        return;
    }

    if (cache->dudv < 0 || cache->waveness < 0) {
        return;
    }

    static int nLoggedPrograms = 0;
    static uint32 lastLoggedProgram = 0;
    if (nLoggedPrograms < 6 && program != lastLoggedProgram) {
        nLoggedPrograms++;
        lastLoggedProgram = program;
    }

    const GLint dudvLoc = cache->dudv;
    const GLint fogLoc = cache->fog;
    const GLint wavenessLoc = cache->waveness;
    const GLint refractLoc = cache->refraction;

    if (dudvLoc >= 0 && ms_dudvTexID) {
        gl_Uniform1i(dudvLoc, TEXUNIT_DUDV);
        gl_ActiveTexture(GL_TEXTURE0 + TEXUNIT_DUDV);
        gl_BindTexture(GL_TEXTURE_2D, ms_dudvTexID);
        if (g_pBoundTextures) {
            g_pBoundTextures[TEXUNIT_DUDV] = ms_dudvTexID;
        }
    }

    if (fogLoc >= 0) {
        const uint32 fogTexID = GetTargetTexID(ms_pFogTarget);
        gl_Uniform1i(fogLoc, TEXUNIT_FOG);
        gl_ActiveTexture(GL_TEXTURE0 + TEXUNIT_FOG);
        gl_BindTexture(GL_TEXTURE_2D, fogTexID);
        if (g_pBoundTextures) {
            g_pBoundTextures[TEXUNIT_FOG] = fogTexID;
        }
    }

    if (refractLoc >= 0 && ms_pRefractionTarget) {
        const uint32 refractTexID = GetTargetTexID(ms_pRefractionTarget);
        gl_Uniform1i(refractLoc, TEXUNIT_REFRACTION);
        gl_ActiveTexture(GL_TEXTURE0 + TEXUNIT_REFRACTION);
        gl_BindTexture(GL_TEXTURE_2D, refractTexID);
        if (g_pBoundTextures) {
            g_pBoundTextures[TEXUNIT_REFRACTION] = refractTexID;
        }
    }

    if (wavenessLoc >= 0) {
        const GLfloat waveValue = ms_dudvTexID ? ms_fWaveness : 0.0f;
        gl_Uniform1fv(wavenessLoc, 1, &waveValue);
    }

}
