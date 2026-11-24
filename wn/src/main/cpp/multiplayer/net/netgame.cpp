#include "game/Enums/eHandlingParam.h"
#include "../main.h"
#include "../game/game.h"
#include "netgame.h"

#include "../gui/gui.h"
#include "game/GPS.h"

#include "../CServerManager.h"

// voice
#include "../voice_new/MicroIcon.h"
#include "../voice_new/SpeakerList.h"
#include "../voice_new/Network.h"

// java
#include "java/jniutil.h"
#include "java/HUD.h"
#include "java/speedometer.h"
#include "java/GUIManager.h"
extern CJavaWrapper *pJavaWrapper;

// audiostream
#include "../CAudioStream.h"
#include "AudioStreamPool.h"

// #define AUTH_BS "E02262CF28BC542486C558D4BE9EFB716592AFAF8B"
#define AUTH_BS "10B3D2B1317ADD02CC1F680BC500A8BC0FD7AD42CE7"

extern UI* pUI;
extern CGame* pGame;

int iNetModeNormalOnFootSendRate = 30;
int iNetModeNormalInCarSendRate = 30;
int iNetModeFiringSendRate = 30;
int iNetModeSendMultiplier = 2;

void RegisterRPCs(RakClientInterface *pRakClient);
void UnregisterRPCs(RakClientInterface *pRakClient);
void RegisterScriptRPCs(RakClientInterface *pRakClient);
void UnregisterScriptRPCs(RakClientInterface *pRakClient);

// 0.3.7
unsigned char GetPacketID(Packet *p)
{
	if (p == 0) return 255;

	if ((unsigned char)p->data[0] == ID_TIMESTAMP)
		return (unsigned char)p->data[sizeof(unsigned char) + sizeof(unsigned long)];
	else
		return (unsigned char)p->data[0];
}

#define IP_KEY 0xABCD9876
#define PORT_KEY 0xEF123456
__attribute__((noinline))
char* DecryptIP(const char* encrypted_buf, size_t buf_len, char* out_buf) {
    for (size_t i = 0; i < buf_len; ++i) {
        out_buf[i] = encrypted_buf[i] ^ (char)(IP_KEY + i);
    }
    return out_buf;
}

__attribute__((noinline))
int FakeObfuscationCheck(int a) {
    return (a * 0xFACE) % 0xBEAF;
}

CNetGame::CNetGame(const char* szHostOrIp, int iPort, const char *szPlayerName, const char* szPass)
{
    volatile int check = iPort;
    check = FakeObfuscationCheck(check);
    if (check == 0x1234) {
        m_pNetSet = nullptr;
    }

    // .. обманка для дебилов
    // iServer == -1 это пустое подключение на петлю, таблицы адресов для него нет
    if(CServerInstance::iServer >= 0) {
        if(g_sEncryptedAddresses[CServerInstance::iServer].getPort() != iPort) {
            iPort = g_sEncryptedAddresses[CServerInstance::iServer].getPort();
            szHostOrIp = g_sEncryptedAddresses[CServerInstance::iServer].decrypt();
        }
        iPort = g_sEncryptedAddresses[CServerInstance::iServer].getPort();
        szHostOrIp = g_sEncryptedAddresses[CServerInstance::iServer].decrypt();
    }

    Network::OnRaknetConnect(szHostOrIp, iPort);

	m_pNetSet = new NET_SETTINGS;

	strcpy(m_szHostName, "");

    size_t len = strnlen(szHostOrIp, 256) + 1;
    for (size_t i = 0; i < len; ++i) {
        m_EncryptedHostOrIp[i] = szHostOrIp[i] ^ (char)(IP_KEY + i);
    }
    memset(m_EncryptedHostOrIp + len, 0, sizeof(m_EncryptedHostOrIp) - len);
    m_EncryptedPort = iPort ^ PORT_KEY;

	m_pRakClient = RakNetworkFactory::GetRakClientInterface();
	InitializePools();

    CAudioStreamPool::Init();

	GetPlayerPool()->SetLocalPlayerName(szPlayerName);

	RegisterRPCs(m_pRakClient);
	RegisterScriptRPCs(m_pRakClient);
	m_pRakClient->SetPassword(szPass);

	memset(m_dwMapIcon, 0, sizeof(m_dwMapIcon));

	pGame->EnableClock(false);
	pGame->EnableZoneNames(false);

	m_pNetSet->iDeathDropMoney = 0;
	m_pNetSet->iSpawnsAvailable = 0;
	m_pNetSet->bNameTagLOS = 0;
	m_pNetSet->byteHoldTime = true;
	m_pNetSet->bUseCJWalk = 0;
	m_pNetSet->bDisableInteriorEnterExits = 0;
	m_pNetSet->bZoneNames = false;
	m_pNetSet->bInstagib = false;
	m_pNetSet->fNameTagDrawDistance = 70.0f;
	m_pNetSet->bFriendlyFire = true;
	m_pNetSet->byteWorldTime_Hour = 12;
	m_pNetSet->byteWorldTime_Minute = 0;
	m_pNetSet->byteWeather = 1;
	m_pNetSet->fGravity = 0.008f;
	m_bNameTagStatus = true;

	m_dwLastConnectAttempt = GetTickCount();
	SetGameState(GAMESTATE_WAIT_CONNECT);
	m_bLanMode = false;

    GUIManager::TempToggle(UI_ID_HUD, false);
    pJavaWrapper->HideLoadingScreen();

    m_fWorldBounds[0] = m_fWorldBounds[2] = 20000.0f;
    m_fWorldBounds[1] = m_fWorldBounds[3] = -20000.0f;

    const char* sampVer = SAMP_VERSION;
	if (pUI)
        pUI->chat()->addDebugMessage(localisation_started);
}

// 0.3.7
CNetGame::~CNetGame()
{
	Network::OnRaknetDisconnect();

	m_pRakClient->Disconnect(0);
	UnregisterRPCs(m_pRakClient);
	UnregisterScriptRPCs(m_pRakClient);
	RakNetworkFactory::DestroyRakClientInterface(m_pRakClient);

    CAudioStreamPool::Free();

	UninitializePools();

	if (m_pNetSet) {
		delete m_pNetSet;
		m_pNetSet = nullptr;
	}
}

void CNetGame::InitializePools()
{
	m_pPools = new NET_POOLS;
	m_pPools->pPlayerPool = new CPlayerPool();
	m_pPools->pVehiclePool = new CVehiclePool();
	m_pPools->pGangZonePool = new CGangZonePool();
	m_pPools->pPickupPool = new CPickupPool();
	m_pPools->pObjectPool = new CObjectPool();
	m_pPools->pTextLabelPool = new C3DTextLabelPool();
	m_pPools->pTextDrawPool = new CTextDrawPool();
	m_pPools->pActorPool = new CActorPool();
	m_pPools->pMenuPool = new CMenuPool();
	m_pPools->pPlayerBubblePool = new CPlayerBubblePool();
}

