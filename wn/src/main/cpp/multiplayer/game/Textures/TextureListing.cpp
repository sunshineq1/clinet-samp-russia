//
// Created by weikton on 21.11.2023.
//

#include "TextureListing.h"
#include "../../vendor/armhook/patch.h"

uint32 TextureListing::GetSegSize(uint32 format)
{
    if (format < 0x8C00) {
        if (format == 0x83F0 || format == 0x83F1) {
            return 8;
        }
        if (format == 0x83F2 || format == 0x83F3) {
            return 16;
        }
        return 4;
    }

    switch (format) {
        case 0x8C00:
        case 0x8C02:
            return 16;
        case 0x8C01:
        case 0x8C03:
            return 32;
        case 0x8D64:
            return 8;
        default:
            return 4;
    }
}

uint32 TextureListing::GetRLESegSize() const
{
    return GetSegSize(format);
}

uint32 TextureListing::GetTotalSize() const
{
    return CHook::CallFunction<uint32>("_ZNK14TextureListing12GetTotalSizeEv", this);
}

uint32 TextureListing::GetMipCount() const
{
    if (disableMipChainForce || !HasMipChain()) {
        return 1;
    }

    uint32 w = width;
    uint32 h = flags & 0x7FFF;

    if (w + h < 3) {
        return 1;
    }

    uint32 count = 1;
    do {
        w = (w >> 1) > 1 ? (w >> 1) : 1;
        h = (h >> 1) > 1 ? (h >> 1) : 1;
        ++count;
    } while (w + h > 2);

    return count;
}

void TextureListing::InjectHooks() {
}
