package com.wn.blackrussia.game.ui.tuning

import android.graphics.Color
import android.os.Handler
import android.os.Looper
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.animation.Animation
import android.view.animation.AnimationUtils
import android.widget.SeekBar
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.wn.blackrussia.R
import com.wn.blackrussia.databinding.TuningFragmentColorListBinding
import com.wn.blackrussia.game.common.UILayout
import com.wn.blackrussia.game.ui.donate.CarColorCatalog
import com.wn.blackrussia.game.ui.donate.CarColorItem

class UILayoutTuningColorsList(private val screen: BrTuning) : UILayout() {

    private var root: View? = null
    private lateinit var binding: TuningFragmentColorListBinding
    private lateinit var anim: Animation

    private val handler = Handler(Looper.getMainLooper())

    private val colors = CarColorCatalog.colors
    private var colorAdapter: ColorAdapter? = null

    private var activeSelectorId = -1
    private var detailsCost = 0
    private var valueCurrency = ""
    private var valueTransparency = ""
    private var colorSend = ""

    private var selectedColor: CarColorItem? = null

    // Цвет, который сейчас стоит на машине: он помечается галочкой в списке
    private var startColorValue: Int? = null

    // Пока цвет ни разу не выбран, ползунок прозрачности ничего не шлёт
    private var counter = 0

    val buyThisColor: CarColorItem?
        get() = selectedColor

    val colorItem: String
        get() = colorSend

    val hexValueColorTransparency: String
        get() = valueTransparency