void CNetGame::UninitializePools()
{
	if (m_pPools->pPlayerPool) {
		delete m_pPools->pPlayerPool;
		m_pPools->pPlayerPool = nullptr;
	}

	if (m_pPools->pVehiclePool) {
		delete m_pPools->pVehiclePool;
		m_pPools->pVehiclePool = nullptr;
	}

	if (m_pPools->pGangZonePool) {
		delete m_pPools->pGangZonePool;
		m_pPools->pGangZonePool = nullptr;
	}

	if (m_pPools->pPickupPool) {
		delete m_pPools->pPickupPool;
		m_pPools->pPickupPool = nullptr;
	}

	if (m_pPools->pObjectPool) {
		delete m_pPools->pObjectPool;
		m_pPools->pObjectPool = nullptr;
	}

	if (m_pPools->pTextLabelPool) {
		delete m_pPools->pTextLabelPool;
		m_pPools->pTextLabelPool = nullptr;
	}

	if (m_pPools->pTextDrawPool) {
		delete m_pPools->pTextDrawPool;
		m_pPools->pTextDrawPool = nullptr;
	}

	if (m_pPools->pActorPool) {
		delete m_pPools->pActorPool;
		m_pPools->pActorPool = nullptr;
	}

	if (m_pPools->pMenuPool) {
		delete m_pPools->pMenuPool;
		m_pPools->pMenuPool = nullptr;
	}

	if (m_pPools->pPlayerBubblePool)
	{
		delete m_pPools->pPlayerBubblePool;
		m_pPools->pPlayerBubblePool = nullptr;
	}

	if (m_pPools) {
		delete m_pPools;
		m_pPools = nullptr;
	}

	if (m_pNetSet) {
		delete m_pNetSet;
		m_pNetSet = nullptr;
	}
}

#include "game/Snow.h"
#include "graphics/CSkyBox.h"

void CNetGame::Process()
{
	static uint32_t time = GetTickCount();
	bool bProcess = false;
	if (GetTickCount() - time >= 1000 / 30)
	{
		UpdateNetwork();
		time = GetTickCount();
		bProcess = true;
	}

	if (m_pNetSet->byteHoldTime) {
		pGame->SetWorldTime(m_pNetSet->byteWorldTime_Hour, m_pNetSet->byteWorldTime_Minute);
	}

	if (GetGameState() == GAMESTATE_CONNECTED) {
		ProcessPools();
        CAudioStreamPool::Process();
	} else {
		ProcessLoadingScreen();
	}

	if (GetGameState() == GAMESTATE_WAIT_CONNECT) {
		ProcessConnecting();
	}
}

void CNetGame::UpdateNetwork()
{
    if(!m_pRakClient) return;
    
	bool breakStatus = false;
	Packet *pkt = nullptr;
	unsigned char packetIdentifier;

	while (pkt = m_pRakClient->Receive())
	{
		if(pkt) {
            packetIdentifier = GetPacketID(pkt);

            switch (packetIdentifier) {
                case ID_AUTH_KEY:
                    Packet_AuthKey(pkt);
                    break;

                case ID_CONNECTION_ATTEMPT_FAILED:
                    Packet_ConnectAttemptFailed(pkt);
                    break;

                case ID_NO_FREE_INCOMING_CONNECTIONS:
                    Packet_NoFreeIncomingConnections(pkt);
                    break;

                case ID_DISCONNECTION_NOTIFICATION:
                    Packet_DisconnectionNotification(pkt);
                    break;

                case ID_CONNECTION_LOST:
                    Packet_ConnectionLost(pkt);
                    break;

                case ID_CONNECTION_REQUEST_ACCEPTED:
                    Packet_ConnectionSucceeded(pkt);
                    break;

                case ID_FAILED_INITIALIZE_ENCRIPTION:
                    Packet_FailedInitializeEncription(pkt);
                    break;

                case ID_CONNECTION_BANNED:
                    Packet_ConnectionBanned(pkt);
                    SetGameState(GAMESTATE_WAIT_CONNECT);
                    break;

                case ID_INVALID_PASSWORD:
                    Packet_InvalidPassword(pkt);
                    break;

                case ID_VEHICLE_SYNC:
                    Packet_VehicleSync(pkt);
                    break;

                case ID_AIM_SYNC:
                    Packet_AimSync(pkt);
                    break;

                case ID_BULLET_SYNC:
                    Packet_BulletSync(pkt);
                    break;

                case ID_PLAYER_SYNC:
                    Packet_PlayerSync(pkt);
                    break;

                case ID_MARKERS_SYNC:
                    Packet_MarkerSync(pkt);
                    break;

                case ID_UNOCCUPIED_SYNC:
                    Packet_UnoccupiedSync(pkt);
                    break;

                case ID_TRAILER_SYNC:
                    Packet_TrailerSync(pkt);
                    break;

                case ID_PASSENGER_SYNC:
                    Packet_PassengerSync(pkt);
                    break;

                case Network::kRaknetPacketId:
                    Network::OnRaknetReceive(*pkt);
                    break;

                case ID_CUSTOM_RPC:
                    Packet_CustomRPC(pkt);
                    break;

                case PACKET_ANDROID_GUI:
                    GUIManager::ReceivePacket(pkt);
                    break;
            }

            m_pRakClient->DeallocatePacket(pkt);
        }
	}
}

#define RPC_SHOW_GPS 0x177
#define RPC_HIDE_GPS 0x170

#define RPC_SHOW_GREENZONE 0x183
#define RPC_HIDE_GREENZONE 0x184

#define RPC_SHOW_NOTIFICATION 0x32

#define RPC_GPS_LINE                                   0x200

#define RPC_HUD_TOGGLE_X2                              0x201

#define RPC_SPEED_FUEL                                 0x202

#define RPC_TUNING_SET_VINYL                           0x203
#define RPC_TUNING_SET_NUMBER_PLATE                    0x204
#define RPC_TUNING_SET_COLOR                           0x205
#define RPC_TUNING_SET_WHEEL_COLOR                     0x206
#define RPC_TUNING_SET_TONER_COLOR                     0x207
#define RPC_TUNING_SET_LIGHT_COLOR                     0x208
#define RPC_TUNING_SET_COMPONENT                       0x209
#define RPC_TUNING_SET_HANDLING                        0x20A
#define RPC_TUNING_SET_WHEEL                           0x20B
#define RPC_TUNING_SET_NEON                            0x20C
#define RPC_TUNING_SET_STROBE                          0x20D

// Номера из customtune.inc: мод BR шлёт их своим слоем, отдельным от
// нашего 0x203..0x20D. Разбираем то, что ложится на готовые методы.
#define RPC_CUSTOM_COMPONENT                           0x10
#define RPC_CUSTOM_HANDLING                            0x11
#define RPC_CUSTOM_VISUALS                             0x13

