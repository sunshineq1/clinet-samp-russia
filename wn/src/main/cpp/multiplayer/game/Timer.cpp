//
// Created by weikton on 14.04.2023.
//

#include "Timer.h"
#include <thread>
#include "../vendor/armhook/patch.h"
#include "../java/jniutil.h"
#include "../java/GUIManager.h"
#include "../gui/gui.h"
#include "../net/netgame.h"

extern CJavaWrapper *pJavaWrapper;
extern UI *pUI;
extern CNetGame* pNetGame;

uint8_t* gTimerRunning = nullptr;
uint64_t* oldTimerValue = nullptr;

struct alignas(8) TimerDefinition {
    uint64_t (*GetTime)();
    uint32_t millisecondRatio;
};

static uint64_t GetHighPrecisionTime();

static TimerDefinition timerDef;
static uint64_t suspendTimerValue = 0;
static uint32_t gSuspendDepth = 0;
static bool bWouldBeNewFrame = false;
static uint32_t nLastFrameTime = 0;

float CTimer::TARGET_FPS = 30.0f;
float CTimer::FRAME_TIME_MS = 33.333f;

float CTimer::game_FPS = 0;

bool        CTimer::m_CodePause = false;
bool        CTimer::m_UserPause = false;
float       CTimer::ms_fTimeScale;
uint32_t    CTimer::m_FrameCounter = 0;
uint32_t    CTimer::m_snTimeInMilliseconds = 0;
bool        CTimer::bSkipProcessThisFrame = false;
float       CTimer::ms_fTimeStep = 0;
uint32_t    CTimer::m_snPPPPreviousTimeInMilliseconds;
uint32_t    CTimer::m_snPPPreviousTimeInMilliseconds;
uint32_t    CTimer::m_snPPreviousTimeInMilliseconds;
uint32_t    CTimer::m_snPreviousTimeInMilliseconds;
uint32_t    CTimer::m_snTimeInMillisecondsNonClipped;
uint32_t    CTimer::m_snPreviousTimeInMillisecondsNonClipped;
uint32_t    CTimer::m_snTimeInMillisecondsPauseMode;
float       CTimer::ms_fTimeStepNonClipped;
float       CTimer::ms_fOldTimeStep;

void CTimer::InjectHooks()
{
    CHook::Write(g_libGTASA + (VER_x32 ? 0x006794BC : 0x8509A0), &CTimer::m_CodePause);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x0067892C : 0x84F280), &CTimer::m_FrameCounter);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x00677674 : 0x84CD28), &CTimer::game_FPS);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x006776A8 : 0x84CD90), &CTimer::m_UserPause);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x0067689C : 0x84B1A8), &CTimer::ms_fTimeScale);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x00676FF0 : 0x84C030), &CTimer::m_snTimeInMilliseconds);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x00679248 : 0x8504B8), &CTimer::bSkipProcessThisFrame);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x0067770C : 0x84CE50), &CTimer::ms_fTimeStep);

    CHook::Write(g_libGTASA + (VER_x32 ? 0x0067949C : 0x850960), &CTimer::m_snPPPPreviousTimeInMilliseconds);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x00677DF0 : 0x84DC10), &CTimer::m_snPPPreviousTimeInMilliseconds);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x00679D3C : 0x851A90), &CTimer::m_snPPreviousTimeInMilliseconds);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x006779A8 : 0x84D388), &CTimer::m_snPreviousTimeInMilliseconds);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x00679DBC : 0x851B90), &CTimer::m_snTimeInMillisecondsNonClipped);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x006775F8 : 0x84CC30), &CTimer::m_snPreviousTimeInMillisecondsNonClipped);

    CHook::Write(g_libGTASA + (VER_x32 ? 0x677A28 : 0x84D480), &CTimer::m_snTimeInMillisecondsPauseMode);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x6799B0 : 0x851380), &CTimer::ms_fTimeStepNonClipped);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x676BFC : 0x84B858), &CTimer::ms_fOldTimeStep);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x006764D8 : 0x84AA20), &timerDef);

    gTimerRunning = &*(uint8_t*)(g_libGTASA + (VER_x32 ? 0x0096B524 : 0xBDC5AC));
    oldTimerValue = &*(uint64_t*)(g_libGTASA + (VER_x32 ? 0x96B528 : 0xBDC5B0));

    CHook::Redirect("_ZN6CTimer14StartUserPauseEv", &CTimer::StartUserPause);
    CHook::Redirect("_ZN6CTimer12EndUserPauseEv", &CTimer::EndUserPause);
    CHook::Redirect("_ZN6CTimer4StopEv", &CTimer::Stop);
    CHook::Redirect("_ZN6CTimer21GetIsSlowMotionActiveEv", &CTimer::GetIsSlowMotionActive);

    CHook::Redirect("_ZN6CTimer6UpdateEv", &CTimer::Update);
}

