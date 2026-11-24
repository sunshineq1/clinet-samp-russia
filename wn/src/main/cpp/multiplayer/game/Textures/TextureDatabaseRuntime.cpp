//
// Created by weikton on 11.01.2023.
//

#include "game/SaaArchive.h"
#include "TextureDatabaseRuntime.h"
#include "../RQShader.h"
#include "../vendor/armhook/patch.h"

#include <dirent.h>

static constexpr uint32 MAX_TEXTURE_PAYLOAD = 64u * 1024u * 1024u;

extern uint32 GetRasterExtOffset();

static int OS_FileSetPosition(OSFile file, int position) {
    return CHook::CallFunction<int>("_Z18OS_FileSetPositionPvi", file, position);
}

static int OS_FileReadRt(OSFile file, void *buffer, int numBytes) {
    return CHook::CallFunction<int>("_Z11OS_FileReadPvS_i", file, buffer, numBytes);
}

static RQCapabilities *GetRQCaps() {
    return (RQCapabilities *)(g_libGTASA + (VER_x32 ? 0x6B8B9C : 0x896130));
}

static bool ArchiveHasFile(const char *dbName, const std::string &suffix)
{
    char inside[0xFF];
    snprintf(inside, sizeof(inside), "texdb/%s/%s", dbName, suffix.c_str());

    const char *archive = nullptr;
    uint32_t offset = 0, size = 0;

    return CSaaArchive::Find(inside, archive, offset, size);
}

static bool DirectoryHasFile(const char *dirPath, const std::string &fmt)
{
    DIR *dir = opendir(dirPath);
    if (!dir) {
        return false;
    }

    bool found = false;
    while (struct dirent *entry = readdir(dir)) {
        if (strstr(entry->d_name, fmt.c_str())) {
            found = true;
            break;
        }
    }

    closedir(dir);
    return found;
}

// Список texdb в кеше blackrussia.online описывает только те базы, которые
// клиент BR действительно читает; gta3 и gta_int там опущены, и их .txt лежат
// в архиве в закрытом виде. Игра такой файл разбирает как текст и падает,
// поэтому проверяем заголовок до того, как отдать базу игре.
static bool EntriesLookReadable(const char *dbName)
{
    char relative[0xFF];
    snprintf(relative, sizeof(relative), "texdb/%s/%s.txt", dbName, dbName);

    void *file = nullptr;
    if (CHook::CallFunction<int>("_Z11OS_FileOpen14OSFileDataAreaPPvPKc16OSFileAccessType",
                                 0, &file, relative, 0) != 0 || !file) {
        return false;
    }

    char head[4] = {};
    const int err = CHook::CallFunction<int>("_Z11OS_FileReadPvS_i", file, head, (int)sizeof(head));

    CHook::CallFunction<int>("_Z12OS_FileClosePv", file);

    return err == 0 && memcmp(head, "cat=", sizeof(head)) == 0;
}

