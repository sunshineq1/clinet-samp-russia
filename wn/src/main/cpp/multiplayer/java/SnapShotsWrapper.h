#pragma once

#include <jni.h>
#include <mutex>
#include <queue>
#include <string>

#include "main.h"
#include "game/Core/Vector.h"
#include "game/RW/rwcore.h"

class SnapShotsWrapper
{
public:
    enum class Types
    {
        OBJECT,
        PED,
        VEHICLE
    };

    struct QueueItem
    {
        jobject imageView = nullptr;
        int     modelId   = 0;
        Types   type      = Types::OBJECT;
        CVector rot       = CVector(0.0f, 0.0f, 0.0f);
        float   zoom      = 1.0f;
        uint32  color1    = 0xFFFFFFFF;
        uint32  color2    = 0xFFFFFFFF;
        int64_t key       = 0;
        int     retries   = 0;
        int     settle    = 0;
    };

    // Заявка на плитку винила: рисуется плоской текстурой, без модели и сцены
    struct VinylItem
    {
        std::string name;
        int         size   = 0;
        int64_t     key    = 0;
        int         warmUp = 0;
    };

    static inline jclass clazz = nullptr;

    static inline std::queue<QueueItem> queue;
    static inline std::mutex queueMutex;

    static inline std::queue<VinylItem> vinylQueue;
    static inline std::mutex vinylMutex;

    static void Process();

    // Модель и её текстуры готовы к снимку
    static bool IsModelReady(int modelId);
    static constexpr int kSettleFrames = 8;

private:


    static jbyteArray ConvertTexToBitMapBytes(RwTexture* tex, RwInt32& width, RwInt32& height);
    static void SetToImageView(jbyteArray bytes, int width, int height, jobject imageView, int64_t key);

    static void ProcessVinyls();
    static void DeliverVinyl(jbyteArray bytes, int width, int height, int64_t key);

public:
    static void ReportFailed(int64_t key);
    static void ReportVinylFailed(int64_t key);
    static bool IsStillWanted(jobject imageView, int64_t key);
    static void ReportCancelled(int64_t key);
};
