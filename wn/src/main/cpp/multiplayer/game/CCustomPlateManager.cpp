#include "CCustomPlateManager.h"

#include "main.h"
#include "RW/RenderWare.h"
#include "game.h"
#include "sprite2d.h"
#include "net/netgame.h"
#include "util/CUtil.h"
#include "gui/gui.h"

#include <jni.h>
#include <GLES3/gl32.h>

extern CNetGame* pNetGame;
extern UI* pUI;

RwTexture* CCustomPlateManager::createTexture(ePlateType type, std::string& szNumber, std::string& szRegion)
{
	switch(type) {
		case NUMBERPLATE_TYPE_UA: {
			return createUaPlate(szNumber, szRegion);
		}
		case NUMBERPLATE_TYPE_KZ: {
			return createKzPlate(szNumber, szRegion);
		}
		case NUMBERPLATE_TYPE_BY: {
			return createBuPlate(szNumber, szRegion);
		}
		case NUMBERPLATE_TYPE_RUS: {
			return createRuPlate(szNumber, szRegion);
		}
		case NUMBERPLATE_TYPE_RU_POLICE: {
			return createRuPolicePlate(szNumber, szRegion);
		}
        case NUMBERPLATE_TYPE_GOS: {
			return createVoenPlate(szNumber, szRegion);
		}

		default: {
			return nullptr;
		}
	}
}

#include "Textures/TextureDatabaseRuntime.h"
#include "Models/VehicleModelInfo.h"
void CCustomPlateManager::Initialise()
{
    Log("CCustomPlateManager init");
    ImGuiIO &io = ImGui::GetIO();

    Log("CCustomPlateManager init sprite");
	m_pUaSprite = new CSprite2d();
	m_pRuSprite = new CSprite2d();
	m_pBuSprite = new CSprite2d();
	m_pKzSprite = new CSprite2d();
	m_pRuPoliceSprite = new CSprite2d();
    m_pVoenSprite = new CSprite2d();

    Log("CCustomPlateManager load textures");
    pNoPlateTex = TextureDatabaseRuntime::GetTexture("plate_remap");
    m_pRuSprite->m_pTexture = TextureDatabaseRuntime::GetTexture("plate_ru");
	m_pUaSprite->m_pTexture = TextureDatabaseRuntime::GetTexture("plate_ua");
	m_pBuSprite->m_pTexture = TextureDatabaseRuntime::GetTexture("plate_by");
	m_pKzSprite->m_pTexture = TextureDatabaseRuntime::GetTexture("plate_kz");
	m_pRuPoliceSprite->m_pTexture = TextureDatabaseRuntime::GetTexture("plate_pol");
    m_pVoenSprite->m_pTexture = TextureDatabaseRuntime::GetTexture("plate_voen");
}

void CCustomPlateManager::Shutdown()
{
	if (m_pRuSprite)
	{
		delete m_pRuSprite;
		m_pRuSprite = nullptr;
	}

	if (m_pUaSprite)
	{
		delete m_pUaSprite;
		m_pUaSprite = nullptr;
	}

	if (m_pBuSprite)
	{
		delete m_pBuSprite;
		m_pBuSprite = nullptr;
	}

	if (m_pKzSprite)
	{
		delete m_pKzSprite;
		m_pKzSprite = nullptr;
	}

	if (m_pRuPoliceSprite)
	{
		delete m_pRuPoliceSprite;
		m_pRuPoliceSprite = nullptr;
	}

    if (m_pVoenSprite)
	{
		delete m_pVoenSprite;
		m_pVoenSprite = nullptr;
	}
}

RwTexture* CCustomPlateManager::createUaPlate(std::string& szNumber, std::string& szRegion)
{
	CRGBA white {255, 255, 255, 255};

	CRenderTarget::Begin(256, 64, white.ToRwRGBA(), true);
	m_pUaSprite->Draw(0.0f, 0.0f, 256.0f, 64.0f, &white);

	ImGui::NewFrame();

	// num
	ImGui::GetBackgroundDrawList()->AddText(nullptr, 35.f, ImVec2(44, 10), ImColor(IM_COL32_BLACK), szNumber.c_str());

	ImGui::EndFrame();
	ImGui::Render();
	pUI->renderDrawData(ImGui::GetDrawData());

	RwTexture* pTexture = CRenderTarget::End();

	return pTexture;
}