TextureDatabaseRuntime* TextureDatabaseRuntime::Load(const char *withName, bool fullyLoad, TextureDatabaseFormat forcedFormat)
{
    if (!withName) {
        return nullptr;
    }

    if (!EntriesLookReadable(withName)) {
        Log("texdb %s: список записей нечитаем, база пропущена", withName);
        return nullptr;
    }

    auto *caps = GetRQCaps();

    char texdbPath[0xFF]{};
    snprintf(texdbPath, sizeof(texdbPath), "%stexdb/%s/", g_pszStorage, withName);

    const std::string dxt = std::string(withName) + ".dxt.tmb";
    const std::string etc = std::string(withName) + ".etc.tmb";
    const std::string pvr = std::string(withName) + ".pvr.tmb";

    const bool hasDxt = DirectoryHasFile(texdbPath, dxt) || ArchiveHasFile(withName, dxt);
    const bool hasEtc = DirectoryHasFile(texdbPath, etc) || ArchiveHasFile(withName, etc);
    const bool hasPvr = DirectoryHasFile(texdbPath, pvr) || ArchiveHasFile(withName, pvr);

    if (forcedFormat == DF_DXT && !caps->hasTextureCompressionDXT1OrS3TCCap) {
        if (hasEtc || hasPvr) {
            forcedFormat = (hasPvr && caps->hasTextureCompressionPVRTCCap) ? DF_PVR : DF_ETC;
        }
    } else if (forcedFormat == DF_PVR && !caps->hasTextureCompressionPVRTCCap) {
        if (hasDxt || hasEtc) {
            forcedFormat = (hasDxt && caps->hasTextureCompressionDXT1OrS3TCCap) ? DF_DXT : DF_ETC;
        }
    } else if (forcedFormat == DF_ETC && !hasEtc) {
        if (hasDxt && caps->hasTextureCompressionDXT1OrS3TCCap) {
            forcedFormat = DF_DXT;
        } else if (hasPvr && caps->hasTextureCompressionPVRTCCap) {
            forcedFormat = DF_PVR;
        }
    }

    const bool hasChosen = (forcedFormat == DF_DXT && hasDxt)
                        || (forcedFormat == DF_ETC && hasEtc)
                        || (forcedFormat == DF_PVR && hasPvr);

    if (!hasChosen) {
        if (hasDxt) {
            forcedFormat = DF_DXT;
        } else if (hasEtc) {
            forcedFormat = DF_ETC;
        } else if (hasPvr) {
            forcedFormat = DF_PVR;
        }
    }

    auto* result = CHook::CallFunction<TextureDatabaseRuntime*>(
            "_ZN22TextureDatabaseRuntime4LoadEPKcb21TextureDatabaseFormat",
            withName, fullyLoad, forcedFormat);

    Log("texdb Load %s: формат %d (файлы dxt %d, etc %d, pvr %d; caps dxt %d, atc %d, pvr %d) -> %p",
        withName, (int)forcedFormat, (int)hasDxt, (int)hasEtc, (int)hasPvr,
        (int)caps->hasTextureCompressionDXT1OrS3TCCap,
        (int)caps->hasTextureCompressionATCCap,
        (int)caps->hasTextureCompressionPVRTCCap,
        (void*)result);

    return result;
}

void TextureDatabaseRuntime::ChangeDir(bool isLoader) {
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E8374 : 0x713F87));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E8374 : 0x713F87)), isLoader? "loader/texdb/%s/%s.txt" : "texdb/%s/%s.txt");

    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E8824 : 0x71402B));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E8824 : 0x71402B)), isLoader? "loader/texdb/%s/src/%s.png" : "texdb/%s/src/%s.png");

    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E8384 : 0x713F97));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E8384 : 0x713F97)), isLoader? "loader/texdb/%s/%s.txt.bak" : "texdb/%s/%s.txt.bak");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E87F0 : 0x713FB3));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E87F0 : 0x713FB3)), isLoader? "loader/texdb/%s/%s.unc.tmb" : "texdb/%s/%s.unc.tmb");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E87DC : 0x713FC7));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E87DC : 0x713FC7)), isLoader? "loader/texdb/%s/%s.dxt.tmb" : "texdb/%s/%s.dxt.tmb");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E87C8 : 0x713FDB));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E87C8 : 0x713FDB)), isLoader? "loader/texdb/%s/%s.360.tmb" : "texdb/%s/%s.360.tmb");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E87B4 : 0x713FEF));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E87B4 : 0x713FEF)), isLoader? "loader/texdb/%s/%s.ps3.tmb" : "texdb/%s/%s.ps3.tmb");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E87A0 : 0x714003));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E87A0 : 0x714003)), isLoader? "loader/texdb/%s/%s.pvr.tmb" : "texdb/%s/%s.pvr.tmb");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E878C : 0x714017));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E878C : 0x714017)), isLoader? "loader/texdb/%s/%s.etc.tmb" : "texdb/%s/%s.etc.tmb");

    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E8C34 : 0x71403F));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E8C34 : 0x71403F)), isLoader? "loader/texdb/%s/%s.dxt" : "texdb/%s/%s.dxt");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E8C24 : 0x71404F));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E8C24 : 0x71404F)), isLoader? "loader/texdb/%s/%s.360" : "texdb/%s/%s.360");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E8C14 : 0x71405F));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E8C14 : 0x71405F)), isLoader? "loader/texdb/%s/%s.ps3" : "texdb/%s/%s.ps3");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E8C04 : 0x71406F));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E8C04 : 0x71406F)), isLoader? "loader/texdb/%s/%s.pvr" : "texdb/%s/%s.pvr");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E8BF4 : 0x71407F));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E8BF4 : 0x71407F)), isLoader? "loader/texdb/%s/%s.etc" : "texdb/%s/%s.etc");

    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x61B46C : 0x757A30));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x61B46C : 0x757A30)), isLoader? "LOADER\\TEXDB" : "TEXDB");
}

