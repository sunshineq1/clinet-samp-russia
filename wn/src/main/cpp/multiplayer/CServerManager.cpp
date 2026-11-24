//
// Created by weikton on 24.04.2025.
//
#include "main.h"
#include "CServerManager.h"
#include "java/GUIManager.h"

#include "gui/gui.h"
extern UI *pUI;
#include "net/netgame.h"
extern CNetGame *pNetGame;
#include "../java/jniutil.h"
extern CJavaWrapper *pJavaWrapper;

const char* g_szServerNames[] = {
        OBFUSCATE("BLACK RUSSIA"),

        OBFUSCATE("TEST"),
        OBFUSCATE("reBR"),
        OBFUSCATE("BLOOD RUSSIA")
};
constexpr size_t MAX_SERVERS = sizeof(g_szServerNames)
        / sizeof(g_szServerNames[0]);

const CServerInstance::CServerInstanceEncrypted g_sEncryptedAddresses[MAX_SERVERS] = {
        // CServerInstance::create(OBFUSCATE("хост"), 1, колво строк в хосте, порт, войс)
        CServerInstance::create(OBFUSCATE("80.242.59.112"), 1, 13, 1373, false),

        CServerInstance::create(OBFUSCATE("185.207.214.14"), 1, 14, 4117, false),
        CServerInstance::create(OBFUSCATE("141.94.184.111"), 1, 14, 1989, false),
        CServerInstance::create(OBFUSCATE("46.174.51.152"), 1, 14, 61974, false)
};

int CServerInstance::iServer = -1;
void CServerInstance::initConnection(int id) {
    if(pNetGame) {
        delete pNetGame;
        pNetGame = nullptr;
    }

    if(pUI && pUI->chat()) {
        pUI->chat()->clear();
    }

    CServerInstance::iServer = id;

    if (id < 0 || (size_t)id >= MAX_SERVERS) {
        Log("Connect: выбран сервер %d, свой (127.0.0.1:7777), ник [%s]",
            id, pSettings ? pSettings->Get().szNickName : "нет настроек");
    } else {
        Log("Connect: сервер %d [%s] -> %s:%d, ник [%s]",
            id, g_szServerNames[id],
            g_sEncryptedAddresses[id].decrypt(),
            (int)g_sEncryptedAddresses[id].getPort(),
            pSettings ? pSettings->Get().szNickName : "нет настроек");
    }

    std::string srvStr = "{\"value\":" + std::to_string(CServerInstance::iServer) + "}";
    GUIManager::LocalPacket(UI_ID_HUD, 3, srvStr);

    if(CServerInstance::iServer == -1) {
        pNetGame = new CNetGame(
                OBFUSCATE("127.0.0.1"),
                7777,
                pSettings->Get().szNickName,
                pSettings->Get().szPassword);
    } else {
        pNetGame = new CNetGame(
                g_sEncryptedAddresses[CServerInstance::iServer].decrypt(),
                g_sEncryptedAddresses[CServerInstance::iServer].getPort(),
                pSettings->Get().szNickName,
                pSettings->Get().szPassword);
    }
}