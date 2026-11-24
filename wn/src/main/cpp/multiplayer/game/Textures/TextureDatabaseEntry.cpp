//
// Created by weikton on 21.11.2023.
//

#include "TextureDatabaseEntry.h"
#include "TextureDatabase.h"
#include "../vendor/armhook/patch.h"

uint32 GetRasterExtOffset()
{
    return *(uint32 *)(g_libGTASA + (VER_x32 ? 0x6B31FC : 0x890108));
}

RwTexture *TextureDatabaseEntry::LoadInstanceHook(TextureDatabaseEntry *thiz, TextureListingContainer *container)
{
    if (!thiz || !container) {
        return nullptr;
    }

    return thiz->LoadInstance(container);
}

RwTexture *TextureDatabaseEntry::LoadInstance(TextureListingContainer *container)
{
    if (!container || !name) {
        return instance;
    }

    RwRaster *raster = container->CreateRaster(this);
    if (!raster) {
        return instance;
    }

    if (instance) {
        if (instance->raster) {
            RwRasterDestroy(instance->raster);
        }
        instance->raster = nullptr;
        RwTextureSetRaster(instance, raster);
    } else {
        instance = RwTextureCreate(raster);
        if (!instance) {
            RwRasterDestroy(raster);
            return nullptr;
        }
        RwTextureSetName(instance, name);
    }

    if (flags & 0x8) {
        raster->privateFlags |= 0x80;

        auto ext = *(uintptr **)((char *)raster + GetRasterExtOffset());
        if (ext && *ext) {
            auto method = *(void (**)(void *, int))(*ext + 8);
            if (method) {
                method(ext, 1);
            }
        }
    }

    if ((flags & 0x20) && detailTexture) {
        raster->privateFlags |= 0x40;
    }

    if (flags & 0x40) {
        raster->privateFlags |= 0x20;
    }

    if (flags & 0x80) {
        raster->privateFlags |= 0x10;
    }

    if (flags & 0x100) {
        raster->privateFlags |= 0x200;
    }

    if (flags & 0x200) {
        raster->privateFlags |= 0x400;
    }

    return instance;
}

void TextureDatabaseEntry::InjectHooks() {
    // Родная LoadInstance работает — не подменяем.
    // CHook::Redirect("_ZN20TextureDatabaseEntry12LoadInstanceERK23TextureListingContainer",
    //                 &TextureDatabaseEntry::LoadInstanceHook);
}
