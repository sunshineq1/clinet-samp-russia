#include "SnapShotsWrapper.h"
#include <vector>

#include <GLES2/gl2.h>

#include "jniutil.h"
#include "game/snapshothelper.h"
#include "game/Streaming.h"
#include "game/Models/ModelInfo.h"
#include "vendor/armhook/patch.h"

extern bool IsPedModel(unsigned int iModelID);
extern CSnapShotHelper* pSnapShotHelper;
extern uint32 GetRasterExtOffset();

static constexpr uint32 RQRASTER_TARGET_OFFSET = VER_x32 ? 0x18 : 0x20;

// ES2RenderTarget::Select
static constexpr uintptr_t ES2_RENDER_TARGET_SELECT = VER_x32 ? 0x001D0958 + 1 : 0x266024;

// ES2Renderer::ReadPixels
static constexpr uintptr_t ES2_READ_PIXELS = VER_x32 ? 0x001B4C20 + 1 : 0x247E98;

static constexpr int kMaxSnapsPerFrame = 1;
static constexpr size_t kMaxQueuedSnaps = 64;
static constexpr int kMaxModelWaitFrames = 30;
static constexpr size_t kMaxQueuedVinyls = 96;

// Полный размер текстуры стриминг подтягивает только после того, как её
// нарисовали: первые кадры в растре лежит мелкий мип-уровень.
static constexpr int kVinylWarmUpFrames = 2;

// Модель успевает пометиться загруженной раньше, чем её текстуры
// окажутся привязаны к материалам. Снимок с такой моделью выходит
// мылом, поэтому после готовности выдерживаем несколько кадров.

// У растра рендер-таргета своя цель отрисовки: GL-текстура принадлежит ей, а не
// записи текстуры, поэтому читать снимок нужно через неё.
static uintptr_t GetRasterRenderTarget(RwRaster* raster)
{
    if (!raster) {
        return 0;
    }

    return *(uintptr_t*)((char*)raster + GetRasterExtOffset() + RQRASTER_TARGET_OFFSET);
}

// Снимок лежит в текстуре рендер-таргета, поэтому читаем его через свой FBO:
// так не нужны зашитые адреса движка, а чужой таргет остаётся нетронутым.
jbyteArray SnapShotsWrapper::ConvertTexToBitMapBytes(RwTexture* tex, RwInt32& width, RwInt32& height)
{
    auto env = CJavaWrapper::GetEnv();

    RwRaster* raster = tex->raster;
    if (!raster) {
        return nullptr;
    }

    const uintptr_t target = GetRasterRenderTarget(raster);
    if (!target) {
        return nullptr;
    }

    width = raster->width;
    height = raster->height;

    if (width <= 0 || height <= 0) {
        return nullptr;
    }

    const size_t size = (size_t)width * (size_t)height * 4;
    auto buffer = new uint8[size];

    GLint previousFbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);

    CHook::CallFunction<void>(g_libGTASA + ES2_RENDER_TARGET_SELECT, target, true);
    CHook::CallFunction<void>(g_libGTASA + ES2_READ_PIXELS, 0, 0, width, height,
                              GL_RGBA, GL_UNSIGNED_BYTE, buffer);

    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)previousFbo);

    jbyteArray bytes = env->NewByteArray((jsize)size);
    env->SetByteArrayRegion(bytes, 0, (jsize)size, reinterpret_cast<const jbyte*>(buffer));

    delete[] buffer;
    return bytes;
}

void SnapShotsWrapper::SetToImageView(jbyteArray bytes, int width, int height, jobject imageView, int64_t key)
{
    auto env = CJavaWrapper::GetEnv();

    static auto method = env->GetStaticMethodID(clazz, "setToImageView", "(Landroid/widget/ImageView;[BIIJ)V");
    env->CallStaticVoidMethod(clazz, method, imageView, bytes, width, height, (jlong)key);
}

