//
// Created by weikton on 05.08.2026.
//

#pragma once

#include "game/common.h"
#include "game/RW/RenderWare.h"

enum RQTargetColorType
{
    TCT_RGB_565   = 0,
    TCT_RGBA_8888 = 1,
};

enum RQTargetDepthType
{
    TDT_None    = 0,
    TDT_LowAcc  = 1,
    TDT_HighAcc = 2,
};

class CWaterFX
{
public:
    static void InjectHooks();

    static void Initialise();
    static void Shutdown();

    static void Update(float timeStep);

    static void CaptureFogMap();
    static void CaptureRefractionMap();

    static void OnShaderSelected(void *shader);
    static void BindReflection();
    static void ApplyWaterTexture(bool useDuDv);

    static bool IsAvailable();

public:
    static inline bool  ms_bEnabled       = true;
    static inline bool  ms_bUseRefraction = true;
    static inline float ms_fWaveness      = 0.0f;
    static inline float ms_fWaveSpeed     = 0.06f;
    static inline bool  ms_bDrawingWater  = false;

private:
    static uint32 GetRasterTexID(RwRaster *raster);
    static uint32 GetTargetTexID(void *target);

    static void EnsureTargets();
    static void EnsureDuDv();
    static void CaptureInto(uint32 texID, int width, int height);

private:
    static inline void  *ms_pRefractionTarget = nullptr;
    static inline void  *ms_pFogTarget        = nullptr;
    static inline uint32 ms_dudvTexID         = 0;
    static inline RwTexture *ms_pDuDvTexture  = nullptr;
    static inline RwTexture *ms_pOriginalWaterTex = nullptr;
    static inline bool   ms_bDuDvChecked     = false;
    static inline int    ms_nDuDvWaitFrames   = 0;
    static constexpr int kDuDvMaxWaitFrames   = 120;
    static inline int    ms_targetWidth       = 0;
    static inline int    ms_targetHeight      = 0;
    static inline bool   ms_bInitialised      = false;
};
