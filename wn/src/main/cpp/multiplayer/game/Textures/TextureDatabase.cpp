//
// Created by weikton on 20.11.2023.
//

#include "TextureDatabase.h"
#include "../../vendor/armhook/patch.h"

enum OSFileDataArea
{
    OSFDA_Storage = 0,
};

enum OSFileAccessType
{
    OSFAT_Read = 0,
};

typedef int OSFileError;

static OSFileError OS_FileOpen(OSFileDataArea area, OSFile *intoFile, const char *filename, OSFileAccessType access) {
    return CHook::CallFunction<OSFileError>("_Z11OS_FileOpen14OSFileDataAreaPPvPKc16OSFileAccessType",
                                            area, intoFile, filename, access);
}

static int OS_FileSize(OSFile file) {
    return CHook::CallFunction<int>("_Z11OS_FileSizePv", file);
}

static OSFileError OS_FileRead(OSFile file, void *buffer, int numBytes) {
    return CHook::CallFunction<OSFileError>("_Z11OS_FileReadPvS_i", file, buffer, numBytes);
}

static OSFileError OS_FileClose(OSFile file) {
    return CHook::CallFunction<OSFileError>("_Z12OS_FileClosePv", file);
}

void RLEDecompress(uint8 *dst, uint32 destSize, const uint8 *src, uint32 srcSize, uint32 segSize, uint32 escape)
{
    if (!dst || !src || segSize == 0 || (int32)destSize < 1 || srcSize == 0) {
        return;
    }

    const uint8 *dstEnd = dst + destSize;
    const uint8 *srcEnd = src + srcSize;

    while (dst < dstEnd && src < srcEnd) {
        if (*src == escape) {
            if (src + 2 + segSize > srcEnd) {
                break;
            }

            uint32 repeat = src[1];
            const uint8 *seg = src + 2;

            for (uint32 i = 0; i < repeat && dst < dstEnd; ++i) {
                memcpy(dst, seg, segSize);
                dst += segSize;
            }

            src += segSize + 2;
        } else {
            if (src + segSize > srcEnd) {
                break;
            }

            memcpy(dst, src, segSize);
            dst += segSize;
            src += segSize;
        }
    }
}

// Пути в texdb относительные — проверяем существование через тот же корень,
// с которым работает OS_FileOpen.
static bool FileExistsInStorage(const char *relativePath)
{
    if (!relativePath || !g_pszStorage) {
        return false;
    }

    char full[512];
    snprintf(full, sizeof(full), "%s%s", g_pszStorage, relativePath);

    return fileExists(full);
}

void TextureDatabase::GetPNGFilename(char *pngBuffer, const char *entryName) {
    snprintf(pngBuffer, 0x100u, "texdb/%s/src/%s.png", name, entryName);
}

bool TextureDatabase::LoadEntries(TextureDatabase *thiz, bool hashNames, bool fromBackup) {
    return CHook::CallFunction<bool>("_ZN15TextureDatabase11LoadEntriesEbb", thiz, hashNames, fromBackup);
}

