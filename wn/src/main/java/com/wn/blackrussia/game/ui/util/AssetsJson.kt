package com.wn.blackrussia.game.ui.util

import com.wn.blackrussia.game.Multiplayer
import org.json.JSONArray
import org.json.JSONObject

object AssetsJson {
    private val cache = HashMap<String, JSONObject>()
    private val arrays = HashMap<String, JSONArray>()

    fun read(path: String): JSONObject? {
        cache[path]?.let { return it }

        val json = runCatching { JSONObject(text(path) ?: return null) }.getOrNull() ?: return null

        cache[path] = json
        return json
    }

    // часть файлов витрины лежит массивом в корне, без объекта-обёртки
    fun readArray(path: String): JSONArray? {
        arrays[path]?.let { return it }

        val json = runCatching { JSONArray(text(path) ?: return null) }.getOrNull() ?: return null

        arrays[path] = json
        return json
    }

    private fun text(path: String): String? = runCatching {
        Multiplayer.getInstance().assets.open(path).bufferedReader(Charsets.UTF_8).use { it.readText() }
    }.getOrNull()
}
