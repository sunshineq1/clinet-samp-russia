#include "CHandlingDefault.h"

#include "main.h"

extern void (*cHandlingDataMgr__ConvertDataToGameUnits)(void* thiz, tHandlingData* handling);

#include <cstring>

tHandlingData CHandlingDefault::ms_handling[CHandlingDefault::kCount]{};
bool CHandlingDefault::ms_stored[CHandlingDefault::kCount]{};
bool CHandlingDefault::ms_bReady = false;

void CHandlingDefault::Store(const tHandlingData* handling) {
    if (!handling) {
        return;
    }

    const int32_t id = handling->m_nVehicleId;
    if (id < 0 || id >= kCount) {
        return;
    }

    memcpy(&ms_handling[id], handling, sizeof(tHandlingData));

    ms_stored[id] = true;
    ms_bReady = true;
}

const tHandlingData* CHandlingDefault::Get(int32_t handlingId) {
    if (handlingId < 0 || handlingId >= kCount || !ms_stored[handlingId]) {
        return nullptr;
    }

    return &ms_handling[handlingId];
}

int32_t CHandlingDefault::Count() {
    int32_t count = 0;

    for (bool stored : ms_stored) {
        if (stored) {
            count++;
        }
    }

    return count;
}

void CHandlingDefault::ConvertToGameUnits(tHandlingData* handling) {
    if (!handling || !cHandlingDataMgr__ConvertDataToGameUnits) {
        return;
    }

    ms_bConvertingOwn = true;
    cHandlingDataMgr__ConvertDataToGameUnits(ms_pManager, handling);
    ms_bConvertingOwn = false;
}

bool CHandlingDefault::EnsureReady() {
    return ms_bReady;
}