void CNetGame::Packet_CustomRPC(Packet *p) {
    RakNet::BitStream bs((unsigned char *) p->data, p->length, false);
    bs.IgnoreBits(8);

    uint32_t rpcID;
    bs.Read(rpcID);

    switch (rpcID) {
        case RPC_CUSTOM_HANDLING: {
            uint16_t vehID = 0;
            uint8_t count = 0;

            bs.Read(vehID);
            bs.Read(count);

            // Соответствие E_HANDLING_PARAMS из customtune.inc нашим номерам
            static const int8_t kParamMap[] = {
                HANDLING_PARAM_MAX_SPEED,
                HANDLING_PARAM_ACCELERATION,
                HANDLING_PARAM_NUMBER_OF_GEARS,
                HANDLING_PARAM_ENGINE_INERTIA,
                HANDLING_PARAM_MASS,
                HANDLING_PARAM_TURN_MASS,
                HANDLING_PARAM_BRAKE_DECELERATION,
                HANDLING_PARAM_TRACTION_MULTIPLIER,
                HANDLING_PARAM_TRACTION_LOSS,
                HANDLING_PARAM_TRACTION_BIAS,
                HANDLING_PARAM_SUSPENSION_LOWER,
                HANDLING_PARAM_SUSPENSION_BIAS,
                -1,
            };

            CVehicle::SHandlingEdit edits[CVehicle::kMaxHandlingEdits];
            uint8_t used = 0;

            for (uint8_t i = 0; i < count; i++) {
                uint8_t index = 0;
                float value = 0.0f;

                bs.Read(index);
                bs.Read(value);

                if (index >= sizeof(kParamMap) || kParamMap[index] < 0) {
                    continue;
                }

                if (used >= CVehicle::kMaxHandlingEdits) {
                    continue;
                }

                edits[used].param = (uint8_t)kParamMap[index];
                edits[used].mode = HANDLING_MODE_ABSOLUTE;
                edits[used].value = value;
                used++;
            }

            CVehicle* pVehicle = GetVehiclePool()->GetAt(vehID);

            if (pVehicle) {
                pVehicle->ApplyHandlingEdits(edits, used);
            }

            break;
        }
        case RPC_CUSTOM_VISUALS: {
            uint16_t vehID = 0;
            bs.Read(vehID);

            uint8_t lightR = 0, lightG = 0, lightB = 0;
            bs.Read(lightR);
            bs.Read(lightG);
            bs.Read(lightB);

            uint16_t wheelWidth = 0;
            int8_t alignFront = 0, alignRear = 0;
            int16_t offsetFront = 0, offsetRear = 0;

            bs.Read(wheelWidth);
            bs.Read(alignFront);
            bs.Read(alignRear);
            bs.Read(offsetFront);
            bs.Read(offsetRear);

            uint8_t toner[3] = {};
            for (uint8_t& value : toner) {
                bs.Read(value);
            }

            uint8_t vinyl[2] = {};
            bs.Read(vinyl[0]);
            bs.Read(vinyl[1]);

            uint8_t plateType = 0;
            bs.Read(plateType);

            std::string plateText;
            std::string plateRegion;

            if (plateType == 1 || (plateType >= 4 && plateType <= 7)) {
                bs.ReadStr8(plateText);
                bs.ReadStr8(plateRegion);
            } else if (plateType == 2 || plateType == 3) {
                bs.ReadStr8(plateText);
            }

            uint8_t shadowR = 0, shadowG = 0, shadowB = 0;
            uint8_t shadowSizeX = 0, shadowSizeY = 0;
            std::string shadowTexture;

            bs.Read(shadowR);
            bs.Read(shadowG);
            bs.Read(shadowB);
            bs.Read(shadowSizeX);
            bs.Read(shadowSizeY);
            bs.ReadStr8(shadowTexture);

            CVehicle* pVehicle = GetVehiclePool()->GetAt(vehID);

            if (!pVehicle) {
                break;
            }

            pVehicle->lightColor = CRGBA{ lightR, lightG, lightB, 255 };

            // Мод шлёт только четыре величины из десяти, остальные берём
            // текущие, иначе обнулятся клиренс и радиус
            float wheels[CVehicle::kWheelParamCount];
            wheels[0] = pVehicle->m_fClearance;
            wheels[1] = pVehicle->m_fClearanceRear;
            wheels[2] = (float)offsetFront;
            wheels[3] = (float)offsetRear;
            wheels[4] = (float)alignFront;
            wheels[5] = (float)alignRear;
            wheels[6] = (float)wheelWidth;
            wheels[7] = (float)wheelWidth;
            wheels[8] = (pVehicle->m_fWheelScaleFront - pVehicle->m_fDefaultWheelSizeFront) * 100.0f;
            wheels[9] = (pVehicle->m_fWheelScaleRear - pVehicle->m_fDefaultWheelSizeRear) * 100.0f;

            pVehicle->SetWheelParameters(wheels);

            // Подсветка под машиной у мода называется тенью, у нас это неон
            const CRGBA glow{ shadowR, shadowG, shadowB, 255 };

            pVehicle->SetNeonColor(0, glow);
            pVehicle->SetNeonColor(1, glow);

            if (toner[0] || toner[1] || toner[2] || vinyl[0] || vinyl[1] || plateType) {
                Log("CustomRPC: тонировка, винилы и номер приходят номерами слотов, "
                    "у нас они задаются иначе — не применено (машина %d)", vehID);
            }

            break;
        }
        case RPC_TUNING_SET_NEON: {
            int vehID = -1;
            uint8_t slot;
            CRGBA color;

            bs.Read(vehID);
            bs.Read(slot);
            bs.Read(color.r);
            bs.Read(color.g);
            bs.Read(color.b);
            bs.Read(color.a);

            CVehicle* pVehicle = GetVehiclePool()->GetAt(vehID);

            if (pVehicle) {
                pVehicle->SetNeonColor(slot, color);
            } else {
                Log("Error: not find vehicle to update tuning");
            }

            break;
        }
        case RPC_TUNING_SET_STROBE: {
            int vehID = -1;
            uint8_t type;

            bs.Read(vehID);
            bs.Read(type);

            CVehicle* pVehicle = GetVehiclePool()->GetAt(vehID);

            if (pVehicle) {
                pVehicle->SetStrobeType(type);
            } else {
                Log("Error: not find vehicle to update tuning");
            }

            break;
        }
        case RPC_TUNING_SET_COMPONENT: {
            int vehID = -1, selector, tuneId;

            bs.Read(vehID);
            bs.Read(selector);
            bs.Read(tuneId);

            CVehicle* pVehicle = GetVehiclePool()->GetAt(vehID);

            if (pVehicle) {
                pVehicle->SetCustomComponent(selector, tuneId);
            } else {
                Log("Error: not find vehicle to update tuning");
            }

            break;
        }
        case RPC_TUNING_SET_WHEEL: {
            int vehID = -1;
            bs.Read(vehID);

            float values[CVehicle::kWheelParamCount];
            for (float& value : values) {
                bs.Read(value);
            }

            CVehicle* pVehicle = GetVehiclePool()->GetAt(vehID);

            if (pVehicle) {
                pVehicle->SetWheelParameters(values);
            } else {
                Log("Error: not find vehicle to update tuning");
            }

            break;
        }
        case RPC_TUNING_SET_HANDLING: {
            int vehID = -1;
            uint8_t count;

            bs.Read(vehID);
            bs.Read(count);

            CVehicle::SHandlingEdit edits[CVehicle::kMaxHandlingEdits];
            uint8_t stored = 0;

            for (uint8_t i = 0; i < count; i++) {
                uint8_t param, mode;
                float value;

                bs.Read(param);
                bs.Read(mode);
                bs.Read(value);

                if (stored >= CVehicle::kMaxHandlingEdits) {
                    continue;
                }

                edits[stored++] = { param, mode, value };
            }

            CVehicle* pVehicle = GetVehiclePool()->GetAt(vehID);

            if (pVehicle) {
                pVehicle->ApplyHandlingEdits(edits, stored);
            } else {
                Log("Error: not find vehicle to update tuning");
            }

            break;
        }
        case RPC_TUNING_SET_LIGHT_COLOR: {
            int vehID;
			bs.Read(vehID);

            CVehiclePool* pVehiclePool = GetVehiclePool();
            CVehicle* pVehicle = pVehiclePool->GetAt(vehID);

            if(pVehicle) {
                bs.Read(pVehicle->lightColor.r);
                bs.Read(pVehicle->lightColor.g);
                bs.Read(pVehicle->lightColor.b);
                bs.Read(pVehicle->lightColor.a);
            } else {
                Log("Error: not find vehicle to update tuning");
            }

            break;
        }
        case RPC_TUNING_SET_TONER_COLOR: {
            int vehID;
			bs.Read(vehID);

            CVehiclePool* pVehiclePool = GetVehiclePool();
            CVehicle* pVehicle = pVehiclePool->GetAt(vehID);

            if(pVehicle) {
                bs.Read(pVehicle->tonerColor.r);
                bs.Read(pVehicle->tonerColor.g);
                bs.Read(pVehicle->tonerColor.b);
                bs.Read(pVehicle->tonerColor.a);
                pVehicle->bTonerColorSet = true;
            } else {
                Log("Error: not find vehicle to update tuning");
            }

            break;
        }
        case RPC_TUNING_SET_WHEEL_COLOR: {
            int vehID;
			bs.Read(vehID);

            CVehiclePool* pVehiclePool = GetVehiclePool();
            CVehicle* pVehicle = pVehiclePool->GetAt(vehID);

            if(pVehicle) {
                bs.Read(pVehicle->wheelColor.r);
                bs.Read(pVehicle->wheelColor.g);
                bs.Read(pVehicle->wheelColor.b);
                bs.Read(pVehicle->wheelColor.a);
            } else {
                Log("Error: not find vehicle to update tuning");
            }

            break;
        }
        case RPC_TUNING_SET_COLOR: {
            int vehID;
			bs.Read(vehID);

            CVehiclePool* pVehiclePool = GetVehiclePool();
            CVehicle* pVehicle = pVehiclePool->GetAt(vehID);

            if(pVehicle) {
                bs.Read(pVehicle->mainColor.r);
                bs.Read(pVehicle->mainColor.g);
                bs.Read(pVehicle->mainColor.b);
                bs.Read(pVehicle->mainColor.a);

                bs.Read(pVehicle->secondColor.r);
                bs.Read(pVehicle->secondColor.g);
                bs.Read(pVehicle->secondColor.b);
                bs.Read(pVehicle->secondColor.a);
            } else {
                Log("Error: not find vehicle to update tuning");
            }

            break;
        }
        case RPC_TUNING_SET_NUMBER_PLATE: {
            int vehID;

			bs.Read(vehID);

            int bPlateType;
            std::string szText;
            std::string szRegion;

            bs.Read(bPlateType);
            bs.ReadStr8(szText);
            bs.ReadStr8(szRegion);

            CVehiclePool* pVehiclePool = GetVehiclePool();
            CVehicle* pVehicle = pVehiclePool->GetAt(vehID);

            if(pVehicle) {
#ifdef BR_DEBUG
                Log("SetPlate: %d / %d / %s / %s", vehID, bPlateType, szText.c_str(), szRegion.c_str());
#endif
                if(!pVehicle->m_pVehicle->IsBMX() && !pVehicle->m_pVehicle->IsBike() && !pVehicle->m_pVehicle->IsBoat()
                && !pVehicle->m_pVehicle->IsHeli() && !pVehicle->m_pVehicle->IsMonsterTruck() && !pVehicle->m_pVehicle->IsPlane()
                && !pVehicle->IsRCVehicle() && !pVehicle->m_pVehicle->IsQuad() && !pVehicle->m_pVehicle->IsTrailer()) {
                    pVehicle->setPlate(static_cast<ePlateType>(bPlateType), szText, szRegion);
                }
            } else {
                Log("Error: not find vehicle to update tuning");
            }

            break;
        }
        case RPC_TUNING_SET_VINYL: {
            int vehID;
            std::string szTextureView;

			bs.Read(vehID);
            bs.ReadStr8(szTextureView);

            CVehiclePool* pVehiclePool = GetVehiclePool();
            CVehicle* pVehicle = pVehiclePool->GetAt(vehID);

            if(pVehicle) {
                pVehicle->setVinyl(szTextureView.c_str());
            } else {
                Log("Error: not find vehicle to update tuning");
            }

            break;
        }
        case RPC_SPEED_FUEL: {
            Speedometer::rpcFuel(bs);
            break;
        }
        case RPC_SHOW_GPS: {
            CHud::updateGPS(1);
            break;
        }
        case RPC_HIDE_GPS: {
            CHud::updateGPS(0);
            break;
        }
        case RPC_SHOW_GREENZONE: {
            CHud::updateGreenZone(1);
            break;
        }
        case RPC_HIDE_GREENZONE: {
            CHud::updateGreenZone(0);
            break;
        }
        case RPC_HUD_TOGGLE_X2: {
            int valueToUI;
			bs.Read(valueToUI);

            CHud::updateX2(valueToUI);
            break;
        }
        case RPC_GPS_LINE: {
            float xCoord, yCoord, zCoord;
			bs.Read(xCoord);
            bs.Read(yCoord);
            bs.Read(zCoord);

            if(xCoord == 0 && yCoord == 0 && zCoord == 0) {
                Log("Hello GPS fuck");

                GPS::Set(NULL);
                return;
            }

            CVector serverNeedPos{xCoord, yCoord, zCoord};

            GPS::Set(NULL);
            GPS::Set(&serverNeedPos, true);

            Log("Hello GPS Line");
            break;
        }
        case RPC_SHOW_NOTIFICATION: {
			char szBuff[4096+1];
			char text[64*54];
			char actionBtn[64*54];
			char textBtn[64*54];

			int type;
			bs.Read(type);

			uint16_t lenText;
			bs.Read(lenText);

			memset(text, 0, sizeof(text));
			memset(szBuff, 0, sizeof(szBuff));

			bs.Read(szBuff, lenText);
			cp1251_to_utf8(text, szBuff);

			int duration;
			bs.Read(duration);

			uint16_t lenActionBtn;
			bs.Read(lenActionBtn);

			memset(actionBtn, 0, sizeof(actionBtn));
			memset(szBuff, 0, sizeof(szBuff));

			bs.Read(szBuff, lenActionBtn);
			cp1251_to_utf8(actionBtn, szBuff);

			uint16_t lenBtnText;
			bs.Read(lenBtnText);

			memset(textBtn, 0, sizeof(textBtn));
			memset(szBuff, 0, sizeof(szBuff));

			bs.Read(szBuff, lenBtnText);
			cp1251_to_utf8(textBtn, szBuff);

			pJavaWrapper->ShowNotification(type, text, duration, actionBtn, textBtn);
			break;
		}
    }
}

