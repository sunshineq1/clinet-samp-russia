package com.wn.blackrussia.game.ui

import android.animation.Animator
import android.animation.AnimatorListenerAdapter
import android.animation.ValueAnimator
import android.view.animation.DecelerateInterpolator
import java.text.NumberFormat
import android.view.View
import androidx.core.content.ContextCompat
import com.google.gson.Gson
import com.wn.blackrussia.R
import com.wn.blackrussia.game.ui.util.Utils
import com.wn.blackrussia.databinding.BrHudBinding
import com.wn.blackrussia.game.GUIManager
import com.wn.blackrussia.game.GUIManager.ACTION_CREATE
import com.wn.blackrussia.game.GUIManager.UI_ID_HUD_MENU
import com.wn.blackrussia.game.Multiplayer
import com.wn.blackrussia.game.ui.util.ConvertViewCoordsToGta
import com.wn.blackrussia.game.ui.util.Util
import kotlinx.coroutines.sync.Mutex
import java.text.DecimalFormat
import java.text.DecimalFormatSymbols
import java.util.Formatter

class BrHud : NativeGui<BrHudBinding>(BrHudBinding::class, chat = true, hud = true) {
    private external fun hudInit()

    private var isHudSetPos = false
    private external fun setRadarBgPos(x1: Float, y1: Float, x2: Float, y2: Float)
    private external fun setRadarPos(x1: Float, y1: Float, width: Float, height: Float)

    init {
        hudInit()

        Multiplayer.getInstance().runOnUiThread {
            binding.brHudGButt.setOnClickListener {
                Multiplayer.getInstance().sendG()
            }
            binding.brHudFist.setOnClickListener {
                Multiplayer.getInstance().specialCall(5)
            }

            binding.brHudRadar.post {
                if (isHudSetPos)
                    return@post

                setRadarBgPos(
                    binding.brHudRadar.x,
                    binding.brHudRadar.y,
                    binding.brHudRadar.width.toFloat(),
                    binding.brHudRadar.height.toFloat()
                )

                val data = ConvertViewCoordsToGta.convertCoordsToGta(
                    ConvertViewCoordsToGta.Data(
                        binding.brHudRadar.x,
                        binding.brHudRadar.y,
                        binding.brHudRadar.width.toFloat(),
                        binding.brHudRadar.height.toFloat()
                    )
                )
                setRadarPos(data.x, data.y, data.width * 0.9f, data.width * 0.9f)
                binding.brHudRadar.visibility = View.INVISIBLE
                isHudSetPos = true
            }
            binding.brHudMenu.setOnClickListener {
                if (Multiplayer.getInstance().isPlayerDriving()) {
                    GUIManager.receiveUiPacket(GUIManager.UI_ID_RADIAL_CAR, ACTION_CREATE)
                } else {
                    GUIManager.receiveUiPacket(UI_ID_HUD_MENU, ACTION_CREATE)
                }
            }

            Utils.ShowLayout(binding.root, true)
        }
    }

    fun updateAmmo(fistId: Int, ammo: Int, ammoIn: Int) {
        Multiplayer.getInstance().runOnUiThread {
            if (ammo > 0 && ammoIn > 0) {
                binding.brHudAmmo.text = ammoIn.toString()
                binding.brHudAmmoIn.text = ("/" + (ammo - ammoIn))
            } else {
                binding.brHudAmmo.text = " "
                binding.brHudAmmoIn.text = " "
            }
            if (fistId >= 0) {
                val backgroundDrawableId = Multiplayer.getInstance().resources.getIdentifier(
                    String.format("weapon_%d", fistId),
                    "drawable",
                    Multiplayer.getInstance().packageName
                )

                if (backgroundDrawableId != 0) {
                    val drawable = ContextCompat.getDrawable(Multiplayer.getInstance(), backgroundDrawableId)
                    if (drawable != null) {
                        binding.brHudFist.background = drawable
                    }
                }
            }
        }
    }

    fun updateG(value: Int) {
        Multiplayer.getInstance().runOnUiThread {
            if (value == 0)
                binding.brHudGButt.visibility = View.GONE
            else
                binding.brHudGButt.visibility = View.VISIBLE
        }
    }

