package com.wn.blackrussia.game.ui.tuning

import android.os.Handler
import android.os.Looper
import android.view.View
import android.widget.Button
import android.widget.ImageView
import android.widget.TextView
import androidx.core.content.ContextCompat
import com.wn.blackrussia.R

class TuningLogicPopupWindowApply(
    private val root: BrTuning,
    private val view: View,
    private val typeMenu: Int
) {

    private lateinit var titleChange: TextView
    private lateinit var textAction: TextView
    private lateinit var valuePrice: TextView
    private lateinit var valueCurrency: TextView
    private lateinit var buttonAction: Button
    private lateinit var buttonCloseWindow: ImageView
    private lateinit var iconMoney: ImageView

    private val handler = Handler(Looper.getMainLooper())

    var selectorId = -1

    private var action = 0
    private var price = 0
    private var currency = ""

    // Снимок выбранного делается в момент показа окна: пока игрок подтверждает
    // покупку, список под окном уже мог отдать пустое значение
    private var detailItem: Tune? = null
    private var tuneVinyl: TuneVinyls? = null
    private var color = ""
    private var collapse = 0

    fun setDataInPopupWindow(action: Int, cost: Int, currency: String) {
        this.action = action
        this.price = cost
        this.currency = currency
    }

    fun startWindow() {
        findViews()
        setDataInLayout()
        setListeners()
    }

    private fun findViews() {
        titleChange = view.findViewById(R.id.change_title_submenu_apply)
        textAction = view.findViewById(R.id.title_price_action)
        valuePrice = view.findViewById(R.id.tuning_value_price_apply)
        valueCurrency = view.findViewById(R.id.tuning_currency_apply)
        buttonAction = view.findViewById(R.id.button_apply_buy)
        buttonCloseWindow = view.findViewById(R.id.button_close_submenu_apply)
        iconMoney = view.findViewById(R.id.icon_money_apply)
    }

    private fun setDataInLayout() {
        when (action) {
            TuningConstants.ACTION_REPAIR -> {
                titleChange.setText(R.string.tuning_submenu_change_title_repair)
                textAction.setText(R.string.tuning_submenu_title_repair)
                buttonAction.setCompoundDrawablesRelativeWithIntrinsicBounds(
                    ContextCompat.getDrawable(view.context, R.drawable.tuning_repair),
                    null, null, null
                )
                buttonAction.setText(R.string.tuning_submenu_button_repeat)
            }

            TuningConstants.ACTION_BUY,
            TuningConstants.ACTION_BUY_COLLAPSE,
            TuningConstants.ACTION_BUY_IN_MAIN_MENU -> {
                titleChange.setText(R.string.tuning_submenu_change_title_buy)
                textAction.setText(R.string.tuning_submenu_title_buy)
                buttonAction.setText(R.string.tuning_submenu_button_buy)
            }

            TuningConstants.ACTION_DIAGNOSTIC -> {
                titleChange.setText(R.string.tuning_submenu_change_title_diagnostic)
                textAction.setText(R.string.tuning_submenu_title_diagnostic)
                buttonAction.setText(R.string.tuning_submenu_button_diagnostic)
            }
        }

        when (currency) {
            CURRENCY_RUB -> iconMoney.setImageResource(R.drawable.tuning_icon_gold)
            CURRENCY_BC -> iconMoney.setImageResource(R.drawable.tuning_icon_black_coin)
        }

        valuePrice.text = BrTuning.priceWithSpaces(price)
        valueCurrency.text = currency

        root.layoutDetailsList.buyThisDetail?.let { detailItem = it }
        root.layoutDetailsList.selectedVinyl?.let { tuneVinyl = it }

        if (root.colorWithTransparency != "") {
            color = root.colorWithTransparency
        }

        if (root.collapse != 0) {
            collapse = root.collapse
        }
    }

    private fun setListeners() {
        buttonAction.setOnClickListener { button ->
            button.startAnimation(root.animation())
            applyAction()
            handler.postDelayed({ root.closeApplyWindow() }, CLOSE_DELAY)
        }

        buttonCloseWindow.setOnClickListener { button ->
            button.startAnimation(root.animation())
            handler.postDelayed({ root.closeApplyWindow() }, CLOSE_DELAY)
        }
    }

    private fun applyAction() {
        when (action) {
            TuningConstants.ACTION_REPAIR ->
                root.sendIntegerData(TuningConstants.OPERATION_REPAIR_DETAIL, root.repairDetail)

            TuningConstants.ACTION_BUY -> buyByTypeMenu()

            // Диагностику запускаем позже закрытия окна, иначе её заставка
            // успевает мигнуть под ещё видимым подтверждением
            TuningConstants.ACTION_DIAGNOSTIC -> handler.postDelayed(
                { root.sendAction(TuningConstants.OPERATION_DIAGNOSTIC) }, DIAGNOSTIC_DELAY
            )

            TuningConstants.ACTION_BUY_COLLAPSE -> root.sendIntegerDataWithSelector(
                TuningConstants.OPERATION_BUY_COLLAPSE, collapse, selectorId
            )

            TuningConstants.ACTION_BUY_IN_MAIN_MENU ->
                root.sendIntegerData(TuningConstants.OPERATION_BUY_DETAIL, selectorId)
        }
    }

    private fun buyByTypeMenu() {
        when (typeMenu) {
            TuningConstants.RGB_TYPE_MENU -> root.sendColorWithSelector(
                TuningConstants.OPERATION_BUY_COLOR, root.rgbColor, selectorId
            )

            TuningConstants.READY_COLOR_TYPE_MENU -> root.sendColorWithSelector(
                TuningConstants.OPERATION_BUY_COLOR, color, selectorId
            )

            TuningConstants.DETAILS_LIST_TYPE_MENU -> detailItem?.let { detail ->
                root.sendIntegerData(TuningConstants.OPERATION_BUY_DETAIL, detail.id)
            }

            TuningConstants.VINYLS_LIST_TYPE_MENU -> tuneVinyl?.let { vinyl ->
                root.sendIntegerData(TuningConstants.OPERATION_BUY_VINYL, vinyl.id)
            }
        }
    }

    companion object {
        private const val CURRENCY_RUB = " ₽"
        private const val CURRENCY_BC = " BC"

        private const val CLOSE_DELAY = 250L
        private const val DIAGNOSTIC_DELAY = 280L
    }
}
