/*
    Plugin-SDK (Grand Theft Auto) source file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/

#include "GPS.h"
#include "net/netgame.h"
#include "net/playerpool.h"
#include "Entity/CPedGTA.h"
#include "game.h"
#include "PathFind.h"
#include "Streaming.h"
#include "Widgets/TouchInterface.h"
#include "Lines.h"
#include "Mobile/MobileMenu/MobileMenu.h"

#include "../net/playerpool.h"
#include "../net/netgame.h"
extern CNetGame* pNetGame;

extern CGame* pGame;

inline bool IsRadarVisible()
{
    auto radar = CTouchInterface::m_pWidgets[WIDGET_RADAR];
    return (radar != NULL && radar->m_bEnabled);
}

inline bool IsGamePaused() { return CTimer::GetIsCodePaused() || CTimer::GetIsUserPaused(); };
short nodesCount = 0;

CRect emptyRect(0, 0, 0, 0), radarRect;

void GPS::Setup2DVertex(RwIm2DVertex &vertex, float x, float y, const RwRGBA& color) {
    vertex.x = x;
    vertex.y = y;
    vertex.u = vertex.v = 0.0f;
    vertex.z = CSprite2d::NearScreenZ + 0.0001f;
    vertex.rhw = CSprite2d::RecipNearClip;
    RwIm2DVertexSetIntRGBA(&vertex, color.red, color.green, color.blue, color.alpha);
}

void SetScissorRect(CRect* pRect)
{
    return ((void (*)(void*))(g_libGTASA + (VER_x32 ? 0x002B3EC4 + 1 : 0x373290)))(pRect);
}

auto lineColor = RwRGBA(128, 0, 128, 255);
void GPS::DoPathDraw()
{
    if (!enabled) return;

    static bool once = false;
    if(!once)
    {
        for(int i = 0; i < NUM_PATH_MAP_AREAS; i ++)
        {
            CStreaming::RequestModel(DATToModelId(i), STREAMING_KEEP_IN_MEMORY);
            CStreaming::LoadAllRequestedModels(false);
        }
        once = true;
    }

    float gpsDistance = 0.0f;
    CVector2D posn;

    auto inHorizontal = CVector2D(-1.0f, -1.0f);
    CRadar::TransformRadarPointToScreenSpace(&posn, &inHorizontal);
    radarRect.left = posn.x + 2.0f;
    radarRect.top = posn.y - 2.0f;

    auto inVertical = CVector2D(1.0f, 1.0f);
    CRadar::TransformRadarPointToScreenSpace(&posn, &inVertical);
    radarRect.right = posn.x - 2.0f;
    radarRect.bottom = posn.y + 2.0f;

    auto flMenuMapScaling = SCREEN_HEIGHT / 448.0f;
    auto playa = pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed();
    if (!playa) return;

    auto ped = playa->m_pPed;
    if (!ped) return;

    auto& gMobileMenu = CMobileMenu::Get();
    bool isGamePaused = IsGamePaused(), bScissors = !isGamePaused;

    CVector fromPos = ped->GetPosition();
    CPathFind &ThePaths = *(CPathFind *) (g_libGTASA + (VER_x32 ? 0x7A61F0 : 0x9869C0));

    ThePaths.DoPathSearch(
            PATH_TYPE_VEH,
            fromPos,
            CNodeAddress(),
            GPS::to,
            resultNodes,
            &nodesCount,
            MAX_NODE_POINTS,
            &gpsDistance,
            1'000'000'000.0f,
            nullptr,
            1'000'000'000.0f,
            false,
            CNodeAddress(),
            false,
            (ped->IsInVehicle() && ped->pVehicle->m_nVehicleSubType == VEHICLE_TYPE_BOAT)
    );

    // Log("GPS: nodesCount - %d", nodesCount);

    if(nodesCount > 0)
    {
        CPathNode* node;
        CVector2D nodePos;
        if (isGamePaused)
        {
            for (int i = 0; i < nodesCount; ++i)
            {
                node = &ThePaths.m_pPathNodes[resultNodes[i].m_wAreaId][resultNodes[i].m_wNodeId];
                nodePos = node->GetNodeCoors();

                CRadar::TransformRealWorldPointToRadarSpace(&nodePos, &nodePos);
                CRadar::LimitRadarPoint(&nodePos);
                CRadar::TransformRadarPointToScreenSpace(&nodePoints[i], &nodePos);
                nodePoints[i] *= flMenuMapScaling;
            }
        }
        else
        {
            for (int i = 0; i < nodesCount; ++i)
            {
                node = &ThePaths.m_pPathNodes[resultNodes[i].m_wAreaId][resultNodes[i].m_wNodeId];
                nodePos = node->GetNodeCoors();
                CRadar::TransformRealWorldPointToRadarSpace(&nodePos, &nodePos);
                CRadar::TransformRadarPointToScreenSpace(&nodePoints[i], &nodePos);
            }
        }

        if(IsRadarVisible() || isGamePaused)
        {
            if (bScissors)
                SetScissorRect(&radarRect);

            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);

            unsigned int vertIndex = 0;
            --nodesCount;

            CVector2D shift[2], dir;
            float angle;
            if (isGamePaused)
            {
                float mp = gMobileMenu.NEW_MAP_SCALE - 140.0f;
                if (mp < 140.0f) mp = 140.0f;
                else if (mp > 960.0f) mp = 960.0f;
                mp = mp / 960.0f + 0.4f;
                mp *= GPS_LINE_WIDTH;

                for (int i = 0; i < nodesCount; i++)
                {
                    dir = nodePoints[i + 1] - nodePoints[i];
                    angle = atan2(dir.y, dir.x);

                    sincosf(angle - 1.5707963f, &shift[0].y, &shift[0].x); shift[0] *= mp;
                    sincosf(angle + 1.5707963f, &shift[1].y, &shift[1].x); shift[1] *= mp;

                    Setup2DVertex(lineVerts[vertIndex], nodePoints[i].x + shift[0].x, nodePoints[i].y + shift[0].y, lineColor);
                    Setup2DVertex(lineVerts[++vertIndex], nodePoints[i + 1].x + shift[0].x, nodePoints[i + 1].y + shift[0].y, lineColor);
                    Setup2DVertex(lineVerts[++vertIndex], nodePoints[i].x + shift[1].x, nodePoints[i].y + shift[1].y, lineColor);
                    Setup2DVertex(lineVerts[++vertIndex], nodePoints[i + 1].x + shift[1].x, nodePoints[i + 1].y + shift[1].y, lineColor);
                    ++vertIndex;
                }
            }
            else
            {
                for (int i = 0; i < nodesCount; i++)
                {
                    dir = nodePoints[i + 1] - nodePoints[i];
                    angle = atan2(dir.y, dir.x);

                    sincosf(angle - 1.5707963f, &shift[0].y, &shift[0].x); shift[0] *= GPS_LINE_WIDTH;
                    sincosf(angle + 1.5707963f, &shift[1].y, &shift[1].x); shift[1] *= GPS_LINE_WIDTH;

                    Setup2DVertex(lineVerts[vertIndex], nodePoints[i].x + shift[0].x, nodePoints[i].y + shift[0].y, lineColor);
                    Setup2DVertex(lineVerts[++vertIndex], nodePoints[i + 1].x + shift[0].x, nodePoints[i + 1].y + shift[0].y, lineColor);
                    Setup2DVertex(lineVerts[++vertIndex], nodePoints[i].x + shift[1].x, nodePoints[i].y + shift[1].y, lineColor);
                    Setup2DVertex(lineVerts[++vertIndex], nodePoints[i + 1].x + shift[1].x, nodePoints[i + 1].y + shift[1].y, lineColor);
                    ++vertIndex;
                }
            }
            RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, lineVerts, 4 * nodesCount);

            if (bScissors)
                SetScissorRect(&emptyRect);
        }
    }

    // Log("RENDER GPS with X: %f Y:%f | isGamePaused: %b IsRadarVisible: %b", GPS::to.x, GPS::to.y, isGamePaused, IsRadarVisible());
}

void GPS::Set(const CVector *to, bool fromServer) {
    if (!to) {
        enabled = false;
        return;
    }

    if(fromServer)
        lineColor = RwRGBA(0, 255, 255, 255);
    else
        lineColor = RwRGBA(200, 0, 200, 255);

    GPS::to = *to;
    enabled = true;
}
