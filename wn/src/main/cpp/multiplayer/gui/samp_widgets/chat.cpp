#include "../gui.h"
#include "../../main.h"
#include "../../game/game.h"
#include "../../net/netgame.h"
#include <algorithm>
#include "../CSettings.h"

#include "java/GUIManager.h"
#include "java/jniutil.h"

extern UI* pUI;
extern CGame* pGame;
extern CNetGame* pNetGame;
extern CSettings* pSettings;
extern CJavaWrapper *pJavaWrapper;

Chat::Chat()
	: ListBox()
{

}

void Chat::addChatMessage(const std::string& message, const std::string& nick, const ImColor& nick_color)
{
	addPlayerMessage(message, nick, nick_color);
}

void Chat::addInfoMessage(const std::string& format, ...)
{
	char tmp_buf[512];

	va_list args;
	va_start(args, format);
	vsprintf(tmp_buf, format.c_str(), args);
	va_end(args);

	addMessage(Encoding::toUtf(tmp_buf), ImColor(0x00, 0xc8, 0xc8));
}

void Chat::addDebugMessage(const std::string& format, ...)
{
	char tmp_buf[512];

	va_list args;
	va_start(args, format);
	vsprintf(tmp_buf, format.c_str(), args);
	va_end(args);

	addMessage(Encoding::toUtf(tmp_buf), ImColor(0xbe, 0xbe, 0xbe));
}

void Chat::addClientMessage(const std::string& message, const ImColor& color)
{
	addMessage(message, color);
}

std::string Chat::wrapText(const std::string& text, size_t limit)
{
	if (limit == 0) return text;

	std::string result;
	result.reserve(text.length() + text.length() / limit + 1);

	size_t visible = 0;
	size_t lastSpace = std::string::npos;
	size_t lineStart = 0;

	for (size_t i = 0; i < text.length(); )
	{
		if (text[i] == '{' && i + 7 < text.length() && text[i + 7] == '}')
		{
			result.append(text, i, 8);
			i += 8;
			continue;
		}

		if (text[i] == 0x0A)
		{
			result += (char)0x0A;
			visible = 0;
			lastSpace = std::string::npos;
			lineStart = result.length();
			++i;
			continue;
		}

		size_t charLen = 1;
		const unsigned char lead = (unsigned char)text[i];
		if      (lead >= 0xF0) charLen = 4;
		else if (lead >= 0xE0) charLen = 3;
		else if (lead >= 0xC0) charLen = 2;
		if (i + charLen > text.length()) charLen = 1;

		if (text[i] == ' ') lastSpace = result.length();

		result.append(text, i, charLen);
		i += charLen;
		++visible;

		if (visible >= limit)
		{
			if (lastSpace != std::string::npos && lastSpace > lineStart)
			{
				result[lastSpace] = (char)0x0A;
				visible = result.length() - lastSpace - 1;
			}
			else
			{
				result += (char)0x0A;
				visible = 0;
			}

			lastSpace = std::string::npos;
			lineStart = result.length();
		}
	}

	return result;
}

void Chat::addMessage(const std::string& message, const ImColor& color)
{
	if (this->itemsCount() > UISettings::chatMaxMessages())
	{
		this->removeItem(0);
	}

	MessageItem* item = new MessageItem(wrapText(message, UISettings::chatWrapLimit()), color);
	this->addItem(item);
	/*if(!active())*/ this->setScrollY(1.0f);
}

void Chat::addPlayerMessage(const std::string& message, const std::string& nick, const ImColor& nick_color)
{
	if (this->itemsCount() > UISettings::chatMaxMessages())
	{
		this->removeItem(0);
	}

	PlayerMessageItem* item = new PlayerMessageItem(wrapText(message, UISettings::chatWrapLimit()), nick, nick_color);
	this->addItem(item);
	/*if(!active())*/ this->setScrollY(1.0f);
}

void Chat::clear()
{
	while (this->itemsCount() > 0)
	{
		this->removeItem(0);
	}

	this->setScrollY(1.0f);
}

void Chat::draw(ImGuiRenderer* renderer)
{
	if (m_bMessagesHidden)
		return;

	ListBox::draw(renderer);
}

void Chat::activateEvent(bool active)
{
	this->setScrollable(true);
}

void Chat::touchPopEvent()
{
	if (pUI->playertablist()->visible() || CTimer::m_CodePause || CTimer::m_UserPause)
        return;

	pUI->keyboard()->show(this);
}

void Chat::keyboardEvent(const std::string& input)
{
	if (input.length() > 0 && pNetGame)
	{
		if (input[0] == '/')
            pNetGame->SendChatCommand(input.c_str());
		else
            pNetGame->SendChatMessage(input.c_str());
	}
}