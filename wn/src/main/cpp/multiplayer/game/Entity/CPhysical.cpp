//
// Created by roman on 11/24/2024.
//

#include "CPhysical.h"
#include "game/Timer.h"
#include "patch.h"
#include "../main.h"
#include "../game/game.h"
#include "../net/netgame.h"

CPhysical::CPhysical() : CEntityGTA()
{
    m_pCollisionList.m_pNode = nullptr;

    CPlaceable::AllocateStaticMatrix();
    m_matrix->SetUnity();

    ResetMoveSpeed();
    ResetTurnSpeed();
    m_vecFrictionMoveSpeed.Set(0.0f, 0.0f, 0.0f);
    m_vecFrictionTurnSpeed.Set(0.0f, 0.0f, 0.0f);
    m_vecForce.Set(0.0f, 0.0f, 0.0f);
    m_vecTorque.Set(0.0f, 0.0f, 0.0f);

    m_fMass = 1.0f;
    m_fTurnMass = 1.0f;
    m_fVelocityFrequency = 1.0f;
    m_fAirResistance = 0.1f;
    m_pMovingList = 0;
    m_nFakePhysics = 0;
    m_nNumEntitiesCollided = 0;
    std::fill(std::begin(m_apCollidedEntities), std::end(m_apCollidedEntities), nullptr);

    m_nPieceType = 0;

    m_fDamageIntensity = 0.0f;
    m_pDamageEntity = nullptr;

    m_vecLastCollisionImpactVelocity.Set(0.0f, 0.0f, 0.0f);
    m_vecLastCollisionPosn.Set(0.0f, 0.0f, 0.0f);

    m_bUsesCollision = true;

    m_vecCentreOfMass.Set(0.0f, 0.0f, 0.0f);

    m_fMovingSpeed = 0.0f;
    m_pAttachedTo = nullptr;
    m_pEntityIgnoredCollision = nullptr;

    m_qAttachedEntityRotation = CQuaternion(0.0f, 0.0f, 0.0f, 0.0f);

    m_fDynamicLighting = 0.0f;
    m_pShadowData = 0;
    m_fPrevDistFromCam = 100.0f;

    m_nPhysicalFlags = 0;
    physicalFlags.bApplyGravity = true;

    m_nContactSurface = SURFACE_DEFAULT; // NOTSA; If not initialize, it will go out of bounds in the SurfaceInfos_c
    m_fContactSurfaceBrightness = 1.0f;
}

CPhysical::~CPhysical() {
//    if (m_pShadowData)
//        g_realTimeShadowMan.ReturnRealTimeShadow(m_pShadowData);

    //m_pCollisionList.Flush();
}

bool CPhysical::IsAdded()
{
    if(this)
    {
        if(*(uintptr*)this == g_libGTASA + (VER_x32 ? 0x00667D14 : 0x830098)) // CPlaceable
            return false;

        if(*(uintptr_t*)m_pMovingList)
            return true;
    }

    return false;
}

// 0x5429F0
void CPhysical::ApplyMoveForce(CVector force)
{
    if (!physicalFlags.bInfiniteMass && !physicalFlags.bDisableMoveForce) {
        if (physicalFlags.bDisableZ)
            force.z = 0.0f;
        m_vecMoveSpeed += force / m_fMass;
    }
}

void CPhysical::ApplyTurnForce(CVector force, CVector point)
{
    if (!physicalFlags.bDisableTurnForce)
    {
        CVector vecCentreOfMassMultiplied{};
        if (!physicalFlags.bInfiniteMass)
            vecCentreOfMassMultiplied = Multiply3x3(GetMatrix(), m_vecCentreOfMass);

        if (physicalFlags.bDisableMoveForce) {
            point.z = 0.0f;
            force.z = 0.0f;
        }
        CVector vecDifference = point - vecCentreOfMassMultiplied;
        m_vecTurnSpeed += CrossProduct(vecDifference, force) / m_fTurnMass;
    }
}

void CPhysical::ApplyMoveSpeed()
{
    if (physicalFlags.bDontApplySpeed || physicalFlags.bDisableMoveForce)
        ResetMoveSpeed();
    else
        GetPosition() += CTimer::GetTimeStep() * m_vecMoveSpeed;
}

