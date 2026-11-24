//
// Created by weikton on 20.11.2023.
//

#pragma once

#include "game/common.h"
#include "TextureListingContainer.h"
#include "TextureDatabaseEntry.h"

template <class A>  struct TDBArray
{
    unsigned int numAlloced;
    unsigned int numEntries;
    A *dataPtr;
};

enum TextureDatabaseFormat
{
    DF_UNC = 0x0,
    DF_DXT = 0x1,
    DF_360 = 0x2,
    DF_PS3 = 0x3,
    DF_PVR = 0x4,
    DF_ETC = 0x5,
    DF_Default = 0x6,
    DF_ALL = 0x7,
};

struct TextureCategory
{
    const char *name;
    uint32 onFootPriority;
    uint32 slowCarPriority;
    uint32 fastCarPriority;
    uint16 defaultFormat;
    uint16 defaultStreamMode;
};
VALIDATE_SIZE(TextureCategory, (VER_x32 ? 0x14 : 0x18));

void RLEDecompress(uint8 *dst, uint32 destSize, const uint8 *src, uint32 srcSize, uint32 segSize, uint32 escape);

typedef void *OSFile;

class TextureDatabase {
public:
    int (**_vptr$TextureDatabase)(void);
    const char *name;
    TDBArray<TextureCategory> categories;
    TDBArray<TextureDatabaseEntry> entries;
    TDBArray<TextureListingContainer> thumbs[6];
    TDBArray<void *> toFree;
    TextureDatabaseFormat loadedFormat;

public:
    static void InjectHooks();

    void LoadEntry(char *line, bool hashName);
    void GetPNGFilename(char *pngBuffer, const char *entryName);

    static bool LoadEntries(TextureDatabase *thiz, bool hashNames, bool fromBackup);
    static bool LoadThumbs(TextureDatabase *thiz, TextureDatabaseFormat forFormat, bool setEntries);
    static bool LoadDataFile(TextureDatabase *thiz, const char *filename,
                             TDBArray<TextureListingContainer> *intoArray);

    static bool AddToFreeList(TextureDatabase *thiz, void *ptr);
    static void ClearArray(TDBArray<TextureListingContainer> *array);
    static bool ResizeArray(TDBArray<TextureListingContainer> *array, uint32 requiredEntries);
};
VALIDATE_SIZE(TextureDatabase, (VER_x32 ? 0x78 : 0xA8));