uint64_t GetMillisecondTime() {

}

// 0x5617E0
void CTimer::Initialise()
{
    gSuspendDepth = 0;
    if (gTimerRunning) *gTimerRunning = false;

    m_CodePause = false;
    m_UserPause = false;
    m_snTimeInMilliseconds = 0;
    m_FrameCounter = 0;
    game_FPS = 0.0f;
    bSkipProcessThisFrame = false;
    m_snTimeInMillisecondsNonClipped = 1;
    m_snTimeInMillisecondsPauseMode = 1;
    m_snPPPPreviousTimeInMilliseconds = 0;
    m_snPPPreviousTimeInMilliseconds = 0;
    m_snPPreviousTimeInMilliseconds = 0;
    m_snPreviousTimeInMilliseconds = 0;
    m_snPreviousTimeInMillisecondsNonClipped = 0;
    ms_fTimeScale = 1.0f;
    ms_fOldTimeStep = 1.0f;
    ms_fTimeStep = 1.0f;

    timerDef.GetTime = GetHighPrecisionTime;
    timerDef.millisecondRatio = 1000;

    if (oldTimerValue) *oldTimerValue = GetHighPrecisionTime();
}

// 0x5618C0
void CTimer::Shutdown() {

}

// 0x5619D0
void CTimer::Suspend()
{
    CHook::CallFunction<void>("_ZN6CTimer7SuspendEv");
}

// 0x561A00
void CTimer::Resume()
{
    CHook::CallFunction<void>("_ZN6CTimer6ResumeEv");
}

// 0x561AA0
void CTimer::Stop()
{
    *gTimerRunning = 0;
    CTimer::m_snPPPPreviousTimeInMilliseconds = CTimer::m_snTimeInMilliseconds;
    CTimer::m_snPPPreviousTimeInMilliseconds = CTimer::m_snTimeInMilliseconds;
    CTimer::m_snPPreviousTimeInMilliseconds = CTimer::m_snTimeInMilliseconds;
    CTimer::m_snPreviousTimeInMilliseconds = CTimer::m_snTimeInMilliseconds;

    CTimer::m_snPreviousTimeInMillisecondsNonClipped = CTimer::m_snTimeInMillisecondsNonClipped;
}

// 0x561AF0
void CTimer::StartUserPause()
{
    if (pUI) pUI->setVisible(false);

    if(pJavaWrapper && pNetGame)
    {
        GUIManager::TempToggle(UI_ID_HUD, false);
        pJavaWrapper->SetPauseState(true);
    }
    m_UserPause = true;
}

// 0x561B00
void CTimer::EndUserPause()
{
    // process resume event
    if (pUI) pUI->setVisible(true);
    if(pJavaWrapper && pNetGame)
    {
        if(pNetGame->GetGameState() == GAMESTATE_CONNECTED) {
            if(pUI->m_bCamEditGui) {
                GUIManager::ClearUIScreen();
            } else if (pUI->m_bChatGuiDisable) {
                GUIManager::ToggleUIChat();
            }
            GUIManager::TempToggle(UI_ID_HUD, true);
        }
        pJavaWrapper->SetPauseState(false);
    }
    m_UserPause = false;
}

