//
// Created by weikton on 05.08.2026.
//
// Настройки графики. Состав пунктов повторяет меню KING RUSSIA
// (res/layout/br_graphic_settings.xml из их APK).

#pragma once

#include "common.h"

enum eWaterQuality : int32 {
    WATER_OFF       = 0,    // без отражений и преломления
    WATER_LOW       = 1,    // только отражение
    WATER_STANDARD  = 2,    // отражение + туман
    WATER_HIGH      = 3,    // отражение + преломление + туман
};

enum eCarReflections : int32 {
    CARREFL_OFF      = 0,
    CARREFL_LOW      = 1,
    CARREFL_STANDARD = 2,
    CARREFL_HIGH     = 3,
};

struct stGraphicsSettings {
    int32 iWaterQuality;        // 0..3  «Качество воды (сильно влияет на FPS)»
    int32 iCarReflections;      // 0..3  «Отражения на авто»
    int32 iDrawDistance;        // 0..100 «Дальность прорисовки»

    bool  bLowVehicleDrawDist;  // «Уменьшенная дальность прорисовки авто»
    bool  bOptimizedRender;     // «Оптимизированная прорисовка»
    bool  bAntialiasing;        // «Сглаживание (убирает пиксели)»
    bool  bEnhancedSky;         // «Улучшенное небо»
};

class CGraphicsSettings
{
public:
    static void Initialise();
    static void Apply();
    static void Load();
    static void Save();

    // Вызывается каждый кадр: подгоняет дальность видимости и начало тумана
    // под ползунок «Дальность прорисовки» — как в libsamp, где меньшее
    // значение придвигает туман и укорачивает прорисовку.
    static void ApplyVehicleDrawDistance(bool bLowered);

    static stGraphicsSettings& Get() { return ms_Settings; }

    static const char* GetQualityLabel(int32 value);
    static const char* GetDrawDistanceLabel(int32 value);

private:
    static inline stGraphicsSettings ms_Settings {
        /* iWaterQuality       */ WATER_STANDARD,
        /* iCarReflections     */ CARREFL_STANDARD,
        /* iDrawDistance       */ 50,
        /* bLowVehicleDrawDist */ false,
        /* bOptimizedRender    */ true,
        /* bAntialiasing       */ false,
        /* bEnhancedSky        */ false,
    };
};
