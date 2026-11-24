package com.wn.blackrussia.game.ui.donate

import android.graphics.Color
import com.wn.blackrussia.game.ui.util.AssetsJson

class CarColorItem(json: org.json.JSONObject) {
    val id = json.optInt("id")
    val gameColor = json.optInt("gameColor")
    val color: Int = runCatching { Color.parseColor(json.optString("color")) }.getOrDefault(Color.BLACK)

    var selected = false
}

// Палитра выбора цвета в обзоре машины лежит рядом с витриной и одинакова для
// всех: сервер о ней ничего не знает.
object CarColorCatalog {
    private const val ASSET = "json/car-colors.json"

    val colors = ArrayList<CarColorItem>()

    fun load() {
        if (colors.isNotEmpty()) return

        val array = AssetsJson.readArray(ASSET) ?: return

        for (i in 0 until array.length()) {
            array.optJSONObject(i)?.let { colors += CarColorItem(it) }
        }

        colors.firstOrNull()?.selected = true
    }
}
