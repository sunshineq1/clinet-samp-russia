#include "../main.h"
#include "VehicleNeon.h"
#include "Entity/CVehicleGTA.h"
#include "Collision/ColModel.h"
#include "Shadows.h"
#include "util/CUtil.h"

RwTexture* CVehicleNeon::GetTexture()
{
    static RwTexture* pTex = nullptr;

    if (!pTex) {
        pTex = CUtil::LoadTextureFromDB(ms_szTextureDb, ms_szTexture);
    }

    return pTex;
}

void CVehicleNeon::ResetTexture()
{
    GetTexture();
}

void CVehicleNeon::SetColor(uint8_t slot, const CRGBA& color)
{
    if (slot >= NEON_SLOT_COUNT) {
        return;
    }

    m_aColors[slot] = color;
}

bool CVehicleNeon::IsSet() const
{
    for (const CRGBA& color : m_aColors) {
        if (color.a != 0) {
            return true;
        }
    }

    return false;
}

void CVehicleNeon::DrawSource(CVehicleGTA* pVehicle, int32_t id, const CRGBA& color,
                              const CVector& vecPosn, const CVector& vecFront, const CVector& vecSide)
{
    if (color.a == 0) {
        return;
    }

    RwTexture* pTexture = GetTexture();
    if (!pTexture) {
        return;
    }

    const float scale = (float)color.a / 255.0f * ms_fBrightness;

    CVector posn = vecPosn;

    CShadows::StoreCarLightShadow(
            pVehicle, id, pTexture, &posn,
            vecFront.x, vecFront.y,
            vecSide.x, vecSide.y,
            (uint8_t)(color.r * scale),
            (uint8_t)(color.g * scale),
            (uint8_t)(color.b * scale),
            ms_fViewAngle, true);
}

void CVehicleNeon::Render(CVehicleGTA* pVehicle) const
{
    if (!pVehicle) {
        return;
    }

    CColModel* pColModel = pVehicle->GetColModel();
    if (!pColModel) {
        return;
    }

    const CVector& vecMin = pColModel->m_boundBox.m_vecMin;
    const CVector& vecMax = pColModel->m_boundBox.m_vecMax;

    const float bottom = vecMin.z + ms_fGroundOffset;

    const CVector vecOrigin = pVehicle->TransformFromObjectSpace(CVector(0.0f, 0.0f, 0.0f));
    const CVector vecGround = pVehicle->TransformFromObjectSpace(CVector(0.0f, 0.0f, bottom));

    const CVector vecLength = pVehicle->TransformFromObjectSpace(CVector(0.0f, vecMax.y, 0.0f)) - vecOrigin;
    const CVector vecWidth  = pVehicle->TransformFromObjectSpace(CVector(vecMax.x, 0.0f, 0.0f)) - vecOrigin;

    const int32_t base = (int32_t)pVehicle->m_nModelIndex * NEON_SLOT_COUNT;

    DrawSource(pVehicle, base + NEON_UNDER, m_aColors[NEON_UNDER], vecGround,
               vecLength * ms_fUnderLength, vecWidth * ms_fUnderWidth);

    if (m_aColors[NEON_LEFT].a == 0 && m_aColors[NEON_RIGHT].a == 0) {
        return;
    }

    const CVector vecShift = vecWidth * ms_fSideOffset;

    DrawSource(pVehicle, base + NEON_LEFT, m_aColors[NEON_LEFT], vecGround - vecShift,
               vecLength * ms_fSideLength, vecWidth * ms_fSideWidth);

    DrawSource(pVehicle, base + NEON_RIGHT, m_aColors[NEON_RIGHT], vecGround + vecShift,
               vecLength * ms_fSideLength, vecWidth * ms_fSideWidth);
}
