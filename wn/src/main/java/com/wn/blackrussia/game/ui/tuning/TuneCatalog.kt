package com.wn.blackrussia.game.ui.tuning

import com.wn.blackrussia.game.ui.util.AssetsJson
import org.json.JSONObject

class Tune(json: JSONObject) {
    val id = json.optInt("id")
    val type = json.optInt("type")
    val selector = json.optInt("selector")
    val tuneId = json.optInt("tuneId")
    val name: String = json.optString("name")
    var imageId: String = json.optString("imageId")
    val currency = json.optInt("currency")
    val def = json.optInt("def")

    var cost = 0
    var thisLocation = 0
    var checked = false
}

class TuneGuiNode(json: JSONObject) {
    var name: String = json.optString("name")
    val opensType = json.optInt("opensType")
    val selectorId = json.optInt("selectorId", -1)
    var imageId: String = json.optString("imageId")
    val names = ArrayList<TuneGuiNode>()

    init {
        json.optJSONArray("names")?.let { array ->
            for (i in 0 until array.length()) {
                array.optJSONObject(i)?.let { names += TuneGuiNode(it) }
            }
        }
    }
}

class TuneGuiScreen(json: JSONObject) {
    val name: String = json.optString("name")
    val footer = ArrayList<TuneGuiNode>()

    init {
        json.optJSONArray("footer")?.let { array ->
            for (i in 0 until array.length()) {
                array.optJSONObject(i)?.let { footer += TuneGuiNode(it) }
            }
        }
    }
}

class TuneVinyls(json: JSONObject) {
    val id = json.optInt("id")
    val nameVinyl: String = json.optString("nameVinyl")

    var selected = false
    var startVinyl = false
    var firstRender = false
}

object TuneCatalog {
    private const val ASSET_SCREENS = "json/tune-screens.json"
    private const val ASSET_ITEMS = "json/tune-items.json"
    private const val ASSET_VINYLS = "json/tune-vinyls.json"

    val screens = ArrayList<TuneGuiScreen>()
    val items = ArrayList<Tune>()
    val vinyls = ArrayList<TuneVinyls>()

    fun load() {
        if (screens.isNotEmpty()) return

        AssetsJson.readArray(ASSET_SCREENS)?.let { array ->
            for (i in 0 until array.length()) {
                array.optJSONObject(i)?.let { screens += TuneGuiScreen(it) }
            }
        }

        AssetsJson.read(ASSET_ITEMS)?.optJSONArray("array")?.let { array ->
            for (i in 0 until array.length()) {
                array.optJSONObject(i)?.let { items += Tune(it) }
            }
        }

        AssetsJson.readArray(ASSET_VINYLS)?.let { array ->
            for (i in 0 until array.length()) {
                array.optJSONObject(i)?.let { vinyls += TuneVinyls(it) }
            }
        }
    }

    fun screen(index: Int): TuneGuiScreen? = screens.getOrNull(index)

    fun itemsForSelector(selector: Int, gosCost: Int): ArrayList<Tune> {
        val out = ArrayList<Tune>()

        for (item in items) {
            if (item.selector != selector) continue

            item.cost = TuningPrices.detailPrice(item.type, gosCost, item.tuneId)
            out += item
        }

        return out
    }

    fun itemById(id: Int): Tune? = items.firstOrNull { it.id == id }
}
