#include "RW/rwcore.h"
#include "sprite2d.h"

class CustomRadar {
public:
    static inline CSprite2d* pTex;

public:
    static void Init();
    static void Render();
};
