#pragma once

#include <unordered_map>

#include <cstring>
#include "Entity/CVehicleGTA.h"
#include "Enums/ePlateType.h"
#include "Enums/eLights.h"
#include "Enums/eLightState.h"
#include "VehicleNeon.h"

class CColLine;
class CCollisionData;

CCollisionData* GetCollisionDataFromModel(int nModelIndex);
CColLine* GetSuspensionLinesFromModel(int nModelIndex, int& numWheels);

enum eTurnState
{
	TURN_OFF,
	TURN_LEFT,
	TURN_RIGHT,
	TURN_ALL
};

enum class eLightsState {
    OFF,
    ON_NEAR,
    HIGH,
};

enum eStrobeType : uint8_t
{
    STROBE_OFF = 0,
    STROBE_TYPE_1,
    STROBE_TYPE_2,
    STROBE_TYPE_3,
    STROBE_TYPE_4,

    STROBE_TYPE_COUNT
};

class CVehicle
{
public:
	CVehicle(int iType, float fX, float fY, float fZ, float fRotation, bool bPreloaded, bool bSiren);
	virtual ~CVehicle();

	int GetVehicleSubtype();
	void AddComponent(int iComponentID);
	void RemoveComponent(int iComponentID);
	void SetPaintJob(uint8_t bytePaintJobID);
	void SetColor(uint8_t iColor1, uint8_t iColor2);
	void LinkToInterior(int iInterior);
	void SetDamageStatus(uint32_t dwPanelDamageStatus, uint32_t dwDoorDamageStatus, uint8_t byteLightDamageStatus);
	void SetTireDamageStatus(uint8_t byteTireDamageStatus);
	void SetPlateText(const char* szPlateText) { strncpy(m_szPlateText, szPlateText, 32); }
	void SetZAngle(float fAngle);
	void ProcessMarkers();
	bool IsOccupied();

	void AttachTrailer();
	void DetachTrailer();
	void SetTrailer(CVehicle* pTrailer);

	bool IsRCVehicle();
	float GetHealth();
	void SetHealth(float fHealth);
	CVehicle* GetTrailer();
	CVehicle* GetTractor();
	float GetTrainSpeed();
	uint16_t GetHydraThrusters();
	float GetBikeLean();
	bool IsATrainPart();
	bool IsATowTruck();
	bool IsATrailer();
	bool VerifyInstance();
	bool IsDriverLocalPlayer();
	void SetInvulnerable(bool bInv);
	bool HasSunk();
	bool HasADriver();

	int GetEngineState() { return m_iEngineState; }

	void ApplyEngineState(int iState);
	void ApplyLightState(int state);

	void SetDoorState(int state);
	void SetComponentOpenState(int iDoor, int iComponent, float fDoorOpenRatio);
	void SetAlarmState(uint16_t wAlarmState) { m_pVehicle->m_nVehicleFlags.bSirenOrAlarm = wAlarmState; }
	void OpenWindow(uint8_t component);
	void CloseWindow(uint8_t component);

	void RemoveEveryoneFromVehicle();

	void UpdateColor();
	bool UpdateLastDrivenTime();

    eTurnState GetTurnLightsState() const;
    void SetTurnLightsState(eTurnState state);

    bool IsTurnBlinkOn() const;

    eTurnState GetTurnLightsBlinkState() const;

    void RenderTurnLights();
    void UpdateSteeringWheel();
    bool GetHandPosition(bool bLeft, CVector &out);
    static inline float ms_fSteeringWheelRatio = 2.6f;
    void DrawTurnlight(bool leftSide);
    void DrawTurnlightPool(const CVector& vecObjOffset, bool bRear, int32 id);
	void DrawRightTurnLight();
	void DrawLeftTurnLight();

    // Правка хендлинга от сервера: множитель к заводскому значению либо
    // готовое значение
    struct SHandlingEdit {
        uint8_t param;
        uint8_t mode;
        float   value;
    };

    static constexpr int32_t kMaxHandlingEdits = 32;
    static constexpr int32_t kWheelParamCount = 10;

    void SetWheelParameters(const float* values);
    void ApplyHandlingEdits(const SHandlingEdit* edits, uint8_t count);
    void RebuildHandling();
    void ReleaseHandling();

    void InitWheelFrames();
    void ProcessWheelsOffset();

    void CopyGlobalSuspensionLinesToPrivate();
    void RebuildSuspensionLines();
    void ReleaseSuspensionLines();

    // Кузовной тюнинг: деталь из data/customtune.dat вешается на dummy-фрейм,
    // а стоковый атомик на его месте гасится.
    bool SetCustomComponent(int32_t selector, int32_t tuneId);
    static inline char ms_szLastComponentError[96] = { 0 };
    void RemoveCustomComponent(int32_t selector);
    void RemoveAllCustomComponents();

    void setPlate(ePlateType type, std::string& szNumber, std::string& szRegion);
    void setVinyl(const char* remapName);
    void processHeadlightsColor(uint8_t& r, uint8_t& g, uint8_t& b);
    void processHeadlightsBeamColor(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a);

    void SetNeonColor(uint8_t slot, const CRGBA& color);
    bool HasNeon() const;

