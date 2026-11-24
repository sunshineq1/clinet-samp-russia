/*
    Plugin-SDK (Grand Theft Auto) source file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "RenderWare.h"
#include "common.h"
#include "RenderWare.h"
#include "Radar.h"
#include "font.h"
#include "World.h"
#include "NodeAddress.h"
#include <cmath>

#define MAX_NODE_POINTS 100000
#define GPS_LINE_WIDTH  4.0f

class GPS {
public:
    static inline bool          enabled {};
    static inline CVector       to {};

    static inline CNodeAddress  resultNodes[MAX_NODE_POINTS];
    static inline CVector2D     nodePoints[MAX_NODE_POINTS];
    static inline RwIm2DVertex  lineVerts[MAX_NODE_POINTS * 4];

    static void Setup2DVertex(RwIm2DVertex &vertex, float x, float y, const RwRGBA& color);
    static void DoPathDraw();
    static void Set(const CVector* to, bool fromServer = false);
};