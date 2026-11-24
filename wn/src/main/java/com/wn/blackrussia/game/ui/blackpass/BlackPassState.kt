package com.wn.blackrussia.game.ui.blackpass

import org.json.JSONObject

class TaskProgress(
    val id: Int,
    val progress: Int,
    val done: Boolean,
    val claimed: Boolean,
    val slot: Int
)

class RatingEntry(val place: Int, val name: String, val exp: Int)

// Всё, что приходит с сервера про конкретного игрока: уровень, опыт, что уже
// забрано и как идут задания.
class BlackPassState {
    var seasonName = ""
    var leaderName = ""
    var secondsLeft = 0

    var level = 1
    var exp = 0
    var expPerLevel = 1500

    var premium = false
    var coins = 0

    // цены знает сервер: пропуск, его расширенная версия и докупка уровня
    var premiumPrice = 0
    var deluxePrice = 0
    var levelPrice = 0

    val claimedStandard = HashSet<Int>()
    val claimedPremium = HashSet<Int>()

    var marathonDay = 0
    var marathonSecondsLeft = 0
    val claimedMarathon = HashSet<Int>()

    val tasks = ArrayList<TaskProgress>()
    var tasksSecondsLeft = 0

    val rating = ArrayList<RatingEntry>()
    var ownPlace = 0

    fun applyMain(json: JSONObject) {
        seasonName = json.optString("sn", seasonName)
        leaderName = json.optString("ln", leaderName)
        secondsLeft = json.optInt("td", secondsLeft)

        level = json.optInt("lvl", level)
        exp = json.optInt("exp", exp)
        expPerLevel = json.optInt("need", expPerLevel)

        premium = json.optInt("prem") == 1
        coins = json.optInt("coins", coins)

        premiumPrice = json.optInt("pr", premiumPrice)
        deluxePrice = json.optInt("pp", deluxePrice)
        levelPrice = json.optInt("lp", levelPrice)

        json.optJSONArray("cs")?.let { array ->
            claimedStandard.clear()
            for (i in 0 until array.length()) claimedStandard += array.optInt(i)
        }

        json.optJSONArray("cp")?.let { array ->
            claimedPremium.clear()
            for (i in 0 until array.length()) claimedPremium += array.optInt(i)
        }

        marathonDay = json.optInt("mday", marathonDay)
        marathonSecondsLeft = json.optInt("mtime", marathonSecondsLeft)

        json.optJSONArray("mc")?.let { array ->
            claimedMarathon.clear()
            for (i in 0 until array.length()) claimedMarathon += array.optInt(i)
        }
    }

    fun applyTasks(json: JSONObject) {
        tasksSecondsLeft = json.optInt("time", tasksSecondsLeft)

        val array = json.optJSONArray("tasks") ?: return

        tasks.clear()
        for (i in 0 until array.length()) {
            val entry = array.optJSONObject(i) ?: continue

            tasks += TaskProgress(
                id = entry.optInt("id"),
                progress = entry.optInt("p"),
                done = entry.optInt("d") == 1,
                claimed = entry.optInt("c") == 1,
                slot = entry.optInt("s")
            )
        }
    }

    fun taskAt(slot: Int): TaskProgress? = tasks.firstOrNull { it.slot == slot }

    // Сервер выдаёт порцию на день, а не заполняет все слоты: показывать
    // остальные ячейки замком нечестно, их просто нет.
    private fun countIn(from: Int, to: Int) = tasks.count { it.slot in from until to }

    val standardRows get() = countIn(SLOT_STANDARD, SLOT_PREMIUM)
    val premiumRows get() = countIn(SLOT_PREMIUM, SLOT_MARATHON)
    val marathonRows get() = countIn(SLOT_MARATHON, SLOT_MARATHON + MARATHON_TASKS)

    fun applyRating(json: JSONObject) {
        ownPlace = json.optInt("me", ownPlace)

        val array = json.optJSONArray("top") ?: return

        rating.clear()
        for (i in 0 until array.length()) {
            val entry = array.optJSONObject(i) ?: continue

            rating += RatingEntry(i + 1, entry.optString("n"), entry.optInt("e"))
        }
    }

    fun isUnlocked(level: Int) = level <= this.level

    fun canClaim(level: Int, premiumTrack: Boolean): Boolean {
        if (!isUnlocked(level)) return false
        if (premiumTrack && !premium) return false

        return !isClaimed(level, premiumTrack)
    }

    fun isClaimed(level: Int, premiumTrack: Boolean) =
        if (premiumTrack) claimedPremium.contains(level) else claimedStandard.contains(level)

    companion object {
        const val SLOT_STANDARD = 0
        const val SLOT_PREMIUM = 10
        const val SLOT_MARATHON = 20

        // Сетка заданий пропуска: десять строк по стандартному и премиальному
        const val PASS_TASKS = 20
        const val MARATHON_TASKS = 10

        const val EXP_STANDARD = 100
        const val EXP_PREMIUM = 200
        const val EXP_MARATHON = 50

        fun expForSlot(slot: Int) = when {
            slot >= SLOT_MARATHON -> EXP_MARATHON
            slot >= SLOT_PREMIUM -> EXP_PREMIUM
            else -> EXP_STANDARD
        }
    }
}