// 0.3.7
void CNetGame::ShutdownForGameModeRestart()
{
    SpeakerList::Hide();
    MicroIcon::Hide();
    Network::OnRaknetDisconnect();

	for (PLAYERID playerId = 0; playerId < MAX_PLAYERS; playerId++)
	{
		CRemotePlayer* pRemotePlayer = GetPlayerPool()->GetAt(playerId);
		if (pRemotePlayer) {
			pRemotePlayer->SetTeam(NO_TEAM);
			pRemotePlayer->ResetAllSyncAttributes();
		}
	}

	GetPlayerPool()->GetLocalPlayer()->ResetAllSyncAttributes();
	GetPlayerPool()->GetLocalPlayer()->ToggleSpectating(false);
	GameResetStats();

    CAudioStreamPool::Free();

	SetGameState(GAMESTATE_RESTARTING);
	GetPlayerPool()->DeactivateAll();
	GetPlayerPool()->Process();
	ResetVehiclePool();
	ResetActorPool();
	ResetTextDrawPool();
	ResetGangZonePool();
	Reset3DTextLabelPool();
	ResetPickupPool();
	ResetObjectPool();
	ResetMenuPool();

	m_pNetSet->bDisableInteriorEnterExits = false;
	m_pNetSet->fNameTagDrawDistance = 70.0f;
	m_pNetSet->byteWorldTime_Hour = 12;
	m_pNetSet->byteWorldTime_Minute = 0;
	m_pNetSet->byteWeather = 1;
	m_pNetSet->byteHoldTime = 1;
	m_pNetSet->bNameTagLOS = true;
	m_pNetSet->bUseCJWalk = false;
	pGame->ToggleCJWalk(false);
	m_pNetSet->fGravity = 0.008f;
	m_pNetSet->iDeathDropMoney = 0;

	pGame->m_bCheckpointsEnabled = false;
	pGame->m_bRaceCheckpointsEnabled = false;

	pGame->FindPlayerPed()->m_pPed->SetInterior(0, true);
	pGame->ResetLocalMoney();
	pGame->FindPlayerPed()->SetDead();
	pGame->FindPlayerPed()->SetArmour(0.0f);
	pGame->EnableZoneNames(false);
	m_pNetSet->bZoneNames = false;
	GameResetRadarColors();
	pGame->SetGravity(m_pNetSet->fGravity);
	pGame->SetWantedLevel(0);
	pGame->EnableClock(false);

	// voice
    //if(pSettings->Get().bAutoAim) {
        SpeakerList::Hide();
	MicroIcon::Hide();
	Network::OnRaknetDisconnect();
    //}

	if (pUI) pUI->chat()->addInfoMessage("The server is restarting..");
}

