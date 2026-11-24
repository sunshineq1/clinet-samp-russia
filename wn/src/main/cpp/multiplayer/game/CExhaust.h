#pragma once

#include <unordered_map>

#include "common.h"
#include "Core/Vector.h"

class CVehicle;
class CObject;
struct CVehicleGTA;

class CExhaust
{
public:
    static constexpr int kExhaustEffectModelId = 18695;
    static constexpr uint32 kBurstLifetimeMs = 180;
    static constexpr uint32 kBurstCooldownMs = 120;
    static constexpr uint32 kNitroCooldownMs = 260;
    static constexpr uint32 kIdleStateLifetimeMs = 8000;
    static constexpr uint32 kExitVehicleSuppressMs = 1500;
    static constexpr float  kExhaustVerticalOffset = -1.65f;
    static constexpr uint8  kBrakeBurstLimit = 5;
    static constexpr uint8  kInvalidGear = 0xFF;

    static inline float ms_fEffectScale = 0.75f;

    struct EffectPair {
        CObject* primary{};
        CObject* secondary{};

        [[nodiscard]] bool HasAny() const {
            return primary || secondary;
        }
    };

    struct VehicleState {
        uintptr_t vehicleToken{};
        uint8  lastGear{kInvalidGear};
        uint32 lastSeenTick{};
        uint32 lastBurstTick{};
        uint32 lastNitroTick{};
        uint32 burstSpawnTick{};
        uint8  brakeBurstCount{};
        bool   wasBrakePressed{};
        EffectPair activeBurst{};
    };

public:
    static void Inject();
    static void OnVehicleProcess(CVehicleGTA* vehicle);
    static void OnNitroFired();
    static void OnExitVehicleRequest();

private:
    static inline std::unordered_map<uint16, VehicleState> ms_states;
    static inline uint32 ms_lastServiceTick = 0;
    static inline uint32 ms_lastExitVehicleRequestTick = 0;

    static void DestroyObject(CObject*& object);
    static void DestroyBurst(VehicleState& state);
    static bool EnsureEffectModelLoaded();
    static CVehicle* GetLocalVehicle();
    static uint16 FindVehicleId(CVehicle* vehicle);
    static bool IsAutomobile(CVehicle* vehicle);
    static bool IsVehicleVisible(CVehicle* vehicle);
    static bool CanUseExhaust(CVehicle* vehicle);
    static bool ResolveExhaustOffsets(CVehicle* vehicle, CVector& primary, bool& hasSecondary, CVector& secondary);
    static CObject* CreateAttachedBurst(CVehicle* vehicle, uint16 vehicleId, const CVector& offset);
    static void ApplyEffectScale(CObject* effect);
    static VehicleState& GetOrResetState(uint16 vehicleId, CVehicle* vehicle, uint32 now);
    static void ServiceStates(uint32 now);
    static void SpawnBurst(CVehicle* vehicle, VehicleState& state, uint32 now);
};
