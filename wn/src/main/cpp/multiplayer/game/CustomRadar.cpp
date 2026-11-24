#include "CustomRadar.h"
#include "../main.h"

#include "common.h"
#include "../util/CUtil.h"

#include "../vendor/armhook/patch.h"

#include "java/GUIManager.h"
#include "java/jniutil.h"

void CustomRadar::Init() {
    pTex = new CSprite2d();
    static const CUtil::SNamedTexture kBack[] = {
        { "samp", "hud_back" },
        { "newtextures", "hud_back" },
    };

    pTex->m_pTexture = CUtil::LoadTextureAny("Radar", kBack);
}

void CustomRadar::Render() {
    CRGBA radarColor;
    radarColor.a = 255;
    radarColor.r = 255;
    radarColor.g = 255;
    radarColor.b = 255;

    pTex->Draw(CJavaWrapper::radarBgPos1.x, CJavaWrapper::radarBgPos1.y, CJavaWrapper::radarBgPos2.x, CJavaWrapper::radarBgPos2.x, &radarColor);
}