CVector CPhysical::GetSpeed(CVector point)
{
#ifdef USE_DEFAULT_FUNCTIONS
    CVector outSpeed;
    ((CVector *(__thiscall*)(CPhysical*, CVector*, CVector))0x542CE0)(this, outSpeed, point);
    return outSpeed;
#else
    CVector vecCentreOfMassMultiplied;
    if (!physicalFlags.bInfiniteMass)
        vecCentreOfMassMultiplied = Multiply3x3(GetMatrix(), m_vecCentreOfMass);

    CVector distance = point - vecCentreOfMassMultiplied;
    CVector vecTurnSpeed = m_vecTurnSpeed + m_vecFrictionTurnSpeed;
    CVector speed = CrossProduct(vecTurnSpeed, distance);
    speed += m_vecMoveSpeed + m_vecFrictionMoveSpeed;
    return speed;
#endif
}

void CPhysical::ApplyMoveForce(float x, float y, float z)
{
    return ApplyMoveForce(CVector(x, y ,z));
}

void CPhysical::ApplyTurnSpeed()
{
    if (physicalFlags.bDontApplySpeed) {
        ResetTurnSpeed();
    }
    else
    {
        CVector vecTurnSpeedTimeStep = CTimer::GetTimeStep() * m_vecTurnSpeed;
        CVector vecCrossProduct;
        CrossProduct(&vecCrossProduct, &vecTurnSpeedTimeStep, &GetRight());
        GetRight() += vecCrossProduct;
        CrossProduct(&vecCrossProduct, &vecTurnSpeedTimeStep, &GetForward());
        GetForward() += vecCrossProduct;
        CrossProduct(&vecCrossProduct, &vecTurnSpeedTimeStep, &GetUp());
        GetUp() += vecCrossProduct;
        if (!physicalFlags.bInfiniteMass && !physicalFlags.bDisableMoveForce) {
            CVector vecNegativeCentreOfMass = m_vecCentreOfMass * -1.0f;
            CVector vecCentreOfMassMultiplied = Multiply3x3(GetMatrix(), vecNegativeCentreOfMass);
            GetPosition() += CrossProduct(vecTurnSpeedTimeStep, vecCentreOfMassMultiplied);
        }
    }
}

void CPhysical::Add() {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x3FCE3C + 1 : 0x4E0608), this);
}

void CPhysical::Remove() {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x3FD02C + 1 : 0x4E07EC), this);
}


