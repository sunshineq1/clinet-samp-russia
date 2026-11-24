//
// Created by weikton on 22.07.2025.
//
#pragma once
#define ENABLE_SKYBOX

#include "../game/Entity/Object.h"
#include "../game/Object.h"

// todo: обфус

enum eSkyType {
    SKY_DAWN    = 0,    // BR_Sky_Rasv  / BR_Nebo_Rasv
    SKY_DAY     = 1,    // BR_Sky_Day   / BR_Nebo_Day
    SKY_SUNSET  = 2,    // BR_Sky_Zak   / BR_Nebo_Zakat
    SKY_NIGHT   = 3,    // BR_Sky_Night / BR_Nebo_Nght

    SKY_TYPE_COUNT = 4
};

class CSkyBox {
public:
    static inline bool ms_bEnabled = false;
    static void Process(bool bReflectionPass = false);
    static void Shutdown();

    static void SetRotSpeed(float mFRotSpeed);
    static bool IsNeedRender();

    static void  SetSkyType(int type);
    static int   GetSkyType()   { return m_iSkyType; }
    static void  SetMixCoef(float coef);
    static float GetMixCoef()   { return m_fSkyMixCoef; }

    static void SetAutoTimeOfDay(bool enable) { m_bAutoTimeOfDay = enable; }

    static bool IsAvailable() { return m_bAssetsChecked && m_pClump[0] != nullptr; }

    static bool GetFogColour(CRGBA &out);

private:
    static void UpdateFromGameClock();
    static void EnsureAssets();
    static void RenderDome(RpClump *clump, RwTexture *texture, uint8 alpha);
    static float GetClumpRadius(RpClump *clump);

    static RpClump*   LoadClumpFile(const char *fileName);
    static RpClump*   LoadClumpModel(int32_t modelId);
    static RwTexture* LoadPngFile(const char *fileName);
    static RwObject*  ApplyTextureCallback(RwObject *object, void *data);

private:
    static constexpr const char* SKY_MODELS[SKY_TYPE_COUNT] = {
        "BR_Sky_Rasv", "BR_Sky_Day", "BR_Sky_Zak", "BR_Sky_Night",
    };

    static constexpr const char* SKY_MODELS_BLUR[SKY_TYPE_COUNT] = {
        "BR_Sky_Rasv_bl", "BR_Sky_Day_bl", "BR_Sky_Zak_bl", "BR_Sky_Night_bl",
    };

    static inline bool m_bClumpFromFile[2]{};

    static constexpr int32_t SKY_MODEL_IDS[SKY_TYPE_COUNT] = {
        10656, 10657, 10658, 10659,
    };
    static constexpr int32_t SKY_MODEL_IDS_BLUR[SKY_TYPE_COUNT] = {
        10662, 10663, 10664, 10665,
    };
    static constexpr const char* SKY_TEXTURES[SKY_TYPE_COUNT] = {
        "BR_Nebo_Rasv", "BR_Nebo_Day", "BR_Nebo_Zakat", "BR_Nebo_Nght",
    };

    static inline RpClump*   m_pClump[2]{};
    static inline float      m_fClumpRadius[2]{};
    static inline RwTexture* m_pTex[SKY_TYPE_COUNT]{};
    static inline RwTexture* m_pTexBlur[SKY_TYPE_COUNT]{};

    static inline int   m_iSkyType       = SKY_DAY;
    static inline float m_fSkyMixCoef    = 0.0f;
    static inline bool  m_bAutoTimeOfDay = true;
    static inline bool  m_bAssetsChecked = false;
    static inline CRGBA m_HorizonColour[SKY_TYPE_COUNT]{};
    static inline CRGBA* ms_pPendingAverage = nullptr;

    static inline float m_fObjSize   = 0.55f;
    static inline float m_fRotSpeed  = 0.00015f;
    static inline float m_fRotation  = 0.0f;
    static inline bool  m_bNeedRender = false;
    static inline bool  m_bReflectionPass = false;
};