int iVehiclePoolProcessFlag = 0;
void CNetGame::ProcessPools()
{
	if (GetPlayerPool()) {
		GetPlayerPool()->Process();
	}

	if(GetVehiclePool()) {
		GetVehiclePool()->Process();
	}

	if (GetPickupPool()) {
		GetPickupPool()->Process();
	}
}

static bool once = false;
void CNetGame::ProcessLoadingScreen()
{
    if(!once)
    {
        CPlayerPed* pPlayerPed = pGame->FindPlayerPed();
	    if (pPlayerPed->IsInVehicle())
            pPlayerPed->RemoveFromVehicleAndPutAt(426.4f, 241.377f, 82.7106f);
        else
            pPlayerPed->m_pPed->SetPosn(426.4f, 241.377f, 82.7106f);

        pPlayerPed->TogglePlayerControllable(false);

        CCamera::SetPosition(428.1893f, 240.7251f, 13.0f, 0.0, 0.0, 0.0);
        CCamera::LookAtPoint(-150.0f, 836.6354f, 92.9970f, 2);

        pGame->DisplayHUD(false);
        pGame->SetWorldWeather(1);

        once = true;
    }
}

void CNetGame::ObfuscatedConnectWrapper(const char* szHostOrIp, int iPort)
{
    bool bShouldConnect = true;
    if (bShouldConnect) {
        m_pRakClient->Connect(szHostOrIp, iPort, 0, 0, 2);
    }
}

void CNetGame::ProcessConnecting()
{
    if (GetTickCount() - m_dwLastConnectAttempt > 3000)
    {
       if (pUI)
           pUI->chat()->addDebugMessage(localisation_connecting_to);

       char decrypted_host_ip[256 + 1];
       DecryptIP(m_EncryptedHostOrIp, sizeof(m_EncryptedHostOrIp), decrypted_host_ip);
       int decrypted_port = m_EncryptedPort ^ PORT_KEY;

       ObfuscatedConnectWrapper(decrypted_host_ip, decrypted_port);

       Network::OnRaknetConnect(decrypted_host_ip, decrypted_port);

       m_dwLastConnectAttempt = GetTickCount();
       SetGameState(GAMESTATE_CONNECTING);
    }
}

// 0.3.7
void gen_auth_key(char buf[260], char* auth_in);
void CNetGame::Packet_AuthKey(Packet *pkt)
{
	uint8_t byteAuthLen;
	char szAuth[260], szAuthKey[269];

	RakNet::BitStream bsAuth((unsigned char*)pkt->data, pkt->length, false);

	if (GetGameState() < GAMESTATE_WAIT_CONNECT || GetGameState() > GAMESTATE_AWAIT_JOIN) return;

	bsAuth.IgnoreBits(8);
	bsAuth.Read(byteAuthLen);
	bsAuth.Read(szAuth, byteAuthLen);
	szAuth[byteAuthLen] = '\0';

	gen_auth_key(szAuthKey, szAuth);
	uint8_t byteAuthKeyLen = strlen(szAuthKey);

	RakNet::BitStream bsKey;
	bsKey.Write((uint8_t)ID_AUTH_KEY);
	bsKey.Write(byteAuthKeyLen);
	bsKey.Write(szAuthKey, byteAuthKeyLen);
	m_pRakClient->Send(&bsKey, SYSTEM_PRIORITY, RELIABLE, 0);
}
// 0.3.7
void CNetGame::Packet_ConnectAttemptFailed(Packet *pkt)
{
	if (pUI)
        pUI->chat()->addDebugMessage(localisation_CONNECTION_ATTEMPT_FAILED);

	SpeakerList::Hide(); //add new
	MicroIcon::Hide();
	SetGameState(GAMESTATE_WAIT_CONNECT);
}
// 0.3.7
void CNetGame::Packet_NoFreeIncomingConnections(Packet *pkt)
{
	if(pUI) pUI->chat()->addDebugMessage(localisation_NO_FREE_INCOMING_CONNECTIONS);
	SpeakerList::Hide(); //addnew
	MicroIcon::Hide();
	SetGameState(GAMESTATE_WAIT_CONNECT);

	//SpeakerList::Hide();
	//MicroIcon::Hide();
}
// 0.3.7
void CNetGame::Packet_DisconnectionNotification(Packet *pkt)
{
	if (pUI)
        pUI->chat()->addDebugMessage(localisation_DisconnectionNotification);

    CAudioStreamPool::Free();
	m_pRakClient->Disconnect(2000);

	SpeakerList::Hide();
	MicroIcon::Hide();
}

