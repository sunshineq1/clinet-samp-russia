#include "GUIManager.h"

#include "../game/Widgets/WidgetGta.h"
#include "../net/netgame.h"
extern CNetGame *pNetGame;

#include "../game/game.h"
extern CGame *pGame;

#include "../gui/gui.h"
extern UI* pUI;

void GUIManager::ToggleUIChat() {
    pUI->m_bChatGuiDisable = !pUI->m_bChatGuiDisable;

    pUI->chat()->setVisible(true);
    pUI->chat()->setMessagesHidden(pUI->m_bChatGuiDisable);
}

void GUIManager::ClearUIScreen() {
    pUI->m_bCamEditGui = !pUI->m_bCamEditGui;

    const bool bShow = !pUI->m_bCamEditGui;

    pUI->setVisible(true);
    pUI->buttonpanel()->setVisible(bShow);
    pUI->voicebutton()->setVisible(bShow);

    pUI->chat()->setVisible(true);
    pUI->chat()->setMessagesHidden(pUI->m_bCamEditGui || pUI->m_bChatGuiDisable);

    TempToggle(UI_ID_HUD, bShow);
}

void GUIManager::ReceivePacket(Packet *p) {
    RakNet::BitStream bs(p->data, p->length, false);
    bs.IgnoreBits(8);

    uint32 uiId;
    int32 actionId;
    std::string json;

    bs.Read(uiId);
    bs.Read(actionId);

    uint32 jsonLength = 0;
    if (bs.Read(jsonLength) && jsonLength > 0 && jsonLength <= 0x10000) {
        json.resize(jsonLength);
        if (!bs.Read(json.data(), jsonLength)) {
            json.clear();
        }
    }

    auto env = CJavaWrapper::GetEnv();
    jstring jdata = env->NewStringUTF( cp1251_to_utf8(json).c_str() );

    auto method = env->GetStaticMethodID(clazz, "receiveUiPacket", "(IILjava/lang/String;)V");
    env->CallStaticVoidMethod(clazz, method, uiId, actionId, jdata);

    env->DeleteLocalRef(jdata);
}

void GUIManager::LocalPacket(uint32 uiId, int32 actionId, std::string json, ...) {
    auto env = CJavaWrapper::GetEnv();
    jstring jdata = env->NewStringUTF( cp1251_to_utf8(json).c_str());

    auto method = env->GetStaticMethodID(clazz, "receiveUiPacket", "(IILjava/lang/String;)V");
    env->CallStaticVoidMethod(clazz, method, uiId, actionId, jdata);

    env->DeleteLocalRef(jdata);
}

void GUIManager::ExecuteAction(int uiId, eActionId actionId) {
    auto env = CJavaWrapper::GetEnv();

    jstring json = env->NewStringUTF( "" );

    auto method = env->GetStaticMethodID(clazz, "receiveUiPacket", "(IILjava/lang/String;)V");
    env->CallStaticVoidMethod(clazz, method, uiId, actionId, json);

    env->DeleteLocalRef(json);
}

void GUIManager::Create(int uiId) {
    ExecuteAction(uiId, eActionId::ACTION_CREATE);
}

void GUIManager::Destroy(int uiId) {
    ExecuteAction(uiId, eActionId::ACTION_DESTROY);
}

void GUIManager::TempToggle(int uiId, bool toggle) {
    auto actionId = (toggle ? eActionId::ACTION_SHOW : eActionId::ACTION_HIDE);

    ExecuteAction(uiId, actionId);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wn_blackrussia_game_ui_NativeGui_00024Companion_nativeSendPacket(JNIEnv *env, jobject thiz, jint ui_id, jint action_id, jstring json) {
    std::string sendData;

    const char* data = env->GetStringUTFChars(json, nullptr);
    jsize length = env->GetStringUTFLength(json);
    sendData.assign(data, length);
    env->ReleaseStringUTFChars(json, data);

    CGame::PostToMainThread([=] {
        RakNet::BitStream bs;
        bs.Write((uint8_t) PACKET_ANDROID_GUI);
        bs.Write((uint32_t) ui_id);
        bs.Write((int32_t) action_id);
        bs.Write((uint32_t) sendData.length());
        bs.Write(sendData.data(), sendData.length());

        pNetGame->GetRakClient()->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0);
    });
}
extern "C"
JNIEXPORT void JNICALL
Java_com_wn_blackrussia_game_Multiplayer_sendJsonData(JNIEnv *env, jobject thiz, jint guiid, jbyteArray data)
{
    if (!data) {
        return;
    }

    const jsize length = env->GetArrayLength(data);
    std::string sendData;
    sendData.resize((size_t)length);

    env->GetByteArrayRegion(data, 0, length, (jbyte *)sendData.data());

    CGame::PostToMainThread([=] {
        if (!pNetGame || !pNetGame->GetRakClient()) {
            return;
        }

        RakNet::BitStream bs;
        bs.Write((uint8_t) PACKET_ANDROID_GUI);
        bs.Write((uint32_t) guiid);
        bs.Write((int32_t) 0);
        bs.Write((uint32_t) sendData.length());
        bs.Write(sendData.data(), sendData.length());

        pNetGame->GetRakClient()->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0);
    });
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wn_blackrussia_game_ui_NativeGui_00024Companion_nativeUpdateHudState(
    JNIEnv* env, jobject thiz, jboolean hud, jboolean chat)
{
    GUIManager::drawHud = hud;
    GUIManager::drawChat = chat;

    const bool hideHud  = (hud  == JNI_FALSE);
    const bool hideChat = (chat == JNI_FALSE);

    CGame::PostToMainThread([hideHud, hideChat] {
        if (pUI) pUI->setInterfaceHidden(hideHud, hideChat);
    });
}