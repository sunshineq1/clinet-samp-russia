package com.wn.blackrussia.game.ui.tuning

import android.os.Handler
import android.os.Looper
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.animation.Animation
import android.view.animation.AnimationUtils
import android.widget.SeekBar
import com.skydoves.colorpickerview.listeners.ColorEnvelopeListener
import com.wn.blackrussia.R
import com.wn.blackrussia.databinding.TuningFragmentRgbColorBinding
import com.wn.blackrussia.game.common.UILayout
import com.wn.blackrussia.game.ui.donate.CarColorCatalog

class UILayoutTuningRGBColor(private val screen: BrTuning) : UILayout() {

    private var root: View? = null

    private lateinit var binding: TuningFragmentRgbColorBinding
    private lateinit var anim: Animation

    private val handler = Handler(Looper.getMainLooper())

    private var activeSelectorId = -1
    private var detailsCost = 0
    private var valueCurrency = ""
    private var transparency = ""

    // Пока экран не открыт, крутилка цвета молчит: иначе превью улетало бы в
    // игру и с соседнего экрана готовых цветов
    private var isActiveLayout = false

    var color = ""
        private set

    var isRGBColor = false
        private set

    override fun getView(): View? = root

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup): View {
        binding = TuningFragmentRgbColorBinding.inflate(inflater)
        root = binding.root

        anim = AnimationUtils.loadAnimation(context, R.anim.button_click)

        binding.colorPickerRgb.setColorListener(ColorEnvelopeListener { envelope, _ ->
            binding.alphaTileViewRgbColor.setPaintColor(envelope.color)
            color = envelope.hexCode

            if (isActiveLayout) {
                screen.sendColorPreview(TuningConstants.OPERATION_PREVIEW_COLOR, color)
            }
        })

        binding.colorPickerRgb.attachBrightnessSlider(binding.tuningSeekbarRgbColor)

        binding.blockWithThisRgbColor.setOnClickListener { view ->
            view.startAnimation(anim)
            showColorPicker()
        }

        binding.buttonSelectedColor.setOnClickListener { view ->
            view.startAnimation(anim)

            detailsCost = screen.selectorPrice(activeSelectorId)
            setPrice()

            handler.postDelayed({ showPriceButtons() }, CLICK_DELAY)

            screen.sendColorPreview(TuningConstants.OPERATION_PREVIEW_COLOR, color)
            binding.tuningSeekbarRgbTransparency.progress = 0
        }

        binding.buttonViewCarRgbColor.setOnClickListener { view ->
            view.startAnimation(anim)

            screen.saveLayoutForViewCar(BrTuning.LAYOUT_RGB_COLOR)
            handler.postDelayed({ screen.listenerLayout(BrTuning.LAYOUT_VIEW_CAR) }, CLICK_DELAY)
        }

        binding.tuningPriceRgbColor.setOnClickListener { view ->
            view.startAnimation(anim)

            handler.postDelayed({
                screen.applyBuyNewItem(
                    TuningConstants.ACTION_BUY, detailsCost, valueCurrency,
                    activeSelectorId, TuningConstants.RGB_TYPE_MENU
                )
                isRGBColor = true
            }, CLICK_DELAY)
        }

        binding.backToSubmenuRgbColor.setOnClickListener { view ->
            view.startAnimation(anim)
            handler.postDelayed({ screen.backToSubmenu() }, CLICK_DELAY)
        }

        binding.listReadyColorsRgbColor.setOnClickListener {
            screen.listenerLayout(BrTuning.LAYOUT_COLORS_LIST_AFTER_BACK)
            isRGBColor = false
        }

        binding.tuningSeekbarRgbTransparency.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                binding.valueTransparencyColor.text = "${seekBar.progress}%"

                transparency = Integer.toHexString(
                    (seekBar.max - seekBar.progress) * 159 / 100 + 96
                ).uppercase()

                if (transparency.length == 1) {
                    transparency = "0$transparency"
                }

                sendColorToClient()
            }

            override fun onStartTrackingTouch(seekBar: SeekBar) {}

            override fun onStopTrackingTouch(seekBar: SeekBar) {}
        })

        binding.buttonResetToningRgb.setOnClickListener {
            screen.applyResetDetails(
                activeSelectorId, TuningConstants.RGB_TYPE_MENU, TuningConstants.DEFAULT_DETAILS_TYPE
            )
        }

        return binding.root
    }

    override fun onLayoutShown() {}

    override fun onLayoutClose() {}

    fun setStartParameters(selectorId: Int) {
        isActiveLayout = false
        isRGBColor = false

        binding.colorPickerRgb.selectCenter()

        color = ""
        transparency = ""
        detailsCost = 0

        binding.tuningSeekbarRgbTransparency.progress = 0

        showColorPicker()

        activeSelectorId = selectorId
        binding.buttonResetToningRgb.visibility = if (isToning(selectorId)) View.VISIBLE else View.INVISIBLE
    }

    fun isThisLayout() {
        isActiveLayout = true
    }

    fun isNotThisLayout() {
        isActiveLayout = false
    }

    fun setStartColor() {
        val index = startColorIndex(activeSelectorId)

        if (index == -1) {
            return
        }

        val hex = screen.allColors.getOrNull(index) ?: return
        val value = hex.removePrefix("#").toLongOrNull(16) ?: return

        binding.colorPickerRgb.setInitialColor(value.toInt() or ALPHA_OPAQUE)
    }

    fun setEmptyRGBColor() {
        color = ""
        isRGBColor = false
    }

    // Порядок цветов машины жёстко задан: кузов, диски, тонировка перед и зад,
    // фары, неон целиком и по бортам
    private fun startColorIndex(selectorId: Int) = when (selectorId) {
        TuningConstants.SELECTOR_COLOR_BODY -> 0
        TuningConstants.SELECTOR_COLOR_DISCS_1, TuningConstants.SELECTOR_COLOR_DISCS_2 -> 1
        TuningConstants.SELECTOR_TONING_FRONT -> 2
        TuningConstants.SELECTOR_TONING_BACK -> 3
        TuningConstants.SELECTOR_COLOR_LIGHT -> 4
        TuningConstants.SELECTOR_COLOR_NEON_ALL -> 5
        TuningConstants.SELECTOR_COLOR_NEON_LEFT -> 6
        TuningConstants.SELECTOR_COLOR_NEON_RIGHT -> 7
        else -> -1
    }

    private fun showColorPicker() {
        binding.menuBlockRadialColor.visibility = View.VISIBLE
        binding.seekbarViewRgbColor.visibility = View.VISIBLE
        binding.menuBlockRgbColor.visibility = View.INVISIBLE
        binding.seekbarBlockRgbTransparency.visibility = View.INVISIBLE
    }

    private fun showPriceButtons() {
        binding.menuBlockRadialColor.visibility = View.INVISIBLE
        binding.seekbarViewRgbColor.visibility = View.INVISIBLE
        binding.menuBlockRgbColor.visibility = View.VISIBLE

        // Прозрачность есть только у стёкол
        if (isToning(activeSelectorId)) {
            binding.seekbarBlockRgbTransparency.visibility = View.VISIBLE
        }
    }

    private fun isToning(selectorId: Int) =
        selectorId == TuningConstants.SELECTOR_TONING_FRONT ||
            selectorId == TuningConstants.SELECTOR_TONING_BACK

    private fun setPrice() {
        binding.tuningValuePriceRgbColor.text = BrTuning.priceWithSpaces(detailsCost)
        valueCurrency = RUBLE
        binding.tuningCurrencyRgbColor.text = RUBLE
    }

    // Прозрачность занимает два старших разряда, поэтому цвет каждый раз
    // пересобирается из хвоста
    private fun sendColorToClient() {
        if (color.isNotEmpty()) {
            color = color.substring(2)
        }

        color = transparency + color
        screen.sendColorPreview(TuningConstants.OPERATION_PREVIEW_COLOR, color)
    }

    private companion object {
        const val CLICK_DELAY = 250L
        const val RUBLE = "₽"
        const val ALPHA_OPAQUE = 0xFF shl 24
    }
}
