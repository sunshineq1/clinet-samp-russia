#include "main.h"
#include "game.h"
#include "CFirstPersonCamera.h"
#include "Entity/CPedGTA.h"
#include "Enums/ePedBones.h"
#include "Camera.h"
#include "Scene.h"
#include "Draw.h"
#include "../vendor/armhook/patch.h"

#include "net/netgame.h"
extern CNetGame* pNetGame;

static bool IsFinite(const CVector& v)
{
    return v.x == v.x && v.y == v.y && v.z == v.z;
}

RwMatrix* CFirstPersonCamera::GetPedBoneMatrix(CPedGTA* pPed, int32 boneId)
{
    if (!pPed || !pPed->m_pRwClump) {
        return nullptr;
    }

    pPed->UpdateRpHAnim();

    RpHAnimHierarchy* pHier = GetAnimHierarchyFromSkinClump(pPed->m_pRwClump);
    if (!pHier || !pHier->pMatrixArray) {
        return nullptr;
    }

    const int32 index = RpHAnimIDGetIndex(pHier, boneId);
    if (index < 0) {
        return nullptr;
    }

    return &pHier->pMatrixArray[index];
}

static void (*CCam__Process_1stPerson)(CCam*, const CVector&, float, float, float);

void CFirstPersonCamera::Process_1stPerson(CCam* pCam, const CVector& vecTarget,
                                           float fTargetOrient, float fSpeedVar, float fUnused)
{
    const bool bWeaponMode =
            CHook::CallFunction<bool>("_ZN4CCam22GetWeaponFirstPersonOnEv", pCam);

    if (bWeaponMode || IsSuspended() || !pNetGame) {
        CCam__Process_1stPerson(pCam, vecTarget, fTargetOrient, fSpeedVar, fUnused);
        return;
    }

    auto pLocal = pNetGame->GetPlayerPool()->GetLocalPlayer();
    CPlayerPed* pPed = pLocal ? pLocal->GetPlayerPed() : nullptr;
    if (!pPed || !pPed->m_pPed) {
        CCam__Process_1stPerson(pCam, vecTarget, fTargetOrient, fSpeedVar, fUnused);
        return;
    }

    CCam__Process_1stPerson(pCam, vecTarget, fTargetOrient, fSpeedVar, fUnused);

    CVehicleGTA* pVeh = pPed->m_pPed->IsInVehicle() ? pPed->m_pPed->pVehicle : nullptr;

    if (pVeh) {
        ProcessCameraInVeh(pCam, pPed, pVeh);
    } else {
        ProcessCameraOnFoot(pCam, pPed);
    }

    CHook::CallFunction<void>("_ZN4CCam20GetVectorsReadyForRWEv", pCam);
}

void CFirstPersonCamera::InjectHooks()
{
    (void)CCam__Process_1stPerson;
}

bool CFirstPersonCamera::IsSuspended()
{
    if (!m_bEnabled) {
        return true;
    }

    return CCamera::Get().WhoIsInControlOfTheCamera != 0;
}

//
//   sincosf(Alpha,             &sinA, &cosA);      // CCam+0xAC
//   sincosf(m_fHorizontalAngle,&sinB, &cosB);      // CCam+0xBC
//   Front = (cosA*cosB, cosA*sinB, sinA);
//   Front.Normalise();
//
void CFirstPersonCamera::ApplyFrontFromAngles(CCam* pCam)
{
    float fSinA, fCosA, fSinB, fCosB;
    sincosf(pCam->Alpha, &fSinA, &fCosA);
    sincosf(pCam->m_fHorizontalAngle, &fSinB, &fCosB);

    pCam->Front.x = fCosA * fCosB;
    pCam->Front.y = fCosA * fSinB;
    pCam->Front.z = fSinA;

    pCam->Front.Normalise();
}

void CFirstPersonCamera::AlignPedToCamera(CCam* pCam, CPlayerPed* pPed)
{
    if (!m_bRotatePedWithCamera) {
        return;
    }

    CPedGTA* pGtaPed = pPed->m_pPed;
    if (!pGtaPed) {
        return;
    }

    const float fYaw = atan2f(pCam->Front.y, pCam->Front.x);
    CHook::CallFunction<void>("_ZN4CPed11SetLookFlagEfbb", pGtaPed, fYaw, true, false);
    CHook::CallFunction<void>("_ZN4CPed12SetLookTimerEj", pGtaPed, (uint32)200);

    const float fHeading = fYaw - (M_PI_F / 2.0f);

    pGtaPed->m_fCurrentRotation = fHeading;
    pGtaPed->m_fAimingRotation  = fHeading;

    pGtaPed->SetHeading(fHeading);
    pGtaPed->UpdateRW();
    pGtaPed->UpdateRwFrame();
}

bool CFirstPersonCamera::IsLookBehindPressed()
{
    void* pPad = CHook::CallFunction<void*>("_ZN4CPad6GetPadEi", 0);
    if (!pPad) {
        return false;
    }

    return CHook::CallFunction<bool>("_ZN4CPad19GetLookBehindForCarEv", pPad);
}

void CFirstPersonCamera::ApplyCommon(CCam* pCam, float fNearPlane)
{
    RwCameraSetNearClipPlane(Scene.m_pRwCamera, fNearPlane);

    if (m_fFov > 0.0f) {
        pCam->FOV = m_fFov;
        pCam->FOVSpeed = 0.0f;
    }
}