// Ячейку списка могли переиспользовать, пока заявка ждала очереди. Рисовать
// такой снимок незачем: это самая дорогая работа кадра, а результат выбросят.
bool SnapShotsWrapper::IsStillWanted(jobject imageView, int64_t key)
{
    auto env = CJavaWrapper::GetEnv();

    static auto method = env->GetStaticMethodID(clazz, "isStillWanted", "(Landroid/widget/ImageView;J)Z");
    return env->CallStaticBooleanMethod(clazz, method, imageView, (jlong)key) == JNI_TRUE;
}

// Заявку отменили из-за прокрутки: модель тут ни при чём
void SnapShotsWrapper::ReportCancelled(int64_t key)
{
    auto env = CJavaWrapper::GetEnv();

    static auto method = env->GetStaticMethodID(clazz, "onSnapCancelled", "(J)V");
    env->CallStaticVoidMethod(clazz, method, (jlong)key);
}

// Заказ снят: без этого он навсегда останется помеченным как выполняемый
void SnapShotsWrapper::ReportFailed(int64_t key)
{
    auto env = CJavaWrapper::GetEnv();

    static auto method = env->GetStaticMethodID(clazz, "onSnapFailed", "(J)V");
    env->CallStaticVoidMethod(clazz, method, (jlong)key);
}

void SnapShotsWrapper::DeliverVinyl(jbyteArray bytes, int width, int height, int64_t key)
{
    auto env = CJavaWrapper::GetEnv();

    static auto method = env->GetStaticMethodID(clazz, "onVinylReady", "(J[BII)V");
    env->CallStaticVoidMethod(clazz, method, (jlong)key, bytes, width, height);
}

// Винила нет в базах текстур или его не удалось прочитать
void SnapShotsWrapper::ReportVinylFailed(int64_t key)
{
    auto env = CJavaWrapper::GetEnv();

    static auto method = env->GetStaticMethodID(clazz, "onVinylFailed", "(J)V");
    env->CallStaticVoidMethod(clazz, method, (jlong)key);
}

// TryLoadModel усыпляет игровой поток до трёх секунд на каждую незагруженную
// модель — на витрине это мгновенное зависание. Поэтому только просим модель и
// ждём её обычными кадрами, ничего не блокируя.
//
// Просим без STREAMING_KEEP_IN_MEMORY: с ним модель остаётся в памяти навсегда,
// и полка из сотни скинов с аксессуарами забивает стример до отказа.
bool SnapShotsWrapper::IsModelReady(int modelId)
{
    if (modelId <= 0) {
        return false;
    }

    auto info = CModelInfo::GetModelInfo(modelId);
    if (!info) {
        return false;
    }

    // Модели без записи в архивах не существует: ждать её бесполезно
    if (!CStreaming::GetInfo(modelId).HasCdPosnAndSize()) {
        return false;
    }

    if (!CStreaming::IsModelLoaded(modelId))
    {
        CStreaming::RequestModel(modelId, STREAMING_GAME_REQUIRED);
        return false;
    }

    // Модель и её текстуры - разные ресурсы стриминга. Дождавшись только модели,
    // снимок ловит её без текстур или с чужими, отсюда мыло. Ждём и словарь.
    if (info->m_nTxdIndex >= 0)
    {
        const int32 txd = TXDToModelId(info->m_nTxdIndex);

        if (!CStreaming::IsModelLoaded(txd))
        {
            CStreaming::RequestModel(txd, STREAMING_GAME_REQUIRED);
            return false;
        }
    }

    return info->m_pColModel != nullptr;
}

