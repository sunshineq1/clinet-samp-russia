#include "jniutil.h"
#include "GUIManager.h"

#include "game/game.h"
extern CGame *pGame;

#include "net/netgame.h"
extern CNetGame *pNetGame;

typedef void* (*OSThreadFunction)(void*);
struct ThreadLaunchData {
    void* thread_struct;
    OSThreadFunction func;
    char thread_name[32];
};

void* CJavaWrapper::NVThreadSpawnProc(void* arg) {
    std::unique_ptr<ThreadLaunchData> data(static_cast<ThreadLaunchData*>(arg));

    bool attached = false;
    JNIEnv* env = nullptr;
    jint status = javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

    if (status == JNI_EDETACHED) {
        if (javaVM->AttachCurrentThread(&env, nullptr) == 0) {
            attached = true;
        }
    }

    if (env && data->thread_name[0] != '\0') {
        jclass threadClass = env->FindClass("java/lang/Thread");
        if (threadClass) {
            jmethodID currentThread = env->GetStaticMethodID(threadClass, "currentThread", "()Ljava/lang/Thread;");
            jmethodID setName = env->GetMethodID(threadClass, "setName", "(Ljava/lang/String;)V");

            if (currentThread && setName) {
                jobject threadObj = env->CallStaticObjectMethod(threadClass, currentThread);
                jstring nameStr = env->NewStringUTF(data->thread_name);
                env->CallVoidMethod(threadObj, setName, nameStr);

                env->DeleteLocalRef(nameStr);
                env->DeleteLocalRef(threadObj);
            }
            env->DeleteLocalRef(threadClass);
        }
    }

    void* result = data->func(data->thread_struct);
    if (attached) {
        javaVM->DetachCurrentThread();
    }
    return result;
}

