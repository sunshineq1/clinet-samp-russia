#pragma once

#include "main.h"
#include "../net/netgame.h"

#include "jniutil.h"
#include <string>

#define UI_ID_HUD 0
#define UI_ID_SELECTSERVER 1
#define UI_ID_HUD_MENU 2
#define UI_ID_SPAWN 3
#define UI_ID_PLAYERS_LIST 25
#define UI_ID_RADIAL_CAR 27

class GUIManager {
public:
    static inline jclass clazz;

    enum class eActionId {
        ACTION_CREATE     = -1,
        ACTION_DESTROY    = -2,

        // если нужно временно скрыть не удаляя
        ACTION_SHOW       = -3,
        ACTION_HIDE       = -4,

        ACTION_ONCLOSED   = -5,
        ACTION_GOTOP      = -6
    };

    static inline bool drawHud  = false;
    static inline bool drawChat = false;

public:
    static void ReceivePacket(Packet* p);
    static void LocalPacket(uint32 uiId, int32 actionId, std::string json = "", ...);

    static void ClearUIScreen();
    static void ToggleUIChat();

    static void ExecuteAction(int uiId, eActionId actionId);
    static void Create(int uiId);
    static void TempToggle(int uiId, bool toggle);
    static void Destroy(int uiId);
};