void TextureDatabaseRuntime::Register(TextureDatabaseRuntime *toRegister) {
    if (!toRegister) {
        return;
    }
    CHook::CallFunction<void>("_ZN22TextureDatabaseRuntime8RegisterEPS_", toRegister);
}

static TDBArray<TextureDatabaseRuntime*>& EngineLoaded() {
    return *(TDBArray<TextureDatabaseRuntime*>*)(g_libGTASA + (VER_x32 ? 0x6BD180 : 0x89AA18));
}

static TDBArray<TextureDatabaseRuntime*>& EngineRegistered() {
    return *(TDBArray<TextureDatabaseRuntime*>*)(g_libGTASA + (VER_x32 ? 0x6BD174 : 0x89AA08));
}

void TextureDatabaseRuntime::CompactRegistered() {
    auto& registered = EngineRegistered();

    if (!registered.dataPtr) {
        if (registered.numEntries) {
            registered.numEntries = 0;
        }
        return;
    }

    uint32 dst = 0;
    for (uint32 src = 0; src < registered.numEntries; src++) {
        if (registered.dataPtr[src])
            registered.dataPtr[dst++] = registered.dataPtr[src];
    }

    if (dst != registered.numEntries) {
        registered.numEntries = dst;
    } else {
    }
}

void TextureDatabaseRuntime::Unregister(TextureDatabaseRuntime *toUnregister) {
    if (!toUnregister) {
        return;
    }
    CHook::CallFunction<void>("_ZN22TextureDatabaseRuntime10UnregisterEPS_", toUnregister);
}

RwTexture* TextureDatabaseRuntime::GetTexture(const char *name) {
    if (!name) {
        return nullptr;
    }
    return ((RwTexture*(*)(const char*))(g_libGTASA + (VER_x32 ? 0x001E9C64 + 1 : 0x286718)))(name);
}

TextureDatabaseRuntime* TextureDatabaseRuntime::GetDatabase(const char *dbName)
{
    auto& loaded = EngineLoaded();

    if (!dbName || !loaded.dataPtr) {
        return nullptr;
    }

    for (uint32 i = 0; i < loaded.numEntries; ++i) {
        TextureDatabaseRuntime *db = loaded.dataPtr[i];
        if (db && db->name && strcmp(db->name, dbName) == 0) {
            return db;
        }
    }

    return nullptr;
}

uint32 TextureDatabaseRuntime::FinalizeTexture(uint32 entryIndex)
{
    if (!entries.dataPtr || entryIndex >= entries.numEntries) {
        return 0;
    }

    TextureDatabaseEntry &entry = entries.dataPtr[entryIndex];
    entry.flags = (entry.flags & 0x0FFF) | 0x3000;

    if (!entry.instance || !entry.instance->raster) {
        return 0;
    }

    RwRaster *raster = entry.instance->raster;

    auto ext = *(uint32 **)((char *)raster + GetRasterExtOffset());
    if (ext) {
        return ext[1] * ext[2];
    }

    if (raster->width > 0 && raster->height > 0) {
        return (uint32)raster->width * (uint32)raster->height;
    }

    return 0;
}

