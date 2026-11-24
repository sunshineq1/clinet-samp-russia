package com.wn.blackrussia.game.ui.tuning

import android.os.Handler
import android.os.Looper
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.animation.Animation
import android.view.animation.AnimationUtils
import android.widget.RelativeLayout
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.LinearLayoutManager
import com.wn.blackrussia.R
import com.wn.blackrussia.databinding.TuningFragmentDiagnosticsBinding
import com.wn.blackrussia.game.common.UILayout

class UILayoutTuningDiagnostics(private val screen: BrTuning) : UILayout(),
    TuningDetailsInDiagnosticAdapter.OnClickDetailInDiagnosticItem {

    private var root: View? = null
    private lateinit var binding: TuningFragmentDiagnosticsBinding
    private lateinit var anim: Animation
    private lateinit var shortListParams: RelativeLayout.LayoutParams

    private val details = ArrayList<TuningDetailDiagnosticItem>()
    private var adapter: TuningDetailsInDiagnosticAdapter? = null

    private val costs = IntArray(DETAILS_IN_ENGINE)
    private var gosCost = 0
    private var diagnosticCost = 0
    private var repairCost = 0

    private var statusDiagnostic = -1

    var thisRepairDetail = -1
        private set

    private val handler = Handler(Looper.getMainLooper())

    override fun getView(): View? = root

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup): View {
        binding = TuningFragmentDiagnosticsBinding.inflate(inflater)
        root = binding.root

        anim = AnimationUtils.loadAnimation(context, R.anim.button_click)

        // Короткий список деталей не достаёт до правого края, поэтому кнопки
        // переезжают к невидимой опоре
        shortListParams = RelativeLayout.LayoutParams(
            RelativeLayout.LayoutParams.WRAP_CONTENT,
            RelativeLayout.LayoutParams.WRAP_CONTENT
        ).apply {
            addRule(RelativeLayout.ALIGN_END, R.id.help_view_in_diagnostic)
            addRule(RelativeLayout.ALIGN_BOTTOM, R.id.help_view_in_diagnostic)
        }

        binding.tuningDiagnostics.setOnClickListener { view ->
            view.startAnimation(anim)

            if (statusDiagnostic == TuningConstants.STATUS_NOT_ACTUAL_DIAGNOSTIC) {
                screen.applyBuyNewItem(
                    TuningConstants.ACTION_DIAGNOSTIC, diagnosticCost, RUBLES,
                    NO_SELECTOR, TuningConstants.OTHER_TYPE_MENU
                )
                return@setOnClickListener
            }

            if (statusDiagnostic == TuningConstants.STATUS_ACTUAL_DIAGNOSTIC && repairCost != 0) {
                screen.sendIntegerData(TuningConstants.OPERATION_CHECK_FOR_REPAIR_DETAIL, thisRepairDetail)
            }
        }

        binding.backToSubmenuDiagnostics.setOnClickListener { view ->
            view.startAnimation(anim)
            handler.postDelayed({ screen.backToSubmenu() }, CLICK_DELAY)
        }

        return binding.root
    }

    override fun onLayoutShown() {}

    override fun onLayoutClose() {}

    fun setStatusDiagnosticAndChangeParameters(status: Int) {
        statusDiagnostic = status
        changeViewButton(status)
        showStatus(status)
    }

    fun setStartParameters(detailsStatus: List<Int>) {
        gosCost = screen.gosCost
        fillCosts()
        diagnosticCost = Math.round(gosCost * 0.005 / 100.0 + 2000.0).toInt()

        if (detailsStatus.isEmpty()) {
            return
        }

        if (detailsStatus.size == DETAILS_IN_ELECTRO) {
            fillElectroDetails()
        } else {
            fillEngineDetails()
        }

        for (i in details.indices) {
            details[i].valueState = detailsStatus[i]
            details[i].cost = costs[i]
        }

        adapter = TuningDetailsInDiagnosticAdapter(screen, context, details, this, statusDiagnostic)

        binding.recvDetailsListDiagnostics.layoutManager =
            LinearLayoutManager(binding.root.context, LinearLayoutManager.HORIZONTAL, false)
        binding.recvDetailsListDiagnostics.adapter = adapter

        repairCost = 0

        if (details.size < SHORT_LIST_SIZE) {
            binding.menuBlockDiagnostics.layoutParams = shortListParams
        }
    }

    fun updateStatusDetail(idDetail: Int) {
        for (i in details.indices) {
            if (i == idDetail) {
                details[i].valueState = FULL_STATE
                adapter?.notifyItemChanged(i)
            }
        }
    }

    fun showApplyRepair() {
        screen.applyBuyNewItem(
            TuningConstants.ACTION_REPAIR, repairCost, RUBLES,
            NO_SELECTOR, TuningConstants.OTHER_TYPE_MENU
        )
    }

    fun startDiagnostic() {
        screen.startDiagnostic()
    }

    override fun clickInDiagnostic(detailsItem: TuningDetailDiagnosticItem, position: Int, view: View) {
        adapter?.setOnlyItemClickable(position)
        repairCost = detailsItem.cost
        thisRepairDetail = detailsItem.id
    }

    private fun fillCosts() {
        costs[0] = Math.round(gosCost * 1.0 / 100.0).toInt()
        costs[1] = Math.round(gosCost * 0.005 / 100.0 + 2000.0).toInt()
        costs[2] = Math.round(gosCost * 0.5 / 100.0).toInt()
        costs[3] = Math.round(gosCost * 1.0 / 100.0).toInt()
        costs[4] = Math.round(gosCost * 0.5 / 100.0).toInt()
        costs[5] = Math.round(gosCost * 0.005 / 100.0 + 500.0).toInt()
        costs[6] = Math.round(gosCost * 0.005 / 100.0 + 5000.0).toInt()
    }

    private fun fillElectroDetails() {
        details.clear()
        details += TuningDetailDiagnosticItem(0, "Электродвигатель", "t_electro_engine_icon")
        details += TuningDetailDiagnosticItem(1, "Подвеска", "t_suspension_icon")
        details += TuningDetailDiagnosticItem(2, "Аккумулятор", "t_battery_icon")
    }

    private fun fillEngineDetails() {
        details.clear()
        details += TuningDetailDiagnosticItem(0, "Двигатель", "t_engine_icon")
        details += TuningDetailDiagnosticItem(1, "Воздушный фильтр", "t_air_filter_icon")
        details += TuningDetailDiagnosticItem(2, "Топливная система", "t_fuel_system_icon")
        details += TuningDetailDiagnosticItem(3, "Трансмиссия", "t_transmission_icon")
        details += TuningDetailDiagnosticItem(4, "Подвеска", "t_suspension_icon")
        details += TuningDetailDiagnosticItem(5, "Свечи зажигания", "t_spark_plug_icon")
        details += TuningDetailDiagnosticItem(6, "Аккумулятор", "t_battery_engine_icon")
    }

    private fun showStatus(status: Int) {
        if (status == TuningConstants.STATUS_NOT_ACTUAL_DIAGNOSTIC) {
            binding.titleActualDiagnostics.setText(R.string.tuning_diagnostic_status_no_actual)
            binding.titleActualDiagnostics.background =
                ContextCompat.getDrawable(context, R.drawable.tuning_bg_not_actual_diagnostics)
        } else {
            binding.titleActualDiagnostics.setText(R.string.tuning_diagnostic_status_actual)
            binding.titleActualDiagnostics.background =
                ContextCompat.getDrawable(context, R.drawable.tuning_bg_actual_diagnostics)
        }
    }

    private fun changeViewButton(status: Int) {
        if (status == TuningConstants.STATUS_NOT_ACTUAL_DIAGNOSTIC) {
            binding.tuningDiagnostics.setCompoundDrawablesRelativeWithIntrinsicBounds(
                ContextCompat.getDrawable(context, R.drawable.tuning_icon_diagnostics), null, null, null
            )
            binding.tuningDiagnostics.setText(R.string.tuning_title_diagnostic_button)
        } else {
            binding.tuningDiagnostics.setCompoundDrawablesRelativeWithIntrinsicBounds(
                ContextCompat.getDrawable(context, R.drawable.tuning_repair), null, null, null
            )
            binding.tuningDiagnostics.setText(R.string.tuning_submenu_button_repeat)
        }
    }

    private companion object {
        const val RUBLES = " ₽"

        // Экран диагностики не привязан ни к одному селектору тюнинга
        const val NO_SELECTOR = -1

        const val DETAILS_IN_ELECTRO = 3
        const val DETAILS_IN_ENGINE = 7

        // Ниже этого числа карточек список не занимает всю ширину экрана
        const val SHORT_LIST_SIZE = 4

        const val FULL_STATE = 100

        const val CLICK_DELAY = 250L
    }
}