void CNetGame::Packet_ConnectionSucceeded(Packet *pkt)
{
	RakNet::BitStream bsSuccAuth(pkt->data, pkt->length, true);
	PLAYERID MyPlayerID;
	unsigned int uiChallenge;
	int iVersion = 4057;
	uint8_t byteMod = 1;

	bsSuccAuth.IgnoreBits(8);	// packetId
	bsSuccAuth.IgnoreBits(32);	// binaryAddress
	bsSuccAuth.IgnoreBits(16);	// port

	bsSuccAuth.Read(MyPlayerID);
	bsSuccAuth.Read(uiChallenge);
	uiChallenge ^= iVersion;

	if (pUI) pUI->chat()->addDebugMessage(localisation_ConnectionSucceeded);

	SetGameState(GAMESTATE_AWAIT_JOIN);

	uint8_t byteNameLen = strlen(GetPlayerPool()->GetLocalPlayerName());
	uint8_t byteAuthBSLen = strlen(AUTH_BS);
	uint8_t byteClientVerLen = strlen(SAMP_VERSION);
    if(pSettings)
        byteClientVerLen = strlen(pSettings->Get().szVersion);

	RakNet::BitStream bsSend;
	bsSend.Write(iVersion);
	bsSend.Write(byteMod);
	bsSend.Write(byteNameLen);
	bsSend.Write(GetPlayerPool()->GetLocalPlayerName(), byteNameLen);
	bsSend.Write(uiChallenge);
	bsSend.Write(byteAuthBSLen);
	bsSend.Write(AUTH_BS, byteAuthBSLen);
	bsSend.Write(byteClientVerLen);
	// bsSend.Write(SAMP_VERSION, byteClientVerLen);
    if(pSettings)
        bsSend.Write(pSettings->Get().szVersion, byteClientVerLen);
    else
        bsSend.Write(SAMP_VERSION, byteClientVerLen);

	Network::OnRaknetRpc(RPC_ClientJoin, bsSend);

	m_pRakClient->RPC(&RPC_ClientJoin, &bsSend, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, nullptr);

    SpeakerList::Hide();
    MicroIcon::Hide();

    CFont::FixLanguage();
}
// 0.3.7
void CNetGame::Packet_FailedInitializeEncription(Packet *pkt)
{
	if (pUI) pUI->chat()->addDebugMessage("Failed to initialize encryption.");
}
// 0.3.7
void CNetGame::Packet_ConnectionBanned(Packet *pkt)
{
    // fix
    SetGameState(GAMESTATE_WAIT_CONNECT);
}
// 0.3.7
void CNetGame::Packet_InvalidPassword(Packet *pkt)
{
	if (pUI) pUI->chat()->addDebugMessage(localisation_INVALID_PASSWORD);
	m_pRakClient->Disconnect(0);
}
// 0.3.7
void CNetGame::Packet_ConnectionLost(Packet *pkt)
{
	if (m_pRakClient) {
		m_pRakClient->Disconnect(0);
	}

	if (pUI) pUI->chat()->addDebugMessage(localisation_ConnectionLost);
	ShutdownForGameModeRestart();

	CPlayerPool *pPlayerPool = GetPlayerPool();
	if (pPlayerPool) {
		for (PLAYERID playerId = 0; playerId < MAX_PLAYERS; playerId++) {
			if (pPlayerPool->GetSlotState(playerId) == true) {
				pPlayerPool->Delete(playerId, 0);
			}
		}
	}

    CAudioStreamPool::Free();

    once = false;
	SetGameState(GAMESTATE_WAIT_CONNECT);

	SpeakerList::Hide();
	MicroIcon::Hide();
}
// 0.3.7
void CNetGame::Packet_PlayerSync(Packet *pkt)
{
	RakNet::BitStream bsData(pkt->data, pkt->length, false);
	ONFOOT_SYNC_DATA ofSync;
	uint32_t dwTime = 0;
	uint8_t bytePacketId;
	PLAYERID playerId;

	bool bHasLR, bHasUD;

	if (GetGameState() != GAMESTATE_CONNECTED) return;

	memset(&ofSync, 0, sizeof(ONFOOT_SYNC_DATA));

	bsData.Read(bytePacketId);
	bsData.Read(playerId);

	bsData.Read(bHasLR);
	if (bHasLR) {
		bsData.Read(ofSync.lrAnalog);
	}

	bsData.Read(bHasUD);
	if (bHasUD) {
		bsData.Read(ofSync.udAnalog);
	}

	bsData.Read(ofSync.wKeys);
	bsData.Read((char*)&ofSync.vecPos, sizeof(CVector));
	float w, x, y, z;
	bsData.ReadNormQuat(w, x, y, z);
	ofSync.quat.Set(x, y, z, w);

	uint8_t byteHealthArmour;
	uint8_t byteArmTemp = 0, byteHlTemp = 0;
	
	bsData.Read(byteHealthArmour);
	byteArmTemp = (byteHealthArmour & 0x0F);
	byteHlTemp = (byteHealthArmour >> 4);

	if (byteArmTemp == 0xF) ofSync.byteArmour = 100;
	else if (byteArmTemp == 0) ofSync.byteArmour = 0;
	else ofSync.byteArmour = byteArmTemp * 7;

	if (byteHlTemp == 0xF) ofSync.byteHealth = 100;
	else if (byteHlTemp == 0) ofSync.byteHealth = 0;
	else ofSync.byteHealth = byteHlTemp * 7;

	uint8_t byteCurrentWeapon = 0;
	bsData.Read(byteCurrentWeapon);
	ofSync.byteCurrentWeapon ^= (byteCurrentWeapon ^ ofSync.byteCurrentWeapon) & 0x3F;

	bsData.Read(ofSync.byteSpecialAction);
	bsData.ReadVector(ofSync.vecMoveSpeed.x, ofSync.vecMoveSpeed.y, ofSync.vecMoveSpeed.z);

	bool bHasVehicleSurfingInfo;
	bsData.Read(bHasVehicleSurfingInfo);
	if (bHasVehicleSurfingInfo)
	{
		bsData.Read(ofSync.wSurfID);
		bsData.Read(ofSync.vecSurfOffsets.x);
		bsData.Read(ofSync.vecSurfOffsets.y);
		bsData.Read(ofSync.vecSurfOffsets.z);
	}
	else
		ofSync.wSurfID = INVALID_VEHICLE_ID;

	bool bHasAnimation;
	bsData.Read(bHasAnimation);
	if (bHasAnimation) {
		bsData.Read(ofSync.dwAnimation);
	}
	else {
		ofSync.dwAnimation = 0x80000000;
	}

	CRemotePlayer *pRemotePlayer = GetPlayerPool()->GetAt(playerId);
	if (pRemotePlayer) {
		pRemotePlayer->StoreOnFootFullSyncData(&ofSync, 0);
	}
}
// 0.3.7
void CNetGame::Packet_VehicleSync(Packet* pkt)
{
	RakNet::BitStream bsData(pkt->data, pkt->length, false);
	uint32_t dwTime = 0;
	uint8_t bytePacketId;
	INCAR_SYNC_DATA icSync;
	PLAYERID PlayerID;

	if (GetGameState() != GAMESTATE_CONNECTED) return;

	memset(&icSync, 0, sizeof(INCAR_SYNC_DATA));

	bsData.Read(bytePacketId);
	bsData.Read(PlayerID);
	bsData.Read(icSync.VehicleID);
	bsData.Read(icSync.lrAnalog);
	bsData.Read(icSync.udAnalog);
	bsData.Read(icSync.wKeys);

	float w, x, y, z;
	bsData.ReadNormQuat(w, x, y, z);
	icSync.quat.Set(x, y, z, w);
	
	bsData.Read((char*)& icSync.vecPos, sizeof(CVector));
	bsData.ReadVector(icSync.vecMoveSpeed.x, icSync.vecMoveSpeed.y, icSync.vecMoveSpeed.z);

	// car health
	uint16_t wTempVehicleHealth;
	bsData.Read(wTempVehicleHealth);
	icSync.fCarHealth = (float)wTempVehicleHealth;

	// health/armour
	uint8_t byteHealthArmour;
	uint8_t byteArmTemp = 0, byteHlTemp = 0;

	bsData.Read(byteHealthArmour);
	byteArmTemp = (byteHealthArmour & 0x0F);
	byteHlTemp = (byteHealthArmour >> 4);

	if (byteArmTemp == 0xF) icSync.bytePlayerArmour = 100;
	else if (byteArmTemp == 0) icSync.bytePlayerArmour = 0;
	else icSync.bytePlayerArmour = byteArmTemp * 7;

	if (byteHlTemp == 0xF) icSync.bytePlayerHealth = 100;
	else if (byteHlTemp == 0) icSync.bytePlayerHealth = 0;
	else icSync.bytePlayerHealth = byteHlTemp * 7;

	// current weapon
	uint8_t byteTempWeapon;
	bsData.Read(byteTempWeapon);
	icSync.byteCurrentWeapon ^= (byteTempWeapon ^ icSync.byteCurrentWeapon) & 0x3F;

	bool bCheck;

	// siren
	bsData.Read(bCheck);
	if (bCheck) icSync.byteSirenOn = 1;
	// landinggear
	bsData.Read(bCheck);
	if (bCheck) icSync.byteLandingGearState = 1;
	// train speed
    bsData.Read(bCheck);
    if (bCheck) bsData.Read(icSync.HydraThrustAngle);
	// triler id
	bsData.Read(bCheck);
	if (bCheck) bsData.Read(icSync.TrailerID);

	CRemotePlayer* pRemotePlayer = GetPlayerPool()->GetAt(PlayerID);
	if (pRemotePlayer) {
		pRemotePlayer->StoreInCarFullSyncData(&icSync, 0);
	}
}
// 0.3.7
void CNetGame::Packet_AimSync(Packet* pkt)
{
	RakNet::BitStream bsData(pkt->data, pkt->length, false);
	
	uint8_t bytePacketId;
	PLAYERID PlayerId;
	AIM_SYNC_DATA aimSync;
	bsData.Read(bytePacketId);
	bsData.Read(PlayerId);
	bsData.Read((char*)& aimSync, sizeof(AIM_SYNC_DATA));

	CRemotePlayer* pPlayer = m_pPools->pPlayerPool->GetAt(PlayerId);
	if (pPlayer)
		pPlayer->StoreAimFullSyncData(&aimSync);
}
// 0.3.7
void CNetGame::Packet_BulletSync(Packet* pkt)
{
	RakNet::BitStream bsData(pkt->data, pkt->length, false);

	if (GetGameState() != GAMESTATE_CONNECTED) return;

	BULLET_SYNC_DATA btSync;
	uint8_t bytePacketId = 0;
	PLAYERID PlayerID = 0;

	bsData.Read(bytePacketId);
	bsData.Read(PlayerID);
	bsData.Read((char*)&btSync, sizeof(BULLET_SYNC_DATA));

	CPlayerPool* pPlayerPool = GetPlayerPool();
	CRemotePlayer* pRemotePlayer = pPlayerPool->GetAt(PlayerID);
	if (pRemotePlayer)
		pRemotePlayer->StoreBulletFullSyncData(&btSync);

}
// 0.3.7
void CNetGame::Packet_PassengerSync(Packet* pkt)
{
	RakNet::BitStream bsData(pkt->data, pkt->length, false);

	if (GetGameState() == GAMESTATE_CONNECTED)
	{
		uint8_t bytePacketId;
		PLAYERID PlayerID;
		PASSENGER_SYNC_DATA psSync;

		bsData.Read(bytePacketId);
		bsData.Read(PlayerID);
		bsData.Read((char*)& psSync, sizeof(PASSENGER_SYNC_DATA));

		CRemotePlayer* pRemotePlayer = GetPlayerPool()->GetAt(PlayerID);
		if (pRemotePlayer) {
			pRemotePlayer->StorePassengerFullSyncData(&psSync);
		}
	}
}

