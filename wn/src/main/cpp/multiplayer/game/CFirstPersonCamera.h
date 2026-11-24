#pragma once
#include "font.h"
#include "Cam.h"
#include <string>

class CFirstPersonCamera
{
public:
    static inline bool m_bEnabled;

    static inline float m_fVirtualCameraYaw;

    static inline CVector m_vecOffsetOnFoot{ 0.12f, 0.0f, 0.06f };
    static inline CVector m_vecOffsetDucked{ 0.18f, 0.0f, 0.02f };

    static inline float m_fNearOnFoot   = 0.2f;
    static inline float m_fNearInVeh    = 0.05f;
    static inline float m_fNearOnBike   = 0.3f;

    static inline float m_fFov = 0.0f;

    static inline bool m_bRotatePedWithCamera = true;

public:
    static void InjectHooks();

    static void Process_1stPerson(CCam* pCam, const CVector& vecTarget,
                                  float fTargetOrient, float fSpeedVar, float fUnused);

    static bool IsSuspended();

    static void ProcessCameraOnFoot(CCam* pCam, CPlayerPed* pPed);
    static void ProcessCameraInVeh(CCam* pCam, CPlayerPed* pPed, CVehicleGTA* pVeh);

    static RwMatrix* GetPedBoneMatrix(CPedGTA* pPed, int32 boneId);

    static void ApplyFrontFromAngles(CCam* pCam);

private:
    static void AlignPedToCamera(CCam* pCam, CPlayerPed* pPed);

    static bool IsLookBehindPressed();

    static CVector GetVehicleOffset(CVehicleGTA* pVeh, float& fNearPlane);
    static void    KeepYawThroughTransition(CCam* pCam, bool bInVehicle);
    static void    ApplyCommon(CCam* pCam, float fNearPlane);
};
