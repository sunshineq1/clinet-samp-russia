#pragma once

#include <cstdint>

#include "rgba.h"
#include "Vector.h"

class CVehicleGTA;

enum eNeonSlot : uint8_t
{
    NEON_UNDER = 0,
    NEON_LEFT  = 1,
    NEON_RIGHT = 2,

    NEON_SLOT_COUNT
};

class CVehicleNeon
{
public:
    void SetColor(uint8_t slot, const CRGBA& color);
    bool IsSet() const;
    void Render(CVehicleGTA* pVehicle) const;

    // Размеры и яркость подбираются на глаз командой /tuneneonp, поэтому
    // это настройки, а не константы. Доли от габаритов кузова.
    static inline float ms_fUnderLength  = 1.15f;
    static inline float ms_fUnderWidth   = 1.20f;
    static inline float ms_fSideLength   = 1.00f;
    static inline float ms_fSideWidth    = 0.35f;
    static inline float ms_fSideOffset   = 0.80f;

    // Насколько поднять пятно над низом кузова, чтобы оно легло на землю
    static inline float ms_fGroundOffset = 0.05f;

    static inline float ms_fBrightness   = 1.00f;

    // Косинус предельного угла обзора; у поворотников тут 7.0, то есть без отсечки
    static inline float ms_fViewAngle    = 7.0f;

    static inline char ms_szTexture[32]  = "headlight1";
    static inline char ms_szTextureDb[16] = "txd";

    static void ResetTexture();

private:
    static RwTexture* GetTexture();
    static void DrawSource(CVehicleGTA* pVehicle, int32_t id, const CRGBA& color,
                           const CVector& vecPosn, const CVector& vecFront, const CVector& vecSide);

    CRGBA m_aColors[NEON_SLOT_COUNT] {
            { 255, 255, 255, 0 },
            { 255, 255, 255, 0 },
            { 255, 255, 255, 0 }
    };
};
