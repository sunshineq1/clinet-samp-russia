package com.wn.blackrussia.game.ui.tuning

import android.os.Handler
import android.os.Looper
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.animation.Animation
import android.view.animation.AnimationUtils
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.wn.blackrussia.R
import com.wn.blackrussia.databinding.TuningFragmentTypeListBinding
import com.wn.blackrussia.game.common.UILayout

class UILayoutTuningMain(private val screen: BrTuning) : UILayout(),
    TuningTypeAdapter.OnClickTuningType {

    private var binding: TuningFragmentTypeListBinding? = null

    private lateinit var anim: Animation

    private var activeScreen: TuneGuiScreen? = null
    private var typeAdapter: TuningTypeAdapter? = null

    private val tuningTypes = ArrayList<TuningMenuType>()

    val selectedNodesList = ArrayList<TuneGuiNode>()

    var dataInActiveScreen: TuneGuiNode? = null
        private set

    private val handler = Handler(Looper.getMainLooper())

    override fun getView(): View? = binding?.root

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup): View {
        val view = TuningFragmentTypeListBinding.inflate(inflater)
        binding = view

        anim = AnimationUtils.loadAnimation(context, R.anim.button_click)

        rebuildTypes()

        typeAdapter = TuningTypeAdapter(context, tuningTypes, this)

        view.recvTypeTuningList.layoutManager =
            LinearLayoutManager(view.root.context, RecyclerView.HORIZONTAL, false)
        view.recvTypeTuningList.adapter = typeAdapter

        view.buttonViewCarType.setOnClickListener { button ->
            button.startAnimation(anim)

            handler.postDelayed({
                screen.setEmptyThisSubmenu()
                screen.listenerLayout(BrTuning.LAYOUT_VIEW_CAR)
            }, CLICK_DELAY)
        }

        return view.root
    }

    override fun onLayoutShown() {}

    override fun onLayoutClose() {}

    fun setVisibleMainView(visible: Boolean) {
        binding?.typeViewMain?.visibility = if (visible) View.VISIBLE else View.INVISIBLE
    }

    fun rebuildTypes() {
        activeScreen = screen.activeScreen

        tuningTypes.clear()
        activeScreen?.footer?.forEach { tuningTypes += TuningMenuType(it.name) }

        typeAdapter?.notifyDataSetChanged()
    }

    override fun click(tuningType: TuningMenuType, position: Int, view: View) {
        typeAdapter?.setOnlyItemClickable(position)

        val node = activeScreen?.footer?.getOrNull(position) ?: return
        dataInActiveScreen = node

        if (node.opensType != TuningConstants.OTHER_TYPE_MENU) {
            screen.setClickMainMenu(node.opensType)
            return
        }

        val allowedSelectors = screen.correctSelectorsOld(screen.correctSelectors())

        selectedNodesList.clear()
        for (child in node.names) {
            when {
                child.opensType == TuningConstants.READY_COLOR_TYPE_MENU &&
                    child.selectorId != TuningConstants.SELECTOR_VINYL -> {
                    for (selector in allowedSelectors) {
                        if (child.selectorId == selector) selectedNodesList += child
                    }
                }

                child.selectorId == TuningConstants.SELECTOR_VINYL -> {
                    if (screen.carId !in CARS_WITHOUT_VINYLS) selectedNodesList += child
                }

                else -> selectedNodesList += child
            }
        }

        renameEngineSelectors()

        screen.setDataInNewSubmenu(selectedNodesList)
        screen.listenerLayout(BrTuning.LAYOUT_SUBMENU)
    }

    // У электромобилей те же пять селекторов силовой установки зовутся иначе
    private fun renameEngineSelectors() {
        val electric = screen.carId in ELECTRIC_CARS

        for (node in selectedNodesList) {
            val renamed = when (node.selectorId) {
                TuningConstants.SELECTOR_DIFFERENTIAL ->
                    if (electric) "Батарея" to "t_battery_icon"
                    else "Дифференциалы" to "t_differential_icon"

                TuningConstants.SELECTOR_BRAKERS -> "Тормоза" to "t_brakers_icon"

                TuningConstants.SELECTOR_AIR_FILTER ->
                    if (electric) "Трансмиссия" to "t_transmission_icon"
                    else "Турбонаддув" to "t_air_filter_icon"

                TuningConstants.SELECTOR_ENGINE ->
                    if (electric) "Электродвигатель" to "t_electro_engine_icon"
                    else "Двигатели" to "t_engine_icon"

                TuningConstants.SELECTOR_SUPERCHARGER ->
                    if (electric) "Контроллер" to "t_chip_icon"
                    else "Нагнетатели" to "t_supercharger_icon"

                else -> null
            } ?: continue

            node.name = renamed.first
            node.imageId = renamed.second
        }
    }

    private companion object {
        const val CLICK_DELAY = 250L

        // Модели, у которых нет винилов: селектор винилов им не показываем
        val CARS_WITHOUT_VINYLS = intArrayOf(
            402, 410, 411, 413, 414, 415, 416, 427, 429, 433, 436, 438, 440, 445,
            456, 459, 462, 470, 480, 490, 494, 495, 502, 503, 505, 526, 540, 541,
            559, 565, 579, 587, 589, 603, 604
        )

        val ELECTRIC_CARS = intArrayOf(
            TuningConstants.ID_TESLA_MODEL_X,
            TuningConstants.ID_TESLA_MODEL_S,
            TuningConstants.ID_PORSCHE_TAYCAN,
            TuningConstants.ID_MERCEDES_EQS
        )
    }
}
