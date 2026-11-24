//
// Created by weiktonpenka on 04.09.2023.
//

#pragma once


#include "WidgetGta.h"
#include "../sprite2d.h"

class CWidgetButton : public CWidgetGta
{
public:
    uint32      m_nButtonFlags;
    float       m_fPulssateTime;
    CSprite2d   m_SpriteFill;
    float       m_fFillLevel;
    float       m_fTVTop;
    float       m_fTVBottom;

public:
    static void InjectHooks();
};