// Плитку винила рисуем по одной за кадр: каждая - это полный проход
// рендер-таргета и чтение пикселей с видеокарты.
void SnapShotsWrapper::ProcessVinyls()
{
    std::string name;
    int size = 0;
    int64_t key = 0;
    bool ready = false;

    {
        std::lock_guard<std::mutex> lock(vinylMutex);
        if (vinylQueue.empty()) {
            return;
        }

        VinylItem& item = vinylQueue.front();

        name = item.name;
        size = item.size;
        key = item.key;
        ready = ++item.warmUp > kVinylWarmUpFrames;

        if (ready) {
            vinylQueue.pop();
        }
    }

    RwTexture* tex = pSnapShotHelper->CreateVinylSnapShot(name.c_str(), size);

    if (!tex)
    {
        {
            std::lock_guard<std::mutex> lock(vinylMutex);

            if (!ready && !vinylQueue.empty() && vinylQueue.front().key == key) {
                vinylQueue.pop();
            }
        }

        ReportVinylFailed(key);
        return;
    }

    if (ready)
    {
        RwInt32 width = 0, height = 0;

        auto env = CJavaWrapper::GetEnv();

        if (jbyteArray bytes = ConvertTexToBitMapBytes(tex, width, height))
        {
            DeliverVinyl(bytes, width, height, key);
            env->DeleteLocalRef(bytes);
        }
        else
        {
            ReportVinylFailed(key);
        }
    }

    tex->refCount = 0;
    RwTextureDestroy(tex);
}