bool TextureDatabase::LoadThumbs(TextureDatabase *thiz, TextureDatabaseFormat forFormat, bool setEntries)
{
    if (!thiz || !thiz->name) {
        return false;
    }

    static const char *extensions[] = {
        ".unc.tmb",
        ".dxt.tmb",
        ".360.tmb",
        ".ps3.tmb",
        ".pvr.tmb",
        ".etc.tmb",
    };

    if ((int)forFormat < 0 || (int)forFormat >= (int)ARRAY_SIZE(extensions)) {
        forFormat = DF_ETC;
    }

    char thumbPath[256]{};
    snprintf(thumbPath, sizeof(thumbPath), "texdb/%s/%s%s", thiz->name, thiz->name, extensions[(int)forFormat]);

    if (FileExistsInStorage(thumbPath)) {
        return LoadDataFile(thiz, thumbPath, &thiz->thumbs[forFormat]);
    }

    // Запрошенного формата в кеше нет. Вместо падения берём любой, который есть:
    // без этого база грузится с пустыми миниатюрами и роняет SortEntries.
    static const TextureDatabaseFormat fallback[] = { DF_DXT, DF_ETC, DF_PVR, DF_UNC };

    for (auto candidate : fallback) {
        if (candidate == forFormat) {
            continue;
        }

        char altPath[256]{};
        snprintf(altPath, sizeof(altPath), "texdb/%s/%s%s", thiz->name, thiz->name, extensions[(int)candidate]);

        if (FileExistsInStorage(altPath)) {
            Log("TextureDatabase: %s нет в формате %d, беру %d", thiz->name, (int)forFormat, (int)candidate);
            thiz->loadedFormat = candidate;
            return LoadDataFile(thiz, altPath, &thiz->thumbs[candidate]);
        }
    }

    Log("TextureDatabase: у %s нет ни одного файла миниатюр", thiz->name);
    return LoadDataFile(thiz, thumbPath, &thiz->thumbs[forFormat]);
}

bool TextureDatabase::AddToFreeList(TextureDatabase *thiz, void *ptr)
{
    TDBArray<void *> &toFree = thiz->toFree;

    if (toFree.numEntries + 1 > toFree.numAlloced) {
        uint32 newSize = (3 * (toFree.numEntries + 1) / 2) + 3;

        void **newData = (void **)malloc(newSize * sizeof(void *));
        if (!newData) {
            return false;
        }

        if (toFree.dataPtr) {
            memcpy(newData, toFree.dataPtr, toFree.numEntries * sizeof(void *));
            free(toFree.dataPtr);
        }

        toFree.dataPtr = newData;
        toFree.numAlloced = newSize;
    }

    toFree.dataPtr[toFree.numEntries++] = ptr;
    return true;
}

void TextureDatabase::ClearArray(TDBArray<TextureListingContainer> *array)
{
    if (!array) {
        return;
    }

    for (uint32 i = 0; i < array->numEntries; ++i) {
        if (array->dataPtr[i].ownsData) {
            free(array->dataPtr[i].data);
        }
        array->dataPtr[i].data = nullptr;
        array->dataPtr[i].dataSize = 0;
    }

    free(array->dataPtr);
    array->dataPtr = nullptr;
    array->numEntries = 0;
    array->numAlloced = 0;
}

bool TextureDatabase::ResizeArray(TDBArray<TextureListingContainer> *array, uint32 requiredEntries)
{
    if (!array) {
        return false;
    }

    if (requiredEntries <= array->numAlloced) {
        return true;
    }

    uint32 newSize = (3 * requiredEntries / 2) + 3;

    auto *newData = (TextureListingContainer *)realloc(array->dataPtr, newSize * sizeof(TextureListingContainer));
    if (!newData) {
        return false;
    }

    array->dataPtr = newData;
    array->numAlloced = newSize;
    return true;
}

// SortEntries ходит по thumbs теми же индексами, что и по entries. Если файла
// миниатюр нет, массив всё равно обязан быть нужной длины — иначе падение.
static bool FillEmptyThumbs(TextureDatabase *thiz, TDBArray<TextureListingContainer> *intoArray)
{
    if (!TextureDatabase::ResizeArray(intoArray, thiz->entries.numEntries)) {
        return false;
    }

    for (uint32 i = intoArray->numEntries; i < thiz->entries.numEntries; ++i) {
        memset(&intoArray->dataPtr[i], 0, sizeof(TextureListingContainer));
    }

    intoArray->numEntries = thiz->entries.numEntries;
    return true;
}