uint32 TextureDatabaseRuntime::LoadFullTexture(uint32 entryIndex)
{
    if (!entries.dataPtr || entryIndex >= entries.numEntries) {
        return 0;
    }

    TextureDatabaseEntry &entry = entries.dataPtr[entryIndex];

    if (loadedFormat == DF_UNC) {
        if (!entry.name) {
            return 0;
        }

        char filename[256]{};
        GetPNGFilename(filename, entry.name);
        CHook::CallFunction<void>("_ZN20TextureDatabaseEntry7LoadPNGEPKc", &entry, filename);
        return FinalizeTexture(entryIndex);
    }

    if (!streamFile || !fullDataOffsets.dataPtr || entryIndex >= fullDataOffsets.numEntries) {
        return 0;
    }

    OS_FileSetPosition(streamFile, (int)fullDataOffsets.dataPtr[entryIndex]);

    TextureListingContainer buffer{};
    if (OS_FileReadRt(streamFile, &buffer.listing, sizeof(TextureListing)) != 0) {
        return 0;
    }

    buffer.dataSize = buffer.listing.GetTotalSize();
    buffer.data = nullptr;
    buffer.ownsData = true;

    if (buffer.dataSize == 0 || buffer.dataSize > MAX_TEXTURE_PAYLOAD) {
        return 0;
    }

    uint32 compressedSize = 0;
    uint32 escape = 0;
    if (OS_FileReadRt(streamFile, &compressedSize, 4) != 0 ||
        OS_FileReadRt(streamFile, &escape, 4) != 0) {
        return 0;
    }

    const uint32 segSize = buffer.listing.GetRLESegSize();
    if (segSize == 0) {
        return 0;
    }

    if (escape != 0) {
        if (compressedSize < 4 || compressedSize > MAX_TEXTURE_PAYLOAD) {
            return 0;
        }

        const uint32 srcSize = compressedSize - 4;
        auto *src = (uint8 *)malloc(srcSize);
        if (!src) {
            return 0;
        }

        if (OS_FileReadRt(streamFile, src, (int)srcSize) != 0) {
            free(src);
            return 0;
        }

        const uint32 alignedSize = ((((buffer.dataSize + segSize - 1) & ~(segSize - 1)) + 3) & ~3u) + segSize;
        void *data = malloc(alignedSize);
        if (!data) {
            free(src);
            return 0;
        }
        memset(data, 0, alignedSize);
        buffer.data = data;

        RLEDecompress((uint8 *)data, buffer.dataSize, src, srcSize, segSize, escape);
        free(src);
    } else {
        const uint32 alignedSize = ((buffer.dataSize + 3) & ~3u) + segSize;
        void *data = malloc(alignedSize);
        if (!data) {
            return 0;
        }
        memset(data, 0, alignedSize);
        buffer.data = data;

        if (OS_FileReadRt(streamFile, data, (int)buffer.dataSize) != 0) {
            free(data);
            return 0;
        }
    }

    bool loadedScaled = false;

    if (buffer.listing.HasMipChain() && GetRQCaps()->unk_08 && buffer.listing.width >= 5 &&
        categories.dataPtr && entry.category < categories.numEntries &&
        categories.dataPtr[entry.category].onFootPriority != 10 && entry.format != 7) {

        TextureListingContainer scaled{};
        scaled.listing = buffer.listing;
        scaled.ownsData = false;

        scaled.listing.width = buffer.listing.width >> 1;
        scaled.listing.flags = (buffer.listing.flags & 0x8000) | ((buffer.listing.flags >> 1) & 0x3FFF);

        const uint32 scaledSize = scaled.listing.GetTotalSize();
        if (scaled.listing.width > 0 && scaledSize > 0 && scaledSize <= buffer.dataSize) {
            scaled.data = (char *)buffer.data + buffer.dataSize - scaledSize;
            scaled.dataSize = scaledSize;

            CHook::CallFunction<RwTexture*>(
                "_ZN20TextureDatabaseEntry12LoadInstanceERK23TextureListingContainer", &entry, &scaled);
            loadedScaled = true;
        }
    }

    if (!loadedScaled) {
        CHook::CallFunction<RwTexture*>(
            "_ZN20TextureDatabaseEntry12LoadInstanceERK23TextureListingContainer", &entry, &buffer);
    }

    free(buffer.data);
    buffer.data = nullptr;

    return FinalizeTexture(entryIndex);
}

