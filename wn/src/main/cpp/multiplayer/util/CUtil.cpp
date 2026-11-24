//
// Created by roman on 11/19/2024.
//

#include <dlfcn.h>
#include "CUtil.h"
#include "game/Textures/TextureDatabaseRuntime.h"
#include "vendor/armhook/patch.h"

uintptr_t CUtil::FindLib(const char* libname)
{
    void* handle = dlopen(libname, RTLD_LAZY);
    if (handle) {
        void* symbol = dlsym(handle, "JNI_OnLoad");
        if (symbol) {
            Dl_info info;
            if (dladdr(symbol, &info) != 0) {
                return reinterpret_cast<uintptr_t>(info.dli_fbase);
            }
        }
        dlclose(handle);
    }

    return 0;
}

RwTexture* CUtil::LoadTextureFromDB(const char* dbname, const char* texture)
{
    TextureDatabaseRuntime* db_handle = TextureDatabaseRuntime::GetDatabase(dbname);
    if(!db_handle)
    {
        Log("Error: Database not found! (%s)", dbname);
        return nullptr;
    }

    TextureDatabaseRuntime::Register(db_handle);

    bool hasSibling = false;
    const int entryIndex = CHook::CallFunction<int>(
            "_ZN22TextureDatabaseRuntime8GetEntryEPKcRb", db_handle, texture, &hasSibling);
    if (entryIndex >= 0) {
        db_handle->LoadFullTexture((uint32)entryIndex);
    }

    auto tex = CUtil::GetTexture(texture);

    TextureDatabaseRuntime::Unregister(db_handle);

    if(!tex)
    {
        Log("Error: Texture (%s) not found in database (%s)", texture, dbname);
        return nullptr;
    }

    return tex;
}

RwTexture *CUtil::LoadTextureAny(const char *tag, const SNamedTexture *list, int count)
{
    for (int i = 0; i < count; i++)
    {
        RwTexture *tex = CUtil::LoadTextureFromDB(list[i].db, list[i].name);

        if (tex)
        {
            Log("%s: %s / %s", tag, list[i].db, list[i].name);
            return tex;
        }
    }

    Log("%s: не нашлось ни одного из %d имён", tag, count);
    return nullptr;
}

RwTexture* CUtil::GetTexture(const char* name)
{
    auto tex = TextureDatabaseRuntime::GetTexture(name);
    if (!tex)
    {
        //tex = CUtil::LoadTextureFromDB("gta3", "ahoodfence2");
        Log("WARNING! No tex = %s", name);
        return nullptr;
    }
    ++tex->refCount;

    return tex;
}

void __fastcall CUtil::TransformPoint(RwV3d &result, const CSimpleTransform &t, const RwV3d &v)
{
    float cos_heading = cosf(t.m_fHeading);
    float sin_heading = sinf(t.m_fHeading);

    result = {
            t.m_vPosn.x + cos_heading * v.x - sin_heading * v.y,
            t.m_vPosn.y + sin_heading * v.x + cos_heading * v.y,
            v.z + t.m_vPosn.z
    };
}

float CUtil::GetDistanceBetween3DPoints(const RwV3d f, const RwV3d s)
{
    return sqrt(pow(s.x - f.x, 2) + pow(s.y - f.y, 2) + pow(s.z - f.z, 2));
}

#include <algorithm>

#include <unistd.h> // system api
#include <sys/mman.h>
#include <assert.h> // assert()
#include <dlfcn.h> // dlopen

static const int table[128] =
{
// 80
0x82D0,     0x83D0,     0x9A80E2,   0x93D1,     0x9E80E2,   0xA680E2,   0xA080E2,   0xA180E2,
0xAC82E2,   0xB080E2,   0x89D0,     0xB980E2,   0x8AD0,     0x8CD0,     0x8BD0,     0x8FD0,
// 90
0x92D1,     0x9880E2,   0x9980E2,   0x9C80E2,   0x9D80E2,   0xA280E2,   0x9380E2,   0x9480E2,
0,          0xA284E2,   0x99D1,     0xBA80E2,   0x9AD1,     0x9CD1,     0x9BD1,     0x9FD1,
// A0
0xA0C2,     0x8ED0,     0x9ED1,     0x88D0,     0xA4C2,     0x90D2,     0xA6C2,     0xA7C2,
0x81D0,     0xA9C2,     0x84D0,     0xABC2,     0xACC2,     0xADC2,     0xAEC2,     0x87D0,
// B0
0xB0C2,     0xB1C2,     0x86D0,     0x96D1,     0x91D2,     0xB5C2,     0xB6C2,     0xB7C2,
0x91D1,     0x9684E2,   0x94D1,     0xBBC2,     0x98D1,     0x85D0,     0x95D1,     0x97D1,
// C0
0x90D0,     0x91D0,     0x92D0,     0x93D0,     0x94D0,     0x95D0,     0x96D0,     0x97D0,
0x98D0,     0x99D0,     0x9AD0,     0x9BD0,     0x9CD0,     0x9DD0,     0x9ED0,     0x9FD0,
// D0
0xA0D0,     0xA1D0,     0xA2D0,     0xA3D0,     0xA4D0,     0xA5D0,     0xA6D0,     0xA7D0,
0xA8D0,     0xA9D0,     0xAAD0,     0xABD0,     0xACD0,     0xADD0,     0xAED0,     0xAFD0,
// E0
0xB0D0,     0xB1D0,     0xB2D0,     0xB3D0,     0xB4D0,     0xB5D0,     0xB6D0,     0xB7D0,
0xB8D0,     0xB9D0,     0xBAD0,     0xBBD0,     0xBCD0,     0xBDD0,     0xBED0,     0xBFD0,
// F0
0x80D1,     0x81D1,     0x82D1,     0x83D1,     0x84D1,     0x85D1,     0x86D1,     0x87D1,
0x88D1,     0x89D1,     0x8AD1,     0x8BD1,     0x8CD1,     0x8DD1,     0x8ED1,     0x8FD1
};