JNIEnv *CJavaWrapper::GetEnv() {
    JNIEnv *env = nullptr;
    int getEnvStat = javaVM->GetEnv((void **) &env, JNI_VERSION_1_6);

    if (getEnvStat == JNI_EDETACHED) {
        Log("GetEnv: not attached");
        if (javaVM->AttachCurrentThread(&env, NULL) != 0) {
            Log("Failed to attach");
            return nullptr;
        }
    }
    if (getEnvStat == JNI_EVERSION) {
        Log("GetEnv: version not supported");
        return nullptr;
    }

    if (getEnvStat == JNI_ERR) {
        Log("GetEnv: JNI_ERR");
        return nullptr;
    }

    return env;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_wn_blackrussia_game_ui_BrHud_setRadarBgPos(JNIEnv *env, jobject thiz, jfloat x1,
                                                        jfloat y1,
                                                        jfloat x2, jfloat y2) {
    CJavaWrapper::radarBgPos1.x = x1;
    CJavaWrapper::radarBgPos1.y = y1;

    CJavaWrapper::radarBgPos2.x = x2;
    CJavaWrapper::radarBgPos2.y = y2;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_wn_blackrussia_game_ui_BrHud_setRadarPos(JNIEnv *env, jobject thiz, jfloat x1,
                                                            jfloat y1, jfloat width,
                                                            jfloat height) {
    CJavaWrapper::radarPos.x = x1;
    CJavaWrapper::radarPos.y = y1;
    CJavaWrapper::radarSize.x = width;
    CJavaWrapper::radarSize.y = height;

    // *(uint8_t*)(g_libGTASA + (VER_x32? 0x991FD8 : 0xC20DFC)) = 1;
}

CJavaWrapper::CJavaWrapper(JNIEnv *env, jobject activity)
{
    this->activity = env->NewGlobalRef(activity);

    jclass clas = env->GetObjectClass(activity);
    if(!clas)
    {
        Log("no clas");
        return;
    }

    s_showTab = env->GetMethodID(clas, "showTab", "()V");
    s_hideTab = env->GetMethodID(clas, "hideTab", "()V");
    s_clearTab = env->GetMethodID(clas, "clearTab", "()V");
    s_setTab = env->GetMethodID(clas, "setTab", "(ILjava/lang/String;II)V");

    s_loadingScreenCopyright = env->GetMethodID(clas, "showLoadingScreen", "(Ljava/lang/String;)V");
    s_hideLoadingScreen = env->GetMethodID(clas, "hideLoadingScreen", "()V");

    s_setPauseState = env->GetMethodID(clas, "setPauseState", "(Z)V");
    
    s_ShowDialog = env->GetMethodID(clas, "showDialog", "(IILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");

	s_showInputLayout = env->GetMethodID(clas, "showInputLayout", "()V");
    s_hideInputLayout = env->GetMethodID(clas, "hideInputLayout", "()V");

    s_exitGame = env->GetMethodID(clas, "exitGame", "()V");

    s_showEditObject = env->GetMethodID(clas, "showEditObject", "()V");
    s_hideEditObject = env->GetMethodID(clas, "hideEditObject", "()V");

    s_toggleCustomButtons = env->GetMethodID(clas, "toggleCustomButtons", "(Z)V");

    s_takeScreenshot = env->GetMethodID(clas, "takeScreenshot", "()V");

    s_showGraphicSettings = env->GetMethodID(clas, "showGraphicSettings", "()V");

    s_showNotification = env->GetMethodID(clas, "showNotification","(ILjava/lang/String;ILjava/lang/String;Ljava/lang/String;)V");

    s_toggleSpeed = env->GetMethodID(clas, "toggleSpeed", "(Z)V");
    s_updateSpeed = env->GetMethodID(clas, "updateSpeed", "(IIIIIII)V");

    s_SetVisibleKeyboardStandard = env->GetMethodID(clas, "SetVisibleKeyboardStandard", "(II)V");

    env->DeleteLocalRef(clas);
}

void CJavaWrapper::toggleSpeed(bool state)
{
    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

	if (!env)
	{
		Log("No env");
		return;
	}

    env->CallVoidMethod(this->activity, this->s_toggleSpeed, state);
}
void CJavaWrapper::updateSpeed(int speed, int fuel, int hp, int engine, int light, int lock, int turn)
{
	JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

	if (!env)
	{
		Log("No env");
		return;
	}

	env->CallVoidMethod(this->activity, this->s_updateSpeed, speed, fuel, hp, engine, light, lock, turn);
}

void CJavaWrapper::ShowNotification(int type, char* text, int duration, char* actionforBtn, char* textBtn)
{
	JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

	if (!env)
	{
		Log("No env");
		return;
	}

	jclass strClass = env->FindClass("java/lang/String");
    jmethodID ctorID = env->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V");
    jstring encoding = env->NewStringUTF("UTF-8");

    jbyteArray bytes = env->NewByteArray(strlen(text));
    env->SetByteArrayRegion(bytes, 0, strlen(text), (jbyte*)text);
    jstring jtext = (jstring) env->NewObject(strClass, ctorID, bytes, encoding);

	bytes = env->NewByteArray(strlen(actionforBtn));
    env->SetByteArrayRegion(bytes, 0, strlen(actionforBtn), (jbyte*)actionforBtn);
    jstring jactionforBtn = (jstring) env->NewObject(strClass, ctorID, bytes, encoding);

	bytes = env->NewByteArray(strlen(textBtn));
    env->SetByteArrayRegion(bytes, 0, strlen(textBtn), (jbyte*)textBtn);
    jstring jtextBtn = (jstring) env->NewObject(strClass, ctorID, bytes, encoding);

	env->CallVoidMethod(this->activity, this->s_showNotification, type, jtext, duration, jactionforBtn, jtextBtn);
}

void CJavaWrapper::toggleCustomButtons(bool status)
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_toggleCustomButtons, status);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::TakeScreenshot()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_takeScreenshot);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::ShowGraphicSettings()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_showGraphicSettings);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::ShowKeyboard()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_showInputLayout);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::HideKeyboard()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_hideInputLayout);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::loadingScreenCopyright(const char* str)
{
    JNIEnv* global_env;
    javaVM->GetEnv((void**)&global_env, JNI_VERSION_1_6);

	if (!global_env)
	{
		LOGI("No env");
		return;
	}

	jclass strClass = global_env->FindClass("java/lang/String");
	jmethodID ctorID = global_env->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V");
	jstring encoding = global_env->NewStringUTF("UTF-8");

	jbyteArray bytes = global_env->NewByteArray(strlen(str));
	global_env->SetByteArrayRegion(bytes, 0, strlen(str), (jbyte*)str);
	jstring str1 = (jstring)global_env->NewObject(strClass, ctorID, bytes, encoding);

    global_env->CallVoidMethod(activity, s_loadingScreenCopyright, str1);

    EXCEPTION_CHECK(global_env);
}

