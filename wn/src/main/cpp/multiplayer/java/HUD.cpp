//
// Created by weikton on 10.08.2025.
//

#include "HUD.h"
#include <jni.h>

#include "main.h"
#include "jniutil.h"

extern CGame *pGame;
jobject CHud::thiz = nullptr;

extern "C"
JNIEXPORT void JNICALL
Java_com_wn_blackrussia_game_ui_BrHud_hudInit(JNIEnv *env, jobject thiz) {
    CHud::thiz = env->NewGlobalRef(thiz);
}

void CHud::updateG(int val) {
    JNIEnv *env = CJavaWrapper::GetEnv();

    static auto method = env->GetMethodID(env->GetObjectClass(thiz), "updateG", "(I)V");
    env->CallVoidMethod(thiz, method, val);
}

void CHud::updateGreenZone(int val) {
    JNIEnv *env = CJavaWrapper::GetEnv();

    static auto method = env->GetMethodID(env->GetObjectClass(thiz), "updateGreenZone", "(I)V");
    env->CallVoidMethod(thiz, method, val);
}
void CHud::updateGPS(int val) {
    JNIEnv *env = CJavaWrapper::GetEnv();

    static auto method = env->GetMethodID(env->GetObjectClass(thiz), "updateGPS", "(I)V");
    env->CallVoidMethod(thiz, method, val);
}

void CHud::updateX2(int val) {
    JNIEnv *env = CJavaWrapper::GetEnv();

    static auto method = env->GetMethodID(env->GetObjectClass(thiz), "updateX2", "(I)V");
    env->CallVoidMethod(thiz, method, val);
}

void CHud::updateAmmo() {
    CPlayerPed* pPlayerPed = pGame->FindPlayerPed();
    JNIEnv *env = CJavaWrapper::GetEnv();

    static auto method = env->GetMethodID(env->GetObjectClass(thiz), "updateAmmo", "(III)V");
    env->CallVoidMethod(thiz, method,
                            (int) pPlayerPed->m_pPed->m_aWeapons[pPlayerPed->m_pPed->m_nActiveWeaponSlot].dwType,
                            (int) pPlayerPed->m_pPed->m_aWeapons[pPlayerPed->m_pPed->m_nActiveWeaponSlot].dwAmmo,
                            (int) pPlayerPed->m_pPed->m_aWeapons[pPlayerPed->m_pPed->m_nActiveWeaponSlot].dwAmmoInClip);
}
































extern "C"
JNIEXPORT void JNICALL
Java_com_prime_mobile_gui_Speedometer_ClickSpedometr(JNIEnv *env, jobject thiz, jint turn_id,
jboolean toggle) {
// RakNet::BitStream bsSend;
// bsSend.Write((uint8) ID_CUSTOM_RPC);
// bsSend.Write((uint8) RPC_TURN_SIGNAL);
// bsSend.Write((uint8) turn_id);
// bsSend.Write((uint8) toggle);

// pNetGame->GetRakClient()->Send(&bsSend, SYSTEM_PRIORITY, RELIABLE_SEQUENCED, 0);
}
