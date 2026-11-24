//
// Created by weikton on 11.04.2023.
//
#include "VehicleModelInfo.h"
#include "../vendor/armhook/patch.h"

#include "../RW/rpworld.h"
#include "ModelInfo.h"

#include "game/CCustomPlateManager.h"
#include "game/VisibilityPlugins.h"
#include "util/CUtil.h"
#include "game/Textures/TextureDatabaseRuntime.h"

void CVehicleModelInfo::CVehicleModelInfo__SetClump(RpClump* clump)
{
    m_pVehicleStruct = new CVehicleStructure();
    CClumpModelInfo__SetClump(clump);
    SetAtomicRenderCallbacks();

    RwObjectNameIdAssocation* (&ms_vehicleDescs)[NUM_VEHICLE_MODEL_DESCS] = *(RwObjectNameIdAssocation*(*)[NUM_VEHICLE_MODEL_DESCS])(g_libGTASA + (VER_x32 ? 0x00687C7C : 0x8614C0));
    CClumpModelInfo::SetFrameIds(ms_vehicleDescs[m_nVehicleType]);
    SetRenderPipelines();
    PreprocessHierarchy();
    ReduceMaterialsInVehicle();
}

void CVehicleModelInfo::CVehicleModelInfo__DeleteRwObject()
{
    delete m_pVehicleStruct;
    m_pVehicleStruct = nullptr;
    CClumpModelInfo__DeleteRwObject();
}

void CVehicleModelInfo::SetAtomicRenderCallbacks()
{
   CHook::CallFunction<void>("_ZN17CVehicleModelInfo24SetAtomicRenderCallbacksEv", this);
}

CVehicleModelInfo::CVehicleStructure::CVehicleStructure() : m_aUpgrades()
{
    for (auto& vecPos : m_avDummyPos)
        vecPos.Set(0.0F, 0.0F, 0.0F);

    for (auto& upgrade : m_aUpgrades)
        upgrade.m_nParentComponentId = -1;

    memset(m_apExtras, 0, sizeof(m_apExtras));
    m_nNumExtras = 0;
    m_nMaskComponentsDamagable = 0;
}

CVehicleModelInfo::CVehicleStructure::~CVehicleStructure()
{
    for (int32 i = 0; i < m_nNumExtras; ++i) {
        auto atomic = m_apExtras[i];
        auto frame = RpAtomicGetFrame(atomic);
        RpAtomicDestroy(atomic);
        RwFrameDestroy(frame);
    }
}

void CVehicleModelInfo::ReduceMaterialsInVehicle()
{
   CHook::CallFunction<void>("_ZN17CVehicleModelInfo24ReduceMaterialsInVehicleEv", this);
}

void CVehicleModelInfo::PreprocessHierarchy()
{
   CHook::CallFunction<void>("_ZN17CVehicleModelInfo19PreprocessHierarchyEv", this);
}

void CVehicleModelInfo::SetRenderPipelines()
{
    CHook::CallFunction<void>("_ZN17CVehicleModelInfo18SetRenderPipelinesEv", this);
}

void* CVehicleModelInfo::CVehicleStructure::operator new(size_t size)
{
    return m_pInfoPool->New();
}

void CVehicleModelInfo::CVehicleStructure::operator delete(void* data)
{
    m_pInfoPool->Delete(reinterpret_cast<CVehicleStructure*>(data));
}

RpMaterial* RemoveWindowAlphaCB(RpMaterial* mat, void* data)
{
    if (mat->color.alpha != 255) {
        AddStoredMaterial(RpMaterialGetColor(mat));

        mat->color = {0, 0, 0, 0};
    }
    return mat;
}

RpMaterial* HasTonerMaterialCB(RpMaterial* mat, void* data)
{
    if (!mat->texture) {
        return mat;
    }

    if (!strcmp(mat->texture->name, "remaptoner") || !strcmp(mat->texture->name, "remap_toner")) {
        *(bool*)data = true;
    }

    return mat;
}

