#include "../main.h"
#include "../game/game.h"
#include "../net/netgame.h"
#include "gui.h"
#include "../playertags.h"
#include "../net/playerbubblepool.h"

// voice
#include "../voice_new/Plugin.h"
#include "../voice_new/MicroIcon.h"
#include "../voice_new/SpeakerList.h"
#include "../voice_new/Network.h"

#include "../gui/samp_widgets/voicebutton.h"
#include "game/Textures/TextureDatabaseRuntime.h"
#include "game/Streaming.h"
#include "game/Pools.h"
#include "../net/CObjectPoolDraw.h"

extern CGame* pGame;
extern CNetGame* pNetGame;
extern CPlayerTags* pPlayerTags;
extern UI* pUI;

UI::UI(const ImVec2& display_size, const std::string& font_path)
	: Widget(), ImGuiWrapper(display_size, font_path)
{
	UISettings::Initialize(display_size);
	this->setFixedSize(display_size);
}

bool UI::initialize()
{
	if (!ImGuiWrapper::initialize()) return false;

	m_splashScreen = new SplashScreen();
	this->addChild(m_splashScreen);
	m_splashScreen->setFixedSize(size());
	m_splashScreen->setPosition(ImVec2(0.0f, 0.0f));
	m_splashScreen->setVisible(true);

	m_chat = new Chat();
	this->addChild(m_chat);
	m_chat->setFixedSize(UISettings::chatSize());
	m_chat->setPosition(UISettings::chatPos());
	m_chat->setItemSize(UISettings::chatItemSize());
	m_chat->setVisible(false);

	m_buttonPanel = new ButtonPanel();
	this->addChild(m_buttonPanel);
	m_buttonPanel->setFixedSize(UISettings::buttonPanelSize());
	m_buttonPanel->setPosition(UISettings::buttonPanelPos());
	m_buttonPanel->setVisible(false);

	m_voiceButton = new VoiceButton();
	this->addChild(m_voiceButton);
	m_voiceButton->setFixedSize(UISettings::buttonVoiceSize());
	m_voiceButton->setPosition(UISettings::buttonVoicePos());
	m_voiceButton->setVisible(false);

	m_dialog = new Dialog();
	this->addChild(m_dialog);
	m_dialog->setVisible(false);
	m_dialog->setMinSize(UISettings::dialogMinSize());
	m_dialog->setMaxSize(UISettings::dialogMaxSize());

	m_keyboard = new Keyboard();
	this->addChild(m_keyboard);
	m_keyboard->setFixedSize(UISettings::keyboardSize());
	m_keyboard->setPosition(UISettings::keyboardPos());
	m_keyboard->setVisible(false);

	m_playerTabList = new PlayerTabList();
	this->addChild(m_playerTabList);
	m_playerTabList->setMinSize(UISettings::dialogMinSize());
	m_playerTabList->setMaxSize(UISettings::dialogMaxSize());
	m_playerTabList->setVisible(false);

    label = new Label(" ", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    pUI->addChild(label);

    label2 = new Label(" ", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    pUI->addChild(label2);

    label3 = new Label(" ", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    pUI->addChild(label3);

    label4 = new Label(" ", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    pUI->addChild(label4);

	// Label* d_label1;
	// d_label1 = new Label(OBFUSCATE("reBR"), ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2.65);
	// this->addChild(d_label1);
	// d_label1->setPosition(ImVec2(3.0, 3.0));

    m_objectDraw = new CObjectPoolDraw();
	return true;
}

void UI::loadTxd() {
    Log("Load gui textures..");
    static const CUtil::SNamedTexture kEnter[] = {
        { "mobile", "hud_enterandexit" },
        { "mobile", "enterandexit" },
    };

    m_pPassengerButtonTex = CUtil::LoadTextureAny("Passenger", kEnter);
}

#include "../java/speedometer.h"
#include "../java/jniutil.h"
void UI::updateChatAnchor()
{
	const ImVec2 radarMin(CJavaWrapper::radarBgPos1.x, CJavaWrapper::radarBgPos1.y);
	const ImVec2 radarMax(CJavaWrapper::radarBgPos2.x, CJavaWrapper::radarBgPos2.y);

	if (radarMax.x <= radarMin.x || radarMax.y <= radarMin.y)
		return;

	if (radarMin.x == m_lastRadarMin.x && radarMin.y == m_lastRadarMin.y &&
		radarMax.x == m_lastRadarMax.x && radarMax.y == m_lastRadarMax.y)
		return;

	m_lastRadarMin = radarMin;
	m_lastRadarMax = radarMax;

	const float gap = (radarMax.x - radarMin.x) * 0.12f;

	ImVec2 pos(radarMax.x + gap, radarMin.y);

	const float maxWidth = size().x - pos.x - gap;
	if (maxWidth > 0.0f && m_chat->width() > maxWidth)
	{
		m_chat->setFixedSize(ImVec2(maxWidth, m_chat->height()));
		m_chat->setItemSize(ImVec2(maxWidth, UISettings::chatItemSize().y));
	}

	m_chat->setPosition(pos);
	m_chat->performLayout();
}

void UI::setInterfaceHidden(bool hideHud, bool hideChat)
{
	// Возврат делаем не по снимку, а к нормальному игровому состоянию: снимок
	// мог быть взят в момент, когда чат или кнопки были скрыты чем-то другим,
	// и тогда после закрытия окна они уже не появлялись.
	if (hideHud != m_bInterfaceHudHidden)
	{
		m_bInterfaceHudHidden = hideHud;

		m_buttonPanel->setVisible(!hideHud && !m_bCamEditGui);
		m_voiceButton->setVisible(!hideHud && !m_bCamEditGui);
	}

	if (hideChat != m_bInterfaceChatHidden)
	{
		m_bInterfaceChatHidden = hideChat;

		m_chat->setVisible(!hideChat);
		m_chat->setMessagesHidden(hideChat || m_bChatGuiDisable || m_bCamEditGui);
	}
}

void UI::render()
{
	ImGuiWrapper::render();

	updateChatAnchor();

    // if (m_objectDraw && m_bObjInfo)
    //     m_objectDraw->Render(renderer());

    renderDebug();

    if(pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()->m_pPed->IsInVehicle() && !pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()->m_pPed->IsAPassenger()
            && !CTimer::m_UserPause && !m_bCamEditGui && !m_bInterfaceHudHidden && !CJavaWrapper::GUIDialogShow)
    {
        Speedometer::toggleVisibility(true);
        Speedometer::update();
    }
    else
    {
        Speedometer::toggleVisibility(false);
    }

    ProcessPushedTextdraws();

	if (m_bNeedClearMousePos) {
		ImGuiIO& io = ImGui::GetIO();
		io.MousePos = ImVec2(-1, -1);
		m_bNeedClearMousePos = false;
	}
}

extern bool bNeedEnterVehicle;

#include "game/Widgets/TouchInterface.h"
#include "game/Mobile/MobileMenu/MobileMenu.h"

void UI::shutdown()
{
	ImGuiWrapper::shutdown();
}

void UI::drawList()
{
	if (!visible()) return;

	if (!m_bInterfaceHudHidden)
	{
		if (pPlayerTags) pPlayerTags->Render(renderer());
		if (pNetGame && pNetGame->GetTextLabelPool()) pNetGame->GetTextLabelPool()->Render(renderer());
		if (pNetGame && pNetGame->GetPlayerBubblePool()) pNetGame->GetPlayerBubblePool()->Render(renderer());
	}

    draw(renderer());

    renderBuildBanner();
}

void UI::touchEvent(const ImVec2& pos, TouchType type)
{
	if (m_keyboard->visible() && m_keyboard->contains(pos))
	{
		m_keyboard->touchEvent(pos, type);
		return;
	}

	if (m_dialog->visible() && m_dialog->contains(pos))
	{
		m_dialog->touchEvent(pos, type);
		return;
	}

	Widget::touchEvent(pos, type);
}

enum eTouchType
{
	TOUCH_POP = 1,
	TOUCH_PUSH = 2,
	TOUCH_MOVE = 3
};

bool UI::OnTouchEvent(int type, bool multi, int x, int y)
{
	ImGuiIO& io = ImGui::GetIO();

	VoiceButton* vbutton = pUI->voicebutton();
	switch (type)
	{
	case TOUCH_PUSH:
		io.MousePos = ImVec2(x, y);
		io.MouseDown[0] = true;
		break;

	case TOUCH_POP:
		io.MouseDown[0] = false;
		m_bNeedClearMousePos = true;
		break;

	case TOUCH_MOVE:
		io.MousePos = ImVec2(x, y);
		break;
	}

	return true;
}

#include "../CSettings.h"
extern CGame *pGame;
extern CSettings* pSettings;
void UI::renderDebug()
{
    // if(!pSettings->Get().iFPSCounter)
        return;

    char szStr[60];
    ImVec2 pos = ImVec2(ScaleX(40.0f), ScaleY(1080.0f - GetFontSize() * 5));
    static float fps = 120.f;
    static auto lastTick = CTimer::m_snTimeInMillisecondsNonClipped;
    struct mallinfo memInfo = mallinfo();
    int totalAllocatedMB  = memInfo.uordblks / (1024 * 1024);
    if(CTimer::m_snTimeInMillisecondsNonClipped - lastTick > 500) {
        lastTick = CTimer::m_snTimeInMillisecondsNonClipped;
        fps = std::clamp(CTimer::game_FPS, 10.f, (float) RsGlobal->maxFPS);
    }
    snprintf(&szStr[0], sizeof(szStr), VER_x32? "ARM7\nFPS: %.0f\nMEMORY: %d MB" : "ARM8\nFPS: %.0f\nMEMORY: %d MB", fps, totalAllocatedMB);

    label->setText(&szStr[0]);
    label->setPosition(pos);
}

void UI::renderBuildBanner()
{
#ifndef WN_BUILD_HASH
#define WN_BUILD_HASH "dev"
#endif

    static float fps = 60.0f;
    static auto lastTick = CTimer::m_snTimeInMillisecondsNonClipped;
    if (CTimer::m_snTimeInMillisecondsNonClipped - lastTick > 500) {
        lastTick = CTimer::m_snTimeInMillisecondsNonClipped;
        fps = std::clamp(CTimer::game_FPS, 1.0f, (float)RsGlobal->maxFPS);
    }

    const int usedMB  = (int)(CStreaming::ms_memoryUsed / (1024 * 1024));
    const int limitMB = (int)(CStreaming::ms_memoryAvailable / (1024u * 1024u));

    // __DATE__ приходит как "Jun 22 2026" — переводим в 22.06.2026,
    // из __TIME__ берём часы и минуты.
    static char szDate[16];
    if (!szDate[0]) {
        static const char *kMonths = "JanFebMarAprMayJunJulAugSepOctNovDec";
        char month[4] = {};
        int day = 0, year = 0;
        sscanf(__DATE__, "%3s %d %d", month, &day, &year);
        const char *found = strstr(kMonths, month);
        const int monthNum = found ? (int)((found - kMonths) / 3 + 1) : 0;
        snprintf(szDate, sizeof(szDate), "%02d.%02d.%04d", day, monthNum, year);
    }

    char szText[192];
    snprintf(szText, sizeof(szText),
             "WN Native Build %s %.5s (%s) FPS: %.0f Streaming: %d/%d MB",
             szDate, __TIME__, WN_BUILD_HASH, fps, usedMB, limitMB);

    // Строка идёт вдоль нижнего края экрана, слева, мелким шрифтом.
    const float fontSize = ScaleY(19.5f);
    const ImVec2 pos(ScaleX(6.0f), displaySize().y - fontSize - ScaleY(4.0f));

    renderer()->drawText(pos, ImColor(255, 255, 255, 255), szText, nullptr, true, fontSize);
}

void UI::PushToBufferedQueueTextDrawPressed(uint16_t textdrawId)
{
    BUFFERED_COMMAND_TEXTDRAW* pCmd = m_BufferedCommandTextdraws.WriteLock();

    pCmd->textdrawId = textdrawId;

    m_BufferedCommandTextdraws.WriteUnlock();
}

void UI::ProcessPushedTextdraws()
{
    BUFFERED_COMMAND_TEXTDRAW* pCmd = nullptr;
    while (pCmd = m_BufferedCommandTextdraws.ReadLock())
    {
        RakNet::BitStream bs;
        bs.Write(pCmd->textdrawId);
        pNetGame->GetRakClient()->RPC(&RPC_ClickTextDraw, &bs, HIGH_PRIORITY, RELIABLE_SEQUENCED, 0, false, UNASSIGNED_NETWORK_ID, 0);
        m_BufferedCommandTextdraws.ReadUnlock();
    }
}