//
// Created by weikton on 02.02.25.
//

#include "../main.h"

class CBouncingPanel {
public:
    uint16 m_nFrameId{(uint16)-1};
    uint16 m_nAxis{};
    float    m_fAngleLimit{};
    CVector  m_vecRotation{};
    CVector  m_vecPos{};
};

VALIDATE_SIZE(CBouncingPanel, 0x20);