RpAtomic* CVehicleModelInfo::SetEditableMaterialsCB(RpAtomic* atomic, void* data)
{
    if (rwObjectTestFlags(atomic, RpAtomicFlag::rpATOMICRENDER) == 0)
        return atomic;

    auto pVeh = (CVehicle*)data;

    // Альфа-проход назначается один раз при загрузке модели, а в dff тонировка
    // непрозрачная. Помечаем только тот атомик, где она реально есть, иначе в
    // альфа-проход уезжает весь кузов и машина просвечивает насквозь.
    if (pVeh && pVeh->bTonerColorSet && pVeh->tonerColor.a != 255) {
        bool bHasToner = false;
        RpGeometryForAllMaterials(RpAtomicGetGeometry(atomic), HasTonerMaterialCB, &bHasToner);

        if (bHasToner) {
            CVisibilityPlugins::SetAtomicFlag(atomic, eAtomicComponentFlag::ATOMIC_HAS_ALPHA);
        }
    }

    if (CVisibilityPlugins::GetAtomicId(atomic) & eAtomicComponentFlag::ATOMIC_IS_DOOR_WINDOW_OPENED)
        RpGeometryForAllMaterials(RpAtomicGetGeometry(atomic), RemoveWindowAlphaCB, data);

    RpGeometryForAllMaterials(RpAtomicGetGeometry(atomic), SetEditableMaterialsCB, data);
    return atomic;
}

RpMaterial* CVehicleModelInfo::SetEditableMaterialsCB(RpMaterial* mat, void* data)
{
    auto pVeh = (CVehicle*)data;
    int color = *(int*)&mat->color;

    // Log("VEH COLOR INT: %d", color);
    // VEH COLOR INT: -16777216
    // VEH COLOR INT: -3355444
    // VEH COLOR INT: -5308161
    // VEH COLOR INT: -3355444

    if(pVeh->m_pVehicle->IsBMX() || pVeh->m_pVehicle->IsBoat() || pVeh->m_pVehicle->IsHeli() || pVeh->m_pVehicle->IsMonsterTruck() || pVeh->m_pVehicle->IsPlane()
       || pVeh->m_pVehicle->IsRCVehicleModelID() || pVeh->m_pVehicle->IsQuad() || pVeh->m_pVehicle->IsTrailer()) {
        return mat;
    }

    if (color == 0xff00ff3c) {
        AddStoredMaterial(RpMaterialGetColor(mat));
        if (pVeh->m_pVinylTex) {
            AddStoredMaterial(&mat->texture);
            mat->texture = pVeh->m_pVinylTex;
            mat->color = {255, 255, 255, 255};

            return mat;
        }

        mat->color = { pVeh->mainColor.r, pVeh->mainColor.g, pVeh->mainColor.b, 255 };
        return mat;
    }
    if (color == 0xffaf00ff) {
        AddStoredMaterial(RpMaterialGetColor(mat));

        mat->color = { pVeh->secondColor.r, pVeh->secondColor.g, pVeh->secondColor.b, 255 };
        return mat;
    }
    if (color == 0xff00647d) {
        AddStoredMaterial(RpMaterialGetColor(mat));

        mat->color = { pVeh->wheelColor.r, pVeh->wheelColor.g, pVeh->wheelColor.b, 255 };
        return mat;
    }
    if(mat->texture) {
        if (!strcmp(mat->texture->name, "remaptoner") || !strcmp(mat->texture->name, "remap_toner")) {
            if (!pVeh->bTonerColorSet) {
                return mat;
            }

            AddStoredMaterial(RpMaterialGetColor(mat));

            mat->color = pVeh->tonerColor;

            return mat;
        }

        if (!strcmp(mat->texture->name, "plate_remap")) {
            AddStoredMaterial(&mat->texture);

            if (pVeh->pPlateTexture) {
                mat->texture = pVeh->pPlateTexture;
            } else {
                mat->texture = CCustomPlateManager::GetNoPlateTex();
            }
            mat->color.alpha = 255;

            return mat;
        }

        if (!strcmp(mat->texture->name, "v_t_left") || !strcmp(mat->texture->name, "v_t_right")) {
            const bool bLeft = (mat->texture->name[4] == 'l');
            const eTurnState blink = pVeh->GetTurnLightsBlinkState();

            const bool bLit = (blink == eTurnState::TURN_ALL) ||
                              (bLeft ? blink == eTurnState::TURN_LEFT
                                     : blink == eTurnState::TURN_RIGHT);

            AddStoredMaterial(RpMaterialGetColor(mat));
            AddStoredMaterial(RpMaterialGetSurfaceProperties(mat));

            if (bLit) {
                mat->color = { 255, 255, 255, 255 };
                RpMaterialSetSurfaceProperties(mat, &gLightSurfProps);
            } else {
                mat->color = { 70, 40, 10, 255 };
                RpMaterialSetSurfaceProperties(mat, &gLightOffSurfProps);
            }

            return mat;
        }

        if (RpMaterialGetTexture(mat) == ms_pLightsTexture) {
            int32 iLightIndex = -1;
            if (color == CRGBA(255, 175, 0, 0))
                iLightIndex = 0;
            else if (color == CRGBA(0, 255, 200, 0))
                iLightIndex = 1;
            else if (color == CRGBA(185, 255, 0, 0))
                iLightIndex = 2;
            else if (color == CRGBA(255, 60, 0, 0))
                iLightIndex = 3;

            AddStoredMaterial(RpMaterialGetColor(mat));

            mat->color = pVeh->lightColor;

            if (iLightIndex != -1 && ms_lightsOn[iLightIndex]) {
                AddStoredMaterial(&mat->texture);
                mat->texture = ms_pLightsOnTexture;

                AddStoredMaterial(RpMaterialGetSurfaceProperties(mat));
                RpMaterialSetSurfaceProperties(mat, &gLightSurfProps);
            }
        }
    }

    return mat;
}

