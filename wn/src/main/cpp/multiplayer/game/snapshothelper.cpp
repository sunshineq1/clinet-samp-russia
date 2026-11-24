#include "../main.h"
#include "game.h"
#include "RW/RenderWare.h"
#include "Streaming.h"
#include "Scene.h"
#include "VisibilityPlugins.h"
#include "game/Models/ModelInfo.h"
#include "game/Plugins/RpAnimBlendPlugin/RpAnimBlend.h"
#include "CRenderTarget.h"
#include "sprite2d.h"
#include "rgba.h"
#include "Textures/TextureDatabaseRuntime.h"
#include "../net/netgame.h"
#include <GLES2/gl2.h>

// Размер снимка как в оригинале: плитка витрины растягивает картинку
// на всю ячейку, и меньшее разрешение видно как мыло.
static constexpr RwInt32 SNAPSHOT_SIZE = 512;

static constexpr float VIEW_WINDOW = 0.5f;
static constexpr float PED_VIEW_WIDTH = 0.25f;
static constexpr float PED_VIEW_HEIGHT = 0.5f;

extern CGame* pGame;
extern CNetGame* pNetGame;

CSnapShotHelper::CSnapShotHelper()
{
	m_camera = 0;
	m_light = 0;
	m_frame = 0;
	m_zBuffer = 0;
	m_raster = 0;

	SetUpScene();
}

void CSnapShotHelper::SetUpScene()
{
	// RpLightCreate
	m_light = RpLightCreate(2);
	if (m_light == 0) return;
	float rwColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	// RpLightSetColor
    RpLightSetColor(m_light, reinterpret_cast<const RwRGBAReal *>(rwColor));

	m_zBuffer = RwRasterCreate(256, 256, 0, rwRASTERTYPEZBUFFER);
	m_camera = RwCameraCreate();
	m_frame = RwFrameCreate();
	CVector v = { 0.0f, 0.0f, 50.0f };
	RwFrameTranslate(m_frame, &v, rwCOMBINEPRECONCAT);
    CVector v1 = { 1.0f, 0.0f, 0.0f };
	RwFrameRotate(m_frame, &v1, 90.0f, rwCOMBINEPRECONCAT);

	if (!m_camera) return;
	if (!m_frame) return;

	m_camera->zBuffer = m_zBuffer;

    _rwObjectHasFrameSetFrame(m_camera, m_frame);
    RwCameraSetFarClipPlane(m_camera, 300.0f);
    RwCameraSetNearClipPlane(m_camera, 0.01f);
	RwV2d view = { 0.5f, 0.5f };
    RwCameraSetViewWindow(m_camera, &view);
    RwCameraSetProjection(m_camera, rwPERSPECTIVE);
    RpWorldAddCamera(Scene.m_pRpWorld, m_camera);
}

