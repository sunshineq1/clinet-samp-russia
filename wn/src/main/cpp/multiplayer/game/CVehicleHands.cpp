#include "CVehicleHands.h"

#include <cmath>

#include "playerped.h"
#include "CFirstPersonCamera.h"
#include "Entity/CPedGTA.h"
#include "Entity/CVehicleGTA.h"
#include "Enums/ePedBones.h"
#include "Models/ClumpModelInfo.h"
#include "Plugins/NodeNamePlugin/NodeName.h"
#include "../vendor/armhook/patch.h"

namespace {

struct Mat3 {
    CVector r0, r1, r2;

    static Mat3 Identity()
    {
        return { CVector(1.0f, 0.0f, 0.0f), CVector(0.0f, 1.0f, 0.0f), CVector(0.0f, 0.0f, 1.0f) };
    }

    CVector Apply(const CVector& v) const
    {
        return CVector(
            r0.x * v.x + r0.y * v.y + r0.z * v.z,
            r1.x * v.x + r1.y * v.y + r1.z * v.z,
            r2.x * v.x + r2.y * v.y + r2.z * v.z);
    }
};

float Length(const CVector& v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

float Dot(const CVector& a, const CVector& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

CVector Cross(const CVector& a, const CVector& b)
{
    return CVector(a.y * b.z - a.z * b.y,
                   a.z * b.x - a.x * b.z,
                   a.x * b.y - a.y * b.x);
}

bool Normalise(CVector& v)
{
    const float len = Length(v);
    if (len < 0.0001f) {
        return false;
    }

    v.x /= len;
    v.y /= len;
    v.z /= len;
    return true;
}

Mat3 RotationBetween(const CVector& from, const CVector& to)
{
    CVector axis = Cross(from, to);
    const float sine = Length(axis);
    const float cosine = Dot(from, to);

    if (sine < 0.0001f) {
        return Mat3::Identity();
    }

    axis.x /= sine;
    axis.y /= sine;
    axis.z /= sine;

    const float angle = atan2f(sine, cosine);
    const float c = cosf(angle);
    const float s = sinf(angle);
    const float t = 1.0f - c;

    Mat3 m;
    m.r0 = CVector(t * axis.x * axis.x + c,          t * axis.x * axis.y - s * axis.z, t * axis.x * axis.z + s * axis.y);
    m.r1 = CVector(t * axis.x * axis.y + s * axis.z, t * axis.y * axis.y + c,          t * axis.y * axis.z - s * axis.x);
    m.r2 = CVector(t * axis.x * axis.z - s * axis.y, t * axis.y * axis.z + s * axis.x, t * axis.z * axis.z + c);
    return m;
}

CVector ToVector(const RwV3d& v)
{
    return CVector(v.x, v.y, v.z);
}

void RotateBasis(RwMatrix* matrix, const Mat3& rotation)
{
    const CVector right = rotation.Apply(ToVector(matrix->right));
    const CVector up    = rotation.Apply(ToVector(matrix->up));
    const CVector at    = rotation.Apply(ToVector(matrix->at));

    matrix->right = RwV3d{ right.x, right.y, right.z };
    matrix->up    = RwV3d{ up.x, up.y, up.z };
    matrix->at    = RwV3d{ at.x, at.y, at.z };
}

void SetPosition(RwMatrix* matrix, const CVector& pos)
{
    matrix->pos = RwV3d{ pos.x, pos.y, pos.z };
}

void MoveChildBone(RwMatrix* bone, const CVector& parentOld, const CVector& parentNew, const Mat3& rotation)
{
    if (!bone) {
        return;
    }

    const CVector local = ToVector(bone->pos) - parentOld;
    RotateBasis(bone, rotation);
    SetPosition(bone, parentNew + rotation.Apply(local));
}

bool SolveArm(CPedGTA* pPed, bool bLeft, const CVector& target)
{
    const int32 upperId = bLeft ? BONE_L_UPPER_ARM : BONE_R_UPPER_ARM;
    const int32 foreId  = bLeft ? BONE_L_FORE_ARM  : BONE_R_FORE_ARM;
    const int32 handId  = bLeft ? BONE_L_HAND      : BONE_R_HAND;

    RwMatrix* upper = CFirstPersonCamera::GetPedBoneMatrix(pPed, upperId);
    RwMatrix* fore  = CFirstPersonCamera::GetPedBoneMatrix(pPed, foreId);
    RwMatrix* hand  = CFirstPersonCamera::GetPedBoneMatrix(pPed, handId);

    if (!upper || !fore || !hand) {
        return false;
    }

    const CVector shoulder = ToVector(upper->pos);
    const CVector elbow    = ToVector(fore->pos);
    const CVector wrist    = ToVector(hand->pos);

    const float upperLength = Length(elbow - shoulder);
    const float foreLength  = Length(wrist - elbow);
    if (upperLength < 0.0001f || foreLength < 0.0001f) {
        return false;
    }

    CVector toTarget = target - shoulder;
    float reach = Length(toTarget);
    if (reach < 0.0001f) {
        return false;
    }

    const float minReach = fabsf(upperLength - foreLength) + 0.001f;
    const float maxReach = (upperLength + foreLength) * CVehicleHands::ms_fReachLimit;

    reach = std::clamp(reach, minReach, maxReach);

    CVector direction = toTarget;
    if (!Normalise(direction)) {
        return false;
    }

    const CVector reachTarget = shoulder + direction * reach;

    CVector currentBend = elbow - shoulder;
    CVector bend = currentBend - direction * Dot(currentBend, direction);
    if (!Normalise(bend)) {
        bend = Cross(direction, CVector(0.0f, 0.0f, 1.0f));
        if (!Normalise(bend)) {
            return false;
        }
    }

    const float along = (upperLength * upperLength - foreLength * foreLength + reach * reach) / (2.0f * reach);
    const float offset = sqrtf(std::max(upperLength * upperLength - along * along, 0.0f));

    const CVector newElbow = shoulder + direction * along + bend * offset;

    CVector oldUpperDir = elbow - shoulder;
    CVector newUpperDir = newElbow - shoulder;
    if (!Normalise(oldUpperDir) || !Normalise(newUpperDir)) {
        return false;
    }

    const Mat3 upperRotation = RotationBetween(oldUpperDir, newUpperDir);

    const CVector wristAfterUpper = shoulder + upperRotation.Apply(wrist - shoulder);

    CVector oldForeDir = wristAfterUpper - newElbow;
    CVector newForeDir = reachTarget - newElbow;
    if (!Normalise(oldForeDir) || !Normalise(newForeDir)) {
        return false;
    }

    const Mat3 foreRotation = RotationBetween(oldForeDir, newForeDir);

    RwMatrix* finger    = CFirstPersonCamera::GetPedBoneMatrix(pPed, bLeft ? BONE_L_FINGER : BONE_R_FINGER);
    RwMatrix* fingerTip = CFirstPersonCamera::GetPedBoneMatrix(pPed, bLeft ? BONE_L_FINGER_01 : BONE_R_FINGER_01);

    MoveChildBone(finger,    wrist, reachTarget, foreRotation);
    MoveChildBone(fingerTip, wrist, reachTarget, foreRotation);

    RotateBasis(hand, foreRotation);
    SetPosition(hand, reachTarget);

    RotateBasis(fore, foreRotation);
    SetPosition(fore, newElbow);

    RotateBasis(upper, upperRotation);

    return true;
}

RwObject* WheelRadiusCB(RwObject* object, void* data)
{
    if (object && object->type == rpATOMIC) {
        auto radius = (float*)data;
        const float value = ((RpAtomic*)object)->boundingSphere.radius;

        if (value > *radius) {
            *radius = value;
        }
    }

    return object;
}

RwFrame* GetSteeringWheelFrame(RpClump* clump, float& radius)
{
    RwFrame* outer = CClumpModelInfo::GetFrameFromName(clump, "br_steering");
    if (!outer) {
        return nullptr;
    }

    RwFrame* wheel = nullptr;
    for (RwFrame* child = outer->child; child; child = child->next) {
        const RwChar* name = GetFrameNodeName(child);

        if (name && !strcmp(name, "br_steering")) {
            wheel = child;
            break;
        }
    }

    if (!wheel) {
        wheel = outer;
    }

    radius = 0.0f;
    RwFrameForAllObjects(wheel, WheelRadiusCB, &radius);

    if (radius < 0.05f) {
        radius = CVehicleHands::ms_fDefaultWheelRadius;
    }

    return wheel;
}

bool GetWheelHandTarget(CVehicleGTA* pVehicle, bool bLeft, CVector& out)
{
    if (!pVehicle) {
        return false;
    }

    RpClump* clump = (RpClump*)pVehicle->m_pRwObject;
    if (!clump || clump->object.type != rpCLUMP) {
        return false;
    }

    RwFrame* frame = CClumpModelInfo::GetFrameFromName(clump, bLeft ? "LHandPos" : "RHandPos");
    if (frame) {
        RwMatrix* ltm = RwFrameGetLTM(frame);

        if (ltm) {
            out = ToVector(ltm->pos);
            return true;
        }
    }

    float radius = 0.0f;
    RwFrame* wheel = GetSteeringWheelFrame(clump, radius);
    if (!wheel) {
        return false;
    }

    RwMatrix* ltm = RwFrameGetLTM(wheel);
    if (!ltm) {
        return false;
    }

    const CVector centre = ToVector(ltm->pos);
    const CVector across = ToVector(ltm->right);
    const float grip = radius * CVehicleHands::ms_fGripRadiusScale;

    out = bLeft ? (centre - across * grip) : (centre + across * grip);
    return true;
}

}

namespace
{

bool MirrorAcrossDriver(CPedGTA* pPed, CVehicleGTA* pVehicle, const CVector& known, CVector& out)
{
    RwMatrix* leftArm  = CFirstPersonCamera::GetPedBoneMatrix(pPed, BONE_L_UPPER_ARM);
    RwMatrix* rightArm = CFirstPersonCamera::GetPedBoneMatrix(pPed, BONE_R_UPPER_ARM);

    if (!leftArm || !rightArm) {
        return false;
    }

    const CVector chest = (ToVector(leftArm->pos) + ToVector(rightArm->pos)) * 0.5f;

    CMatrix& mat = pVehicle->GetMatrix();
    CVector across = mat.GetRight();
    if (!Normalise(across)) {
        return false;
    }

    out = known - across * (2.0f * Dot(known - chest, across));
    return true;
}

}

void CVehicleHands::Apply(CPedGTA* pPed)
{
    if (!ms_bEnabled || !pPed || !pPed->m_pRwClump) {
        return;
    }

    CVehicleGTA* pVehicle = pPed->pVehicle;
    if (!pVehicle || pVehicle->pDriver != pPed) {
        return;
    }

    CVector rightTarget;
    CVector leftTarget;

    bool bHasRight = GetWheelHandTarget(pVehicle, false, rightTarget);
    bool bHasLeft  = GetWheelHandTarget(pVehicle, true, leftTarget);

    if (bHasRight && !bHasLeft) {
        bHasLeft = MirrorAcrossDriver(pPed, pVehicle, rightTarget, leftTarget);
    } else if (bHasLeft && !bHasRight) {
        bHasRight = MirrorAcrossDriver(pPed, pVehicle, leftTarget, rightTarget);
    }

    if (bHasRight) {
        SolveArm(pPed, false, rightTarget);
    }

    if (bHasLeft) {
        SolveArm(pPed, true, leftTarget);
    }
}

static void (*CPed__Render)(CPedGTA* pPed);

static void CPed__Render_hook(CPedGTA* pPed)
{
    CVehicleHands::Apply(pPed);
    CPed__Render(pPed);
}

void CVehicleHands::InjectHooks()
{
    CHook::InlineHook("_ZN4CPed6RenderEv", &CPed__Render_hook, &CPed__Render);
}
