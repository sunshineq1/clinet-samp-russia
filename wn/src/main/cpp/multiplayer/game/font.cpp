#include "../main.h"
#include "font.h"
#include "../vendor/armhook/patch.h"

void CFont::Initialise() {
	CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x0046FD8C + 1 : 0x55BFEC));
}

void CFont::AsciiToGxtChar(const char* ascii, uint16_t* gxt)
{
	return ((void(*)(const char*, uint16_t*))(g_libGTASA + (VER_x32 ? 0x005A83C0 + 1 : 0x6CBEF0)))(ascii, gxt);
}

void CFont::SetScale(float x, float y)
{
	*(float*)(g_libGTASA + (VER_x32 ? 0xA297B8 : 0xCCC324 + 0x4)) = x;
	*(float*)(g_libGTASA + (VER_x32 ? 0xA297BC : 0xCCC324 + 0x8)) = y;
}

void CFont::SetColor(uint32_t* dwColor)
{
	return ((void(*)(uint32_t*))(g_libGTASA + (VER_x32 ? 0x005AB038 + 1 : 0x6CEE0C)))(dwColor);
}

void CFont::SetJustify(uint8_t justify)
{
	return ((void(*)(uint8_t))(g_libGTASA + (VER_x32 ? 0x005AB364 + 1 : 0x6CF1CC)))(justify);
}

void CFont::SetOrientation(uint8_t orientation)
{
	return ((void(*)(uint8_t))(g_libGTASA + (VER_x32 ? 0x005AB374 + 1 : 0x6CF1DC)))(orientation);
}

void CFont::SetWrapX(float wrapX)
{
	return ((void(*)(float))(g_libGTASA + (VER_x32 ? 0x005AB248 + 1 : 0x6CF0B0)))(wrapX);
}

void CFont::SetCentreSize(float size)
{
	return ((void(*)(float))(g_libGTASA + (VER_x32 ? 0x005AB258 + 1 : 0x6CF0C0)))(size);
}
void Font_SetRightJustifyWrap(float wrap)
{
	((int (*)(float))(g_libGTASA + (VER_x32 ? 0x05AB268 + 1 : 0x0053384C)))(wrap);
	//0053384C
}

void CFont::SetBackground(uint8_t bBackground, uint8_t bOnlyText)
{
	return ((void(*)(uint8_t, uint8_t))(g_libGTASA + (VER_x32 ? 0x005AB330 + 1 : 0x6CF18C)))(bBackground, bOnlyText);
}

void CFont::SetBackgroundColor(uint32_t* dwColor)
{
	return ((void(*)(uint32_t*))(g_libGTASA + (VER_x32 ? 0x005AB340 + 1 : 0x6CF1A0)))(dwColor);
}

void CFont::SetProportional(uint8_t prop)
{
	return ((void(*)(uint8_t))(g_libGTASA + (VER_x32 ? 0x005AB320 + 1 : 0x6CF17C)))(prop);
}

void CFont::SetDropColor(uint32_t* dwColor)
{
	return ((void(*)(uint32_t*))(g_libGTASA + (VER_x32 ? 0x005AB320 + 1 : 0x6CF0F0)))(dwColor);
}

void CFont::SetDropShadowPosition(uint8_t pos)
{
	return ((void(*)(uint8_t))(g_libGTASA + (VER_x32 ? 0x005A8ADC + 1 : 0x6CC8E8)))(pos);
}

/*
void CFont::PrintString(float fX, float fY, const uint16_t* szText)
{
	return ((void(*)(float, float, const uint16_t*))(g_libGTASA + 0x5353B4 + 1))(fX, fY, szText);
}
*/

template<size_t N>
void StringReplace(const char* haystack, const char (&needle)[N], char* dst, auto&& Replace) {
    // Based on https://stackoverflow.com/a/32413923
    auto        pDst = dst;
    const char* pHS  = haystack;

    for(;;) {
        const char* pNeedle = strstr(pHS, needle);

        // walked past last occurrence of needle; copy remaining part
        if (!pNeedle) {
            pDst = std::copy(pHS, pHS + strlen(pHS), pDst);
            break;
        }

        // copy part before needle
        pDst = std::copy(pHS, pNeedle, pDst);

        // replace portion of string
        pDst = Replace(pDst);

        // adjust pointers, move on
        pHS = pNeedle + (N - 1);
    }
    *pDst = 0; // Null terminate
}

void ReplaceStringInPlace(std::string& subject, const std::string& search,
                          const std::string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
}