// 0.3.7
RwTexture* CSnapShotHelper::CreateObjectSnapShot(int iModel, uint32_t dwColor, CVector* vecRot, float fZoom)
{
#ifdef BR_DEBUG
    Log("CreateObjectSnapShot: %d, %f, %f, %f", iModel, vecRot->x, vecRot->y, vecRot->z);
#endif
    // модель к этому моменту уже загружена очередью снимков: блокирующая
    // загрузка здесь вешала игровой поток на секунды
    if (!CStreaming::IsModelLoaded(iModel))
    {
        CStreaming::RequestModel(iModel, STREAMING_GAME_REQUIRED | STREAMING_KEEP_IN_MEMORY);
        return nullptr;
    }

    auto pRwObject = ModelInfoCreateInstance(iModel);
    if(pRwObject == nullptr)
    {
        Log("pRwObject = null");
        return nullptr;
    }

    float fRadius = CModelInfo::GetModelInfo(iModel)->m_pColModel->GetBoundRadius();
    CVector vecCenter = CModelInfo::GetModelInfo(iModel)->m_pColModel->GetBoundCenter();

    auto parent = (RwFrame*)pRwObject->parent;

    if (parent == nullptr)
        return nullptr;

    RwV3d v = {
            -vecCenter.x,
            (-0.1f - (fRadius * 2.25f)) * fZoom,
            50.0f - vecCenter.z
    };
    RwFrameTranslate(parent, &v, rwCOMBINEPRECONCAT);
    if (vecRot->x != 0.0f)
    {
        v = {1.0f, 0.f, 0.f};
        RwFrameRotate(parent, &v, vecRot->x, rwCOMBINEPRECONCAT);
    }

    if (vecRot->y != 0.0f)
    {
        v = {0.0f, 1.f, 0.f};
        RwFrameRotate(parent, &v, vecRot->y, rwCOMBINEPRECONCAT);
    }

    if (vecRot->z != 0.0f)
    {
        v = {0.0f, 0.f, 1.f};
        RwFrameRotate(parent, &v, vecRot->z, rwCOMBINEPRECONCAT);
    }

    CRenderTarget::Begin(SNAPSHOT_SIZE, SNAPSHOT_SIZE, (RwRGBA*)&dwColor, false);
    RenderClumpOrAtomic((uintptr_t)pRwObject);

    DestroyAtomicOrClump(reinterpret_cast<uintptr_t>(pRwObject));

    CStreaming::RemoveModelIfNoRefs(iModel);
    return CRenderTarget::End();
}
// 0.3.7
RwTexture* CSnapShotHelper::CreatePedSnapShot(int iModel, uint32_t dwColor, CVector* vecRot, float fZoom)
{
	/*Log("Ped snapshot: %d", iModel);

	RwRaster* raster = RwRasterCreate(256, 256, 32, rwRASTERFORMAT8888 | rwRASTERTYPECAMERATEXTURE);
	// RwTextureCreate
	RwTexture* bufferTexture = RwTextureCreate(raster);

	CPlayerPed* pPed = new CPlayerPed(208, 0, 0.0f, 0.0f, 0.0f, 0.0f);

	if (!raster || !bufferTexture || !pPed) return 0;

	float posZ = iModel == 162 ? 50.15f : 50.05f;
	float posY = fZoom * -2.25f;
	pPed->m_pPed->SetPosn(0.0f, posY, posZ);
	pPed->SetModelIndex(iModel);
	//pPed->m_pPed->SetGravityProcessing(false);
	pPed->m_pPed->SetCollisionChecking(false);

	RwMatrix mat = pPed->m_pPed->GetMatrix().ToRwMatrix();

    CVector axis { 1.0f, 0.0f, 0.0f };
    if (vecRot->x != 0.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->x);
    }
    axis.Set( 0.0f, 1.0f, 0.0f );
    if (vecRot->y != 0.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->y);
    }
    axis.Set( 0.0f, 0.0f, 1.0f );
    if (vecRot->z != 0.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->z);
    }

	pPed->m_pPed->SetMatrix((CMatrix&)mat);

	// set camera frame buffer //
    m_camera->frameBuffer = raster;
	// CVisibilityPlugins::SetRenderWareCamera
    CVisibilityPlugins::SetRenderWareCamera(m_camera);

    RwCameraClear(m_camera, reinterpret_cast<RwRGBA *>(&dwColor), 3);
	RwCameraBeginUpdate((RwCamera*)m_camera);
	RpWorldAddLight(Scene.m_pRpWorld, m_light);

	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
	RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)false);

    DefinedState();

	pPed->m_pPed->Add();

	RpAnimBlendClumpUpdateAnimations(pPed->m_pPed->m_pRwClump, 100.0f, 1);
    RenderEntity(pPed->m_pPed);

	RwCameraEndUpdate((RwCamera*)m_camera);

	RpWorldRemoveLight(Scene.m_pRpWorld, m_light);

	pPed->m_pPed->Remove();

	delete pPed;

    CStreaming::RemoveModelIfNoRefs(iModel);

	return bufferTexture;*/
    Log("CreatePedSnapShot: %d, %f, %f, %f", iModel, vecRot->x, vecRot->y, vecRot->z);

    float posZ = 49.75f;
    // При -2.20 макушка упиралась в край кадра, поэтому отодвигаем чуть дальше
    float posY = fZoom * -2.60f;

    auto pPed = new CPlayerPed(208, iModel, 0.0f, posY, posZ, 0.0f);

    pPed->m_pPed->SetCollisionChecking(false);

    RwMatrix mat;
    pPed->m_pPed->GetMatrix(&mat);

    CVector axis { 1.0f, 0.0f, 0.0f };
    if (vecRot->x != 0.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->x);
    }
    axis.Set( 0.0f, 1.0f, 0.0f );
    if (vecRot->y != 0.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->y);
    }
    axis.Set( 0.0f, 0.0f, 1.0f );
    if (vecRot->z != 0.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->z);
    }

    pPed->m_pPed->SetMatrix((CMatrix&)mat);
    pPed->m_pPed->UpdateRW();

    // Пед узкий и высокий: в квадратном кадре он занимает меньше четверти
    // ширины, и растянутый на плитку выглядит мылом. Сужаем окно обзора и
    // кадр в той же пропорции - картинка та же, но пикселей на педа вдвое
    // больше.
    CRenderTarget::SetViewWindow(PED_VIEW_WIDTH, PED_VIEW_HEIGHT);
    CRenderTarget::Begin(SNAPSHOT_SIZE / 2, SNAPSHOT_SIZE, (RwRGBA*)&dwColor, false);

    RpAnimBlendClumpUpdateAnimations(pPed->m_pPed->m_pRwClump, 100.f, true);

    RenderEntity(pPed->m_pPed);

    delete pPed;

    auto tex = CRenderTarget::End();

    CRenderTarget::SetViewWindow(VIEW_WINDOW, VIEW_WINDOW);

    return tex;
}