void SnapShotsWrapper::Process()
{
    if (!pSnapShotHelper || !clazz) {
        return;
    }

    ProcessVinyls();


    for (int done = 0; done < kMaxSnapsPerFrame; ++done)
    {
        QueueItem item;

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (queue.empty()) {
                return;
            }

            item = queue.front();
            queue.pop();
        }

        uint32 background = 0;
        RwTexture* tex = nullptr;

        if (!IsStillWanted(item.imageView, item.key))
        {
            ReportCancelled(item.key);
            CJavaWrapper::GetEnv()->DeleteGlobalRef(item.imageView);
            continue;
        }

        // Модель витрины может быть ещё не в стриме. Снимок без неё уходит в
        // разыменование нулевой коллизии, поэтому сначала грузим и ждём: загрузка
        // асинхронная, с первого кадра модель почти никогда не готова.
        if (!IsModelReady(item.modelId))
        {
            item.settle = 0;

            if (++item.retries <= kMaxModelWaitFrames)
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                queue.push(item);
                continue;
            }

            Log("SnapShotsWrapper: модель %d так и не загрузилась", item.modelId);

            ReportFailed(item.key);
            CJavaWrapper::GetEnv()->DeleteGlobalRef(item.imageView);
            continue;
        }

        // Модель готова, но текстурам нужно ещё несколько кадров, чтобы дойти
        // до материалов: снятый сразу снимок выходит размытым.
        if (++item.settle <= kSettleFrames)
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            queue.push(item);
            continue;
        }

        CStreaming::ms_bSnapshotMode = true;

        switch (item.type)
        {
            case Types::PED:
                // Чужой модели CPlayerPed подставит нулевую и уйдёт ждать её
                // загрузки, усыпив игровой поток
                if (!IsPedModel(item.modelId))
                {
                    CStreaming::ms_bSnapshotMode = false;

                    ReportFailed(item.key);
                    CJavaWrapper::GetEnv()->DeleteGlobalRef(item.imageView);
                    continue;
                }

                try {
                    tex = pSnapShotHelper->CreatePedSnapShot(item.modelId, background, &item.rot, item.zoom);
                } catch (...) {
                    tex = nullptr;
                }
                break;

            case Types::VEHICLE:
                try {
                    tex = pSnapShotHelper->CreateVehicleSnapShot(item.modelId, background, &item.rot, item.zoom,
                                                                 item.color1, item.color2);
                } catch (...) {
                    tex = nullptr;
                }
                break;

            case Types::OBJECT:
                try {
                    tex = pSnapShotHelper->CreateObjectSnapShot(item.modelId, background, &item.rot, item.zoom);
                } catch (...) {
                    tex = nullptr;
                }
                break;
        }

        CStreaming::ms_bSnapshotMode = false;

        auto env = CJavaWrapper::GetEnv();

        if (tex)
        {
            RwInt32 width = 0, height = 0;

            if (jbyteArray bytes = ConvertTexToBitMapBytes(tex, width, height))
            {
                SetToImageView(bytes, width, height, item.imageView, item.key);
                env->DeleteLocalRef(bytes);
            }

            tex->refCount = 0;
            RwTextureDestroy(tex);
        }
        else
        {
            ReportFailed(item.key);
        }

        env->DeleteGlobalRef(item.imageView);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wn_blackrussia_game_ui_util_EntitySnaps_nativeAddToQueue(
    JNIEnv* env, jobject thiz, jobject imageView, jint type, jint modelId,
    jfloat rotX, jfloat rotY, jfloat rotZ, jfloat zoom, jint color1, jint color2, jlong key)
{
    SnapShotsWrapper::QueueItem item {
        env->NewGlobalRef(imageView),
        modelId,
        (SnapShotsWrapper::Types)type,
        CVector(rotX, rotY, rotZ),
        zoom,
        (uint32)color1,
        (uint32)color2,
        (int64_t)key
    };

    std::lock_guard<std::mutex> lock(SnapShotsWrapper::queueMutex);

    // при быстрой прокрутке заявок набегает больше, чем движок успевает рисовать
    while (SnapShotsWrapper::queue.size() >= kMaxQueuedSnaps)
    {
        SnapShotsWrapper::ReportCancelled(SnapShotsWrapper::queue.front().key);

        env->DeleteGlobalRef(SnapShotsWrapper::queue.front().imageView);
        SnapShotsWrapper::queue.pop();
    }

    SnapShotsWrapper::queue.push(item);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wn_blackrussia_game_ui_util_EntitySnaps_nativeRequestVinyl(
    JNIEnv* env, jobject thiz, jstring name, jint size, jlong key)
{
    const char* utf = name ? env->GetStringUTFChars(name, nullptr) : nullptr;

    if (!utf || size <= 0) {
        if (utf) {
            env->ReleaseStringUTFChars(name, utf);
        }

        SnapShotsWrapper::ReportVinylFailed((int64_t)key);
        return;
    }

    SnapShotsWrapper::VinylItem item;
    item.name = utf;
    item.size = size;
    item.key  = (int64_t)key;

    env->ReleaseStringUTFChars(name, utf);

    std::vector<int64_t> dropped;

    {
        std::lock_guard<std::mutex> lock(SnapShotsWrapper::vinylMutex);

        // при быстрой прокрутке заявок набегает больше, чем движок успевает рисовать
        while (SnapShotsWrapper::vinylQueue.size() >= kMaxQueuedVinyls)
        {
            dropped.push_back(SnapShotsWrapper::vinylQueue.front().key);
            SnapShotsWrapper::vinylQueue.pop();
        }

        SnapShotsWrapper::vinylQueue.push(item);
    }

    for (int64_t droppedKey : dropped) {
        SnapShotsWrapper::ReportVinylFailed(droppedKey);
    }
}

// Проверка до показа: модели мода в стриме может не быть вовсе, и тогда предмет
// незачем ни рисовать, ни держать в списке.
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_wn_blackrussia_game_ui_util_EntitySnaps_nativeIsModelAvailable(
    JNIEnv* env, jobject thiz, jint modelId)
{
    if (modelId <= 0) {
        return JNI_FALSE;
    }

    // Коллизия появляется только после стриминга, а тут нужен ответ до него:
    // достаточно того, что модель описана и лежит в архивах игры.
    if (!CModelInfo::GetModelInfo(modelId)) {
        return JNI_FALSE;
    }

    return CStreaming::GetInfo(modelId).HasCdPosnAndSize() ? JNI_TRUE : JNI_FALSE;
}
