//
// Created by weikton on 05.09.2025.
//
#pragma once

#include <jni.h>
#include "../net/netgame.h"

#define SPEEDOMETERRPC_FUEL 0x103

class Speedometer {
public:
    static void toggleVisibility(bool state);

    static void update();
    static int iFuel;

    static void rpcFuel(RakNet::BitStream bs);
};