RwTexture* CCustomPlateManager::createBuPlate(std::string& szNumber, std::string& szRegion)
{
	CRGBA white {255, 255, 255, 255};

	CRenderTarget::Begin(256, 64, white.ToRwRGBA(), true);
	m_pBuSprite->Draw(0.0f, 0.0f, 256.0f, 64.0f, &white);

	ImGui::NewFrame();

	// num
	ImGui::GetBackgroundDrawList()->AddText(nullptr, 40.f, ImVec2(42, 10), ImColor(IM_COL32_BLACK), szNumber.c_str());

	ImGui::EndFrame();
	ImGui::Render();
	pUI->renderDrawData(ImGui::GetDrawData());

	RwTexture* pTexture = CRenderTarget::End();

	return pTexture;
}

RwTexture* CCustomPlateManager::createKzPlate(std::string& szNumber, std::string& szRegion)
{
	CRGBA white {255, 255, 255, 255};

	CRenderTarget::Begin(256, 64, white.ToRwRGBA(), true);
	m_pKzSprite->Draw(0.0f, 0.0f, 256.0f, 64.0f, &white);

	ImGui::NewFrame();

	// num
	ImGui::GetBackgroundDrawList()->AddText(nullptr, 36.f, ImVec2(40, 10), ImColor(IM_COL32_BLACK), szNumber.c_str(),
											nullptr, 160.f);

	// region
	ImGui::GetBackgroundDrawList()->AddText(nullptr, 35.f, ImVec2(201, 10), ImColor(IM_COL32_BLACK), szRegion.c_str());

	ImGui::EndFrame();
	ImGui::Render();
	pUI->renderDrawData(ImGui::GetDrawData());

	RwTexture* pTexture = CRenderTarget::End();

	return pTexture;
}

RwTexture* CCustomPlateManager::createRuPolicePlate(std::string& szNumber, std::string& szRegion)
{
	CRGBA white {255, 255, 255, 255};

	CRenderTarget::Begin(256, 64, white.ToRwRGBA(), true);
	m_pRuPoliceSprite->Draw(0.0f, 0.0f, 256.0f, 64.0f, &white);

	ImGui::NewFrame();

	// num
    ImGui::PushFont(m_pRuFont);

    float xPos = 17.0f;
    for (auto it = szNumber.begin(); it != szNumber.end(); ++it) {
        char c = *it;

        ImVec2 fontSize = std::isdigit(c) ? ImVec2(50.f, 3) : ImVec2(40.f, 11);
        ImVec2 textSize = ImGui::CalcTextSize(&c, &c + 1, false, fontSize.x);

        float endMargin = 7.0f;
        if (it != szNumber.end() - 1 && std::isdigit(c) && !std::isdigit(*(it + 1))) {
            endMargin += 6;
        }

        if (it != szNumber.end() - 1 && !std::isdigit(c) && std::isdigit(*(it + 1))) {
            endMargin += 6;
        }

        ImGui::GetBackgroundDrawList()->AddText(nullptr, fontSize.x + 25, ImVec2(xPos + 17, fontSize.y - 12.5), ImColor(IM_COL32_WHITE), &c, &c + 1);
        xPos += textSize.x + endMargin;
    }

	// region
    if(szRegion.length() == 2)
	    ImGui::GetBackgroundDrawList()->AddText(nullptr, 38.f, ImVec2(210, 3), ImColor(IM_COL32_WHITE), szRegion.c_str());
    else
        ImGui::GetBackgroundDrawList()->AddText(nullptr, 32.f, ImVec2(205, 3), ImColor(IM_COL32_WHITE), szRegion.c_str());

    ImGui::PopFont();
	ImGui::EndFrame();
	ImGui::Render();
	pUI->renderDrawData(ImGui::GetDrawData());

	RwTexture* pTexture = CRenderTarget::End();

	return pTexture;
}

