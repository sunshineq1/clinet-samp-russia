#include "CExhaust.h"

#include "game.h"
#include "Camera.h"
#include "Streaming.h"
#include "object.h"
#include "playerped.h"
#include "vehicle.h"
#include "tHandlingData.h"
#include "Models/ModelInfo.h"
#include "Models/VehicleModelInfo.h"
#include "Enums/ePedState.h"
#include "net/netgame.h"

extern CGame* pGame;
extern CNetGame* pNetGame;

void CExhaust::DestroyObject(CObject*& object)
{
    if (!object) {
        return;
    }

    delete object;
    object = nullptr;
}

void CExhaust::DestroyBurst(VehicleState& state)
{
    DestroyObject(state.activeBurst.primary);
    DestroyObject(state.activeBurst.secondary);
    state.burstSpawnTick = 0;
}

bool CExhaust::EnsureEffectModelLoaded()
{
    if (CStreaming::IsModelLoaded(kExhaustEffectModelId)) {
        return true;
    }

    return CStreaming::TryLoadModel(kExhaustEffectModelId);
}

CVehicle* CExhaust::GetLocalVehicle()
{
    if (!pGame || !pNetGame) {
        return nullptr;
    }

    CPlayerPed* player = pGame->FindPlayerPed();
    if (!player || !player->IsInVehicle() || player->IsAPassenger()) {
        return nullptr;
    }

    return player->GetCurrentVehicle();
}

uint16 CExhaust::FindVehicleId(CVehicle* vehicle)
{
    if (!vehicle || !vehicle->m_pVehicle || !pNetGame) {
        return INVALID_VEHICLE_ID;
    }

    CVehiclePool* pool = pNetGame->GetVehiclePool();
    if (!pool) {
        return INVALID_VEHICLE_ID;
    }

    return pool->FindIDFromGtaPtr(vehicle->m_pVehicle);
}

bool CExhaust::IsAutomobile(CVehicle* vehicle)
{
    if (!vehicle || !vehicle->m_pVehicle) {
        return false;
    }

    return vehicle->GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR;
}

bool CExhaust::IsVehicleVisible(CVehicle* vehicle)
{
    if (!vehicle || !vehicle->m_pVehicle) {
        return false;
    }

    const CVector vehiclePosition = vehicle->m_pVehicle->GetPosition();
    return CCamera::Get().IsSphereVisible(&vehiclePosition, 6.0f);
}

bool CExhaust::CanUseExhaust(CVehicle* vehicle)
{
    if (!vehicle || !vehicle->m_pVehicle) {
        return false;
    }

    if (!vehicle->VerifyInstance() || !IsAutomobile(vehicle) || !IsVehicleVisible(vehicle)) {
        return false;
    }

    if (!vehicle->m_pVehicle->m_nVehicleFlags.bEngineOn || vehicle->m_pVehicle->m_nVehicleFlags.bEngineBroken) {
        return false;
    }

    const tHandlingData* handling = vehicle->m_pVehicle->m_pHandlingData;
    if (handling && handling->m_bNoExhaust) {
        return false;
    }

    return true;
}

bool CExhaust::ResolveExhaustOffsets(CVehicle* vehicle, CVector& primary, bool& hasSecondary, CVector& secondary)
{
    hasSecondary = false;

    if (!vehicle || !vehicle->m_pVehicle) {
        return false;
    }

    auto* modelInfo = CModelInfo::GetVehicleModelInfo(vehicle->m_pVehicle->m_nModelIndex);
    if (!modelInfo || !modelInfo->m_pVehicleStruct) {
        return false;
    }

    primary = *modelInfo->GetModelDummyPosition(DUMMY_EXHAUST);
    secondary = *modelInfo->GetModelDummyPosition(DUMMY_EXHAUST_SECONDARY);

    if (primary.IsZero() && secondary.IsZero()) {
        return false;
    }

    if (primary.IsZero()) {
        primary = secondary;
        secondary.Reset();
        return true;
    }

    if (!secondary.IsZero()) {
        hasSecondary = true;
        return true;
    }

    const tHandlingData* handling = vehicle->m_pVehicle->m_pHandlingData;
    if (handling && handling->m_bDoubleExhaust) {
        secondary = primary;
        secondary.x = -secondary.x;
        hasSecondary = true;
    }

    primary.z += kExhaustVerticalOffset;
    if (hasSecondary) {
        secondary.z += kExhaustVerticalOffset;
    }

    return true;
}