bool TextureDatabase::LoadDataFile(TextureDatabase *thiz, const char *filename,
                                   TDBArray<TextureListingContainer> *intoArray)
{
    if (!thiz || !filename || !intoArray) {
        return false;
    }

    OSFile file = nullptr;
    if (OS_FileOpen(OSFDA_Storage, &file, filename, OSFAT_Read) != 0 || !file) {
        FillEmptyThumbs(thiz, intoArray);
        return false;
    }

    int rawSize = OS_FileSize(file);
    if (rawSize <= 0) {
        OS_FileClose(file);
        FillEmptyThumbs(thiz, intoArray);
        return false;
    }

    auto fileSize = (uint32)rawSize;
    auto *fileData = (char *)malloc(fileSize);
    if (!fileData) {
        OS_FileClose(file);
        FillEmptyThumbs(thiz, intoArray);
        return false;
    }

    OS_FileRead(file, fileData, (int)fileSize);
    OS_FileClose(file);

    if (!AddToFreeList(thiz, fileData)) {
        free(fileData);
        return false;
    }

    const char *cursor = fileData;
    uint32 remaining = fileSize;

    // SortEntries идёт по entries и обращается к thumbs по тем же индексам,
    // поэтому массив обязан быть той же длины. Даже если разбор оборвётся,
    // оставшиеся элементы остаются нулевыми, но существуют.
    if (!ResizeArray(intoArray, thiz->entries.numEntries)) {
        return false;
    }

    for (uint32 i = 0; i < thiz->entries.numEntries; ++i) {
        TextureListingContainer &container = intoArray->dataPtr[intoArray->numEntries];
        memset(&container, 0, sizeof(container));
        ++intoArray->numEntries;

        if (!thiz->entries.dataPtr) {
            break;
        }

        if (thiz->entries.dataPtr[i].flags & 4) {
            continue;
        }

        if (remaining < 16) {
            Log("TextureDatabase::LoadDataFile: %s обрезан на записи %u", filename, i);
            break;
        }

        memcpy(&container.listing, cursor, sizeof(TextureListing));
        container.dataSize = container.listing.GetTotalSize();

        uint32 rawSkip = *(const uint32 *)(cursor + 8);
        if (rawSkip < 4) {
            Log("TextureDatabase::LoadDataFile: %s битая запись %u", filename, i);
            break;
        }

        uint32 skipSize = rawSkip - 4;
        uint32 escape = *(const uint32 *)(cursor + 12);
        const char *payload = cursor + 16;

        if (remaining - 16 < skipSize) {
            Log("TextureDatabase::LoadDataFile: %s выход за конец на записи %u", filename, i);
            break;
        }

        if (escape == 0) {
            container.data = (void *)payload;
            container.ownsData = false;
        } else {
            uint32 segSize = container.listing.GetRLESegSize();
            uint32 alignedSize = ((container.dataSize + segSize - 1) & ~(segSize - 1)) + 0x40;

            void *data = malloc(alignedSize);
            if (!data) {
                break;
            }
            memset(data, 0, alignedSize);

            container.data = data;
            container.ownsData = true;

            RLEDecompress((uint8 *)data, container.dataSize, (const uint8 *)payload, skipSize, segSize, escape);
        }

        // Запись: 8 (listing) + 4 (rawSkip) + 4 (escape) + payload.
        // rawSkip считается от поля escape, поэтому payload = rawSkip - 4 = skipSize,
        // а шаг по записи — 16 + skipSize.
        cursor += skipSize + 16;
        remaining -= skipSize + 16;
    }

    return true;
}

void TextureDatabase::InjectHooks() {
    // Родной загрузчик миниатюр работает — не подменяем его. Наши LoadThumbs и
    // LoadDataFile остаются доступными (и уже сверены с дизассемблером), но
    // включать их без нужды нельзя: они заменяют рабочий путь загрузки texdb.
    // CHook::Redirect("_ZN15TextureDatabase10LoadThumbsE21TextureDatabaseFormatb", &TextureDatabase::LoadThumbs);
    // CHook::Redirect("_ZN15TextureDatabase12LoadDataFileEPKcR8TDBArrayI23TextureListingContainerE",
    //                 &TextureDatabase::LoadDataFile);
}

void TextureDatabase::LoadEntry(char *line, bool hashName) {
}
