package com.wn.blackrussia.game.ui.util

import android.animation.Animator
import android.animation.AnimatorListenerAdapter
import android.app.Activity
import android.content.ActivityNotFoundException
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.text.Html
import android.text.Spanned
import android.util.DisplayMetrics
import android.view.View
import android.view.WindowManager
import android.widget.Toast
import com.google.gson.Gson
import com.wn.blackrussia.game.Multiplayer
import java.io.File
import java.util.Timer
import java.util.regex.Pattern
import kotlin.concurrent.schedule

object KUtils {
    private val screenSizeInternal: Pair<Int, Int> by lazy { calculateScreenSize() }

    val screenSize: Pair<Int, Int>
        get() = screenSizeInternal

    private fun calculateScreenSize(): Pair<Int, Int> {
        val displayMetrics = DisplayMetrics()
        val windowManager = Multiplayer.getInstance().getSystemService(Context.WINDOW_SERVICE) as WindowManager
        windowManager.defaultDisplay.getRealMetrics(displayMetrics)

        return Pair(displayMetrics.widthPixels, displayMetrics.heightPixels)
    }

    fun pxFromDp(dp: Float): Float {
        return dp * Multiplayer.getInstance().resources.displayMetrics.density
    }

    @JvmStatic
    fun ShowLayout(view: View?, isAnim: Boolean) {
        if (view != null) {
            if (isAnim) {
                view.alpha = 0.0f
                fadeIn(view)
            } else {
                view.alpha = 1.0f
            }
            view.visibility = View.VISIBLE
            view.invalidate()
        }
    }

    @JvmStatic
    fun HideLayout(view: View?, isAnim: Boolean) {
        if (view != null) {
            if (isAnim) {
                fadeOut(view)
                return
            }
            view.alpha = 0.0f
            view.visibility = View.GONE
            view.invalidate()
        }
    }

    private fun fadeIn(view: View?) {
        view?.animate()?.setDuration(300)?.setListener(object : AnimatorListenerAdapter() {
            override fun onAnimationEnd(animation: Animator) {
                super.onAnimationEnd(animation)
            }
        })?.alpha(1.0f)
    }

    private fun fadeOut(view: View?) {
        view?.animate()?.setDuration(300)?.setListener(object : AnimatorListenerAdapter() {
            override fun onAnimationEnd(animation: Animator) {
                view.visibility = View.GONE
                super.onAnimationEnd(animation)
            }
        })?.alpha(0.0f)
    }

    @JvmStatic
    fun HideLayoutWithoutGone(view: View?, isAnim: Boolean) {
        if (view != null) {
            if (isAnim) {
                fadeOutWithoutGone(view)
                return
            }
            view.alpha = 0.0f
            view.visibility = View.INVISIBLE
            view.invalidate()
        }
    }

    private fun fadeInWithoutGone(view: View?) {
        view?.animate()?.setDuration(150)?.setListener(object : AnimatorListenerAdapter() {
            override fun onAnimationEnd(animation: Animator) {
                super.onAnimationEnd(animation)
            }
        })?.alpha(1.0f)
    }

    private fun fadeOutWithoutGone(view: View?) {
        view?.animate()?.setDuration(150)?.setListener(object : AnimatorListenerAdapter() {
            override fun onAnimationEnd(animation: Animator) {
                view.visibility = View.INVISIBLE
                super.onAnimationEnd(animation)
            }
        })?.alpha(0.0f)
    }

    @JvmStatic
    fun transfromColors(inputText: String): Spanned {
        val pattern = Pattern.compile("\\{(.{6})\\}([^\\{]*)")
        val matcher = pattern.matcher(inputText)
        val spannableStringBuilder = StringBuilder()
        var currentIndex = 0

        while (matcher.find()) {
            val colorHex = matcher.group(1)
            val textToColor = matcher.group(2)

            val startIndex = matcher.start()
            val endIndex = matcher.end()

            spannableStringBuilder.append(inputText.substring(currentIndex, startIndex))
            spannableStringBuilder.append("<font color='#$colorHex'>$textToColor</font>")
            currentIndex = endIndex
        }

        spannableStringBuilder.append(inputText.substring(currentIndex))

        return Html.fromHtml(spannableStringBuilder.toString().replace("\n", "<br>"))
    }

    @JvmStatic
    fun transformColorsToBlack(inputText: String): Spanned {
        val pattern = Pattern.compile("\\{.{6}\\}")
        val matcher = pattern.matcher(inputText)
        val processedText = matcher.replaceAll("")

        return Html.fromHtml(processedText.replace("\n", "<br>"), Html.FROM_HTML_MODE_LEGACY)
    }

    fun openURL(activity: Activity, url: String) {
        val address = Uri.parse(url)
        val openlink = Intent(Intent.ACTION_VIEW, address)

        try {
            activity.startActivity(openlink)
        } catch (e: ActivityNotFoundException) {
            Toast.makeText(activity, "Не удалось открыть ссылку", Toast.LENGTH_LONG).show()
        }
    }

    fun runAfterDelay(delayMillis: Long, action: () -> Unit) {
        Timer().schedule(delayMillis) {
            Multiplayer.getInstance().runOnUiThread {
                action()
            }
        }
    }

    inline fun <reified T> saveToJsonFile(data: T, filename: String) {
        val file = File(Multiplayer.getInstance().getExternalFilesDir(null), filename)
        file.parentFile?.mkdirs()

        val jsonString = Gson().toJson(data)
        file.writeText(jsonString)
    }

    inline fun <reified T> loadFromJsonFile(filename: String): T {
        val file = File(Multiplayer.getInstance().getExternalFilesDir(null), filename)
        file.parentFile?.mkdirs()

        if(file.exists()) {
            val jsonString = file.readText()
            return Gson().fromJson(jsonString, T::class.java)
        }
        return T::class.java.newInstance()
    }
}