void CExhaust::ApplyEffectScale(CObject* effect)
{
    if (!effect || ms_fEffectScale >= 0.999f) {
        return;
    }

    CPhysical* entity = effect->m_pEntity;
    if (!entity || !entity->m_matrix) {
        return;
    }

    CMatrix& matrix = entity->GetMatrix();
    matrix.GetRight()   *= ms_fEffectScale;
    matrix.GetForward() *= ms_fEffectScale;
    matrix.GetUp()      *= ms_fEffectScale;

    entity->UpdateRwFrame();
}

CObject* CExhaust::CreateAttachedBurst(CVehicle* vehicle, uint16 vehicleId, const CVector& offset)
{
    if (!vehicle || vehicleId == INVALID_VEHICLE_ID || !pGame) {
        return nullptr;
    }

    CObject* effect = pGame->NewObject(kExhaustEffectModelId, CVector(0.0f, 0.0f, 0.0f), CVector(0.0f, 0.0f, 0.0f), 200.0f);
    if (!effect || !effect->m_dwGTAId) {
        delete effect;
        return nullptr;
    }

    CVector attachOffset = offset;
    CVector rotation(0.0f, 0.0f, 180.0f);
    effect->SetAttachedVehicle(vehicleId, &attachOffset, &rotation);
    effect->AttachToVehicle(vehicle);

    ApplyEffectScale(effect);
    return effect;
}

CExhaust::VehicleState& CExhaust::GetOrResetState(uint16 vehicleId, CVehicle* vehicle, uint32 now)
{
    VehicleState& state = ms_states[vehicleId];
    const uintptr_t token = reinterpret_cast<uintptr_t>(vehicle ? vehicle->m_pVehicle : nullptr);

    if (state.vehicleToken != token) {
        DestroyBurst(state);
        state = {};
        state.vehicleToken = token;
        state.lastGear = vehicle && vehicle->m_pVehicle ? vehicle->m_pVehicle->m_nCurrentGear : kInvalidGear;
    }

    state.lastSeenTick = now;
    return state;
}

void CExhaust::ServiceStates(uint32 now)
{
    for (auto& [vehicleId, state] : ms_states) {
        ApplyEffectScale(state.activeBurst.primary);
        ApplyEffectScale(state.activeBurst.secondary);
    }

    if (ms_lastServiceTick && now - ms_lastServiceTick < 50) {
        return;
    }
    ms_lastServiceTick = now;

    for (auto it = ms_states.begin(); it != ms_states.end();) {
        VehicleState& state = it->second;

        if (state.activeBurst.HasAny() && state.burstSpawnTick && now - state.burstSpawnTick >= kBurstLifetimeMs) {
            DestroyBurst(state);
        }

        const bool isIdle = !state.activeBurst.HasAny() && state.lastSeenTick && now - state.lastSeenTick >= kIdleStateLifetimeMs;
        if (isIdle) {
            it = ms_states.erase(it);
            continue;
        }

        ++it;
    }
}

void CExhaust::SpawnBurst(CVehicle* vehicle, VehicleState& state, uint32 now)
{
    if (!CanUseExhaust(vehicle) || !EnsureEffectModelLoaded()) {
        return;
    }

    if (state.lastBurstTick && now - state.lastBurstTick < kBurstCooldownMs) {
        return;
    }

    CVector primary;
    CVector secondary;
    bool hasSecondary = false;
    if (!ResolveExhaustOffsets(vehicle, primary, hasSecondary, secondary)) {
        return;
    }

    const uint16 vehicleId = FindVehicleId(vehicle);
    if (vehicleId == INVALID_VEHICLE_ID) {
        return;
    }

    DestroyBurst(state);

    state.activeBurst.primary = CreateAttachedBurst(vehicle, vehicleId, primary);
    if (hasSecondary && primary != secondary) {
        state.activeBurst.secondary = CreateAttachedBurst(vehicle, vehicleId, secondary);
    }

    if (!state.activeBurst.HasAny()) {
        return;
    }

    state.lastBurstTick = now;
    state.burstSpawnTick = now;
}

