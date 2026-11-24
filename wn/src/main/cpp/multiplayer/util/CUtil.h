//
// Created by roman on 11/19/2024.
//

#pragma once
#include "../main.h"
#include "game/SimpleTransform.h"


class CUtil {
public:
    static uintptr_t FindLib(const char *libname);

    // Converts degrees to radians
    // keywords: 0.017453292 flt_8595EC
    constexpr static float DegreesToRadians(float angleInDegrees) {
        return angleInDegrees * PI / 180.0F;
    }

    // Converts radians to degrees
    // 57.295826
    constexpr static float RadiansToDegrees(float angleInRadians) {
        return angleInRadians * 180.0F / PI;
    }

    static RwTexture *LoadTextureFromDB(const char *dbname, const char *texture);

    // Кеши расходятся именами и базами: перебирает пары «база, имя»
    // и отдаёт первую нашедшуюся. tag — для строки в логе.
    struct SNamedTexture {
        const char *db;
        const char *name;
    };

    static RwTexture *LoadTextureAny(const char *tag, const SNamedTexture *list, int count);

    template <int N>
    static RwTexture *LoadTextureAny(const char *tag, const SNamedTexture (&list)[N]) {
        return LoadTextureAny(tag, list, N);
    }

    static RwTexture* GetTexture(const char* name);

    static void __fastcall TransformPoint(RwV3d &result, const CSimpleTransform &t, const RwV3d &v);

    static float GetDistanceBetween3DPoints(const RwV3d f, const RwV3d s);
};

std::string cp1251_to_utf8(const std::string& in);
void cp1251_to_utf8(char *out, const char *in, unsigned int len = 0);
std::string cp1251_to_utf8(const char* in);
std::string utf8_to_cp1251(const std::string& utf8_str);

