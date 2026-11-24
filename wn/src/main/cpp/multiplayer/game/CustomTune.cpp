#include "CustomTune.h"

#include "CFileMgr.h"
#include "game/Streaming.h"
#include "game/Models/ModelInfo.h"
#include "main.h"

#include <cstdio>
#include <cstring>

std::unordered_map<int32_t, SCustomTunePart> CCustomTune::ms_parts;
char CCustomTune::ms_lastPath[512] = { 0 };

namespace {

const char* const kPaths[] = {
    "data/customtune.dat",
    "DATA/customtune.dat",
    "data/CUSTOMTUNE.DAT",
    "DATA/CUSTOMTUNE.DAT",
    "SAMP/customtune.dat",
};

}

bool CCustomTune::Load() {
    ms_parts.clear();
    ms_lastPath[0] = 0;

    FILE* file = nullptr;

    for (const char* path : kPaths) {
        file = CFileMgr::OpenFile(path, "rb");

        if (file) {
            strncpy(ms_lastPath, CFileMgr::ms_path, sizeof(ms_lastPath) - 1);
            break;
        }
    }

    if (!file) {
        Log("CustomTune: файл не найден, последняя попытка %s", CFileMgr::ms_path);
        return false;
    }

    char line[256];
    int skipped = 0;

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\r' || line[0] == '\n') {
            continue;
        }

        SCustomTunePart part{};
        char dummy[32]{};
        char hide[32]{};

        const int read = sscanf(line, "%d %d %d %31s %d %31s",
                                &part.tuneId, &part.vehicleModel, &part.type,
                                dummy, &part.componentModel, hide);

        if (read != 6) {
            skipped++;
            continue;
        }

        strncpy(part.dummy, dummy, sizeof(part.dummy) - 1);

        if (strcmp(hide, "null") != 0) {
            strncpy(part.hideAtomic, hide, sizeof(part.hideAtomic) - 1);
        }

        ms_parts[part.tuneId] = part;
    }

    CFileMgr::CloseFile(file);

    Log("CustomTune: %s, деталей %u, пропущено строк %d",
        ms_lastPath, (unsigned)ms_parts.size(), skipped);

    return !ms_parts.empty();
}

const SCustomTunePart* CCustomTune::Find(int32_t tuneId) {
    if (ms_parts.empty()) {
        Load();
    }

    auto it = ms_parts.find(tuneId);
    if (it == ms_parts.end()) {
        return nullptr;
    }

    return &it->second;
}

int32_t CCustomTune::CountForModel(int32_t modelId) {
    int32_t count = 0;

    for (const auto& entry : ms_parts) {
        if (entry.second.vehicleModel == modelId) {
            count++;
        }
    }

    return count;
}

void CCustomTune::SelfCheck() {
    int32_t total = 0, inArchive = 0, loaded = 0;
    int32_t sampleId = -1, sampleMissing = -1;

    for (const auto& pair : ms_parts) {
        const int32_t model = pair.second.componentModel;
        total++;

        if (model <= 0 || model >= CModelInfo::NUM_MODEL_INFOS) {
            continue;
        }

        if (!CModelInfo::GetModelInfo(model)) {
            continue;
        }

        if (CStreaming::GetInfo(model).HasCdPosnAndSize()) {
            inArchive++;

            if (sampleId < 0) {
                sampleId = model;
            }
        } else if (sampleMissing < 0) {
            sampleMissing = model;
        }

        if (CStreaming::IsModelLoaded(model)) {
            loaded++;
        }
    }

    Log("CustomTune: деталей %d, с местом в архиве %d, уже загружено %d",
        total, inArchive, loaded);

    if (sampleId > 0) {
        CStreaming::RequestModel(sampleId, STREAMING_GAME_REQUIRED);
        CStreaming::LoadAllRequestedModels(false);

        auto info = CModelInfo::GetModelInfo(sampleId);
        Log("CustomTune: пробная модель %d — в потоке %d, объект %d",
            sampleId, (int)CStreaming::IsModelLoaded(sampleId),
            info && info->m_pRwObject ? 1 : 0);
    }

    if (sampleMissing > 0) {
        Log("CustomTune: без места в архиве, например %d", sampleMissing);
    }
}