void CExhaust::Inject()
{
    ms_states.clear();
    ms_lastServiceTick = 0;
    ms_lastExitVehicleRequestTick = 0;
}

void CExhaust::OnVehicleProcess(CVehicleGTA* vehicle)
{
    const uint32 now = GetTickCount();
    ServiceStates(now);

    CPlayerPed* player = pGame ? pGame->FindPlayerPed() : nullptr;
    const bool isPlayerExitingVehicle = player && player->m_pPed &&
        (player->IsExitingVehicle() || player->m_pPed->m_nPedState == PEDSTATE_EXIT_CAR);

    CVehicle* localVehicle = GetLocalVehicle();
    if (!localVehicle || !localVehicle->m_pVehicle || localVehicle->m_pVehicle != vehicle) {
        return;
    }

    const uint16 vehicleId = FindVehicleId(localVehicle);
    if (vehicleId == INVALID_VEHICLE_ID) {
        return;
    }

    VehicleState& state = GetOrResetState(vehicleId, localVehicle, now);
    if (isPlayerExitingVehicle) {
        state.brakeBurstCount = 0;
        state.wasBrakePressed = false;
        state.lastGear = localVehicle->m_pVehicle->m_nCurrentGear;
        return;
    }

    if (!CanUseExhaust(localVehicle)) {
        state.lastGear = localVehicle->m_pVehicle->m_nCurrentGear;
        return;
    }

    const uint8 currentGear = localVehicle->m_pVehicle->m_nCurrentGear;
    const bool gearChanged = state.lastGear != kInvalidGear && state.lastGear != currentGear;
    const bool isAccelerating = localVehicle->m_pVehicle->m_fGasPedal > 0.05f;
    const bool isChangingGearAudio = localVehicle->m_pVehicle->m_nVehicleFlags.bAudioChangingGear;
    const bool isBrakePressed = localVehicle->m_pVehicle->m_fBreakPedal > 0.05f;
    const bool isExitSuppressed = ms_lastExitVehicleRequestTick && now - ms_lastExitVehicleRequestTick < kExitVehicleSuppressMs;

    if ((gearChanged || isChangingGearAudio) && isAccelerating) {
        SpawnBurst(localVehicle, state, now);
    }

    if (isExitSuppressed || isPlayerExitingVehicle || !isBrakePressed) {
        state.brakeBurstCount = 0;
        state.wasBrakePressed = false;
    } else if (state.brakeBurstCount < kBrakeBurstLimit) {
        SpawnBurst(localVehicle, state, now);
        if (state.lastBurstTick == now) {
            state.brakeBurstCount++;
        }
        state.wasBrakePressed = true;
    }

    state.lastGear = currentGear;
}

void CExhaust::OnNitroFired()
{
    const uint32 now = GetTickCount();
    ServiceStates(now);

    CVehicle* localVehicle = GetLocalVehicle();
    if (!localVehicle || !localVehicle->m_pVehicle) {
        return;
    }

    const uint16 vehicleId = FindVehicleId(localVehicle);
    if (vehicleId == INVALID_VEHICLE_ID) {
        return;
    }

    VehicleState& state = GetOrResetState(vehicleId, localVehicle, now);
    if (!CanUseExhaust(localVehicle)) {
        return;
    }

    if (state.lastNitroTick && now - state.lastNitroTick < kNitroCooldownMs) {
        return;
    }

    SpawnBurst(localVehicle, state, now);
    state.lastNitroTick = now;
}

void CExhaust::OnExitVehicleRequest()
{
    ms_lastExitVehicleRequestTick = GetTickCount();

    for (auto& [vehicleId, state] : ms_states) {
        state.brakeBurstCount = 0;
        state.wasBrakePressed = false;
    }
}