void ReplaceTextColorsAndKeys(std::string &str)
{
    // Special text symbols
    ReplaceStringInPlace(str, "~u~", "\u2191");   // Up arrow (↑)
    ReplaceStringInPlace(str, "~d~", "\u2193");   // Down arrow (↓)
    ReplaceStringInPlace(str, "~<~", "\u2190");   // Left arrow (←)
    ReplaceStringInPlace(str, "~>~", "\u2192");   // Right arrow (→)
    ReplaceStringInPlace(str, "]", "*");          // Displays a * symbol

    // Key name replacements
    ReplaceStringInPlace(str, "~k~~PED_FIREWEAPON~", "LCTRL/LMB"); // Fire weapon
    ReplaceStringInPlace(str, "~k~~PED_JUMPING~", "LSHIFT");       // Jump
    ReplaceStringInPlace(str, "~k~~PED_SPRINT~", "SPACE");         // Sprint
    ReplaceStringInPlace(str, "~k~~PED_DUCK~", "C");               // Crouch
    ReplaceStringInPlace(str, "~k~~PED_ANSWER_PHONE~", "TAB");     // Action
    ReplaceStringInPlace(str, "~k~~VEHICLE_ENTER_EXIT~", "ENTER"); // Enter/Exit vehicle
    ReplaceStringInPlace(str, "~k~~VEHICLE_FIREWEAPON~", "LALT");  // Fire vehicle weapon
    ReplaceStringInPlace(str, "~k~~VEHICLE_HANDBRAKE~", "SPACE");  // Handbrake
    ReplaceStringInPlace(str, "~k~~VEHICLE_LOOKLEFT~", "Q");       // Look left
    ReplaceStringInPlace(str, "~k~~VEHICLE_LOOKRIGHT~", "E");      // Look right
    ReplaceStringInPlace(str, "~k~~GO_FORWARD~", "UP");            // Move forward
    ReplaceStringInPlace(str, "~k~~GO_BACK~", "DOWN");             // Move backward
    ReplaceStringInPlace(str, "~k~~GO_LEFT~", "LEFT");             // Move left
    ReplaceStringInPlace(str, "~k~~GO_RIGHT~", "RIGHT");           // Move right
    ReplaceStringInPlace(str, "~k~~PED_LOOKBEHIND~", "NUM1/MMB");  // Look behind
    ReplaceStringInPlace(str, "~k~~TOGGLE_SUBMISSIONS~", "NUM1/MMB/NUMPAD+"); // Submission
    ReplaceStringInPlace(str, "~k~~SNEAK_ABOUT~", "LALT");         // Walk
    ReplaceStringInPlace(str, "~k~~VEHICLE_TURRETUP~", "NUM8");    // Turret up
    ReplaceStringInPlace(str, "~k~~VEHICLE_TURRETDOWN~", "NUM2");  // Turret down
    ReplaceStringInPlace(str, "~k~~VEHICLE_TURRETLEFT~", "NUM4");  // Turret left
    ReplaceStringInPlace(str, "~k~~VEHICLE_TURRETRIGHT~", "NUM6"); // Turret right
    ReplaceStringInPlace(str, "~k~~CONVERSATION_YES~", "Y");       // Conversation yes
    ReplaceStringInPlace(str, "~k~~CONVERSATION_NO~", "N");        // Conversation no
    ReplaceStringInPlace(str, "~k~~GROUP_CONTROL_BWD~", "H");      // Group control backward
    ReplaceStringInPlace(str, "~k~~GROUP_CONTROL_FWD~", "G");      // Group control forward
    ReplaceStringInPlace(str, "~k~~VEHICLE_STEERUP~", "UP");       // Steer up
    ReplaceStringInPlace(str, "~k~~VEHICLE_STEERDOWN~", "DOWN");   // Steer down
    ReplaceStringInPlace(str, "~k~~VEHICLE_STEERLEFT~", "LEFT");   // Steer left
    ReplaceStringInPlace(str, "~k~~VEHICLE_STEERRIGHT~", "RIGHT"); // Steer right
    ReplaceStringInPlace(str, "~k~~VEHICLE_HORN~", "H/CAPSLOCK");  // Horn
    ReplaceStringInPlace(str, "~k~~VEHICLE_BRAKE~", "S");          // Brake
    ReplaceStringInPlace(str, "~k~~VEHICLE_ACCELERATE~", "W");     // Accelerate
    ReplaceStringInPlace(str, "~k~~VEHICLE_FIREWEAPON_ALT~", "ALT GR/LCTRL/NUM0"); // Alternative fire
    // Add more key replacements as needed
}


void CFont::PrintString(float posX, float posY, const char* string)
{
	uint16_t* gxt_string = new uint16_t[0xFF];

    std::string str = string;

    ReplaceTextColorsAndKeys(str);
	CFont::AsciiToGxtChar(str.c_str(), gxt_string);

    //Log("PrintString %s", str.c_str());

	((void (*)(float, float, uint16_t*))(g_libGTASA + (VER_x32 ? 0x005AA200 + 1 : 0x6CDEB0)))(posX, posY, gxt_string);
	delete gxt_string;
	((void (*)())(g_libGTASA + (VER_x32 ? 0x005A9120 + 1 : 0x6CCEA0)))();//53411C ; _DWORD CFont::RenderFontBuffer(CFont *__hidden this)
}

void CFont::SetFontStyle(uint8_t style)
{
	return ((void(*)(uint8_t))(g_libGTASA + (VER_x32 ? 0x005AB1BC + 1 : 0x6CEFF8)))(style);
}

void CFont::SetEdge(uint8_t edge)
{
	return ((void(*)(uint8_t))(g_libGTASA + (VER_x32 ? 0x005AB2EC + 1 : 0x6CF148)))(edge);
}

#include "Mobile/MobileMenu/MobileMenu.h"
void CFont::FixLanguage() {
    CMobileMenu::ms_MobileSettings[MS_Language].value = 0;
    CHook::CallFunction<void>("_Z18Menu_ApplyLanguagev");
}