extern CNetGame* pNetGame;
extern CGame* pGame;
void (*CPhysical__Add)(CPhysical* _thiz);
void CPhysical__Add_Hook(CPhysical* _thiz)
{
    if (pNetGame)
    {
        CPlayerPed* pPlayerPed = pGame->FindPlayerPed();
        if (pPlayerPed)
        {
            for (auto iter : pPlayerPed->m_aAttachedObject)
            {
                auto attach = iter.second;

                CObject* pObject = attach.pObject;
                if (pObject && pObject->m_pEntity && pObject->m_pEntity == _thiz && pObject->m_pEntity->m_matrix)
                {
                    if (pObject->m_pEntity->GetMatrix().m_pos.x > 20000.0f || pObject->m_pEntity->GetMatrix().m_pos.y > 20000.0f || pObject->m_pEntity->GetMatrix().m_pos.z > 20000.0f ||
                            pObject->m_pEntity->GetMatrix().m_pos.x < -20000.0f || pObject->m_pEntity->GetMatrix().m_pos.y < -20000.0f || pObject->m_pEntity->GetMatrix().m_pos.z < -20000.0f)
                    {
                        return;
                    }
                }
            }
        }

        CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();

        if (pPlayerPool)
        {
            for (size_t i = 0; i < MAX_PLAYERS; i++)
            {
                if (pPlayerPool->GetSlotState(i))
                {
                    CRemotePlayer* pRemotePlayer = pPlayerPool->GetAt(i);
                    if (pRemotePlayer)
                    {
                        if (pRemotePlayer->GetPlayerPed() && pRemotePlayer->GetPlayerPed()->m_pPed && pRemotePlayer->GetPlayerPed()->m_pPed->IsAdded())
                        {
                            pPlayerPed = pRemotePlayer->GetPlayerPed();

                            // check attached object valid position
                            for (auto iter : pPlayerPed->m_aAttachedObject)
                            {
                                auto attach = iter.second;

                                CObject* pObject = attach.pObject;
                                if (pObject && pObject->m_pEntity && pObject->m_pEntity == _thiz
                                    && pObject->m_pEntity->m_matrix)
                                {
                                    if (pObject->m_pEntity->GetMatrix().m_pos.x > 20000.0f || pObject->m_pEntity->GetMatrix().m_pos.y > 20000.0f || pObject->m_pEntity->GetMatrix().m_pos.z > 20000.0f ||
                                        pObject->m_pEntity->GetMatrix().m_pos.x < -20000.0f || pObject->m_pEntity->GetMatrix().m_pos.y < -20000.0f || pObject->m_pEntity->GetMatrix().m_pos.z < -20000.0f)
                                    {
                                        return;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    CPhysical__Add(_thiz);
}

void (*CPhysical__RemoveAndAdd)(CPhysical* _thiz);
void CPhysical__RemoveAndAdd_Hook(CPhysical* _thiz)
{
    if (pNetGame)
    {
        CPlayerPed* pPlayerPed = pGame->FindPlayerPed();
        if (pPlayerPed)
        {
            for (auto iter : pPlayerPed->m_aAttachedObject)
            {
                auto attach = iter.second;

                CObject* pObject = attach.pObject;
                if (pObject && pObject->m_pEntity && pObject->m_pEntity == _thiz && pObject->m_pEntity->m_matrix)
                {
                    if (pObject->m_pEntity->GetMatrix().m_pos.x > 20000.0f || pObject->m_pEntity->GetMatrix().m_pos.y > 20000.0f || pObject->m_pEntity->GetMatrix().m_pos.z > 20000.0f ||
                        pObject->m_pEntity->GetMatrix().m_pos.x < -20000.0f || pObject->m_pEntity->GetMatrix().m_pos.y < -20000.0f || pObject->m_pEntity->GetMatrix().m_pos.z < -20000.0f)
                    {
                        return;
                    }
                }
            }
        }

        CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();

        if (pPlayerPool)
        {
            for (size_t i = 0; i < MAX_PLAYERS; i++)
            {
                if (pPlayerPool->GetSlotState(i))
                {
                    CRemotePlayer* pRemotePlayer = pPlayerPool->GetAt(i);
                    if (pRemotePlayer)
                    {
                        if (pRemotePlayer->GetPlayerPed() && pRemotePlayer->GetPlayerPed()->m_pPed && pRemotePlayer->GetPlayerPed()->m_pPed->IsAdded())
                        {
                            pPlayerPed = pRemotePlayer->GetPlayerPed();

                            for (auto iter : pPlayerPed->m_aAttachedObject)
                            {
                                auto attach = iter.second;

                                CObject* pObject = attach.pObject;
                                if (pObject && pObject->m_pEntity && pObject->m_pEntity == _thiz && pObject->m_pEntity->m_matrix)
                                {
                                    if (pObject->m_pEntity->GetMatrix().m_pos.x > 20000.0f || pObject->m_pEntity->GetMatrix().m_pos.y > 20000.0f || pObject->m_pEntity->GetMatrix().m_pos.z > 20000.0f ||
                                        pObject->m_pEntity->GetMatrix().m_pos.x < -20000.0f || pObject->m_pEntity->GetMatrix().m_pos.y < -20000.0f || pObject->m_pEntity->GetMatrix().m_pos.z < -20000.0f)
                                    {
                                        return;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    CPhysical__RemoveAndAdd(_thiz);
}

void CPhysical::InjectHooks() {
    //CHook::InlineHook("_ZN9CPhysical3AddEv", &CPhysical__Add_Hook, &CPhysical__Add);
    //CHook::InlineHook("_ZN9CPhysical12RemoveAndAddEv", &CPhysical__RemoveAndAdd_Hook, &CPhysical__RemoveAndAdd);
}