    void SetStrobeType(uint8_t type);
    void ProcessStrobes();
    void SetLightState(eLights light, eLightsState_Old state);
    eLightsState_Old GetLightState(eLights light);

public:
    CVehicleGTA* m_pVehicle;
	CVehicle* m_pTrailer;
    uintptr 		m_dwGTAId;

    // .. BR Start
	CRGBA       mainColor		    { 0, 0, 0, 255 };
    CRGBA       wheelColor		    { 0, 0, 0, 255 };
	CRGBA       secondColor		    { 0, 0, 0, 255 };
    CRGBA       tonerColor		    { 255, 255, 255, 255 };
    bool        bTonerColorSet       = false;
	CRGBA       lightColor		    { 255, 255, 255, 255 };

    CVehicleNeon neon;

    // Пока мигалка включена, её цвет перекрывает lightColor
    eLightsState_Old m_savedLightLeft  = VEHICLE_LIGHT_OK;
    eLightsState_Old m_savedLightRight = VEHICLE_LIGHT_OK;

    CRGBA       strobeColor         { 255, 255, 255, 255 };
    eStrobeType m_iStrobeType       = STROBE_OFF;
    uint32_t    m_dwStrobeStep      = 0;
    uint32_t    m_dwLastStrobeTime  = 0;

    RwTexture*  pPlateTexture       = nullptr;
	RwTexture*  m_pVinylTex 		= nullptr;

    struct SCustomComponent {
        int32_t   modelId  = -1;
        RpAtomic* pAtomic  = nullptr;
        RwFrame*  pFrame   = nullptr;
        char      hidden[32] {};
    };

    std::unordered_map<int32_t, SCustomComponent> m_customComponents;

    RwFrame* m_pWheelLF = nullptr;
    RwFrame* m_pWheelRF = nullptr;
    RwFrame* m_pWheelRB = nullptr;
    RwFrame* m_pWheelLB = nullptr;

    RwMatrix m_initialWheelMatrix[4]{};

    // Клумп, с которого сняты фреймы колёс. Модель могут подменить на лету,
    // и тогда старые указатели повиснут.
    RpClump* m_pWheelClump = nullptr;

    float m_fWheelAngleFront = 0.0f;
    float m_fWheelAngleBack  = 0.0f;
    float m_fWheelOffsetFront = 0.0f;
    float m_fWheelOffsetBack  = 0.0f;
    float m_fWheelWidthFront  = 0.0f;
    float m_fWheelWidthBack   = 0.0f;

    float m_fWheelScaleFront = 0.0f;
    float m_fWheelScaleRear  = 0.0f;
    float m_fDefaultWheelSizeFront = 0.0f;
    float m_fDefaultWheelSizeRear  = 0.0f;

    float m_fClearance      = 0.0f;
    float m_fClearanceRear  = 0.0f;

    // Свои линии подвески: у колеса изменённого радиуса и коллизия своя, а
    // линии в CColModel общие на всю модель
    CColLine* m_pSuspensionLines = nullptr;
    bool      bHasSuspensionLines = false;
    int32_t   m_nSuspensionLineCount = 0;
    int32_t   m_nSuspensionLinesCopied = 0;
    bool      m_bSuspensionRebuildPending = false;

    bool     m_bWheelOffsetDoneFront = true;
    bool     m_bWheelOffsetDoneBack  = true;
    uint32_t m_uiLastWheelOffset = 0;

    tHandlingData* m_pCustomHandling = nullptr;
    tHandlingData* m_pStockHandling = nullptr;
    SHandlingEdit  m_handlingEdits[kMaxHandlingEdits]{};
    uint8_t        m_handlingEditCount = 0;
    // .. BR End

	bool m_bPreloaded;
	int m_iEngineState;
    eLightsState m_bIsLightOn = eLightsState::OFF;
	bool m_bIsInvulnerable;
	bool m_bIsLocked;
	bool m_bDoorsLocked;
    eTurnState m_iTurnState = TURN_OFF;
    uint32_t m_nTurnStateTime = 0;
	uint8_t m_byteObjectiveVehicle;
	bool m_bSpecialMarkerEnabled;
	uint8_t m_byteColor1;
	uint8_t m_byteColor2;
	bool m_bHaveColor;
	bool m_bSiren;
	char m_szPlateText[32];
	uint32_t m_dwMarkerID;

	void UpdateDamageStatus(uint32_t dwPanelDamage, uint32_t dwDoorDamage, uint8_t byteLightDamage, uint8_t byteWheelDamage);
	void SetPanelDamageStatus(uint32_t dwPanelStatus);
	uint32_t GetPanelDamageStatus();
	void SetDoorDamageStatus(uint32_t dwDoorStatus);
	uint32_t GetDoorDamageStatus();
	void SetLightDamageStatus(uint8_t byteLightStatus);
	uint8_t GetLightDamageStatus();
	void SetWheelPoppedStatus(uint8_t byteWheelStatus);
	uint8_t GetWheelPoppedStatus();

    bool SirenEnabled();

	void EnableSiren(bool bState);

	bool IsLandingGearNotUp();

	bool			m_bHasBeenDriven;
	uint32_t		m_dwTimeSinceLastDriven;\
};