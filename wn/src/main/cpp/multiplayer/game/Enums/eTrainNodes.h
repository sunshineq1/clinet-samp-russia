//
// Created by weikton on 02.02.25.
//

#pragma once
#include <cstdint>

enum eTrainNodes {
    TRAIN_NODE_NONE   = 0,
    TRAIN_DOOR_LF     = 1,
    TRAIN_DOOR_RF     = 2,
    TRAIN_WHEEL_RF1   = 3,
    TRAIN_WHEEL_RF2   = 4,
    TRAIN_WHEEL_RF3   = 5,
    TRAIN_WHEEL_RB1   = 6,
    TRAIN_WHEEL_RB2   = 7,
    TRAIN_WHEEL_RB3   = 8,
    TRAIN_WHEEL_LF1   = 9,
    TRAIN_WHEEL_LF2   = 10,
    TRAIN_WHEEL_LF3   = 11,
    TRAIN_WHEEL_LB1   = 12,
    TRAIN_WHEEL_LB2   = 13,
    TRAIN_WHEEL_LB3   = 14,
    TRAIN_BOGIE_FRONT = 15,
    TRAIN_BOGIE_REAR  = 16,

    TRAIN_NUM_NODES
};