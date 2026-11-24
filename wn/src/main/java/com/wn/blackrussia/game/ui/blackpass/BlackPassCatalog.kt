package com.wn.blackrussia.game.ui.blackpass

import com.wn.blackrussia.game.ui.util.AssetsJson
import org.json.JSONObject

class BlackPassReward(json: JSONObject) {
    val id = json.optInt("id")
    val name: String = json.optString("name")
    val typeId = json.optInt("typeId")
    val awardId = json.optInt("awardId")
    val count = json.optInt("count")
    val typeReward = json.optInt("typeReward")

    companion object {
        const val TYPE_EXP = 0
        const val TYPE_MONEY = 1
        const val TYPE_CAR = 2
        const val TYPE_COINS = 3
        const val TYPE_ITEM = 5
        const val TYPE_CASE = 6
        const val TYPE_COUPON_X2 = 18
        const val TYPE_VIP = 19

        // В сезонной таблице для тех же наград встречается второй набор номеров
        const val TYPE_VIP_ALT = 4
        const val TYPE_ITEM_ALT = 7
        const val TYPE_CAR_ALT = 8
        const val TYPE_HOUSE = 10
    }
}

class BlackPassTask(json: JSONObject) {
    val id = json.optInt("id")
    val type: String = json.optString("type")
    val text: String = json.optString("text")
    val description: String = json.optString("description")
    val count = json.optInt("count", 1)
    val subtype = json.optInt("subtype")
    val typeInt = json.optInt("typeint")
    val award = json.optInt("award")
    val complexity = json.optInt("complexity")

    val isWeekly: Boolean
        get() = complexity > 0
}

// Награды сезона и список заданий одинаковы у всех игроков, поэтому лежат в
// assets; сервер присылает только прогресс.
object BlackPassCatalog {
    private const val ASSET = "json/black-pass.json"

    val standard = ArrayList<BlackPassReward>()
    val premium = ArrayList<BlackPassReward>()
    val marathon = ArrayList<BlackPassReward>()
    val tasks = ArrayList<BlackPassTask>()

    var levelPrice = 150
        private set

    var loaded = false
        private set

    val levelCount: Int
        get() = maxOf(standard.size, premium.size)

    fun load() {
        if (loaded) return

        val root = AssetsJson.read(ASSET) ?: return

        root.optJSONArray("StandartLevel")?.let { array ->
            for (i in 0 until array.length()) {
                array.optJSONObject(i)?.let { standard += BlackPassReward(it) }
            }
        }

        root.optJSONArray("PremiumLevel")?.let { array ->
            for (i in 0 until array.length()) {
                array.optJSONObject(i)?.let { premium += BlackPassReward(it) }
            }
        }

        root.optJSONArray("MarathonLevel")?.let { array ->
            for (i in 0 until array.length()) {
                array.optJSONObject(i)?.let { marathon += BlackPassReward(it) }
            }
        }

        root.optJSONArray("Tasks")?.let { array ->
            for (i in 0 until array.length()) {
                array.optJSONObject(i)?.let { tasks += BlackPassTask(it) }
            }
        }

        levelPrice = root.optInt("LevelPrices", levelPrice)

        loaded = true
    }

    fun standardAt(level: Int): BlackPassReward? = standard.getOrNull(level - 1)

    fun premiumAt(level: Int): BlackPassReward? = premium.getOrNull(level - 1)

    fun task(id: Int): BlackPassTask? = tasks.firstOrNull { it.id == id }
}