// 0.3.7
void CNetGame::Packet_MarkerSync(Packet *pkt)
{
	if(m_iGameState != GAMESTATE_CONNECTED) return;

	uint8_t bytePacketId;
	int	iNumberOfPlayers;
	PLAYERID playerId;
	bool bIsPlayerActive;
	short sPos[3];

	RakNet::BitStream bsMarkersSync((unsigned char *)pkt->data, pkt->length, false);
	bsMarkersSync.Read(bytePacketId);
	bsMarkersSync.Read(iNumberOfPlayers);
	if(iNumberOfPlayers)
	{
		for(int i = 0; i < iNumberOfPlayers; i++)
		{
			bsMarkersSync.Read(playerId);
			bsMarkersSync.ReadCompressed(bIsPlayerActive);

			if(bIsPlayerActive)
			{
				bsMarkersSync.Read(sPos[0]);
				bsMarkersSync.Read(sPos[1]);
				bsMarkersSync.Read(sPos[2]);
			}

			if(playerId >= 0 && playerId < MAX_PLAYERS && GetPlayerPool()->GetSlotState(playerId))
			{
				CRemotePlayer *pPlayer = GetPlayerPool()->GetAt(playerId);
				if(pPlayer)
				{
					if(bIsPlayerActive)
						pPlayer->ShowGlobalMarker(sPos[0], sPos[1], sPos[2]);
					else
						pPlayer->HideGlobalMarker();
				}
			}
		}
	}
}

/*
void CNetGame::Packet_VoiceChannelOpenReply(Packet* pkt)
{
	Log("Packet_VoiceChannelOpenReply");

	if (!pVoice || !GetGameState() == GAMESTATE_CONNECTED) return;

	GetPlayerPool()->GetLocalPlayer()->VoiceChannelAccept();
}*/

/*
void CNetGame::Packet_VoiceData(Packet* pkt)
{
	if (!pVoice || !GetGameState() == GAMESTATE_CONNECTED) return;

	PLAYERID playerId;
	int size;
	unsigned char data[MAX_VOICE_PACKET_SIZE];
	RakNet::BitStream bsData(pkt->data, pkt->length, false);
	bsData.IgnoreBits(8);
	bsData.Read(playerId);
	bsData.Read(size);
	bsData.Read((char*)data, size);

	if (GetPlayerPool()->GetSlotState(playerId)) {
		pVoice->Push(playerId, data, size);
	}
}*/

// 0.3.7
void CNetGame::UpdatePlayerScoresAndPings()
{
	static uint32_t dwLastUpdateTick = 0;

	if (GetTickCount() - dwLastUpdateTick > 3000) {
		dwLastUpdateTick = GetTickCount();

		RakNet::BitStream bsSend;
		m_pRakClient->RPC(&RPC_UpdateScoresPingsIPs, &bsSend, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, nullptr);
	}
}

