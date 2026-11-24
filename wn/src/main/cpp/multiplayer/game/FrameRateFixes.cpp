//
// Created by weikton on 05.08.2026.
//

#include "FrameRateFixes.h"
#include "Timer.h"
#include "Entity/CPhysical.h"
#include "Entity/CVehicleGTA.h"
#include "../vendor/armhook/patch.h"

static constexpr float kBaseTimeStep = 50.0f / 30.0f;

static void (*CPhysical__ApplyAirResistance)(CPhysical *self);
static void CPhysical__ApplyAirResistance_hook(CPhysical *self)
{
    // fuck u
}

static void (*CWaterCannons__Update)();
static void CWaterCannons__Update_hook()
{
    // fuck u
}

static void (*CProjectileInfo__Update)();
static void CProjectileInfo__Update_hook()
{
    // fuck u
}

static void (*CPlane__PreRender)(void *self);
static void CPlane__PreRender_hook(void *self)
{
    // fuck u
}

static void (*CWeather__AddRain)();
static void CWeather__AddRain_hook()
{
    // fuck u
}

static void (*CVehicle__AddExhaustParticles)(void *self);
static void CVehicle__AddExhaustParticles_hook(void *self)
{
    // fuck u
}

static uint32 (*CVehicle__AddWheelDirtAndWater)(void *self, void *colPoint, uint32 produceDrops, uint8 spinning, uint8 inWater);
static uint32 CVehicle__AddWheelDirtAndWater_hook(void *self, void *colPoint, uint32 produceDrops, uint8 spinning, uint8 inWater)
{
    // fuck u
}

static float (*CVehicle__ProcessWheelRotation)(void *self, int32 wheelState, const CVector *forward, const CVector *wheelSpeed, float radius);
static float CVehicle__ProcessWheelRotation_hook(void *self, int32 wheelState, const CVector *forward, const CVector *wheelSpeed, float radius)
{
    // fuck u
}

static float *g_pHandlingTimeStep = nullptr;

static void (*CVehicle__ProcessWheel)(void *self, CVector &wheelFwd, CVector &wheelRight, CVector &contactSpeed, CVector &contactPoint,
                                      int32 wheelsOnGround, float thrust, float brake, float adhesion, int8 wheelId,
                                      float *wheelSpeed, void *wheelState, uint16 wheelStatus);
static void CVehicle__ProcessWheel_hook(void *self, CVector &wheelFwd, CVector &wheelRight, CVector &contactSpeed, CVector &contactPoint,
                                        int32 wheelsOnGround, float thrust, float brake, float adhesion, int8 wheelId,
                                        float *wheelSpeed, void *wheelState, uint16 wheelStatus)
{
        // fuck u
}

static void (*CVehicle__ProcessBikeWheel)(void *self, CVector &wheelFwd, CVector &wheelRight, CVector &contactSpeed, CVector &contactPoint,
                                          int32 wheelsOnGround, float thrust, float brake, float adhesion, float sideAdhesion,
                                          int8 wheelId, float *wheelSpeed, void *wheelState, int wheelSpecial, uint16 wheelStatus);
static void CVehicle__ProcessBikeWheel_hook(void *self, CVector &wheelFwd, CVector &wheelRight, CVector &contactSpeed, CVector &contactPoint,
                                            int32 wheelsOnGround, float thrust, float brake, float adhesion, float sideAdhesion,
                                            int8 wheelId, float *wheelSpeed, void *wheelState, int wheelSpecial, uint16 wheelStatus)
{
    // fuck u
}

static bool (*CTaskSimpleUseGun__ControlGunMove)(void *self, CVector2D *moveVec);
static bool CTaskSimpleUseGun__ControlGunMove_hook(void *self, CVector2D *moveVec)
{
    // fuck u
}

static bool (*ClimbProcessPed)(void *self, void *ped);
static bool ClimbProcessPed_hook(void *self, void *ped)
{
    // fuck u
}

void CFrameRateFixes::InjectHooks()
{
    // fuck u
}
