//
// Created by weikton on 10.08.2025.
//

#pragma once
#include "game/common.h"
#include "net/netgame.h"

class CHud {
public:
    static jobject thiz;

    static void updateAmmo();
    static void updateG(int val);

    static void updateGreenZone(int val);
    static void updateGPS(int val);

    static void updateX2(int val);
};