    fun updateGreenZone(value: Int) {
        Multiplayer.getInstance().runOnUiThread {
            if (value == 0)
                binding.brHudGreenZone.visibility = View.GONE
            else
                binding.brHudGreenZone.visibility = View.VISIBLE
        }
    }
    fun updateGPS(value: Int) {
        Multiplayer.getInstance().runOnUiThread {
            if (value == 0)
                binding.brHudGps.visibility = View.GONE
            else
                binding.brHudGps.visibility = View.VISIBLE
        }
    }

    fun updateX2(value: Int) {
        Multiplayer.getInstance().runOnUiThread {
            if (value == 0)
                binding.brHudDoubleLogo.visibility = View.GONE
            else
                binding.brHudDoubleLogo.visibility = View.VISIBLE
        }
    }

    private var moneyShown = 0
    private var moneyAnimator: ValueAnimator? = null

    private fun formatMoney(money: Int): String {
        val formatter = DecimalFormat()
        val symbols = DecimalFormatSymbols.getInstance()
        symbols.groupingSeparator = '.'
        formatter.decimalFormatSymbols = symbols

        return formatter.format(money)
    }

    fun formatMoneyAsBR(money: Int) {
        moneyAnimator?.cancel()

        if (moneyShown == money) {
            binding.brHudMoney.text = formatMoney(money)
            return
        }

        val difference = kotlin.math.abs(money - moneyShown)
        if (difference < MONEY_COUNT_MIN_DIFF) {
            moneyShown = money
            binding.brHudMoney.text = formatMoney(money)
            return
        }

        val from = moneyShown
        val duration = (MONEY_COUNT_MS_PER_UNIT * difference)
            .coerceIn(MONEY_COUNT_MIN_MS, MONEY_COUNT_MAX_MS)

        moneyAnimator = ValueAnimator.ofInt(from, money).apply {
            this.duration = duration
            interpolator = DecelerateInterpolator()
            addUpdateListener { animation ->
                moneyShown = animation.animatedValue as Int
                binding.brHudMoney.text = formatMoney(moneyShown)
            }
            addListener(object : AnimatorListenerAdapter() {
                override fun onAnimationEnd(animation: Animator) {
                    moneyShown = money
                    binding.brHudMoney.text = formatMoney(money)
                }
            })
            start()
        }
    }

    companion object BalanceAnimation {
        private const val MONEY_COUNT_MIN_DIFF = 100
        private const val MONEY_COUNT_MS_PER_UNIT = 2L
        private const val MONEY_COUNT_MIN_MS = 350L
        private const val MONEY_COUNT_MAX_MS = 1200L
    }

    fun close() {
        moneyAnimator?.cancel()
        moneyAnimator = null

        Utils.HideLayout(binding.root, true)
        destroy()
    }

    private val packetHealth = 0
    private val packetArmour = 1
    private val packetMoney = 2
    private val packetServer = 3
    private val packetFist = 4
    private val packetStars = 5
    data class PacketGetOne(
        val value: Int
    )
    data class PacketGetTwo(
        val value1: Int,
        val value2: Int
    )
    data class PacketGetThree(
        val value1: Int,
        val value2: Int,
        val value3: Int
    )
    override fun receivePacket(actionId: Int, json: String) {
        when(actionId) {
            packetHealth -> {
                val json = Gson().fromJson(json, PacketGetOne::class.java)
                binding.brHudHeart.progress = json.value
            }
            packetArmour -> {
                val json = Gson().fromJson(json, PacketGetOne::class.java)
                binding.brHudArmour.progress = json.value
            }
            packetMoney -> {
                val json = Gson().fromJson(json, PacketGetOne::class.java)
                formatMoneyAsBR(json.value)
            }
            packetServer -> {
                val json = Gson().fromJson(json, PacketGetOne::class.java)
                when(json.value) {
                    -1 -> { // .. test server
                        binding.brHudLogo.visibility = View.INVISIBLE
                    }
                    0 -> {
                        binding.brHudLogo.visibility = View.VISIBLE
                        binding.brHudLogo.setImageResource(R.drawable.logobrred)
                    }
                    1 -> {
                        binding.brHudLogo.visibility = View.VISIBLE
                        binding.brHudLogo.setImageResource(R.drawable.logobrgreen)
                    }
                    2 -> {
                        binding.brHudLogo.visibility = View.VISIBLE
                        binding.brHudLogo.setImageResource(R.drawable.logobrblue)
                    }
                    3 -> {
                        binding.brHudLogo.visibility = View.VISIBLE
                        binding.brHudLogo.setImageResource(R.drawable.logobryellow)
                    }
                }
            }
        }
    }
}