#pragma once
#include <jni.h>
#include "game/common.h"

#define EXCEPTION_CHECK(env) \
	if ((env)->ExceptionCheck()) \ 
	{ \
		(env)->ExceptionDescribe(); \
		(env)->ExceptionClear(); \
		return; \
	}

class CJavaWrapper
{
public:
    static JNIEnv* GetEnv();
    static void* NVThreadSpawnProc(void* arg);

    static inline bool GUIDialogShow = false;

    CJavaWrapper(JNIEnv *env, jobject activity);
    ~CJavaWrapper() {};
    
    const char* GetClipboardString();
    void SetPauseState(bool pause);
    void loadingScreenCopyright(const char* str);
    void HideLoadingScreen();
    void ShowTab();
    void HideTab();
    void SetTab(int id, char* names, int score, int pings);
    void ClearTab();
    
    void ShowKeyboard();
    void HideKeyboard();

	void exitGame();

    void MakeDialog(int dialogId, int dialogTypeId, char* caption, char* content, char* leftBtnText, char* rightBtnText);

    void ShowNotification(int type, char* text, int duration, char* actionforBtn, char* textBtn);
	void ShowMenu();

	void ShowEditObject();
	void HideEditObject();

    void TakeScreenshot();
    void ShowGraphicSettings();

    jobject activity;
    jmethodID s_setPauseState;
    jmethodID s_loadingScreenCopyright;
    jmethodID s_hideLoadingScreen;
    jmethodID s_ShowDialog;
    jmethodID s_showTab;
    jmethodID s_setTab;
    jmethodID s_clearTab;
    jmethodID s_hideTab;
    jmethodID s_showInputLayout;
    jmethodID s_hideInputLayout;
	jmethodID s_exitGame;
	jmethodID s_showEditObject;
	jmethodID s_hideEditObject;
    jmethodID s_takeScreenshot;
    jmethodID s_showGraphicSettings;
    jmethodID s_showNotification;

    jmethodID s_toggleSpeed,
              s_updateSpeed;
    void updateSpeed(int speed, int fuel, int hp, int engine, int light, int lock, int turn);
    void toggleSpeed(bool state);

    jmethodID s_toggleCustomButtons;
    void toggleCustomButtons(bool state);

    // radar
    static inline CVector2D radarBgPos1;
    static inline CVector2D radarBgPos2;

    static inline CVector2D radarPos;
    static inline CVector2D radarSize;

    jmethodID s_SetVisibleKeyboardStandard;
    void SetVisibleKeyboard(bool active, int type);
};