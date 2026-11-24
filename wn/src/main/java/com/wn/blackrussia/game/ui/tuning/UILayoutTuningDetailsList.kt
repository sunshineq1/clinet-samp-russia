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
import androidx.recyclerview.widget.RecyclerView
import com.wn.blackrussia.R
import com.wn.blackrussia.databinding.TuningFragmentDetailsListBinding
import com.wn.blackrussia.game.common.UILayout

class UILayoutTuningDetailsList(private val root: BrTuning) : UILayout(),
    TuningDetailsAdapter.OnClickDetailItem, TuningVinylsAdapter.OnClickVinylItem {

    private lateinit var binding: TuningFragmentDetailsListBinding
    private lateinit var anim: Animation

    private var rootView: View? = null

    private val handler = Handler(Looper.getMainLooper())

    private var activeSelectorId = -1

    private var detailsAdapter: TuningDetailsAdapter? = null
    private var detailsItems: List<Tune> = ArrayList()

    private var vinylsAdapter: TuningVinylsAdapter? = null
    private var vinyls: List<TuneVinyls> = ArrayList()

    private var detailsCost = 0
    private var costVinyl = 0
    private var valueCurrency = ""

    private var thisActions = -1
    private var isBox = false
    private var isGetType = false

    var buyThisDetail: Tune? = null
        private set

    var selectedVinyl: TuneVinyls? = null
        private set

    var nameThisWindow: String
        get() = binding.titleDetailsList.text.toString()
        set(value) {
            binding.titleDetailsList.text = value
        }

    override fun getView(): View? = rootView

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup): View {
        binding = TuningFragmentDetailsListBinding.inflate(inflater)
        rootView = binding.root

        anim = AnimationUtils.loadAnimation(context, R.anim.button_click)
        setVisiblePriceBlock(detailsCost)

        vinyls = TuneCatalog.vinyls
        for (vinyl in vinyls) {
            vinyl.firstRender = true
        }

        binding.tuningPriceDetail.setOnClickListener { view ->
            view.startAnimation(anim)
            handler.postDelayed({ applyThisAction() }, CLICK_DELAY)
        }

        binding.backToSubmenuDetail.setOnClickListener { view ->
            view.startAnimation(anim)
            handler.postDelayed({ root.backToSubmenu() }, CLICK_DELAY)
        }

        binding.buttonViewCarDetailsList.setOnClickListener { view ->
            view.startAnimation(anim)
            root.saveLayoutForViewCar(BrTuning.LAYOUT_DETAILS_LIST)
            handler.postDelayed({ root.listenerLayout(BrTuning.LAYOUT_VIEW_CAR) }, CLICK_DELAY)
        }

        binding.buttonResetDetail.setOnClickListener {
            if (activeSelectorId == TuningConstants.SELECTOR_VINYL) {
                root.applyResetDetails(activeSelectorId, TuningConstants.VINYLS_LIST_TYPE_MENU,
                    TuningConstants.DEFAULT_DETAILS_TYPE)
            } else {
                root.applyResetDetails(activeSelectorId, TuningConstants.DETAILS_LIST_TYPE_MENU,
                    establishedType())
            }
        }

        binding.buttonInfoBrake.setOnClickListener { view ->
            view.startAnimation(anim)
            handler.postDelayed({ root.listenerLayout(BrTuning.LAYOUT_VIEW_OPTIONS) }, CLICK_DELAY)
        }

        return binding.root
    }

    override fun onLayoutShown() {
    }

    override fun onLayoutClose() {
    }

    fun setActiveSelectorId(selectorId: Int) {
        activeSelectorId = selectorId
    }

    fun setVisibleBody(isVisible: Boolean) {
        binding.root.visibility = if (isVisible) View.VISIBLE else View.INVISIBLE
    }

    // ---- список деталей --------------------------------------------------

    fun setNewParametersInDetailList(items: List<Tune>) {
        isBox = false
        detailsItems = items

        cleaningInterface()
        setNewDataInList(items)
        visibleForDetailsInfo(false)

        binding.buttonInfoBrake.visibility =
            if (activeSelectorId in ENGINE_SELECTORS) View.VISIBLE else View.INVISIBLE

        isGetType = activeSelectorId in BODY_PART_SELECTORS
        visibleForResetDetail(!isGetType)
    }

    private fun setNewDataInList(items: List<Tune>) {
        setIconForTeslaOrNot(items)

        detailsAdapter = TuningDetailsAdapter(root, context, items, this)
        setNewParamsForBlock(items)
        detailsAdapter?.setOnlyDetailClickable(NO_POSITION)
        setDetailsInView()
    }

    private fun setIconForTeslaOrNot(items: List<Tune>) {
        val electric = root.carId == TuningConstants.ID_TESLA_MODEL_X ||
            root.carId == TuningConstants.ID_TESLA_MODEL_S ||
            root.carId == TuningConstants.ID_PORSCHE_TAYCAN ||
            root.carId == TuningConstants.ID_MERCEDES_EQS

        for (item in items) {
            if (item.type == TuningConstants.DEFAULT_DETAILS_TYPE) {
                continue
            }

            item.imageId = when (item.selector) {
                TuningConstants.SELECTOR_DIFFERENTIAL ->
                    if (electric) "t_battery_icon" else "t_differential_icon"

                TuningConstants.SELECTOR_BRAKERS -> "t_brakers_icon"

                TuningConstants.SELECTOR_AIR_FILTER ->
                    if (electric) "t_transmission_icon" else "t_air_filter_icon"

                TuningConstants.SELECTOR_ENGINE ->
                    if (electric) "t_electro_engine_icon" else "t_engine_icon"

                TuningConstants.SELECTOR_SUPERCHARGER ->
                    if (electric) "t_chip_icon" else "t_supercharger_icon"

                else -> item.imageId
            }
        }
    }

    // Короткий список помещается под полкой, длинный поднимает блок кнопок над ней
    private fun setNewParamsForBlock(items: List<Tune>) {
        val anchorId = if (items.size < COMPACT_LIST_LIMIT) {
            R.id.help_view_in_detail_list
        } else {
            R.id.help_view_above_list_in_detail_list
        }

        val params = RelativeLayout.LayoutParams(
            RelativeLayout.LayoutParams.WRAP_CONTENT,
            RelativeLayout.LayoutParams.WRAP_CONTENT)

        params.addRule(RelativeLayout.ALIGN_END, anchorId)
        params.addRule(RelativeLayout.ALIGN_BOTTOM, anchorId)

        binding.menuBlockDetail.layoutParams = params
    }

    private fun setDetailsInView() {
        binding.recvDetailsList.layoutManager = horizontalLayout()
        binding.recvDetailsList.adapter = detailsAdapter
    }

    // ---- список винилов --------------------------------------------------

    fun setNewParametersWithVinyls() {
        isBox = false
        costVinyl = root.selectorPrice(activeSelectorId)

        cleaningInterface()
        setListVinyls()
        visibleForDetailsInfo(false)
    }

    private fun setListVinyls() {
        vinylsAdapter = TuningVinylsAdapter(root, context, vinyls, this, costVinyl)
        setVinylsInView()
        vinylsAdapter?.setOnlyVinylClickable(NO_POSITION)
    }

    private fun setVinylsInView() {
        binding.recvDetailsList.layoutManager = horizontalLayout()
        binding.recvDetailsList.adapter = vinylsAdapter
        binding.recvDetailsList.isDrawingCacheEnabled = true
        binding.recvDetailsList.drawingCacheQuality = View.DRAWING_CACHE_QUALITY_HIGH
        binding.recvDetailsList.setItemViewCacheSize(vinyls.size)
    }

    private fun horizontalLayout(): RecyclerView.LayoutManager =
        LinearLayoutManager(binding.root.context, RecyclerView.HORIZONTAL, false)

    // ---- состояние элементов ---------------------------------------------

    fun updateStatusDetailAfterPurchase(idDetail: Int) {
        for (i in detailsItems.indices) {
            if (detailsItems[i].id != idDetail) {
                continue
            }

            detailsItems[i].thisLocation = TuningConstants.DETAIL_STATUS_BOUGHT
            detailsAdapter?.notifyItemChanged(i)
            changeViewButtonForDetails(detailsItems[i])
        }
    }

    fun updateStatusDetailAfterEstablished(idNewDetail: Int, idOldDetail: Int) {
        var installed = false

        for (i in detailsItems.indices) {
            if (detailsItems[i].id != idNewDetail) {
                continue
            }

            detailsItems[i].thisLocation = TuningConstants.DETAIL_STATUS_ESTABLISHED
            detailsAdapter?.notifyItemChanged(i)
            changeViewButtonForDetails(detailsItems[i])
            installed = true
        }

        if (!installed) {
            return
        }

        for (i in detailsItems.indices) {
            if (detailsItems[i].id == idOldDetail) {
                detailsItems[i].thisLocation = TuningConstants.DETAIL_STATUS_BOUGHT
                detailsAdapter?.notifyItemChanged(i)
            }
        }
    }

    fun updateStatusDetailAfterReset(idOldDetail: Int) {
        for (i in detailsItems.indices) {
            if (detailsItems[i].id == idOldDetail) {
                detailsItems[i].thisLocation = TuningConstants.DETAIL_STATUS_BOUGHT
                detailsAdapter?.notifyItemChanged(i)
            }
        }
    }

    fun updateStatusVinylsAfterEstablished(idVinyls: Int) {
        var establishedPosition = NO_POSITION

        for (i in vinyls.indices) {
            if (vinyls[i].id != idVinyls) {
                continue
            }

            vinyls[i].startVinyl = true
            vinylsAdapter?.notifyItemChanged(i)
            changeViewButtonForVinyls(vinyls[i])
            establishedPosition = i
        }

        if (establishedPosition == NO_POSITION) {
            return
        }

        for (i in vinyls.indices) {
            if (vinyls[i].startVinyl && i != establishedPosition) {
                vinyls[i].startVinyl = false
                vinylsAdapter?.notifyItemChanged(i)
            }
        }
    }

    fun setStartVinyl(nameVinyl: String) {
        for (i in vinyls.indices) {
            if (vinyls[i].nameVinyl == nameVinyl) {
                vinyls[i].startVinyl = true
                vinylsAdapter?.notifyItemChanged(i)
            }
        }
    }

    fun resetVinyls() {
        for (i in vinyls.indices) {
            if (vinyls[i].startVinyl) {
                vinyls[i].startVinyl = false
                vinylsAdapter?.notifyItemChanged(i)
                changeViewButtonForVinyls(vinyls[i])
            }
        }
    }

    fun setEmptyObjectForDetail() {
        buyThisDetail = null
    }

    fun setEmptyObjectForVinyl() {
        selectedVinyl = null
    }

    // ---- выбор в списке --------------------------------------------------

    override fun click(detailsItem: Tune, getPosition: Int, view: View) {
        detailsAdapter?.setOnlyDetailClickable(getPosition)

        if (detailsItem.thisLocation == TuningConstants.DETAIL_STATUS_CAN_BUY) {
            detailsCost = detailsItem.cost
            setCurrency(detailsItem)
            setPrice()
        } else {
            detailsCost = 0
        }

        changeViewButtonIsBox(detailsItem)
        changeViewButtonForDetails(detailsItem)
        buyThisDetail = detailsItem

        if (activeSelectorId in PREVIEW_SELECTORS) {
            root.sendDetailOrCollapse(TuningConstants.OPERATION_PREVIEW_DETAIL, detailsItem.tuneId)
        }

        if (activeSelectorId == TuningConstants.SELECTOR_HORN ||
            activeSelectorId == TuningConstants.SELECTOR_EXHAUST_SOUND) {
            root.sendSound(TuningConstants.OPERATION_LISTEN_SOUND, detailsItem.tuneId)
        }

        if (isGetType) {
            visibleForResetDetail(
                detailsItem.thisLocation == TuningConstants.DETAIL_STATUS_ESTABLISHED)
        }
    }

    override fun click(tuneVinyl: TuneVinyls, getPosition: Int, view: View) {
        vinylsAdapter?.setOnlyVinylClickable(getPosition)

        if (!tuneVinyl.startVinyl) {
            detailsCost = costVinyl
            valueCurrency = CURRENCY_RUB
            setPrice()
        } else {
            detailsCost = 0
        }

        changeViewButtonForVinyls(tuneVinyl)
        selectedVinyl = tuneVinyl

        root.sendVinyl(TuningConstants.OPERATION_PREVIEW_VINYL, tuneVinyl.nameVinyl)
    }

    private fun applyThisAction() {
        when (thisActions) {
            TuningConstants.ACTION_BUY -> {
                val typeMenu = if (activeSelectorId == TuningConstants.SELECTOR_VINYL) {
                    TuningConstants.VINYLS_LIST_TYPE_MENU
                } else {
                    TuningConstants.DETAILS_LIST_TYPE_MENU
                }

                root.applyBuyNewItem(TuningConstants.ACTION_BUY, detailsCost, valueCurrency,
                    activeSelectorId, typeMenu)
            }

            TuningConstants.ACTION_OPEN_BOX -> {
                root.listenerLayout(BrTuning.LAYOUT_OPENING_BOX)
                setVisibleBody(false)
            }

            TuningConstants.ACTION_SET_DETAIL -> buyThisDetail?.let { detail ->
                root.sendIntegerData(TuningConstants.OPERATION_INSTALL_DETAIL, detail.id)
            }
        }
    }

    private fun establishedType(): Int {
        val detail = buyThisDetail

        if (isGetType && detail != null &&
            detail.thisLocation == TuningConstants.DETAIL_STATUS_ESTABLISHED) {
            return detail.type
        }

        return TuningConstants.DEFAULT_DETAILS_TYPE
    }

    // ---- вид блока кнопок ------------------------------------------------

    private fun cleaningInterface() {
        detailsCost = 0
        setVisiblePriceBlock(detailsCost)
    }

    private fun visibleForDetailsInfo(isVisible: Boolean) {
        if (isVisible) {
            binding.buttonViewCarDetailsList.visibility = View.INVISIBLE
            binding.buttonInfoBrake.visibility = View.VISIBLE
        } else {
            binding.buttonViewCarDetailsList.visibility = View.VISIBLE
            binding.buttonInfoBrake.visibility = View.INVISIBLE
        }
    }

    private fun visibleForResetDetail(isVisible: Boolean) {
        binding.buttonResetDetail.visibility = if (isVisible) View.VISIBLE else View.INVISIBLE
    }

    private fun setVisiblePriceBlock(cost: Int) {
        val visibility = if (cost != 0) View.VISIBLE else View.INVISIBLE

        binding.titlePriceBlockDetails.visibility = visibility
        binding.tuningPriceDetail.visibility = visibility
    }

    private fun changeViewButtonIsBox(detailsItem: Tune) {
        if (detailsItem.name != NAME_BOX) {
            isBox = false
            return
        }

        isBox = true
        thisActions = TuningConstants.ACTION_OPEN_BOX

        binding.tuningPriceDetail.setCompoundDrawablesRelativeWithIntrinsicBounds(
            ContextCompat.getDrawable(context, R.drawable.tuning_icon_box_white), null, null, null)
        binding.tuningPriceDetail.setText(R.string.tuning_title_button_info)
        binding.tuningPriceDetail.visibility = View.VISIBLE
        binding.titlePriceBlockDetails.visibility = View.INVISIBLE
    }

    private fun changeViewButtonForDetails(detailsItem: Tune) {
        if (isBox) {
            return
        }

        when (detailsItem.thisLocation) {
            TuningConstants.DETAIL_STATUS_BOUGHT -> {
                thisActions = TuningConstants.ACTION_SET_DETAIL

                binding.tuningPriceDetail.setCompoundDrawablesRelativeWithIntrinsicBounds(
                    ContextCompat.getDrawable(context, R.drawable.tuning_repair), null, null, null)
                binding.tuningPriceDetail.setText(R.string.tuning_title_button_set_detail)
                binding.tuningPriceDetail.visibility = View.VISIBLE
                binding.titlePriceBlockDetails.visibility = View.INVISIBLE
            }

            TuningConstants.DETAIL_STATUS_ESTABLISHED -> {
                binding.titlePriceBlockDetails.visibility = View.INVISIBLE
                binding.tuningPriceDetail.visibility = View.INVISIBLE
            }

            else -> showPriceOnButton()
        }
    }

    private fun changeViewButtonForVinyls(tuneVinyl: TuneVinyls) {
        if (tuneVinyl.startVinyl) {
            binding.titlePriceBlockDetails.visibility = View.INVISIBLE
            binding.tuningPriceDetail.visibility = View.INVISIBLE
            return
        }

        showPriceOnButton()
    }

    private fun showPriceOnButton() {
        thisActions = TuningConstants.ACTION_BUY

        binding.tuningPriceDetail.setCompoundDrawablesRelativeWithIntrinsicBounds(
            null, null, null, null)
        setPrice()
        setVisiblePriceBlock(detailsCost)
    }

    private fun setCurrency(detailsItem: Tune) {
        valueCurrency = if (detailsItem.currency == TuningConstants.CURRENCY_RUB_MONEY) {
            CURRENCY_RUB
        } else {
            CURRENCY_BC
        }
    }

    private fun setPrice() {
        binding.tuningPriceDetail.text = BrTuning.priceWithSpaces(detailsCost) + valueCurrency
    }

    private companion object {
        const val CLICK_DELAY = 250L
        const val COMPACT_LIST_LIMIT = 4
        const val NO_POSITION = -1

        const val NAME_BOX = "Коробка"
        const val CURRENCY_RUB = " ₽"
        const val CURRENCY_BC = " BC"

        // Моторный отсек — только у этих селекторов открывается окно характеристик
        val ENGINE_SELECTORS = setOf(
            TuningConstants.SELECTOR_DIFFERENTIAL,
            TuningConstants.SELECTOR_BRAKERS,
            TuningConstants.SELECTOR_AIR_FILTER,
            TuningConstants.SELECTOR_ENGINE,
            TuningConstants.SELECTOR_SUPERCHARGER)

        // Кузовные детали идут комплектами, поэтому сброс у них по типу детали
        val BODY_PART_SELECTORS = setOf(
            TuningConstants.SELECTOR_SPOILER,
            TuningConstants.SELECTOR_BUMPER,
            TuningConstants.SELECTOR_THRESHOLDS,
            TuningConstants.SELECTOR_MUFFLER,
            TuningConstants.SELECTOR_SPLITTER,
            TuningConstants.SELECTOR_ROOF,
            TuningConstants.SELECTOR_HOOD,
            TuningConstants.SELECTOR_TRUNK,
            TuningConstants.SELECTOR_FENDER_EXTENSION,
            TuningConstants.SELECTOR_BUMPER_GRILL,
            TuningConstants.SELECTOR_SECOND_SPOILER)

        val PREVIEW_SELECTORS = setOf(
            TuningConstants.SELECTOR_VINYL,
            TuningConstants.SELECTOR_STROBOSCOPE,
            TuningConstants.SELECTOR_SPOILER,
            TuningConstants.SELECTOR_BUMPER,
            TuningConstants.SELECTOR_THRESHOLDS,
            TuningConstants.SELECTOR_MUFFLER,
            TuningConstants.SELECTOR_SPLITTER,
            TuningConstants.SELECTOR_ROOF,
            TuningConstants.SELECTOR_DISCS,
            TuningConstants.SELECTOR_SPINNING_DISCS,
            TuningConstants.SELECTOR_HOOD,
            TuningConstants.SELECTOR_TRUNK,
            TuningConstants.SELECTOR_FENDER_EXTENSION,
            TuningConstants.SELECTOR_BUMPER_GRILL,
            TuningConstants.SELECTOR_SECOND_SPOILER)
    }
}