static uint32 LoadFullTextureHook(TextureDatabaseRuntime *thiz, uint32 entryIndex)
{
    if (!thiz || !thiz->name) {
        return 0;
    }
    return thiz->LoadFullTexture(entryIndex);
}

TDBArray<uint32>* TextureDatabaseRuntime::GetStreamingQueue(TextureDatabaseRuntime *db, int queueType)
{
    if (!db) {
        return nullptr;
    }

    switch (queueType) {
        case 0: return &db->priorityStreamingQueue;
        case 1: return &db->renderedStreamingQueue;
        case 2: return &db->unrenderedStreamingQueue;
        default: return nullptr;
    }
}

uint32 TextureDatabaseRuntime::ProcessTextureQueue(TDBArray<uint32> *queue, TextureDatabaseRuntime *db,
                                                   int queueType, uint32 batchLoadLimit, float batchThreshold)
{
    if (!queue || !queue->dataPtr || !db) {
        return 0;
    }

    uint32 processed = 0;
    const bool isUnrendered = (queueType == 2);
    const bool checkLimit = (batchLoadLimit != MAX_TEXELS_IN_MEMORY);

    while (queue->numEntries > 0) {
        const uint32 size = queue->numEntries;
        const uint32 textureId = isUnrendered ? queue->dataPtr[size - 1] : queue->dataPtr[0];

        if (!isUnrendered && size > 1) {
            memmove(queue->dataPtr, queue->dataPtr + 1, sizeof(uint32) * (size - 1));
        }

        queue->numEntries = size - 1;

        processed += db->LoadFullTexture(textureId);

        if (checkLimit && (float)processed > batchThreshold) {
            break;
        }
    }

    return processed;
}

void TextureDatabaseRuntime::MarkAllTexturesAsUnrendered(uint32 numEntries)
{
    auto& loaded = EngineLoaded();

    if (!loaded.dataPtr) {
        return;
    }

    for (uint32 i = 0; i < numEntries; ++i) {
        if (loaded.dataPtr[i]) {
            CHook::CallFunction<void>("_ZN22TextureDatabaseRuntime24MarkTexturesAsUnrenderedEv", loaded.dataPtr[i]);
        }
    }
}

void TextureDatabaseRuntime::UpdateStreaming(float deltaTime, bool flush)
{
    auto& loaded = EngineLoaded();

    static int texelOverrunAccumulator = 0;

    auto *caps = GetRQCaps();

    static constexpr float STREAMING_RATE_CAPS[2] = {1024000.0f, 1536000.0f};
    const float rate = caps->isSlowGPU ? 225280.0f : STREAMING_RATE_CAPS[caps->isMaliChip & 1];

    int texelsThisFrame = texelOverrunAccumulator + (int)(rate * deltaTime);

    const uint32 numLoaded = loaded.numEntries;
    if (!loaded.dataPtr || (!numLoaded && !flush)) {
        return;
    }

    if (storedTexels > MAX_TEXELS_IN_MEMORY || flush) {
        const int64 toFree = texelsThisFrame <= 0 ? 0 : 2LL * texelsThisFrame;
        int64 excess = (int64)storedTexels + toFree - (int64)MAX_TEXELS_IN_MEMORY;

        if (numLoaded > 1 && (excess > 0 || flush)) {
            for (int i = (int)numLoaded - 2; i >= 0 && (excess > 0 || flush); --i) {
                TextureDatabaseRuntime *db = loaded.dataPtr[i + 1];
                if (!db) {
                    continue;
                }

                const uint32 limit = flush ? MAX_TEXELS_IN_MEMORY : (uint32)excess;
                const auto freed = CHook::CallFunction<uint32>("_ZN22TextureDatabaseRuntime14DeleteTexturesEj", db, limit);

                excess -= (int64)freed;
                storedTexels = (freed >= storedTexels) ? 0 : storedTexels - freed;

                if (!flush && excess <= 0) {
                    break;
                }
            }
        }
    }

    if (!numLoaded) {
        return;
    }

    const uint32 batchLoadLimit = flush ? MAX_TEXELS_IN_MEMORY : (uint32)(texelsThisFrame > 0 ? texelsThisFrame : 0);
    const float batchThreshold = (float)batchLoadLimit * 1.5f;

    for (int queueType = 0; queueType < 3; ++queueType) {
        int remaining = texelsThisFrame;

        for (int i = (int)numLoaded - 1; i >= 0 && (remaining > 0 || flush); --i) {
            TextureDatabaseRuntime *db = loaded.dataPtr[i];
            if (!db) {
                continue;
            }

            TDBArray<uint32> *queue = GetStreamingQueue(db, queueType);
            if (!queue || !queue->numEntries) {
                continue;
            }

            const uint32 processed = ProcessTextureQueue(queue, db, queueType, batchLoadLimit, batchThreshold);

            storedTexels += processed;
            remaining -= (int)processed;
        }

        texelsThisFrame = remaining;
    }

    MarkAllTexturesAsUnrendered(numLoaded);

    texelOverrunAccumulator = (texelsThisFrame < 0 && !flush) ? texelsThisFrame : 0;
}

