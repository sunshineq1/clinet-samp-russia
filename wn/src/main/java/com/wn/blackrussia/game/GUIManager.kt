package com.wn.blackrussia.game

import com.wn.blackrussia.game.ui.BrChooseServer
import com.wn.blackrussia.game.ui.donate.BrDonate
import com.wn.blackrussia.game.ui.tuning.BrTuning
import com.wn.blackrussia.game.ui.BrHud
import com.wn.blackrussia.game.ui.BrNotification
import com.wn.blackrussia.game.ui.BrPlayersList
import com.wn.blackrussia.game.ui.BrRadialMenuForCar
import com.wn.blackrussia.game.ui.BrSpawnLocation
import com.wn.blackrussia.game.ui.GUIDialogMenu
import com.wn.blackrussia.game.ui.NativeGui
import kotlin.reflect.KClass

object GUIManager {
    const val UI_ID_HUD = 0
    const val UI_ID_SELECTSERVER = 1
    const val UI_ID_HUD_MENU = 2
    const val UI_ID_SPAWN = 3
    const val UI_ID_DONATE = 22
    const val UI_ID_PLAYERS_LIST = 25
    const val UI_ID_RADIAL_CAR = 27
    const val UI_ID_TUNING = 28
    const val UI_ID_BLACKPASS = 35

    // Можно не использовать если рисовка заполняется данными с сервера например.
    // потому что любой пакет с идом GUI создаст ее если она не создана
    const val ACTION_CREATE     = -1
    const val ACTION_DESTROY    = -2

    // если нужно временно скрыть не удаляя
    const val ACTION_SHOW       = -3
    const val ACTION_HIDE       = -4

    // сообщает что рисовка закрылась
    const val ACTION_ONCLOSED   = -5
    // сделать слоем выше всех остальных
    const val ACTION_GOTOP      = -6

    private val list: HashMap<Int, KClass<out Any>> = hashMapOf(
        UI_ID_HUD to BrHud::class,
        UI_ID_SELECTSERVER to BrChooseServer::class,
        UI_ID_HUD_MENU to GUIDialogMenu::class,
        UI_ID_SPAWN to BrSpawnLocation::class,
        UI_ID_DONATE to BrDonate::class,
        UI_ID_PLAYERS_LIST to BrPlayersList::class,
        UI_ID_RADIAL_CAR to BrRadialMenuForCar::class,
        UI_ID_TUNING to BrTuning::class
    )

    private val instances: HashMap<Int, Any> = hashMapOf()

    // Идентификатор, под которым сейчас конструируется рисовка. Экран узнаёт его
    // прямо в конструкторе, поэтому пакет из init уходит с правильным ид, а не с
    // заглушкой: в instances он попадёт только после возврата из newInstance.
    private var constructingId = -999

    // Рисовки, которые сервер велел скрыть; переживает пересоздание объекта
    private val forcedHidden = HashSet<Int>()

    fun isForcedHidden(id: Int) = id in forcedHidden

    @JvmStatic
    fun consumeConstructingId(): Int = constructingId

    // get or create object
    private fun <T : Any> getInstance(id: Int): T? {
        val instance = instances[id]
        if (instance != null) {
            // println(" instance ready. return")
            return instance as T
        }

        val clazz = list[id]
        if(clazz == null) {
            // println("Class not found for ID: $id")
            return null
        }

        val previous = constructingId
        constructingId = id

        val newInst = try {
            clazz.java.newInstance() as T
        } finally {
            constructingId = previous
        }

        instances[id] = newInst
        return newInst
    }

    private fun deleteIfExists(id: Int) {
        instances[id]?.let {
            (it as NativeGui<*>).destroy()
        }
    }

    fun <T : Any> get(id: Int): T? {
        return instances[id] as? T
    }

    @JvmStatic
    fun exists(id: Int): Boolean = instances.containsKey(id)

    @JvmStatic
    fun deleteFromList(id: Int) {
        instances[id]?.let {
            instances.remove(id)
        }
    }

    // «Назад» сначала закрывает открытое окно и только потом уходит в движок,
    // иначе кнопка вываливает игрока из игры прямо из магазина.
    @JvmStatic
    fun handleBackPressed(): Boolean {
        for (id in BACK_CLOSABLE) {
            if (instances.containsKey(id)) {
                deleteIfExists(id)
                return true
            }
        }

        return false
    }

    private val BACK_CLOSABLE = listOf(
        UI_ID_DONATE, UI_ID_PLAYERS_LIST, UI_ID_RADIAL_CAR, UI_ID_TUNING
    )

    @JvmStatic
    fun getUiId(targetClass: Any): Int {
        return instances.entries.find { it.value === targetClass }?.key ?: -999
    }

    @JvmStatic
    fun receiveUiPacket(uiId: Int, actionId: Int, json: String = "") {
        Multiplayer.getInstance().runOnUiThread {
            // ACTION_DESTROY не должен создавать рисовку заново, если её уже нет
            if (actionId == ACTION_DESTROY) {
                deleteIfExists(uiId)
                return@runOnUiThread
            }

            // Скрытие запоминаем до создания рисовки: getInstance создаёт её на
            // месте, а её собственный конструктор досылает показ отложенной
            // задачей и перекрывает только что пришедшую команду скрыть.
            when (actionId) {
                ACTION_HIDE -> forcedHidden += uiId
                ACTION_SHOW, ACTION_CREATE -> forcedHidden -= uiId
            }

            val instance = getInstance<NativeGui<*>>(uiId) ?: return@runOnUiThread

            when (actionId) {
                ACTION_CREATE -> instance.reopen()
                ACTION_HIDE -> instance.toggle(false)
                ACTION_SHOW -> instance.toggle(true)
                ACTION_GOTOP -> instance.bringToTop()
                else -> instance.receivePacket(actionId, json)
            }
        }
    }
}
