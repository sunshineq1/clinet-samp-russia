package com.wn.blackrussia.game.ui.tuning

import android.os.Handler
import android.os.Looper
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.animation.Animation
import android.view.animation.AnimationUtils
import android.widget.SeekBar
import com.wn.blackrussia.R
import com.wn.blackrussia.databinding.TuningFragmentCollapseBinding
import com.wn.blackrussia.game.common.UILayout

class UILayoutTuningCollapse(private val screen: BrTuning) : UILayout() {

    private var root: View? = null
    private lateinit var binding: TuningFragmentCollapseBinding
    private lateinit var anim: Animation

    private val handler = Handler(Looper.getMainLooper())

    private var activeSelectorId = -1
    private var detailsCost = 0
    private var valueCornerToSend = 0
    private var valueCurrency = ""

    var nameChangeParameter: String = ""
        set(value) {
            field = value
            binding.seekbarTitleCollapse.text = value
        }

    val collapse: Int
        get() = valueCornerToSend

    override fun getView(): View? = root

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup): View {
        binding = TuningFragmentCollapseBinding.inflate(inflater)
        root = binding.root

        anim = AnimationUtils.loadAnimation(context, R.anim.button_click)

        binding.valueThisCorner.text = "${binding.tuningSeekbarCollapse.progress}$PERCENT"

        binding.tuningPriceCollapse.setOnClickListener { view ->
            view.startAnimation(anim)
            handler.postDelayed({
                screen.applyBuyNewItem(
                    TuningConstants.ACTION_BUY_COLLAPSE, detailsCost, valueCurrency,
                    activeSelectorId, TuningConstants.COLLAPSE_TYPE_MENU
                )
            }, CLICK_DELAY)
        }

        binding.backToSubmenuCollapse.setOnClickListener { view ->
            view.startAnimation(anim)
            handler.postDelayed({ screen.backToSubmenu() }, CLICK_DELAY)
        }

        binding.buttonViewCarCollapse.setOnClickListener { view ->
            view.startAnimation(anim)
            screen.saveLayoutForViewCar(BrTuning.LAYOUT_COLLAPSE_AFTER_VIEW_CAR)
            handler.postDelayed({ screen.listenerLayout(BrTuning.LAYOUT_VIEW_CAR) }, CLICK_DELAY)
        }

        binding.tuningSeekbarCollapse.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onStartTrackingTouch(seekBar: SeekBar) {}

            override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                valueCornerToSend = seekBar.progress
                binding.valueThisCorner.text = "$valueCornerToSend$PERCENT"
            }

            override fun onStopTrackingTouch(seekBar: SeekBar) {
                valueCornerToSend = seekBar.progress
                screen.sendDetailOrCollapse(TuningConstants.OPERATION_PREVIEW_COLLAPSE, valueCornerToSend)
            }
        })

        binding.buttonResetCollapse.setOnClickListener {
            screen.applyResetDetails(
                activeSelectorId, TuningConstants.COLLAPSE_TYPE_MENU,
                TuningConstants.DEFAULT_DETAILS_TYPE
            )
        }

        return binding.root
    }

    override fun onLayoutShown() {}

    override fun onLayoutClose() {}

    fun setActiveSelectorId(selectorId: Int) {
        activeSelectorId = selectorId
    }

    fun setStartParameters(allCollapse: List<Int>) {
        detailsCost = screen.selectorPrice(activeSelectorId)
        setPrice()
        setStartCollapse(allCollapse)
        binding.valueThisCorner.text = "${binding.tuningSeekbarCollapse.progress}$PERCENT"
    }

    fun setStartCollapse(allCollapse: List<Int>) {
        // Сервер шлёт значения всех регулировок одним списком, в фиксированном порядке
        if (allCollapse.isEmpty()) {
            applyProgress(DEFAULT_PROGRESS)
            return
        }

        when (activeSelectorId) {
            TuningConstants.SELECTOR_FULL_CLEARANCE -> applyProgress(allCollapse[0])
            TuningConstants.SELECTOR_INDIVIDUAL_CLEARANCE -> applyProgress(allCollapse[1])
            TuningConstants.SELECTOR_DEPARTURE_FRONT_WHEELS -> applyProgress(allCollapse[2])
            TuningConstants.SELECTOR_DEPARTURE_BACK_WHEELS -> applyProgress(allCollapse[3])
            TuningConstants.SELECTOR_COLLAPSE_FRONT -> applyProgress(allCollapse[4])
            TuningConstants.SELECTOR_COLLAPSE_BACK -> applyProgress(allCollapse[5])
            TuningConstants.SELECTOR_WIDTH_FRONT_WHEELS -> applyProgress(allCollapse[6])
            TuningConstants.SELECTOR_WIDTH_BACK_WHEELS -> applyProgress(allCollapse[7])
            TuningConstants.SELECTOR_RADIUS_WHEELS -> applyProgress(allCollapse[8])
        }
    }

    fun setEmptyValueCorner() {
        valueCornerToSend = 0
    }

    private fun applyProgress(value: Int) {
        binding.tuningSeekbarCollapse.progress = value
    }

    private fun setPrice() {
        binding.tuningValuePriceCollapse.text = BrTuning.priceWithSpaces(detailsCost)
        valueCurrency = RUBLE
        binding.tuningCurrencyCollapse.text = RUBLE
    }

    private companion object {
        const val CLICK_DELAY = 250L
        const val DEFAULT_PROGRESS = 50
        const val PERCENT = "%"
        const val RUBLE = "\u20BD"
    }
}
