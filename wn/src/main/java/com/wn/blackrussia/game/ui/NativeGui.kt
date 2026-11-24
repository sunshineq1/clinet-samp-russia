package com.wn.blackrussia.game.ui

import android.view.LayoutInflater
import android.view.ViewGroup
import android.view.View
import android.widget.FrameLayout
import androidx.viewbinding.ViewBinding
import com.wn.blackrussia.game.GUIManager
import com.wn.blackrussia.R
import com.wn.blackrussia.game.Multiplayer
import kotlin.reflect.KClass

abstract class NativeGui<T : ViewBinding>(
    private var bindingClass: KClass<T>,
    private val hud: Boolean = true,
    private val chat: Boolean = true,
    inUiView: Boolean = true)
{
    private val rootView = if(inUiView) uiLayout else mainRenderScreen
    protected val binding: T by lazy {
        val inflateMethod = bindingClass.java.getMethod("inflate", LayoutInflater::class.java, ViewGroup::class.java, Boolean::class.java)
        inflateMethod.invoke(null, inflater, rootView, false) as T
    }
    private val viewToAdd by lazy { binding.root }

    // Ид известен уже в конструкторе — реестр сообщает его до вызова newInstance.
    // Иначе пакет, отправленный из init, ушёл бы с заглушкой вместо номера рисовки.
    private val ownUiId: Int = GUIManager.consumeConstructingId()

    private val uiId: Int
        get() = if (ownUiId >= 0) ownUiId else GUIManager.getUiId(this)

    // Закрытие приходит и от кнопки, и от сервера, и от «назад» — второй раз
    // считать счётчики и снимать вид нельзя, иначе интерфейс не возвращается.
    private var destroyed = false

    init {
        Multiplayer.getInstance().runOnUiThread {
            rootView.addView(viewToAdd)

            if(!hud)
                hiddenHud ++

            if(!chat)
                hiddenChat ++

            if(ownUiId == GUIManager.UI_ID_HUD)
                toggle(hiddenHud <= 0 && !GUIManager.isForcedHidden(ownUiId))

            applyInterfaceState()
        }
    }

    open fun destroy() {
        Multiplayer.getInstance().runOnUiThread {
            if (destroyed) {
                return@runOnUiThread
            }
            destroyed = true

            sendPacket(GUIManager.ACTION_ONCLOSED, "{}")

            (viewToAdd.parent as? ViewGroup)?.removeView(viewToAdd)

            if(!hud)
                hiddenHud --

            if(!chat)
                hiddenChat --

            GUIManager.deleteFromList(uiId)

            applyInterfaceState()
        }
    }

    abstract fun receivePacket(actionId: Int, json: String)

    fun sendPacket(actionId: Int, json: String = "") {
        val id = uiId
        if (id < 0) {
            return
        }

        nativeSendPacket(id, actionId, json)
    }

    open fun toggle(show: Boolean) {
        Multiplayer.getInstance().runOnUiThread {
            if(show)
                viewToAdd.visibility = View.VISIBLE
            else
                viewToAdd.visibility = View.GONE
        }
    }

    open fun bringToTop() {
        Multiplayer.getInstance().runOnUiThread {
            viewToAdd.bringToFront()
            rootView.requestLayout()
        }
    }

    // Сервер прислал создание уже живой рисовки: она могла быть спрятана или
    // перекрыта, поэтому возвращаем её наверх и делаем видимой.
    fun reopen() {
        Multiplayer.getInstance().runOnUiThread {
            if (viewToAdd.parent == null) {
                rootView.addView(viewToAdd)
            }

            toggle(true)
            bringToTop()
        }
    }

    companion object {
        private val uiLayout = Multiplayer.getInstance().findViewById<FrameLayout>(R.id.ui_layout)
        private val mainRenderScreen = Multiplayer.getInstance().findViewById<FrameLayout>(R.id.main_fl_root)
        private val inflater = LayoutInflater.from(Multiplayer.getInstance())

        external fun nativeSendPacket(uiId: Int, actionId: Int, data: String)
        external fun nativeUpdateHudState(hud: Boolean, chat: Boolean)

        var hiddenHud = 0
        var hiddenChat = 0

        // Худ живёт в отдельной рисовке, поэтому его прячем не флагом, а самим
        // экраном — иначе игровые кнопки остаются поверх открытого окна.
        fun applyInterfaceState() {
            if (hiddenHud < 0) hiddenHud = 0
            if (hiddenChat < 0) hiddenChat = 0

            val showHud = hiddenHud <= 0

            val hudAllowed = showHud && !GUIManager.isForcedHidden(GUIManager.UI_ID_HUD)

            GUIManager.get<NativeGui<*>>(GUIManager.UI_ID_HUD)?.toggle(hudAllowed)
            nativeUpdateHudState(showHud, hiddenChat <= 0)
        }
    }
}