#include "net/netgame.h"
#include "game/vehicle.h"
extern CNetGame *pNetGame;

void CVehicleModelInfo::SetEditableMaterials(RpClump* clump)
{
    if (clump)
    {
        CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();

        auto vehId = pVehiclePool->FindIDFromRwObject(&clump->object);
        if(vehId == INVALID_VEHICLE_ID) {
            return;
        }

        auto pVehicle = pVehiclePool->GetAt(vehId);
        if (pVehicle) {
            RpClumpForAllAtomics(clump, SetEditableMaterialsCB, pVehicle);
        }
    }
}

void CVehicleModelInfo::ResetEditableMaterials(RpClump* clump)
{
    for (auto& stored : gStoredMaterials)
    {
        memcpy(stored.address, stored.bytes, stored.size);
    }
    gStoredMaterials.clear();
}

void CVehicleModelInfo__SetClump_hook(CVehicleModelInfo* thiz, RpClump* clump) {
    thiz->CVehicleModelInfo__SetClump(clump);
}

void CVehicleModelInfo__DeleteRwObject_hook(CVehicleModelInfo* thiz) {
    thiz->CVehicleModelInfo__DeleteRwObject();
}

void CVehicleModelInfo::InjectHooks() {
    CHook::Write(g_libGTASA + (VER_x32 ? 0x679D00 : 0x851A18), &ms_pLightsTexture);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x67820C : 0x84E440), &ms_pLightsOnTexture);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x676CD4 : 0x84BA08), &ms_lightsOn);

    CHook::InstallPLT(g_libGTASA + (VER_x32 ? 0x66E910 : 0x83DEE0), &SetEditableMaterials);
    CHook::InstallPLT(g_libGTASA + (VER_x32 ? 0x675E30 : 0x849D10), &ResetEditableMaterials);

    // CVehicleStructure::m_pInfoPool = new CPool<CVehicleStructure>(CModelInfo::NUM_VEHICLE_MODEL_INFOS, "VehicleStruct");

    // CHook::Redirect("_ZN17CVehicleModelInfo8SetClumpEP7RpClump", &CVehicleModelInfo__SetClump_hook);
    // CHook::Redirect("_ZN17CVehicleModelInfo14DeleteRwObjectEv", &CVehicleModelInfo__DeleteRwObject_hook);
}