RwTexture* CSnapShotHelper::CreateVehicleSnapShot(int iModel, uint32_t dwColor, CVector* vecRot, float fZoom, uint32_t dwColor1, uint32_t dwColor2)
{
	// Чужой номер уйдёт в CGame::NewVehicle за границу таблицы предзагрузки
	if (iModel < 400 || iModel > 611) {
		return 0;
	}

	if (iModel == 570) {
		iModel = 538;
	}
	else if (iModel == 569) {
		iModel = 537;
	}

	// Перекраска ищет машину в пуле по её RW-объекту: созданная в обход пула
	// остаётся с сырыми материалами модели, то есть зелёной и розовой.
	CVehiclePool* pVehiclePool = pNetGame ? pNetGame->GetVehiclePool() : nullptr;
	if (!pVehiclePool) {
		return 0;
	}

	VEHICLEID snapId = INVALID_VEHICLE_ID;

	for (int slot = MAX_VEHICLES - 1; slot >= 0; slot--) {
		if (!pVehiclePool->GetSlotState(slot)) {
			snapId = (VEHICLEID)slot;
			break;
		}
	}

	if (snapId == INVALID_VEHICLE_ID) {
		return 0;
	}

	NEW_VEHICLE snapVehicle;
	memset(&snapVehicle, 0, sizeof(snapVehicle));

	snapVehicle.VehicleID          = snapId;
	snapVehicle.iVehicleType       = iModel;
	snapVehicle.vecPos             = CVector(0.0f, 0.0f, 50.0f);
	snapVehicle.fRotation          = 0.0f;
	snapVehicle.byteInteriorColor1 = 0xFF;
	snapVehicle.byteInreriorColor2 = 0xFF;
	snapVehicle.fHealth            = 1000.0f;

	if (!pVehiclePool->New(&snapVehicle)) {
		return 0;
	}

	CVehicle* pVehicle = pVehiclePool->GetAt(snapId);

	if (!pVehicle || !pVehicle->m_pVehicle) {
		pVehiclePool->Delete(snapId);
		return 0;
	}

	pVehicle->m_pVehicle->SetCollisionChecking(false);
	float radius = CModelInfo::GetModelInfo(iModel)->m_pColModel->GetBoundRadius();
	float posY = (-1.0f - (radius + radius)) * fZoom;
	if (pVehicle->GetVehicleSubtype() == VEHICLE_SUBTYPE_BOAT) {
		posY = -5.5f - radius * 2.5f;
	}
	pVehicle->m_pVehicle->SetPosn(0.0f, posY, 50.0f);
	pVehicle->mainColor = CRGBA(
		(uint8_t)((dwColor1 >> 16) & 0xFF),
		(uint8_t)((dwColor1 >> 8) & 0xFF),
		(uint8_t)(dwColor1 & 0xFF), 255);

	pVehicle->secondColor = CRGBA(
		(uint8_t)((dwColor2 >> 16) & 0xFF),
		(uint8_t)((dwColor2 >> 8) & 0xFF),
		(uint8_t)(dwColor2 & 0xFF), 255);

	pVehicle->bTonerColorSet = false;
	RwMatrix mat = pVehicle->m_pVehicle->GetMatrix().ToRwMatrix();

    CVector axis { 1.0f, 0.0f, 0.0f };
    if (vecRot->x != 0.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->x);
    }
    axis.Set( 0.0f, 1.0f, 0.0f );
    if (vecRot->y != 0.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->y);
    }
    axis.Set( 0.0f, 0.0f, 1.0f );
    if (vecRot->z != 0.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->z);
    }

	pVehicle->m_pVehicle->SetMatrix((CMatrix&)mat);
    pVehicle->m_pVehicle->UpdateRW();

    CRenderTarget::Begin(SNAPSHOT_SIZE, SNAPSHOT_SIZE, (RwRGBA*)&dwColor, false);

    RenderEntity(pVehicle->m_pVehicle);

	pVehiclePool->Delete(snapId);

	return CRenderTarget::End();
}

// Винил - плоская текстура, ей не нужны ни сцена, ни свет: хватает 2D-квадрата
// в рендер-таргете, как у номерных знаков.
RwTexture* CSnapShotHelper::CreateVinylSnapShot(const char* pszVinylName, int iSize)
{
	if (!pszVinylName || !*pszVinylName || iSize <= 0) {
		return nullptr;
	}

	RwTexture* pVinylTex = TextureDatabaseRuntime::GetTexture(pszVinylName);

	if (!pVinylTex || !pVinylTex->raster) {
		return nullptr;
	}

	CSprite2d sprite;
	sprite.m_pTexture = pVinylTex;

	CRGBA white { 255, 255, 255, 255 };
	RwRGBA background { 0, 0, 0, 0 };

	CRenderTarget::Begin(iSize, iSize, &background, true);
	sprite.Draw(0.0f, 0.0f, (float)iSize, (float)iSize, &white);

	RwTexture* pResult = CRenderTarget::End();

	// текстура принадлежит базе текстур, деструктор спрайта уничтожил бы её
	sprite.m_pTexture = nullptr;

	return pResult;
}