uint32 TextureDatabaseRuntime::GetEntryIndex(TextureDatabaseRuntime *thiz, const char *name, bool *hasSibling)
{
    if (!thiz || !name || !thiz->hashOffsets.dataPtr || !thiz->entries.dataPtr ||
        thiz->numHashes == 0 || thiz->entries.numEntries == 0) {
        return (uint32)-1;
    }

    uint32 hash = 0;
    for (const char *p = name; *p; ++p) {
        hash = 33 * hash + (uint8)*p;
    }
    hash += hash >> 5;

    for (int guard = 0; guard < 32; ++guard) {
        const uint32 bucket = hash % thiz->numHashes;
        if (bucket + 1 >= thiz->hashOffsets.numEntries) {
            return (uint32)-1;
        }

        uint32 index = thiz->hashOffsets.dataPtr[bucket];
        const uint32 end = std::min(thiz->hashOffsets.dataPtr[bucket + 1], thiz->entries.numEntries);

        bool restart = false;
        while (index < end) {
            TextureDatabaseEntry &entry = thiz->entries.dataPtr[index];

            if (entry.nameHash != hash) {
                ++index;
                continue;
            }

            if (hasSibling) {
                *hasSibling |= (entry.flags & 2) != 0;
            }

            if ((entry.flags & 4) == 0) {
                return index;
            }

            if (!entry.matchingName) {
                return (uint32)-1;
            }

            hash = 0;
            for (const char *p = entry.matchingName; *p; ++p) {
                hash = 33 * hash + (uint8)*p;
            }
            hash += hash >> 5;

            restart = true;
            break;
        }

        if (!restart) {
            return (uint32)-1;
        }
    }

    return (uint32)-1;
}

void TextureDatabaseRuntime::InjectHooks() {
    // CHook::Write(g_libGTASA + (VER_x32 ? 0x675FCC : 0x84A020), &detailTextures);
    // CHook::Write(g_libGTASA + (VER_x32 ? 0x676720 : 0x84AEB0), &storedTexels);
    // CHook::Write(g_libGTASA + (VER_x32 ? 0x679768 : 0x850EE8), &loaded);
    // CHook::Write(g_libGTASA + (VER_x32 ? 0x679E70 : 0x851CF8), &registered);

    CHook::Redirect("_ZN22TextureDatabaseRuntime15LoadFullTextureEj", &LoadFullTextureHook);
    // CHook::Redirect("_ZN22TextureDatabaseRuntime15UpdateStreamingEfb", &TextureDatabaseRuntime::UpdateStreaming);
}

RwBool TextureAnnihilate(RwTexture *texture) {
    return CHook::CallFunction<RwBool>(g_libGTASA + (VER_x32 ? 0x1DB33C + 1 : 0x273A1C), texture);
}
