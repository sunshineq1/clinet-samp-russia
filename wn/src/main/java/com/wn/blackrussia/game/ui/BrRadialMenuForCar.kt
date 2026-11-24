package com.wn.blackrussia.game.ui

import android.view.View
import android.view.animation.Animation
import android.view.animation.AnimationUtils
import android.widget.ImageView
import com.wn.blackrussia.R
import com.wn.blackrussia.databinding.RadialMenuDialogBinding
import com.wn.blackrussia.game.Multiplayer
import org.json.JSONObject

class BrRadialMenuForCar : NativeGui<RadialMenuDialogBinding>(
    RadialMenuDialogBinding::class,
    hud = false,
    chat = false
) {
    private class Toggle(
        val id: Int,
        val view: ImageView,
        val checked: Int,
        val unchecked: Int
    ) {
        var isOn = false
    }

    private val toggles = mutableMapOf<Int, Toggle>()

    private lateinit var clickAnim: Animation

    private var turboOpen = false
    private var lightOpen = false
    private var suspensionOpen = false

    init {
        Multiplayer.getInstance().runOnUiThread { initUI() }
    }

    private fun initUI() {
        clickAnim = AnimationUtils.loadAnimation(Multiplayer.getInstance(), R.anim.button_click)

        register(PARKING, binding.buttonParking, R.drawable.radial_parking_checked, R.drawable.radial_parking_not_checked)
        register(KEY, binding.buttonKey, R.drawable.radial_key_checked, R.drawable.radial_key_not_checked)
        register(DOORS, binding.buttonDoor, R.drawable.radial_door_checked, R.drawable.radial_door_not_checked)
        register(LAUNCH_CONTROL, binding.buttonLaunchControl, R.drawable.radial_control_checked, R.drawable.radial_control_not_checked)
        register(ENGINE, binding.buttonEngine, R.drawable.radial_engine_checked, R.drawable.radial_engine_not_checked)

        register(DRIFT, binding.buttonDrift, R.drawable.radial_drift_checked, R.drawable.radial_drift_not_checked)
        register(COMFORT_PLUS, binding.buttonComfortPlus, R.drawable.radial_comfort_plus_checked, R.drawable.radial_comfort_plus_not_checked)
        register(SPORT, binding.buttonSport, R.drawable.radial_sport_checked, R.drawable.radial_sport_not_checked)
        register(SPORT_PLUS, binding.buttonSportPlus, R.drawable.radial_sport_plus_checked, R.drawable.radial_sport_plus_not_checked)

        register(NEON, binding.buttonNeon, R.drawable.radial_neon_checked, R.drawable.radial_neon_not_checked)
        register(HIGH_LIGHT, binding.buttonHighLight, R.drawable.radial_high_light_checked, R.drawable.radial_high_light_not_checked)
        register(HEADLIGHT, binding.buttonHeadlights, R.drawable.radial_headlights_checked, R.drawable.radial_headlights_not_checked)
        register(STROBOSCOPE, binding.buttonStroboscope, R.drawable.radial_stroboscope_checked, R.drawable.radial_stroboscope_not_checked)

        register(PNEWMO_1, binding.buttonPnevmo1, R.drawable.radial_pnevmo1_checked, R.drawable.radial_pnevmo1_not_checked)
        register(PNEWMO_2, binding.buttonPnevmo2, R.drawable.radial_pnevmo2_checked, R.drawable.radial_pnevmo2_not_checked)
        register(PNEWMO_3, binding.buttonPnevmo3, R.drawable.radial_pnevmo3_checked, R.drawable.radial_pnevmo3_not_checked)
        register(HYDRAULIC, binding.buttonHydraulics, R.drawable.radial_hydraulics_checked, R.drawable.radial_hydraulics_not_checked)

        binding.radialClose.setOnClickListener {
            it.startAnimation(clickAnim)
            send(DISMISS_WINDOW)
            destroy()
        }

        binding.buttonTurbo.setOnClickListener {
            it.startAnimation(clickAnim)
            turboOpen = !turboOpen
            setTurboVisible(turboOpen)
        }

        binding.buttonLight.setOnClickListener {
            it.startAnimation(clickAnim)
            lightOpen = !lightOpen
            setLightVisible(lightOpen)
        }

        binding.buttonSuspension.setOnClickListener {
            it.startAnimation(clickAnim)
            suspensionOpen = !suspensionOpen
            setSuspensionVisible(suspensionOpen)
        }

        setTurboVisible(false)
        setLightVisible(false)
        setSuspensionVisible(false)

        binding.root.bringToFront()

        // при повторном открытии состояние надо запросить заново
        send(REQUEST_STATE)
    }

    private fun register(id: Int, view: ImageView, checked: Int, unchecked: Int) {
        val toggle = Toggle(id, view, checked, unchecked)
        toggles[id] = toggle

        view.setOnClickListener {
            it.startAnimation(clickAnim)
            send(id)
        }

        apply(toggle, false)
    }

    private fun apply(toggle: Toggle, on: Boolean) {
        toggle.isOn = on
        toggle.view.setImageResource(if (on) toggle.checked else toggle.unchecked)
    }

    private fun setState(id: Int, on: Boolean) {
        toggles[id]?.let { apply(it, on) }
        refreshSubmenuIcons()
    }

    private fun refreshSubmenuIcons() {
        binding.buttonTurbo.setImageResource(
            if (anyOn(DRIFT, COMFORT_PLUS, SPORT, SPORT_PLUS))
                R.drawable.radial_turbo_checked else R.drawable.radial_turbo_not_checked
        )
        binding.buttonLight.setImageResource(
            if (anyOn(NEON, HIGH_LIGHT, HEADLIGHT, STROBOSCOPE))
                R.drawable.radial_light_checked else R.drawable.radial_light_not_checked
        )
        binding.buttonSuspension.setImageResource(
            if (anyOn(PNEWMO_1, PNEWMO_2, PNEWMO_3, HYDRAULIC))
                R.drawable.radial_suspension_checked else R.drawable.radial_suspension_not_checked
        )
    }

    private fun anyOn(vararg ids: Int) = ids.any { toggles[it]?.isOn == true }

    private fun setTurboVisible(visible: Boolean) {
        val value = if (visible) View.VISIBLE else View.INVISIBLE

        binding.bgTurboSubmenu.visibility = value
        binding.buttonComfortPlus.visibility = value
        binding.buttonDrift.visibility = value
        binding.buttonSport.visibility = value
        binding.buttonSportPlus.visibility = value
    }

    private fun setLightVisible(visible: Boolean) {
        val value = if (visible) View.VISIBLE else View.INVISIBLE

        binding.bgLightSubmenu.visibility = value
        binding.buttonNeon.visibility = value
        binding.buttonHighLight.visibility = value
        binding.buttonHeadlights.visibility = value
        binding.buttonStroboscope.visibility = value
    }

    private fun setSuspensionVisible(visible: Boolean) {
        val value = if (visible) View.VISIBLE else View.INVISIBLE

        binding.bgSuspensionSubmenu.visibility = value
        binding.buttonPnevmo1.visibility = value
        binding.buttonPnevmo2.visibility = value
        binding.buttonPnevmo3.visibility = value
        binding.buttonHydraulics.visibility = value
    }

    private fun send(action: Int) {
        sendPacket(0, JSONObject().put(KEY_ACTION, action).toString())
    }

    override fun receivePacket(actionId: Int, json: String) {
        if (json.isEmpty()) {
            return
        }

        val packet = runCatching { JSONObject(json) }.getOrNull() ?: return

        // при открытии сервер шлёт массив включённых пунктов
        packet.optJSONArray(KEY_STATE)?.let { active ->
            toggles.values.forEach { apply(it, false) }

            for (i in 0 until active.length()) {
                toggles[active.optInt(i)]?.let { apply(it, true) }
            }

            refreshSubmenuIcons()
            return
        }

        if (packet.has(KEY_ACTION)) {
            setState(packet.optInt(KEY_ACTION), packet.optInt(KEY_STATE) != 0)
        }
    }

    private companion object {
        const val KEY_ACTION = "t"
        const val KEY_STATE = "s"

        const val REQUEST_STATE = 100
        const val DISMISS_WINDOW = 0
        const val PARKING = 1
        const val KEY = 2
        const val DOORS = 3
        const val LAUNCH_CONTROL = 5
        const val ENGINE = 6
        const val DRIFT = 8
        const val COMFORT_PLUS = 9
        const val SPORT = 10
        const val SPORT_PLUS = 11
        const val NEON = 12
        const val HIGH_LIGHT = 13
        const val HEADLIGHT = 14
        const val STROBOSCOPE = 15
        const val PNEWMO_1 = 16
        const val PNEWMO_2 = 17
        const val PNEWMO_3 = 18
        const val HYDRAULIC = 19
    }
}