RwTexture* CCustomPlateManager::createRuPlate(std::string& szNumber, std::string& szRegion)
{
	CRGBA white {255, 255, 255, 255};

	CRenderTarget::Begin(256, 64, white.ToRwRGBA(), true);
	m_pRuSprite->Draw(0.0f, 0.0f, 256.0f, 64.0f, &white);

	ImGui::NewFrame();

	// num
    ImGui::PushFont(m_pRuFont);

    float xPos = 17.0f;
    for (auto it = szNumber.begin(); it != szNumber.end(); ++it) {
        char c = *it;

        ImVec2 fontSize = std::isdigit(c) ? ImVec2(50.f, 3) : ImVec2(40.f, 11);
        ImVec2 textSize = ImGui::CalcTextSize(&c, &c + 1, false, fontSize.x);

        float endMargin = 7.0f;
        if (it != szNumber.end() - 1 && std::isdigit(c) && !std::isdigit(*(it + 1))) {
            endMargin += 6;
        }

        if (it != szNumber.end() - 1 && !std::isdigit(c) && std::isdigit(*(it + 1))) {
            endMargin += 6;
        }

        ImGui::GetBackgroundDrawList()->AddText(nullptr, fontSize.x + 25, ImVec2(xPos + 17, fontSize.y - 12.5), ImColor(IM_COL32_BLACK), &c, &c + 1);
        xPos += textSize.x + endMargin;
    }

	// region
    if(szRegion.length() == 2)
	    ImGui::GetBackgroundDrawList()->AddText(nullptr, 38.f, ImVec2(210, 3), ImColor(IM_COL32_BLACK), szRegion.c_str());
    else
        ImGui::GetBackgroundDrawList()->AddText(nullptr, 32.f, ImVec2(205, 3), ImColor(IM_COL32_BLACK), szRegion.c_str());

    ImGui::PopFont();
	ImGui::EndFrame();
	ImGui::Render();
	pUI->renderDrawData(ImGui::GetDrawData());

	RwTexture* pTexture = CRenderTarget::End();

	return pTexture;
}

RwTexture* CCustomPlateManager::createVoenPlate(std::string& szNumber, std::string& szRegion)
{
	CRGBA white {255, 255, 255, 255};

	CRenderTarget::Begin(256, 64, white.ToRwRGBA(), true);
	m_pVoenSprite->Draw(0.0f, 0.0f, 256.0f, 64.0f, &white);

	ImGui::NewFrame();

	// num
    ImGui::PushFont(m_pRuFont);

    float xPos = 17.0f;
    for (auto it = szNumber.begin(); it != szNumber.end(); ++it) {
        char c = *it;

        ImVec2 fontSize = std::isdigit(c) ? ImVec2(50.f, 3) : ImVec2(40.f, 11);
        ImVec2 textSize = ImGui::CalcTextSize(&c, &c + 1, false, fontSize.x);

        float endMargin = 7.0f;
        if (it != szNumber.end() - 1 && std::isdigit(c) && !std::isdigit(*(it + 1))) {
            endMargin += 6;
        }

        if (it != szNumber.end() - 1 && !std::isdigit(c) && std::isdigit(*(it + 1))) {
            endMargin += 6;
        }

        ImGui::GetBackgroundDrawList()->AddText(nullptr, fontSize.x + 25, ImVec2(xPos + 17, fontSize.y - 12.5), ImColor(IM_COL32_WHITE), &c, &c + 1);
        xPos += textSize.x + endMargin;
    }

	// region
    if(szRegion.length() == 2)
	    ImGui::GetBackgroundDrawList()->AddText(nullptr, 38.f, ImVec2(210, 3), ImColor(IM_COL32_WHITE), szRegion.c_str());
    else
        ImGui::GetBackgroundDrawList()->AddText(nullptr, 32.f, ImVec2(205, 3), ImColor(IM_COL32_WHITE), szRegion.c_str());

    ImGui::PopFont();
	ImGui::EndFrame();
	ImGui::Render();
	pUI->renderDrawData(ImGui::GetDrawData());

	RwTexture* pTexture = CRenderTarget::End();

	return pTexture;
}