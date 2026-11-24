package com.wn.blackrussia.game.ui.util

import android.graphics.Bitmap
import android.widget.ImageView
import com.wn.blackrussia.R
import com.wn.blackrussia.game.Multiplayer
import java.nio.ByteBuffer

// Снимок предмета: движок рисует модель в текстуру и отдаёт готовые пиксели.
class SnapShot(
    val type: Int,
    val modelId: Int,
    val rotX: Float = 0.0f,
    val rotY: Float = 0.0f,
    val rotZ: Float = 0.0f,
    val zoom: Float = 1.0f,
    val color1: Int = UNCHANGED_COLOR,
    val color2: Int = UNCHANGED_COLOR
) {
    companion object {
        const val UNCHANGED_COLOR = -1
    }
}

object EntitySnaps {
    const val TYPE_OBJECT = 0
    const val TYPE_PED = 1
    const val TYPE_VEHICLE = 2

    private external fun nativeAddToQueue(
        imageView: ImageView, type: Int, modelId: Int,
        rotX: Float, rotY: Float, rotZ: Float, zoom: Float,
        color1: Int, color2: Int, key: Long
    )

    // Снимки временно отключены: чтение текстуры рендер-таргета ещё не разобрано,
    // а попытки на каждом кадре подвешивают игру. Пока показывается картинка из
    // ресурсов, которую ставит сам список.
    const val ENABLED = true

    private external fun nativeIsModelAvailable(modelId: Int): Boolean

    // Модель мода может отсутствовать в стриме: такой предмет не показываем
    fun isModelAvailable(modelId: Int): Boolean =
        runCatching { nativeIsModelAvailable(modelId) }.getOrDefault(false)

    // Готовые снимки держим у себя: список пересобирает ячейки при каждой
    // прокрутке, а каждый повторный заказ — это заново созданная машина и
    // отрисовка в текстуру на игровом кадре. Без кэша донат просто встаёт.
    //
    // Ключ включает цвет: одна и та же машина в разной окраске — разные снимки,
    // иначе выбор цвета в обзоре ничего не меняет.
    private val cache = HashMap<Long, Bitmap>()
    private val requested = HashSet<Long>()

    private fun keyOf(snap: SnapShot): Long {
        var key = snap.type.toLong()

        key = key * 31 + snap.modelId
        key = key * 31 + snap.color1
        key = key * 31 + snap.color2

        return key
    }

    // Ячейка больше не ждёт снимок: снимаем метку, иначе опоздавшая картинка
    // ляжет поверх значка, который поставили вместо неё.
    fun clear(imageView: ImageView) {
        imageView.setTag(R.id.snap_model_tag, null)
    }

    fun load(snap: SnapShot, imageView: ImageView) {
        if (!ENABLED) {
            return
        }

        val key = keyOf(snap)

        cache[key]?.let {
            imageView.setTag(R.id.snap_model_tag, key)
            imageView.setImageBitmap(it)
            return
        }

        // заказ на этот же снимок уже в работе — второй раз не гоняем
        if (!requested.add(key)) {
            imageView.setTag(R.id.snap_model_tag, key)
            return
        }

        // модель, которой нет в игре, до очереди не доходит вовсе
        if (!isModelAvailable(snap.modelId)) {
            requested.remove(key)
            return
        }

        // ячейку списка могли переиспользовать под другой предмет, пока
        // движок рисовал модель — по метке отбрасываем опоздавший снимок
        imageView.setTag(R.id.snap_model_tag, key)

        nativeAddToQueue(
            imageView, snap.type, snap.modelId,
            snap.rotX, snap.rotY, snap.rotZ, snap.zoom,
            snap.color1, snap.color2, key
        )
    }

    @JvmStatic
    fun setToImageView(imageView: ImageView, bytes: ByteArray, width: Int, height: Int, key: Long) {
        if (bytes.isEmpty() || width <= 0 || height <= 0) {
            return
        }

        val bitmap = bitmapFromRgba(width, height, bytes)
        bitmap.setHasAlpha(true)

        cache[key] = bitmap

        imageView.post {
            if (imageView.getTag(R.id.snap_model_tag) == key) {
                imageView.setImageBitmap(bitmap)
            }
        }
    }

