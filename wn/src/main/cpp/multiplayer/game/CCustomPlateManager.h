#pragma once

#include "CRenderTarget.h"
#include <list>
#include "gui/gui.h"
#include "gui/CFontInstance.h"
#include "gui/CFontRenderer.h"

#include "Enums/ePlateType.h"

struct CSprite2d;
struct RwTexture;

class CCustomPlateManager
{
    static inline RwTexture* pNoPlateTex;

	static inline CSprite2d* m_pRuSprite;
	static inline CSprite2d* m_pUaSprite;
	static inline CSprite2d* m_pBuSprite;
	static inline CSprite2d* m_pKzSprite;
	static inline CSprite2d* m_pRuPoliceSprite;
    static inline CSprite2d* m_pVoenSprite;

public:
	static void Initialise();
	static void Shutdown();

    static inline RwTexture* GetNoPlateTex() { return pNoPlateTex; }

    static inline ImFont* m_pRuFont;

	static RwTexture* createTexture(ePlateType type, std::string& szNumber, std::string& szRegion);
    static RwTexture* createRuPlate(std::string& szNumber, std::string& szRegion);
    static RwTexture* createRuPolicePlate(std::string& szNumber, std::string& szRegion);
	static RwTexture* createKzPlate(std::string& szNumber, std::string& szRegion);
	static RwTexture* createUaPlate(std::string& szNumber, std::string& szRegion);
	static RwTexture* createBuPlate(std::string& szNumber, std::string& szRegion);
    static RwTexture* createVoenPlate(std::string& szNumber, std::string& szRegion);
};