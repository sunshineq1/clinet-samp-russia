#pragma once

#include "tHandlingData.h"

// Снимок заводского хендлинга в файловых единицах — таким он приходит из
// handling.cfg до перевода в игровые. Любая правленая копия обязана снова
// пройти через ConvertDataToGameUnits, иначе передаточные числа не пересчитаются.
class CHandlingDefault {
public:
    static constexpr int32_t kCount = 210;

    static void Store(const tHandlingData* handling);

    static const tHandlingData* Get(int32_t handlingId);

    static bool IsReady() { return ms_bReady; }

    static int32_t Count();

    // Перевод в игровые единицы идёт мимо нашего же перехвата: иначе правленая
    // копия попадёт в снимок и подменит собой заводские значения
    static void ConvertToGameUnits(tHandlingData* handling);

    // Снимок наполняется перехватом при разборе handling.cfg. Если игра успела
    // разобрать его раньше, чем встал перехват, файл перечитывается заново.
    static bool EnsureReady();

    static inline void* ms_pManager = nullptr;
    static inline bool  ms_bConvertingOwn = false;

private:
    static tHandlingData ms_handling[kCount];
    static bool ms_stored[kCount];
    static bool ms_bReady;
};