    override fun getView(): View? = root

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup): View {
        binding = TuningFragmentColorListBinding.inflate(inflater)
        root = binding.root

        anim = AnimationUtils.loadAnimation(context, R.anim.button_click)

        binding.listRgbColors.setOnClickListener {
            screen.listenerLayout(BrTuning.LAYOUT_RGB_COLOR)
            colorSend = ""
        }

        binding.listReadyColors.setOnClickListener { setClickForHeaderColor(false) }

        binding.buttonViewCarColor.setOnClickListener { view ->
            view.startAnimation(anim)
            screen.saveLayoutForViewCar(BrTuning.LAYOUT_COLORS_LIST)
            handler.postDelayed({ screen.listenerLayout(BrTuning.LAYOUT_VIEW_CAR) }, CLICK_DELAY)
        }

        binding.tuningPriceColor.setOnClickListener { view ->
            view.startAnimation(anim)
            handler.postDelayed({
                screen.applyBuyNewItem(
                    TuningConstants.ACTION_BUY, detailsCost, valueCurrency,
                    activeSelectorId, TuningConstants.READY_COLOR_TYPE_MENU
                )
            }, CLICK_DELAY)
        }

        binding.backToSubmenuColor.setOnClickListener { view ->
            view.startAnimation(anim)
            handler.postDelayed({ screen.backToSubmenu() }, CLICK_DELAY)
        }

        binding.tuningSeekbarColor.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                binding.valueTransparencyColor.text = "${seekBar.progress}$PERCENT"

                valueTransparency = ((seekBar.max - seekBar.progress) * ALPHA_RANGE / 100 + ALPHA_MIN)
                    .toString(16)
                    .uppercase()

                if (valueTransparency.length == 1) {
                    valueTransparency = "0$valueTransparency"
                }

                if (counter != 0) {
                    sendColorToClient()
                }
            }

            override fun onStartTrackingTouch(seekBar: SeekBar) {
                if (selectedColor != null) {
                    setVisiblePriceBlock(true)
                }
            }

            override fun onStopTrackingTouch(seekBar: SeekBar) {}
        })

        binding.buttonResetToningReady.setOnClickListener {
            screen.applyResetDetails(
                activeSelectorId, TuningConstants.READY_COLOR_TYPE_MENU,
                TuningConstants.DEFAULT_DETAILS_TYPE
            )
        }

        return binding.root
    }

    override fun onLayoutShown() {}

    override fun onLayoutClose() {}

    fun setStartParameters(selectorId: Int) {
        cleaningInterface()
        setNewDataInList()

        activeSelectorId = selectorId

        updateViewSeekBarAndResetButtonInterface()
    }

    private fun cleaningInterface() {
        colorSend = ""
        detailsCost = 0
        counter = 0

        setVisiblePriceBlock(false)
        setClickForHeaderColor(false)

        binding.tuningSeekbarColor.progress = 0
        binding.valueTransparencyColor.text = "${binding.tuningSeekbarColor.progress}$PERCENT"
    }

    private fun setNewDataInList() {
        if (colors.isEmpty()) {
            return
        }

        val adapter = ColorAdapter()
        colorAdapter = adapter
        adapter.setOnlyColorClickable(NO_SELECTION)

        binding.recvColorsList.layoutManager =
            LinearLayoutManager(binding.root.context, RecyclerView.HORIZONTAL, false)
        binding.recvColorsList.adapter = adapter
    }

    // Прозрачность настраивается только у тонировки, у остальных цветов её нет
    private fun updateViewSeekBarAndResetButtonInterface() {
        if (activeSelectorId == TuningConstants.SELECTOR_TONING_FRONT ||
            activeSelectorId == TuningConstants.SELECTOR_TONING_BACK
        ) {
            binding.seekbarViewColor.visibility = View.VISIBLE
            valueTransparency = OPAQUE
            binding.buttonResetToningReady.visibility = View.VISIBLE
        } else {
            binding.seekbarViewColor.visibility = View.INVISIBLE
            valueTransparency = ""
            binding.buttonResetToningReady.visibility = View.INVISIBLE
        }
    }

    // Цвета машины приходят одним списком в жёстко заданном порядке
    fun setStartColor() {
        val allColors = screen.allColors

        if (allColors.isEmpty()) {
            return
        }

        when (activeSelectorId) {
            TuningConstants.SELECTOR_COLOR_BODY -> updateStartColor(allColors[0])

            TuningConstants.SELECTOR_COLOR_DISCS_1,
            TuningConstants.SELECTOR_COLOR_DISCS_2 -> updateStartColor(allColors[1])

            TuningConstants.SELECTOR_TONING_FRONT -> {
                updateStartColor(allColors[2])
                setTransparency(allColors[2])
            }

            TuningConstants.SELECTOR_TONING_BACK -> {
                updateStartColor(allColors[3])
                setTransparency(allColors[3])
            }

            TuningConstants.SELECTOR_COLOR_LIGHT -> updateStartColor(allColors[4])
            TuningConstants.SELECTOR_COLOR_NEON_ALL -> updateStartColor(allColors[5])
            TuningConstants.SELECTOR_COLOR_NEON_LEFT -> updateStartColor(allColors[6])
            TuningConstants.SELECTOR_COLOR_NEON_RIGHT -> updateStartColor(allColors[7])
        }
    }

    private fun updateStartColor(rawColor: String) {
        var hex = rawColor

        if (hex.length == 8) {
            hex = hex.substring(2)
        } else if (hex.length == 7) {
            hex = hex.substring(1)
        }

        if (hex == "0") {
            hex = "000000"
        }

        val value = runCatching { Color.parseColor("#$hex") }.getOrNull() ?: return
        val previous = startColorValue

        startColorValue = value

        for ((index, item) in colors.withIndex()) {
            if (item.color == value) {
                colorAdapter?.notifyItemChanged(index)
                setVisiblePriceBlock(false)
            } else if (item.color == previous) {
                colorAdapter?.notifyItemChanged(index)
            }
        }
    }

    fun resetColorParameters() {
        val previous = startColorValue
        startColorValue = null

        for ((index, item) in colors.withIndex()) {
            if (item.color == previous) {
                colorAdapter?.notifyItemChanged(index)
            }
        }

        binding.tuningSeekbarColor.progress = 0
        binding.valueTransparencyColor.text = "${binding.tuningSeekbarColor.progress}$PERCENT"
    }

    private fun setTransparency(rawColor: String) {
        val alpha = when (rawColor.length) {
            8 -> rawColor.substring(0, 2)
            7 -> "0" + rawColor.substring(0, 1)
            else -> "00"
        }

        binding.tuningSeekbarColor.progress =
            binding.tuningSeekbarColor.max - (alpha.toInt(16) - ALPHA_MIN) * 100 / ALPHA_RANGE
    }

    fun setEmptyObject() {
        selectedColor = null
    }

    fun setEmptyTransparency() {
        valueTransparency = ""
    }

    private fun onColorClick(item: CarColorItem, position: Int) {
        colorAdapter?.setOnlyColorClickable(position)

        detailsCost = screen.selectorPrice(activeSelectorId)

        setVisiblePriceBlock(true)
        setPrice()

        selectedColor = item
        sendColorToClient()

        counter++
    }

    private fun sendColorToClient() {
        val item = selectedColor ?: return
        val hex = "%06X".format(item.color and 0xFFFFFF)

        colorSend = if (valueTransparency.isEmpty()) OPAQUE + hex else valueTransparency + hex

        screen.sendColorPreview(TuningConstants.OPERATION_PREVIEW_COLOR, colorSend)
    }

    private fun setVisiblePriceBlock(isVisible: Boolean) {
        if (isVisible) {
            binding.titlePriceBlockColor.visibility = View.VISIBLE
            binding.tuningPriceColor.visibility = View.VISIBLE
        } else {
            binding.titlePriceBlockColor.visibility = View.INVISIBLE
            binding.tuningPriceColor.visibility = View.INVISIBLE
        }
    }

    private fun setPrice() {
        binding.tuningValuePriceColor.text = BrTuning.priceWithSpaces(detailsCost)
        valueCurrency = RUBLE
        binding.tuningCurrencyColor.text = RUBLE
    }

    private fun setClickForHeaderColor(isClickedReadyColor: Boolean) {
        val green = ContextCompat.getColor(context, R.color.green_check_element)
        val white = ContextCompat.getColor(context, R.color.white)

        if (isClickedReadyColor) {
            binding.listRgbColors.setTextColor(green)
            binding.underlineRgb.visibility = View.VISIBLE
            binding.listReadyColors.setTextColor(white)
            binding.underlineReadyColors.visibility = View.INVISIBLE
            return
        }

        binding.listRgbColors.setTextColor(white)
        binding.listReadyColors.setTextColor(green)
        binding.underlineRgb.visibility = View.INVISIBLE
        binding.underlineReadyColors.visibility = View.VISIBLE
    }

    private inner class ColorAdapter : RecyclerView.Adapter<ColorAdapter.Holder>() {

        inner class Holder(view: View) : RecyclerView.ViewHolder(view) {
            val swatch: View = view.findViewById(R.id.color)
            val border: View = view.findViewById(R.id.border)
            val checkedIcon: View = view.findViewById(R.id.checked_start_color)

            init {
                view.setOnClickListener {
                    val position = layoutPosition
                    val item = colors[position]

                    item.selected = true
                    notifyItemChanged(position)

                    onColorClick(item, position)
                }
            }
        }

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Holder =
            Holder(
                LayoutInflater.from(parent.context)
                    .inflate(R.layout.tuning_color_item, parent, false)
            )

        override fun getItemCount() = colors.size

        override fun onBindViewHolder(holder: Holder, position: Int) {
            val item = colors[position]

            holder.swatch.setBackgroundColor(item.color)
            holder.checkedIcon.visibility =
                if (item.color == startColorValue) View.VISIBLE else View.INVISIBLE

            holder.border.setBackgroundResource(
                if (item.selected) R.drawable.border_button else R.drawable.border_button_gray
            )
        }

        fun setOnlyColorClickable(position: Int) {
            for ((index, item) in colors.withIndex()) {
                if (item.selected && index != position) {
                    item.selected = false
                    notifyItemChanged(index)
                }
            }
        }
    }

    private companion object {
        const val CLICK_DELAY = 250L
        const val NO_SELECTION = -1
        const val PERCENT = "%"
        const val RUBLE = "₽"
        const val OPAQUE = "FF"

        // Ползунок 0..100% ложится на диапазон альфы 96..255
        const val ALPHA_MIN = 96
        const val ALPHA_RANGE = 159
    }
}
