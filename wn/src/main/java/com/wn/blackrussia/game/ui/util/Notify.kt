package com.wn.blackrussia.game.ui.util

import com.wn.blackrussia.game.Multiplayer
import org.json.JSONObject

// Короткий путь к тем же всплывашкам, что шлёт сервер, только без сети.
object Notify {
    const val TYPE_MONEY_RED = 0
    const val TYPE_MONEY_GREEN = 1
    const val TYPE_TEXT_RED = 2
    const val TYPE_TEXT_GREEN = 3

    fun show(type: Int, text: String, seconds: Int = 3) {
        val json = JSONObject()
            .put("t", type)
            .put("i", text)
            .put("d", seconds)
            .put("a", "")
            .put("k", "")

        Multiplayer.getInstance().localShowNotification(json)
    }

    fun notImplemented() {
        show(TYPE_TEXT_RED, "Не установлено")
    }
}