// 0x561A40
uint32_t CTimer::GetCyclesPerMillisecond()
{
    return CHook::CallFunction<uint32_t>(g_libGTASA + (VER_x32 ? 0x0042100C + 1 : 0x504858));
}

// cycles per ms * 20
// 0x561A50
uint32_t CTimer::GetCyclesPerFrame()
{
    return CHook::CallFunction<uint32_t>(g_libGTASA + (VER_x32 ? 0x0042101C + 1 : 0x504868));
}

uint64_t CTimer::GetCurrentTimeInCycles()
{
    return CHook::CallFunction<uint64_t>(g_libGTASA + (VER_x32 ? 0x00421040 + 1 : 0x504888));
}

// 0x561AD0
bool CTimer::GetIsSlowMotionActive()
{
    return CTimer::ms_fTimeScale < 1.0;
}

static uint64_t GetHighPrecisionTime()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

void CTimer::SetTargetFPS(float fps)
{
    if (fps <= 30.0f) {
        TARGET_FPS = 30.0f;
    } else if (fps <= 60.0f) {
        TARGET_FPS = 60.0f;
    } else if (fps <= 90.0f) {
        TARGET_FPS = 90.0f;
    } else if (fps <= 120.0f) {
        TARGET_FPS = 120.0f;
    } else if (fps <= 144.0f) {
        TARGET_FPS = 144.0f;
    } else {
        TARGET_FPS = fps;
    }

    FRAME_TIME_MS = 1000.0f / TARGET_FPS;
}

bool CTimer::IsNewFrame()
{
    return bWouldBeNewFrame;
}

// 0x5618D0
void CTimer::UpdateVariables(float timeElapsed)
{
    auto timeStepInMilliseconds = static_cast<uint32_t>(timeElapsed);
    if (timeElapsed >= 100.0f) {
        timeStepInMilliseconds = 100;
    }

    float timeStepNormalized = timeElapsed / TIMESTEP_LEN_IN_MS;
    ms_fTimeStepNonClipped = timeStepNormalized;
    m_snTimeInMilliseconds += timeStepInMilliseconds;

    const float minTimeStep = 1.0f / std::max((float)RsGlobal->maxFPS, 255.0f) / TIMESTEP_LEN_IN_MS;
    if (timeStepNormalized < minTimeStep && !m_CodePause && !m_UserPause) {
        timeStepNormalized = minTimeStep;
        ms_fTimeStepNonClipped = minTimeStep;
    }

    ms_fOldTimeStep = ms_fTimeStep;
    ms_fTimeStep = std::max(std::min(timeStepNormalized, 3.0f), 0.00001f);

    ++m_FrameCounter;
}

