//
// Created by weikton on 21.11.2023.
//

#pragma once

#include "../common.h"

#pragma pack(push, 1)
class TextureListing
{
public:
    uint16 nameHash;
    uint16 format;
    uint16 width;
    uint16 flags;

public:
    static inline bool disableMipChainForce = false;

public:
    static void InjectHooks();

    static uint32 GetSegSize(uint32 format);

    uint32 GetTotalSize() const;
    uint32 GetRLESegSize() const;
    uint32 GetMipCount() const;

    uint16 GetHeight() const { return flags & 0x7FFF; }
    bool   HasMipChain() const { return (flags & 0x8000) != 0; }
};
#pragma pack(pop)

VALIDATE_SIZE(TextureListing, 0x8);
