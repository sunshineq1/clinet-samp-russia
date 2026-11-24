#pragma once

#include <cstdint>
#include <unordered_map>

// Таблица кузовных деталей тюнинга из data/customtune.dat.
// Строка файла: tuneId модельТС тип dummyФрейм модельДетали скрываемыйАтомик
struct SCustomTunePart {
    int32_t tuneId;
    int32_t vehicleModel;
    int32_t type;
    int32_t componentModel;
    char    dummy[32];
    char    hideAtomic[32];
};

class CCustomTune {
public:
    static bool Load();

    static const SCustomTunePart* Find(int32_t tuneId);

    static bool IsLoaded() { return !ms_parts.empty(); }

    static int32_t Count() { return (int32_t)ms_parts.size(); }

    static int32_t CountForModel(int32_t modelId);

    static const char* LastPath() { return ms_lastPath; }

    // Разовая сверка после загрузки карт: у скольких деталей модель
    // вообще лежит в архивах и грузится ли образец
    static void SelfCheck();

private:
    static std::unordered_map<int32_t, SCustomTunePart> ms_parts;
    static char ms_lastPath[512];
};
