//
// Created by weikton on 02.02.25.
//

#include "../main.h"
#include "game/Enums/eDoorState.h"

class CDoor {
public:
    float      m_fOpenAngle;
    float      m_fClosedAngle;
    int16      m_nDirn; // Def a bitfield. 2 x 4 bits + some more
    uint8      m_nAxis; // X, Y, Z - Right/left, front/back, up/down
    eDoorState m_nDoorState;
    float      m_fAngle;
    float      m_fPrevAngle;
    float      m_fAngVel;    // Angular velocity

};

VALIDATE_SIZE(CDoor, 0x18);
