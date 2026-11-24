#include "CObjectPoolDraw.h"
#include "../main.h"
#include "../game/Pools.h"
#include "../game/Streaming.h"
#include "../gui/gui.h"

extern UI* pUI;
extern CGame* pGame;

CObjectPoolDraw::CObjectPoolDraw() { }

void CObjectPoolDraw::Render(void* renderer)
{
    for (auto link = CStreaming::ms_rwObjectInstances.usedListHead.next;
         link != &CStreaming::ms_rwObjectInstances.usedListTail;
         link = link->next)
    {
        CEntityGTA* entity = link->data;
        if (!entity) continue;

        auto pos = entity->GetPosition();

        CVector vecOut;
        // CSprite::CalcScreenCoors
        ((void (*)(CVector*, CVector*, float*, float*, bool, bool))(g_libGTASA + (VER_x32 ? 0x005C57E8 + 1 : 0x6E9DF8)))(&pos, &vecOut, 0, 0, 0, 0);

        if (vecOut.z < 1.0f) return;

        ImVec2 cPos = ImVec2(vecOut.x, vecOut.y);

        ((ImGuiRenderer*)renderer)->drawText(cPos, -1, std::to_string(entity->GetModelId()).c_str(), nullptr, true, UISettings::fontSize() / 2.3);
    }
}