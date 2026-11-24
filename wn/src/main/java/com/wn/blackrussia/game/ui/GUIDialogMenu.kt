package com.wn.blackrussia.game.ui

import android.view.View
import androidx.recyclerview.widget.GridLayoutManager
import com.wn.blackrussia.R
import com.wn.blackrussia.databinding.MenuActionDialogBinding
import com.wn.blackrussia.game.Multiplayer
import com.wn.blackrussia.game.ui.adapter.DialogMenuAdapter
import com.wn.blackrussia.game.ui.models.DataDialogMenu
import com.wn.blackrussia.game.ui.util.Util
import java.io.UnsupportedEncodingException
import java.util.*

class GUIDialogMenu : NativeGui<MenuActionDialogBinding>(MenuActionDialogBinding::class, chat = false, hud = false) {
    private val dataDialogMenuArrayList = ArrayList<DataDialogMenu>()
    private var index = -1

    init {
        binding.brMenuCloseNew.setOnClickListener {
            destroy()
        }
        binding.brRecViewMenu.layoutManager = GridLayoutManager(Multiplayer.getInstance(), 4)

        update(false)
        // Multiplayer.getInstance().togglePlayer(1)
        Util.ShowLayout(binding.mainMenuLayoutNewLayout, true)
    }

    fun update(isCommunicating: Boolean) {
        println("Change style")

        if (!isCommunicating) {
            setMenu()
            binding.brMenuTitle.text = "Действия"

            setDataInRecyclerView(object : DialogMenuAdapter.OnUserClickListener {
                override fun click(dataDialogMenu: DataDialogMenu, view: View) {
                    index = dataDialogMenu.id
                    if(index != 3)
                        handleMenuSelection()
                    else
                        update(true)
                }
            })
        } else {
            setDialogMenu()
            binding.brMenuTitle.text = "Общение"

            setDataInRecyclerView(object : DialogMenuAdapter.OnUserClickListener {
                override fun click(dataDialogMenu: DataDialogMenu, view: View) {
                    index = dataDialogMenu.id
                    if(index != 14)
                        handleMenuSelection()
                    else
                        update(false)
                }
            })
        }
    }

    private fun setMenu() {
        dataDialogMenuArrayList.clear()
        dataDialogMenuArrayList.addAll(
            listOf(
                DataDialogMenu(8, R.drawable.br_menu_compass, "Навигатор"),
                DataDialogMenu(1, R.drawable.br_menu_taxi, "Вызов такси"),
                DataDialogMenu(2, R.drawable.br_menu_menu, "Меню"),
                DataDialogMenu(3, R.drawable.br_menu_chat, "Общение"),
                DataDialogMenu(4, R.drawable.br_menu_bag, "Инвентарь"),
                DataDialogMenu(5, R.drawable.br_menu_anim, "Анимации"),
                DataDialogMenu(6, R.drawable.br_menu_ruble, "Донат"),
                DataDialogMenu(7, R.drawable.br_menu_car, "Автомобили")
            )
        )
    }

    private fun setDialogMenu() {
        dataDialogMenuArrayList.clear()
        dataDialogMenuArrayList.addAll(
            listOf(
                DataDialogMenu(9, R.drawable.menu_passport, "Передать паспорт"),
                DataDialogMenu(10, R.drawable.menu_med, "Передать мед.карту"),
                DataDialogMenu(11, R.drawable.menu_paper, "Передать лицензии"),
                DataDialogMenu(12, R.drawable.menu_lic, "Передать ПТС"),
                DataDialogMenu(13, R.drawable.menu_exchange, "Совершить обмен"),
                DataDialogMenu(14, R.drawable.menu_back, "Назад")
            )
        )
    }

    private fun setDataInRecyclerView(onUserClickListener: DialogMenuAdapter.OnUserClickListener) {
        binding.brRecViewMenu.adapter = DialogMenuAdapter(dataDialogMenuArrayList, onUserClickListener)
    }

    private fun handleMenuSelection() {
        try {
            when (index) {
                1 -> Multiplayer.getInstance().sendCommand("/taxi".toByteArray(charset("windows-1251")))
                2 -> Multiplayer.getInstance().sendCommand("/mm".toByteArray(charset("windows-1251")))
                4 -> Multiplayer.getInstance().sendCommand("/inv".toByteArray(charset("windows-1251")))
                5 -> Multiplayer.getInstance().sendCommand("/anim".toByteArray(charset("windows-1251")))
                6 -> Multiplayer.getInstance().sendCommand("/donate".toByteArray(charset("windows-1251")))
                7 -> Multiplayer.getInstance().sendCommand("/car".toByteArray(charset("windows-1251")))
                8 -> Multiplayer.getInstance().sendCommand("/gps".toByteArray(charset("windows-1251")))
                9 -> Multiplayer.getInstance().sendCommand("/pass".toByteArray(charset("windows-1251")))
                10 -> Multiplayer.getInstance().sendCommand("/medpass".toByteArray(charset("windows-1251")))
                11 -> Multiplayer.getInstance().sendCommand("/showlic".toByteArray(charset("windows-1251")))
                12 -> Multiplayer.getInstance().sendCommand("/carpass".toByteArray(charset("windows-1251")))
                13 -> Multiplayer.getInstance().sendCommand("/exchange".toByteArray(charset("windows-1251")))
            }

            destroy()
        } catch (e: UnsupportedEncodingException) {
            e.printStackTrace()
        }
    }

    override fun receivePacket(actionId: Int, json: String) {
    }
}