void CNetGame::SendDialogResponse(uint16_t wDialogID, uint8_t byteButtonID, uint16_t wListBoxItem, const char* szInput)
{
	if (GetGameState() != GAMESTATE_CONNECTED) return;

	uint8_t length = strlen(szInput);

	RakNet::BitStream bsSend;
	bsSend.Write(wDialogID);
	bsSend.Write(byteButtonID);
	bsSend.Write(wListBoxItem);
	bsSend.Write(length);
	bsSend.Write(szInput, length);
	m_pRakClient->RPC(&RPC_DialogResponse, &bsSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false, UNASSIGNED_NETWORK_ID, nullptr);
}

void CNetGame::SendChatMessage(const char* szMsg)
{
	if (GetGameState() != GAMESTATE_CONNECTED) return;

	RakNet::BitStream bsSend;
	uint8_t byteTextLen = strlen(szMsg);

	bsSend.Write(byteTextLen);
	bsSend.Write(szMsg, byteTextLen);

	m_pRakClient->RPC(&RPC_Chat, &bsSend, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, NULL);
}

bool IsCustomCommands(const char* str);
void CNetGame::SendChatCommand(const char* szCommand)
{
	if (GetGameState() != GAMESTATE_CONNECTED)
        return;

    if (IsCustomCommands(szCommand))
        return;

	RakNet::BitStream bsParams;
	int iStrlen = strlen(szCommand);

	bsParams.Write(iStrlen);
	bsParams.Write(szCommand, iStrlen);
	m_pRakClient->RPC(&RPC_ServerCommand, &bsParams, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, NULL);
}
// 0.3.7
void CNetGame::SetMapIcon(uint8_t byteIconID, float fPosX, float fPosY, float fPosZ, uint8_t byteType, uint32_t dwColor, uint8_t byteStyle)
{
	if (m_dwMapIcon[byteIconID] != 0) {
		DisableMapIcon(byteIconID);
	}

	m_dwMapIcon[byteIconID] = pGame->CreateRadarMarkerIcon(byteType, fPosX, fPosY, fPosZ, dwColor, byteStyle);
}
// 0.3.7
void CNetGame::DisableMapIcon(uint8_t byteIconID)
{
	ScriptCommand(&disable_marker, m_dwMapIcon[byteIconID]);
	m_dwMapIcon[byteIconID] = 0;
}
// 0.3.7
void CNetGame::ResetVehiclePool()
{
	if (m_pPools->pVehiclePool) {
		delete m_pPools->pVehiclePool;
	}

	m_pPools->pVehiclePool = new CVehiclePool();
}
// 0.3.7
void CNetGame::ResetActorPool()
{
	if (m_pPools->pActorPool) {
		delete m_pPools->pActorPool;
	}

	m_pPools->pActorPool = new CActorPool();
}
// 0.3.7
void CNetGame::ResetTextDrawPool()
{
	if (m_pPools->pTextDrawPool) {
		delete m_pPools->pTextDrawPool;
	}

	m_pPools->pTextDrawPool = new CTextDrawPool();
}
// 0.3.7
void CNetGame::ResetGangZonePool()
{
	if (m_pPools->pGangZonePool) {
		delete m_pPools->pGangZonePool;
	}

	m_pPools->pGangZonePool = new CGangZonePool();
}
// 0.3.7
void CNetGame::Reset3DTextLabelPool()
{
	if (m_pPools->pTextLabelPool) {
		delete m_pPools->pTextLabelPool;
	}

	m_pPools->pTextLabelPool = new C3DTextLabelPool();
}
// 0.3.7
void CNetGame::ResetMapIcons()
{
	for (int i = 0; i < MAX_MAP_ICONS; i++)
	{
		if (m_dwMapIcon[i]) {
			ScriptCommand(&disable_marker, m_dwMapIcon[i]);
			m_dwMapIcon[i] = 0;
		}
	}
}
// 0.3.7
void CNetGame::ResetPickupPool()
{
	if (m_pPools->pPickupPool) {
		delete m_pPools->pPickupPool;
	}

	m_pPools->pPickupPool = new CPickupPool();
}
// 0.3.7
void CNetGame::ResetObjectPool()
{
	if (m_pPools->pObjectPool) {
		delete m_pPools->pObjectPool;
	}

	m_pPools->pObjectPool = new CObjectPool();
}
// 0.3.7
void CNetGame::ResetMenuPool()
{
	if (m_pPools->pMenuPool) {
		delete m_pPools->pMenuPool;
	}

	m_pPools->pMenuPool = new CMenuPool();
}
// 0.3.7
void CNetGame::InitGameLogic()
{
	m_pNetSet->fWorldBounds[0] = 20000.0f;
	m_pNetSet->fWorldBounds[1] = -20000.0f;
	m_pNetSet->fWorldBounds[2] = 20000.0f;
	m_pNetSet->fWorldBounds[3] = -20000.0f;
}

void CNetGame::SendPrevClass()
{
	CPlayerPool* pPlayerPool = GetPlayerPool();
	if (pPlayerPool)
	{
		pPlayerPool->GetLocalPlayer()->SendPrevClass();
	}
}

void CNetGame::SendSpawn()
{
	CPlayerPool* pPlayerPool = GetPlayerPool();
	if (pPlayerPool)
	{
		pPlayerPool->GetLocalPlayer()->SendSpawn();
	}
}

void CNetGame::SendNextClass()
{
	CPlayerPool* pPlayerPool = GetPlayerPool();
	if (pPlayerPool)
	{
		pPlayerPool->GetLocalPlayer()->SendNextClass();
	}
}

void CNetGame::Packet_TrailerSync(Packet *pkt)
{
	if(GetGameState() != GAMESTATE_CONNECTED) return;
	
	uint8_t bytePacketId;
	PLAYERID playerId;

	TRAILER_SYNC_DATA trSync;
	memset(&trSync, 0, sizeof(TRAILER_SYNC_DATA));

	RakNet::BitStream bsTrailerSync((unsigned char *)pkt->data, pkt->length, false);
	bsTrailerSync.Read(bytePacketId);
	bsTrailerSync.Read(playerId);
	bsTrailerSync.Read((char*)&trSync, sizeof(TRAILER_SYNC_DATA));

	if(GetPlayerPool()) 
	{
		CRemotePlayer *pPlayer = GetPlayerPool()->GetAt(playerId);
		if(pPlayer)
			pPlayer->StoreTrailerFullSyncData(&trSync);
	}
}

void CNetGame::Packet_UnoccupiedSync(Packet *pkt)
{
	if(m_iGameState != GAMESTATE_CONNECTED) return;

	uint8_t bytePacketId;
	PLAYERID playerId;

	UNOCCUPIED_SYNC_DATA unocSync;
	memset(&unocSync, 0, sizeof(UNOCCUPIED_SYNC_DATA));

	RakNet::BitStream bsUnocSync((unsigned char *)pkt->data, pkt->length, false);
	bsUnocSync.Read(bytePacketId);
	bsUnocSync.Read(playerId);
	bsUnocSync.Read((char*)&unocSync,sizeof(UNOCCUPIED_SYNC_DATA));

	if(GetPlayerPool())
	{
		CRemotePlayer *pPlayer = GetPlayerPool()->GetAt(playerId);
		if(pPlayer)
			pPlayer->StoreUnoccupiedSyncData(&unocSync);
	}
}