std::string cp1251_to_utf8(const std::string& in) {
return cp1251_to_utf8(in.c_str());
}

std::string cp1251_to_utf8(const char* in) {
std::string result;

while (*in) {
if (*in & 0x80) {
int v = table[(int)(0x7f & *in++)];
if (!v)
continue;
result += (char)v;
result += (char)(v >> 8);
if (v >>= 16)
result += (char)v;
} else { // ASCII
result += *in++;
}
}

return result;
}

void cp1251_to_utf8(char* out, const char* in, unsigned int len)
{
int count = 0;

while (*in)
{
if (len && (count >= len)) break;

if (*in & 0x80)
{
int v = table[(int)(0x7f & *in++)];
if (!v)
continue;
*out++ = (char)v;
*out++ = (char)(v >> 8);
if (v >>= 16)
* out++ = (char)v;
}
else // ASCII
*out++ = *in++;

count++;
}

*out = 0;
}

#include <unordered_map>

static const std::unordered_map<uint32_t, char> utf8_to_cp1251_map = {
{0x0401, 0xA8}, {0x0451, 0xB8},
{0x0410, 0xC0}, {0x0411, 0xC1}, {0x0412, 0xC2}, {0x0413, 0xC3},
{0x0414, 0xC4}, {0x0415, 0xC5}, {0x0416, 0xC6}, {0x0417, 0xC7},
{0x0418, 0xC8}, {0x0419, 0xC9}, {0x041A, 0xCA}, {0x041B, 0xCB},
{0x041C, 0xCC}, {0x041D, 0xCD}, {0x041E, 0xCE}, {0x041F, 0xCF},
{0x0420, 0xD0}, {0x0421, 0xD1}, {0x0422, 0xD2}, {0x0423, 0xD3},
{0x0424, 0xD4}, {0x0425, 0xD5}, {0x0426, 0xD6}, {0x0427, 0xD7},
{0x0428, 0xD8}, {0x0429, 0xD9}, {0x042A, 0xDA}, {0x042B, 0xDB},
{0x042C, 0xDC}, {0x042D, 0xDD}, {0x042E, 0xDE}, {0x042F, 0xDF},
{0x0430, 0xE0}, {0x0431, 0xE1}, {0x0432, 0xE2}, {0x0433, 0xE3},
{0x0434, 0xE4}, {0x0435, 0xE5}, {0x0436, 0xE6}, {0x0437, 0xE7},
{0x0438, 0xE8}, {0x0439, 0xE9}, {0x043A, 0xEA}, {0x043B, 0xEB},
{0x043C, 0xEC}, {0x043D, 0xED}, {0x043E, 0xEE}, {0x043F, 0xEF},
{0x0440, 0xF0}, {0x0441, 0xF1}, {0x0442, 0xF2}, {0x0443, 0xF3},
{0x0444, 0xF4}, {0x0445, 0xF5}, {0x0446, 0xF6}, {0x0447, 0xF7},
{0x0448, 0xF8}, {0x0449, 0xF9}, {0x044A, 0xFA}, {0x044B, 0xFB},
{0x044C, 0xFC}, {0x044D, 0xFD}, {0x044E, 0xFE}, {0x044F, 0xFF},
};

std::string utf8_to_cp1251(const std::string& utf8_str) {
std::string result;
size_t len = utf8_str.length();
for (size_t i = 0; i < len;) {
unsigned char c = utf8_str[i];
if (c & 0x80) {
uint32_t codepoint = 0;
int num_bytes = 0;

if ((c & 0xE0) == 0xC0) {
codepoint = c & 0x1F;
num_bytes = 1;
} else if ((c & 0xF0) == 0xE0) {
codepoint = c & 0x0F;
num_bytes = 2;
} else if ((c & 0xF8) == 0xF0) {
codepoint = c & 0x07;
num_bytes = 3;
} else {
++i;
continue;
}

while (num_bytes--) {
++i;
if (i >= len) break;
c = utf8_str[i];
if ((c & 0xC0) != 0x80) {
break;
}
codepoint = (codepoint << 6) | (c & 0x3F);
}

auto it = utf8_to_cp1251_map.find(codepoint);
if (it != utf8_to_cp1251_map.end()) {
result += it->second;
} else {
}
} else {
result += c;
}
++i;
}
return result;
}