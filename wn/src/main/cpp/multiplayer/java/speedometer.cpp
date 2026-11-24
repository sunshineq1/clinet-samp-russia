//
// Created by weikton on 05.09.2025.
//
#include "speedometer.h"
#include "main.h"

#include "../java/jniutil.h"
extern CJavaWrapper* pJavaWrapper;

#include "../game/common.h"
#include "../game/Entity/CPedGta.h"

#include "../game/game.h"
extern CGame* pGame;

#include "../net/playerpool.h"
#include "../net/netgame.h"
extern CNetGame* pNetGame;

int Speedometer::iFuel = 30;

void Speedometer::toggleVisibility(bool state) {
    if(pJavaWrapper) {
        pJavaWrapper->toggleSpeed(state);
    }
}

void Speedometer::update() {
    auto *pPed = pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed();
    if(!pPed || !pPed->m_pPed->IsInVehicle()) {
        return;
    }

    CVehicle* pVehicle = pPed->GetCurrentVehicle();
    auto speed = pVehicle->m_pVehicle->GetMoveSpeed().Magnitude() * 175.0f;

    static uint32_t nLastSend = 0;
    static int nLastTurn = -1;

    const uint32_t now = CTimer::m_snTimeInMillisecondsNonClipped;
    const int turn = (int)pVehicle->GetTurnLightsBlinkState();

    if (now - nLastSend < 100 && turn == nLastTurn) {
        return;
    }
    nLastSend = now;
    nLastTurn = turn;

    if(pJavaWrapper) {
        pJavaWrapper->updateSpeed(
                (int) speed,
                Speedometer::iFuel,
                (int) pVehicle->m_pVehicle->fHealth,
                (int) pVehicle->m_iEngineState,
                (int) pVehicle->m_pVehicle->GetLightsStatus(),
                (int) pVehicle->m_bDoorsLocked == 1,
                turn);
    }
}

void Speedometer::rpcFuel(RakNet::BitStream bs) {
    float serverFuel;
    bs.Read(serverFuel);

    Speedometer::iFuel = serverFuel;
}

extern "C" JNIEXPORT void JNICALL
Java_com_wn_blackrussia_game_ui_BrSpeed_specialSpeedometerCall(JNIEnv *env, jobject thiz, jint id) {
    switch (id) {
        case 0: {
            auto *pPlayerPed = pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed();
            CVehicle* pVehicle = pPlayerPed->GetCurrentVehicle();

            auto turnState = pVehicle->GetTurnLightsState();
            if(turnState == eTurnState::TURN_ALL)
                pVehicle->SetTurnLightsState(eTurnState::TURN_OFF);
            else
                pVehicle->SetTurnLightsState(eTurnState::TURN_ALL);
            break;
        }
        case 1: {
            auto *pPlayerPed = pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed();
            CVehicle* pVehicle = pPlayerPed->GetCurrentVehicle();

            auto turnState = pVehicle->GetTurnLightsState();
            if (turnState == eTurnState::TURN_LEFT)
                pVehicle->SetTurnLightsState(eTurnState::TURN_OFF);
            else
                pVehicle->SetTurnLightsState(eTurnState::TURN_LEFT);
            break;
        }
        case 2: {
            auto *pPlayerPed = pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed();
            CVehicle* pVehicle = pPlayerPed->GetCurrentVehicle();

            auto turnState = pVehicle->GetTurnLightsState();
            if(turnState == eTurnState::TURN_RIGHT)
                pVehicle->SetTurnLightsState(eTurnState::TURN_OFF);
            else
                pVehicle->SetTurnLightsState(eTurnState::TURN_RIGHT);
            break;
        }
    }
}