    // Движок спрашивает перед отрисовкой: ячейка ещё ждёт этот снимок или её
    // уже пролистали и переиспользовали под другой предмет.
    @JvmStatic
    fun isStillWanted(imageView: ImageView, key: Long): Boolean =
        imageView.getTag(R.id.snap_model_tag) == key

    // Заявку сняли не по вине модели: она просто уехала с экрана. Такое в счёт
    // неудач не идёт, иначе пролистав список трижды мы бы навсегда его погасили.
    @JvmStatic
    fun onSnapCancelled(key: Long) {
        requested.remove(key)
    }

    // Заказ не дошёл до снимка: снимаем метку, чтобы он не завис «в работе»
    // навсегда, но и повторять его бесконечно не даём — модель может быть
    // сломанной, а каждый заказ стоит игрового кадра.
    @JvmStatic
    fun onSnapFailed(key: Long) {
        requested.remove(key)

        val count = (failures[key] ?: 0) + 1
        failures[key] = count

        if (count >= MAX_FAILURES) {
            requested.add(key)
        }
    }

    private val failures = HashMap<Long, Int>()

    private const val MAX_FAILURES = 3

    // ---- винилы ---------------------------------------------------------

    private external fun nativeRequestVinyl(name: String, size: Int, key: Long)

    // Плитка тюнинга — полоска в 55dp, а винилов больше сотни, и каждый остаётся
    // в кэше до конца сессии: 128 закрывает плитку и стоит 64 КБ вместо 256.
    private const val VINYL_SIZE = 128

    private const val MAX_VINYL_FAILURES = 2

    private val vinylCache = HashMap<String, Bitmap>()
    private val vinylWaiting = HashMap<String, MutableList<(Bitmap) -> Unit>>()
    private val vinylNames = HashMap<Long, String>()
    private val vinylFailures = HashMap<String, Int>()

    private var lastVinylKey = 0L

    // Заявки ставятся с UI-потока, а ответы движка сюда же и перекидываются,
    // поэтому таблицы живут без замков.
    fun loadVinyl(name: String, onReady: (Bitmap) -> Unit) {
        if (name.isEmpty()) {
            return
        }

        vinylCache[name]?.let {
            onReady(it)
            return
        }

        // винил, которого нет в архивах текстур, не появится сколько ни проси
        if ((vinylFailures[name] ?: 0) >= MAX_VINYL_FAILURES) {
            return
        }

        // такой же винил уже рисуется — ждём вместе с остальными ячейками
        vinylWaiting[name]?.let {
            it += onReady
            return
        }

        vinylWaiting[name] = mutableListOf(onReady)

        val key = ++lastVinylKey
        vinylNames[key] = name

        runCatching { nativeRequestVinyl(name, VINYL_SIZE, key) }
            .onFailure { finishVinyl(key, null) }
    }

    @JvmStatic
    fun onVinylReady(key: Long, bytes: ByteArray, width: Int, height: Int) {
        val bitmap = if (bytes.isNotEmpty() && width > 0 && height > 0) {
            bitmapFromRgba(width, height, bytes).apply { setHasAlpha(true) }
        } else {
            null
        }

        onUiThread { finishVinyl(key, bitmap) }
    }

    @JvmStatic
    fun onVinylFailed(key: Long) {
        onUiThread { finishVinyl(key, null) }
    }

    // Движок отвечает со своего кадра, а плитку трогать можно только с UI
    // Активности может уже не быть — тогда выполняем на месте, иначе заявка
    // повиснет в списке и этот винил больше никогда не запросится заново.
    private fun onUiThread(block: () -> Unit) {
        val instance = Multiplayer.getInstance()

        if (instance == null) {
            block()
            return
        }

        instance.runOnUiThread(Runnable(block))
    }

    private fun finishVinyl(key: Long, bitmap: Bitmap?) {
        val name = vinylNames.remove(key) ?: return
        val waiting = vinylWaiting.remove(name) ?: return

        if (bitmap == null) {
            vinylFailures[name] = (vinylFailures[name] ?: 0) + 1
            return
        }

        vinylCache[name] = bitmap
        waiting.forEach { it(bitmap) }
    }

    private fun bitmapFromRgba(width: Int, height: Int, bytes: ByteArray): Bitmap {
        val bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
        bitmap.copyPixelsFromBuffer(ByteBuffer.wrap(bytes))

        return bitmap
    }
}