void CFirstPersonCamera::KeepYawThroughTransition(CCam* pCam, bool bInVehicle)
{
    static bool  bWasInVehicle = false;
    static float fSavedYaw     = 0.0f;
    static int   nRestoreFrames = 0;

    const float fCurrentYaw = atan2f(pCam->Front.y, pCam->Front.x);

    if (bInVehicle != bWasInVehicle) {
        bWasInVehicle  = bInVehicle;
        nRestoreFrames = 4;
    } else if (nRestoreFrames <= 0) {
        fSavedYaw = fCurrentYaw;
        return;
    }

    nRestoreFrames--;

    const float fLenXY = sqrtf(pCam->Front.x * pCam->Front.x + pCam->Front.y * pCam->Front.y);
    if (fLenXY < 0.001f) {
        return;
    }

    pCam->Front.x = cosf(fSavedYaw) * fLenXY;
    pCam->Front.y = sinf(fSavedYaw) * fLenXY;
}

void CFirstPersonCamera::ProcessCameraOnFoot(CCam* pCam, CPlayerPed* pPed)
{
    if (IsSuspended()) {
        return;
    }

    CVector vecOffset = pPed->m_pPed->bIsDucking
                        ? m_vecOffsetDucked
                        : m_vecOffsetOnFoot;

    CVector vecOut;
    RwMatrix* pBone = GetPedBoneMatrix(pPed->m_pPed, BONE_HEAD);


    if (pBone) {
        vecOut.x = pBone->pos.x;
        vecOut.y = pBone->pos.y;
        vecOut.z = pBone->pos.z;

        float fFrontX = pCam->Front.x;
        float fFrontY = pCam->Front.y;
        const float fFlat = sqrtf(fFrontX * fFrontX + fFrontY * fFrontY);
        if (fFlat > 0.001f) {
            fFrontX /= fFlat;
            fFrontY /= fFlat;
        }

        vecOut.x += fFrontX * vecOffset.x - fFrontY * vecOffset.y;
        vecOut.y += fFrontY * vecOffset.x + fFrontX * vecOffset.y;
        vecOut.z += vecOffset.z;
    } else {
        ProjectMatrix(&vecOut, (CMatrix*)&pPed->m_HeadBoneMatrix, &vecOffset);
    }

    if (!IsFinite(vecOut)) {
        pPed->GetBonePosition(BONE_HEAD, &vecOut);
    }
    if (!IsFinite(vecOut)) {
        return;
    }

    KeepYawThroughTransition(pCam, false);

    pCam->Source = vecOut;

    AlignPedToCamera(pCam, pPed);

    ApplyCommon(pCam, m_fNearOnFoot);
}

CVector CFirstPersonCamera::GetVehicleOffset(CVehicleGTA* pVeh, float& fNearPlane)
{
    fNearPlane = m_fNearInVeh;

    if (!pVeh) {
        return { 0.0f, 0.0f, 0.6f };
    }

    if (pVeh->IsBike() || pVeh->IsBMX() || pVeh->IsQuad()) {
        fNearPlane = m_fNearOnBike;
        return { 0.05f, 0.3f, 0.45f };
    }

    if (pVeh->IsBoat()) {
        return { 0.0f, 0.1f, 0.75f };
    }

    if (pVeh->IsHeli() || pVeh->IsPlane()) {
        return { 0.0f, 0.2f, 0.5f };
    }

    return { 0.0f, 0.0f, 0.6f };
}

void CFirstPersonCamera::ProcessCameraInVeh(CCam* pCam, CPlayerPed* pPed, CVehicleGTA* pVeh)
{
    if (IsSuspended()) {
        return;
    }

    CVehicleGTA* pGtaVeh = pPed->GetGtaVehicle();
    if (!pGtaVeh) {
        return;
    }

    float fNearPlane = m_fNearInVeh;
    CVector vecOffset = GetVehicleOffset(pGtaVeh, fNearPlane);

    CVector vecOut;
    RwMatrix mat;

    memcpy(&mat, pPed->m_pPed->m_matrix, sizeof(RwMatrix));

    ProjectMatrix(&vecOut, (CMatrix*)&mat, &vecOffset);

    if (!IsFinite(vecOut)) {
        pPed->GetBonePosition(BONE_HEAD, &vecOut);
    }
    if (!IsFinite(vecOut)) {
        return;
    }

    KeepYawThroughTransition(pCam, true);

    pCam->Source = vecOut;

    if (pGtaVeh->IsBike() || pGtaVeh->IsBMX()) {
        pCam->Up = CVector(0.0f, 0.0f, 1.0f);
        pCam->m_fTilt = 0.0f;
        pCam->m_fTiltSpeed = 0.0f;
    }

    if (IsLookBehindPressed()) {
        pCam->Front.x = -pCam->Front.x;
        pCam->Front.y = -pCam->Front.y;
        CHook::CallFunction<void>("_ZN4CCam20GetVectorsReadyForRWEv", pCam);
    }

    ApplyCommon(pCam, fNearPlane);

    if (!pVeh) {
        if (!pPed->m_pPed->IsAPassenger()) {
            pCam->m_nMode = MODE_1STPERSON;
        }
        return;
    }
}