// 0x561B10
void CTimer::Update()
{
    if (!oldTimerValue || !gTimerRunning) {
        return;
    }

    static bool bTimeSourceSynced = false;
    if (!bTimeSourceSynced) {
        bTimeSourceSynced = true;
        *oldTimerValue = GetHighPrecisionTime();
        nLastFrameTime = m_snTimeInMilliseconds;
    }

    if (TARGET_FPS > 0.0f) {
        const uint64_t budgetUs = (uint64_t)(1000000.0f / TARGET_FPS);
        uint64_t spentUs = GetHighPrecisionTime() - *oldTimerValue;

        if (spentUs < budgetUs) {
            std::this_thread::sleep_for(std::chrono::microseconds(budgetUs - spentUs));

            spentUs = GetHighPrecisionTime() - *oldTimerValue;
            while (spentUs < budgetUs) {
                std::this_thread::yield();
                spentUs = GetHighPrecisionTime() - *oldTimerValue;
            }
        }
    }

    *gTimerRunning = true;

    m_snPPPPreviousTimeInMilliseconds = m_snPPPreviousTimeInMilliseconds;
    m_snPPPreviousTimeInMilliseconds = m_snPPreviousTimeInMilliseconds;
    m_snPPreviousTimeInMilliseconds = m_snPreviousTimeInMilliseconds;
    m_snPreviousTimeInMilliseconds = m_snTimeInMilliseconds;

    const float frameTimeDelta = (float)(m_snTimeInMillisecondsNonClipped - m_snPreviousTimeInMillisecondsNonClipped);
    m_snPreviousTimeInMillisecondsNonClipped = m_snTimeInMillisecondsNonClipped;

    if (frameTimeDelta > 0.0f) {
        game_FPS = 1000.0f / frameTimeDelta;
    }

    const uint64_t currentTime = GetHighPrecisionTime();
    const float elapsedTime = (float)(currentTime - *oldTimerValue);

    const float scaledTime = ms_fTimeScale * elapsedTime;
    const float pausedTime = (m_CodePause || m_UserPause) ? 0.0f : scaledTime;

    const float ratio = timerDef.millisecondRatio ? (float)timerDef.millisecondRatio : 1000.0f;
    const float elapsedMs = pausedTime / ratio;
    const float totalElapsedMs = scaledTime / ratio;

    m_snTimeInMillisecondsPauseMode = (uint32_t)(totalElapsedMs + (float)m_snTimeInMillisecondsPauseMode);

    *oldTimerValue = currentTime;

    static float fMillisecondRemainder = 0.0f;
    const float msWithRemainder = elapsedMs + fMillisecondRemainder;

    uint32_t wholeMs = (uint32_t)msWithRemainder;
    fMillisecondRemainder = msWithRemainder - (float)wholeMs;

    m_snTimeInMillisecondsNonClipped += wholeMs;
    m_snTimeInMilliseconds += std::min<uint32_t>(wholeMs, 300);

    {
        static float fWorstMs = 0.0f, fBestMs = 1000.0f, fAccumMs = 0.0f;
        static int   nFrames = 0;

        fWorstMs = std::max(fWorstMs, elapsedMs);
        fBestMs  = std::min(fBestMs, elapsedMs);
        fAccumMs += elapsedMs;
        nFrames++;

        if (fAccumMs >= 2000.0f) {
            // Log("CTimer: %d кадров за %.0f мс, шаг %.2f..%.2f мс (средний %.2f), timeStep %.3f",
            //    nFrames, fAccumMs, fBestMs, fWorstMs, fAccumMs / (float)nFrames, ms_fTimeStep);
            fWorstMs = 0.0f;
            fBestMs = 1000.0f;
            fAccumMs = 0.0f;
            nFrames = 0;
        }
    }

    bWouldBeNewFrame = false;
    const uint32_t nextFrameTime = nLastFrameTime + (uint32_t)FRAME_TIME_MS;

    if (m_snTimeInMilliseconds >= nextFrameTime) {
        bWouldBeNewFrame = true;
        nLastFrameTime = m_snTimeInMilliseconds;
        m_FrameCounter++;
    }

    float timeStepRaw = elapsedMs / TIMESTEP_LEN_IN_MS;
    ms_fTimeStepNonClipped = timeStepRaw;

    constexpr float MIN_TIME_STEP = 0.01f;
    if (timeStepRaw < MIN_TIME_STEP && !m_CodePause && !m_UserPause) {
        ms_fTimeStepNonClipped = MIN_TIME_STEP;
        timeStepRaw = MIN_TIME_STEP;
    }

    ms_fOldTimeStep = ms_fTimeStep;

    ms_fTimeStep = std::clamp(timeStepRaw, 0.00001f, 3.0f);
}

uint32_t CTimer::GetCurrentUnixTimeMoscow() {
    auto now = std::chrono::system_clock::now();

    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm* ptm = std::gmtime(&now_c);
    ptm->tm_hour += 3;
    std::time_t moscow_time = std::mktime(ptm);

    auto duration = std::chrono::system_clock::from_time_t(moscow_time).time_since_epoch();
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
}