void CJavaWrapper::HideLoadingScreen()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_hideLoadingScreen);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::SetPauseState(bool pause)
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_setPauseState, pause);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::SetTab(int id, char* names, int score, int pings)
{
    JNIEnv* global_env;
    javaVM->GetEnv((void**)&global_env, JNI_VERSION_1_6);

	if (!global_env)
	{
		LOGI("No env");
		return;
	}

	jclass strClass = global_env->FindClass("java/lang/String");
	jmethodID ctorID = global_env->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V"); 
	jstring encoding = global_env->NewStringUTF("UTF-8"); 

	jbyteArray bytes = global_env->NewByteArray(strlen(names)); 
	global_env->SetByteArrayRegion(bytes, 0, strlen(names), (jbyte*)names); 
	jstring str1 = (jstring)global_env->NewObject(strClass, ctorID, bytes, encoding);

    global_env->CallVoidMethod(activity, s_setTab, id, str1, score, pings);

    EXCEPTION_CHECK(global_env);
}

void CJavaWrapper::ShowTab()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_showTab);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::HideTab()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_hideTab);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::ClearTab()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_clearTab);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::MakeDialog(int dialogId, int dialogTypeId, char* caption, char* content, char* leftBtnText, char* rightBtnText)
{
	JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

	if (!env)
	{
		Log("No env");
		return;
	}

	jclass strClass = env->FindClass("java/lang/String");
    jmethodID ctorID = env->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V");
    jstring encoding = env->NewStringUTF("UTF-8");
    jbyteArray bytes = env->NewByteArray(strlen(caption));
    env->SetByteArrayRegion(bytes, 0, strlen(caption), (jbyte*)caption);
    jstring str1 = (jstring)env->NewObject(strClass, ctorID, bytes, encoding);

    jclass strClass1 = env->FindClass("java/lang/String");
    jmethodID ctorID1 = env->GetMethodID(strClass1, "<init>", "([BLjava/lang/String;)V");
    jstring encoding1 = env->NewStringUTF("UTF-8");
    jbyteArray bytes1 = env->NewByteArray(strlen(content));
    env->SetByteArrayRegion(bytes1, 0, strlen(content), (jbyte*)content);
    jstring str2 = (jstring)env->NewObject(strClass1, ctorID1, bytes1, encoding1);

    jclass strClass2 = env->FindClass("java/lang/String");
    jmethodID ctorID2 = env->GetMethodID(strClass2, "<init>", "([BLjava/lang/String;)V");
    jstring encoding2 = env->NewStringUTF("UTF-8");
    jbyteArray bytes2 = env->NewByteArray(strlen(leftBtnText));
    env->SetByteArrayRegion(bytes2, 0, strlen(leftBtnText), (jbyte*)leftBtnText);
    jstring str3 = (jstring)env->NewObject(strClass2, ctorID2, bytes2, encoding2);

    jclass strClass3 = env->FindClass("java/lang/String");
    jmethodID ctorID3 = env->GetMethodID(strClass3, "<init>", "([BLjava/lang/String;)V");
    jstring encoding3 = env->NewStringUTF("UTF-8");
    jbyteArray bytes3 = env->NewByteArray(strlen(rightBtnText));
    env->SetByteArrayRegion(bytes3, 0, strlen(rightBtnText), (jbyte*)rightBtnText);
    jstring str4 = (jstring)env->NewObject(strClass3, ctorID3, bytes3, encoding3);

    /*if(!pNetGame->GetTextDrawPool()->GetState()) {
        GUIManager::TempToggle(UI_ID_HUD, false);
    }*/
    GUIDialogShow = !GUIDialogShow;
    env->CallVoidMethod(activity, s_ShowDialog, dialogId, dialogTypeId, str1, str2, str3, str4);

	EXCEPTION_CHECK(env);
}

void CJavaWrapper::exitGame() {

    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

    if (!env)
    {
        Log("No env");
        return;
    }

    env->CallVoidMethod(this->activity, this->s_exitGame);
}

void CJavaWrapper::ShowEditObject() {

    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

    if (!env)
    {
        Log("No env");
        return;
    }

    env->CallVoidMethod(this->activity, this->s_showEditObject);
}

void CJavaWrapper::HideEditObject() {

    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

    if (!env)
    {
        Log("No env");
        return;
    }

    env->CallVoidMethod(this->activity, this->s_hideEditObject);
}

void CJavaWrapper::SetVisibleKeyboard(bool active, int type)
{
    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

    if (!env)
    {
        Log("No env");
        return;
    }

    env->CallVoidMethod(this->activity, this->s_SetVisibleKeyboardStandard, active? 1 : 0, type);
    EXCEPTION_CHECK(env);
}