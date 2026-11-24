package com.wn.blackrussia.game.ui.tuning

import android.graphics.Bitmap
import android.graphics.drawable.Drawable
import android.os.Handler
import android.os.Looper
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.animation.Animation
import android.view.animation.AnimationUtils
import android.widget.ProgressBar
import android.widget.RelativeLayout
import android.widget.TextView
import androidx.core.content.res.ResourcesCompat
import androidx.recyclerview.widget.GridLayoutManager
import com.wn.blackrussia.R
import com.wn.blackrussia.databinding.TuningDialogBinding
import com.wn.blackrussia.game.Multiplayer
import com.wn.blackrussia.game.ui.NativeGui
import com.wn.blackrussia.game.ui.donate.CarColorCatalog
import com.wn.blackrussia.game.ui.util.EntitySnaps
import org.json.JSONObject

class BrTuning : NativeGui<TuningDialogBinding>(TuningDialogBinding::class, hud = false, chat = false) {

    val layoutMain = UILayoutTuningMain(this)
    val layoutViewCar = UILayoutTuningViewCar(this)
    val layoutDetailsList = UILayoutTuningDetailsList(this)
    val layoutColorsList = UILayoutTuningColorsList(this)
    val layoutCollapse = UILayoutTuningCollapse(this)
    val layoutInfoParameters = UILayoutTuningViewInfoParameters(this)
    val layoutDiagnostics = UILayoutTuningDiagnostics(this)
    val layoutRgbColor = UILayoutTuningRGBColor(this)

    var activeScreen: TuneGuiScreen? = null
        private set
    private var activeNode: TuneGuiNode? = null

    var activeSelectorId = -1
        private set

    var gosCost = 0
        private set
    var carId = 0
        private set

    var playersMoney = 0
        private set

    private var nameCar = ""
    private var thisSubmenu = -1
    private var nameThisCheckSubmenu = ""
    private var savedLayoutForViewCar = -1

    var typeLayout = -1

    var statusDiagnostic = 0
        private set
    var startVinyl = ""
        private set

    private var detailId = -1
    private var oldInstallDetailId = -1

    // Пары «ид детали, состояние» — сервер шлёт их плоским списком
    val correctDetails = ArrayList<Int>()
    private val detailsWithPercent = ArrayList<Int>()
    private val allCollapse = ArrayList<Int>()

    // Текущие цвета машины строками hex, порядок: кузов, диски, тонировка
    // передняя и задняя, фары, неон общий, левый, правый
    private val carColors = ArrayList<String>()

    val allColors: List<String> get() = carColors

    private val submenuTunings = ArrayList<TuneGuiNode>()
    private val boxNames = ArrayList<TuningBoxItem>()
    private val savedVinyls = HashMap<Int, Bitmap>()

    private lateinit var anim: Animation

    private var submenuOverlay: View? = null
    private var applyOverlay: View? = null
    private var boxOverlay: View? = null
    private var resetOverlay: View? = null

    private var submenuAdapter: TuningSubmenuItemsAdapter? = null
    private var applyLogic: TuningLogicPopupWindowApply? = null
    private var openingBox: TuningOpeningBox? = null
    private var addDataForBox: AddDataForBox? = null

    private var progressStatus = 0

    private val handler = Handler(Looper.getMainLooper())

    init {
        TuneCatalog.load()
        CarColorCatalog.load()

        anim = AnimationUtils.loadAnimation(Multiplayer.getInstance(), R.anim.button_click)

        binding.myContainer.addLayout(LAYOUT_MAIN, layoutMain)
        binding.myContainer.addLayout(LAYOUT_VIEW_CAR, layoutViewCar)
        binding.myContainer.addLayout(LAYOUT_DETAILS_LIST, layoutDetailsList)
        binding.myContainer.addLayout(LAYOUT_COLORS_LIST, layoutColorsList)
        binding.myContainer.addLayout(LAYOUT_RGB_COLOR, layoutRgbColor)
        binding.myContainer.addLayout(LAYOUT_COLLAPSE, layoutCollapse)
        binding.myContainer.addLayout(LAYOUT_VIEW_OPTIONS, layoutInfoParameters)
        binding.myContainer.addLayout(LAYOUT_DIAGNOSTIC, layoutDiagnostics)

        binding.headerExit.setOnClickListener { view ->
            view.startAnimation(anim)
            handler.postDelayed({ destroy() }, CLICK_DELAY)
        }

        binding.headerBackToMainMenu.setOnClickListener { view ->
            view.startAnimation(anim)
            handler.postDelayed({
                listenerLayout(LAYOUT_MAIN)
                sendAction(TuningConstants.OPERATION_CLOSE_VIEW_CAR)
                clearStatusForAllMenu()
            }, CLICK_DELAY)
        }
    }

    // Экран приходит из json клиента, поэтому сервер присылает только его номер
    private fun applyStartState(json: JSONObject) {
        activeScreen = TuneCatalog.screen(json.optInt(TuningConstants.KEY_GET_WINDOW))
        layoutMain.rebuildTypes()

        for (item in TuneCatalog.items) {
            item.thisLocation = TuningConstants.DETAIL_STATUS_CAN_BUY
        }

        nameCar = json.optString(TuningConstants.KEY_GET_CAR_NAME)
        binding.titleThisCar.text = nameCar

        playersMoney = json.optInt(TuningConstants.KEY_GET_PLAYERS_MONEY)
        setMoneyInTitle(playersMoney)

        readStatusDetails(json)
        statusDiagnostic = json.optInt(TuningConstants.KEY_GET_STATUS_DIAGNOSTIC)
        readDetailsList(json)

        gosCost = json.optInt(TuningConstants.KEY_GET_GOS_COST)
        carId = json.optInt(TuningConstants.KEY_GET_CAR_ID)

        binding.myContainer.setCurrentLayout(LAYOUT_MAIN)
        binding.headerBackToMainMenu.visibility = View.INVISIBLE

        sendAction(TuningConstants.OPERATION_SAVE_ACTUAL_CAR)

        handler.postDelayed({
            sendAction(TuningConstants.OPERATION_GET_NEW_LIST_COLORS)
            sendAction(TuningConstants.OPERATION_GET_START_VINYLS)
            sendAction(TuningConstants.OPERATION_GET_COLLAPSE)
        }, START_DELAY)
    }

    override fun receivePacket(actionId: Int, json: String) {
        val obj = runCatching { JSONObject(json) }.getOrNull() ?: return

        // Стартовый пакет приходит без ключа действия и несёт номер экрана
        if (!obj.has(TuningConstants.KEY_ACTION_TYPE)) {
            applyStartState(obj)
            return
        }

        onPacketIncoming(obj)
    }

    private fun onPacketIncoming(json: JSONObject) {
        val type = json.optInt(TuningConstants.KEY_ACTION_TYPE)
        val status = json.optInt(TuningConstants.KEY_GET_STATUS_OPERATION)

        when (type) {
            TuningConstants.OPERATION_BUY_COLOR,
            TuningConstants.OPERATION_BUY_COLOR_WITH_TRANSPARENCY -> {
                if (status != 1) return

                updateValueMoney(json)
                sendAction(TuningConstants.OPERATION_SAVE_ACTUAL_CAR)
                sendAction(TuningConstants.OPERATION_GET_NEW_LIST_COLORS)
            }

            TuningConstants.OPERATION_BUY_VINYL -> {
                if (status != 1) return

                updateValueMoney(json)
                readDetailId(json)
                sendAction(TuningConstants.OPERATION_SAVE_ACTUAL_CAR)
                layoutDetailsList.updateStatusVinylsAfterEstablished(detailId)
                startVinyl = layoutDetailsList.selectedVinyl?.nameVinyl ?: ""
            }

            TuningConstants.OPERATION_GET_NEW_LIST_COLORS -> {
                readAllColors(json)
                layoutColorsList.setStartColor()
                layoutRgbColor.setStartColor()
            }

            TuningConstants.OPERATION_GET_START_VINYLS -> {
                startVinyl = json.optString(TuningConstants.KEY_GET_VINYL)
            }

            TuningConstants.OPERATION_GET_COLLAPSE -> {
                readAllCollapse(json)
                layoutCollapse.setStartCollapse(allCollapse)
            }

            TuningConstants.OPERATION_RESET_DETAILS -> {
                if (status != 1) return
                onResetDetails(json)
            }

            TuningConstants.OPERATION_DIAGNOSTIC -> {
                if (status != 1) return

                layoutDiagnostics.startDiagnostic()
                statusDiagnostic = TuningConstants.STATUS_ACTUAL_DIAGNOSTIC
                readStatusDetails(json)
                updateValueMoney(json)

                handler.postDelayed({
                    layoutDiagnostics.setStatusDiagnosticAndChangeParameters(
                        TuningConstants.STATUS_ACTUAL_DIAGNOSTIC)
                    layoutDiagnostics.setStartParameters(detailsWithPercent)
                }, DIAGNOSTIC_DELAY)
            }

            TuningConstants.OPERATION_BUY_DETAIL -> {
                if (status != 1) return

                updateValueMoney(json)
                readDetailId(json)
                readOldInstallDetailId(json)
                layoutDetailsList.updateStatusDetailAfterEstablished(detailId, oldInstallDetailId)
                sendAction(TuningConstants.OPERATION_SAVE_ACTUAL_CAR)
                updateStatusDetailsAfterInstall(detailId, oldInstallDetailId)
            }

            TuningConstants.OPERATION_REPAIR_DETAIL -> {
                if (status != 1) return

                updateValueMoney(json)
                readDetailId(json)
                layoutDiagnostics.updateStatusDetail(detailId)
            }

            TuningConstants.OPERATION_CHECK_FOR_REPAIR_DETAIL -> {
                if (status != 1) return
                layoutDiagnostics.showApplyRepair()
            }

            TuningConstants.OPERATION_INSTALL_DETAIL -> {
                if (status != 1) return

                readDetailId(json)
                readOldInstallDetailId(json)
                layoutDetailsList.updateStatusDetailAfterEstablished(detailId, oldInstallDetailId)
                sendAction(TuningConstants.OPERATION_SAVE_ACTUAL_CAR)
                updateStatusDetailsAfterInstall(detailId, oldInstallDetailId)
            }

            TuningConstants.OPERATION_OPEN_BOX -> {
                if (status == 0) {
                    openingBox?.statusOpeningBox = 0
                    return
                }
                if (status != 1) return

                openingBox?.statusOpeningBox = 1
                openingBox?.playOpening()

                updateValueMoney(json)
                readDetailId(json)
                layoutDetailsList.updateStatusDetailAfterPurchase(detailId)

                val name = addDataForBox?.allNameInBox?.firstOrNull { it.id == detailId }?.name.orEmpty()
                val parts = name.substringBefore('\n').split(' ')

                openingBox?.setDataOpeningBox(parts.getOrElse(0) { "" }, parts.getOrElse(1) { "" })
                updateStatusDetailsAfterBought(detailId, TuningConstants.DETAIL_STATUS_BOUGHT)
            }

            TuningConstants.OPERATION_BUY_COLLAPSE -> {
                if (status != 1) return

                updateValueMoney(json)
                sendAction(TuningConstants.OPERATION_SAVE_ACTUAL_CAR)
                sendAction(TuningConstants.OPERATION_GET_COLLAPSE)
            }
        }
    }

    private fun onResetDetails(json: JSONObject) {
        when (typeLayout) {
            TuningConstants.RGB_TYPE_MENU, TuningConstants.READY_COLOR_TYPE_MENU -> handler.postDelayed({
                sendAction(TuningConstants.OPERATION_SAVE_ACTUAL_CAR)
                sendAction(TuningConstants.OPERATION_GET_NEW_LIST_COLORS)
                layoutColorsList.resetColorParameters()
            }, RESET_DELAY)

            TuningConstants.DETAILS_LIST_TYPE_MENU -> {
                readOldInstallDetailId(json)
                updateStatusDetailsAfterReset(oldInstallDetailId)
                layoutDetailsList.updateStatusDetailAfterReset(oldInstallDetailId)

                handler.postDelayed({ sendAction(TuningConstants.OPERATION_SAVE_ACTUAL_CAR) }, RESET_DELAY)
            }

            TuningConstants.VINYLS_LIST_TYPE_MENU -> {
                startVinyl = ""
                layoutDetailsList.resetVinyls()

                handler.postDelayed({ sendAction(TuningConstants.OPERATION_SAVE_ACTUAL_CAR) }, RESET_DELAY)
            }

            TuningConstants.COLLAPSE_TYPE_MENU -> handler.postDelayed({
                sendAction(TuningConstants.OPERATION_SAVE_ACTUAL_CAR)
                sendAction(TuningConstants.OPERATION_GET_COLLAPSE)
            }, RESET_DELAY)
        }
    }

    // ---- чтение пакета -------------------------------------------------

    private fun updateValueMoney(json: JSONObject) {
        playersMoney = json.optInt(TuningConstants.KEY_GET_PLAYERS_MONEY)
        setMoneyInTitle(playersMoney)
    }

    private fun readDetailId(json: JSONObject) {
        detailId = json.optInt(TuningConstants.KEY_GET_ID_DETAIL, -1)
    }

    private fun readOldInstallDetailId(json: JSONObject) {
        oldInstallDetailId = json.optInt(TuningConstants.KEY_GET_OLD_INSTALL_DETAIL, -1)
    }

    private fun readDetailsList(json: JSONObject) {
        val array = json.optJSONArray(
            TuningConstants.KEY_GET_DETAILS_LIST_WITH_COST_AND_STATUS) ?: return

        correctDetails.clear()
        for (i in 0 until array.length()) {
            correctDetails += array.optInt(i)
        }
    }

    private fun readStatusDetails(json: JSONObject) {
        val array = json.optJSONArray(TuningConstants.KEY_GET_VALUE_STATUS_DETAIL) ?: return

        detailsWithPercent.clear()
        for (i in 0 until array.length()) {
            detailsWithPercent += array.optInt(i)
        }
    }

    private fun readAllColors(json: JSONObject) {
        val array = json.optJSONArray(TuningConstants.KEY_GET_ALL_COLORS) ?: return

        carColors.clear()
        for (i in 0 until array.length()) {
            carColors += array.optString(i)
        }
    }

    private fun readAllCollapse(json: JSONObject) {
        val array = json.optJSONArray(TuningConstants.KEY_GET_ALL_COLLAPSE) ?: return

        allCollapse.clear()
        for (i in 0 until array.length()) {
            allCollapse += array.optInt(i)
        }
    }

    // ---- состояние деталей ---------------------------------------------

    private fun updateStatusDetailsAfterBought(idDetail: Int, location: Int) {
        var found = false

        for (i in correctDetails.indices step 2) {
            if (correctDetails[i] == idDetail) {
                correctDetails[i + 1] = location
                found = true
            }
        }

        if (found) return

        correctDetails += idDetail
        correctDetails += location
    }

    private fun updateStatusDetailsAfterInstall(idNewDetail: Int, idOldDetail: Int) {
        var found = false

        for (i in correctDetails.indices step 2) {
            if (correctDetails[i] == idNewDetail) {
                correctDetails[i + 1] = TuningConstants.DETAIL_STATUS_ESTABLISHED
                found = true
            }
        }

        if (!found) return

        for (i in correctDetails.indices step 2) {
            if (correctDetails[i] == idOldDetail) {
                correctDetails[i + 1] = TuningConstants.DETAIL_STATUS_BOUGHT
            }
        }
    }

    private fun updateStatusDetailsAfterReset(idOldDetail: Int) {
        for (i in correctDetails.indices step 2) {
            if (correctDetails[i] == idOldDetail) {
                correctDetails[i + 1] = TuningConstants.DETAIL_STATUS_BOUGHT
            }
        }
    }

    // Селекторы, которые вообще показываем: все базовые плюс те, где у игрока
    // уже что-то куплено
    fun correctSelectors(): List<Int> {
        val out = LinkedHashSet<Int>()

        for (item in TuneCatalog.items) {
            if (item.def == 1) out += item.selector
        }

        return out.toList()
    }

    fun correctSelectorsOld(base: List<Int>): List<Int> {
        val out = LinkedHashSet<Int>()

        for (i in correctDetails.indices step 2) {
            TuneCatalog.itemById(correctDetails[i])?.let { out += it.selector }
        }

        out += base
        return out.toList()
    }

    fun tuneItemsForSelector(selector: Int): ArrayList<Tune> =
        TuneCatalog.itemsForSelector(selector, gosCost)

    // Базовые детали показываем всегда, купленные добавляем по факту наличия
    fun correctDetailsListFormatTune(items: List<Tune>): List<Tune> {
        val out = ArrayList<Tune>()

        for (item in items) {
            if (item.def == 1) {
                for (i in correctDetails.indices step 2) {
                    if (item.id == correctDetails[i]) item.thisLocation = correctDetails[i + 1]
                }

                out += item
                continue
            }

            for (i in correctDetails.indices step 2) {
                if (item.id != correctDetails[i]) continue

                item.thisLocation = correctDetails[i + 1]
                out += item
            }
        }

        return out
    }

    fun selectorPrice(selector: Int): Int = TuningPrices.selectorPrice(selector, gosCost)

    val colorWithTransparency: String get() = layoutColorsList.colorItem
    val rgbColor: String get() = layoutRgbColor.color
    val collapse: Int get() = layoutCollapse.collapse
    val repairDetail: Int get() = layoutDiagnostics.thisRepairDetail

    // ---- навигация ------------------------------------------------------

    fun listenerLayout(idLayout: Int) {
        when (idLayout) {
            LAYOUT_MAIN -> {
                binding.myContainer.setCurrentLayout(LAYOUT_MAIN)
                layoutMain.setVisibleMainView(true)
                binding.headerBackToMainMenu.visibility = View.INVISIBLE
            }

            LAYOUT_VIEW_CAR -> {
                binding.myContainer.setCurrentLayout(LAYOUT_VIEW_CAR)
                sendAction(TuningConstants.OPERATION_VIEW_CAR)
                binding.headerBackToMainMenu.visibility = View.VISIBLE
            }

            LAYOUT_SUBMENU -> {
                if (submenuTunings.isEmpty()) {
                    sendAction(TuningConstants.OPERATION_GET_GAME_WARNING)
                    return
                }

                showSubmenu()
            }

            LAYOUT_DETAILS_LIST_WITH_DATA_CLEANING -> {
                binding.myContainer.setCurrentLayout(LAYOUT_DETAILS_LIST)
                layoutDetailsList.nameThisWindow = nameThisCheckSubmenu
                layoutDetailsList.setActiveSelectorId(activeSelectorId)

                if (activeSelectorId == TuningConstants.SELECTOR_VINYL) {
                    layoutDetailsList.setNewParametersWithVinyls()
                    layoutDetailsList.setStartVinyl(startVinyl)
                } else {
                    layoutDetailsList.setNewParametersInDetailList(
                        correctDetailsListFormatTune(tuneItemsForSelector(activeSelectorId)))
                }

                binding.headerBackToMainMenu.visibility = View.VISIBLE
            }

            LAYOUT_DETAILS_LIST -> {
                binding.myContainer.setCurrentLayout(LAYOUT_DETAILS_LIST)
                layoutDetailsList.nameThisWindow = nameThisCheckSubmenu
                binding.headerBackToMainMenu.visibility = View.VISIBLE
            }

            LAYOUT_COLORS_LIST -> {
                binding.myContainer.setCurrentLayout(LAYOUT_COLORS_LIST)
                binding.headerBackToMainMenu.visibility = View.VISIBLE
                layoutRgbColor.setStartParameters(activeSelectorId)
                layoutColorsList.setStartParameters(activeSelectorId)
                layoutColorsList.setStartColor()
                layoutRgbColor.setStartColor()
            }

            LAYOUT_COLORS_LIST_AFTER_BACK -> {
                binding.myContainer.setCurrentLayout(LAYOUT_COLORS_LIST)
                binding.headerBackToMainMenu.visibility = View.VISIBLE
                layoutRgbColor.isNotThisLayout()
            }

            LAYOUT_RGB_COLOR -> {
                binding.myContainer.setCurrentLayout(LAYOUT_RGB_COLOR)
                binding.headerBackToMainMenu.visibility = View.VISIBLE
                layoutRgbColor.isThisLayout()
            }

            LAYOUT_COLLAPSE -> {
                binding.myContainer.setCurrentLayout(LAYOUT_COLLAPSE)
                layoutCollapse.setActiveSelectorId(activeSelectorId)
                layoutCollapse.nameChangeParameter = nameThisCheckSubmenu
                layoutCollapse.setStartParameters(allCollapse)
                binding.headerBackToMainMenu.visibility = View.VISIBLE
            }

            LAYOUT_VIEW_OPTIONS -> {
                binding.myContainer.setCurrentLayout(LAYOUT_VIEW_OPTIONS)
                binding.headerBackToMainMenu.visibility = View.VISIBLE
            }

            LAYOUT_OPENING_BOX -> showBoxView()

            LAYOUT_DIAGNOSTIC -> {
                binding.myContainer.setCurrentLayout(LAYOUT_DIAGNOSTIC)
                layoutDiagnostics.setStatusDiagnosticAndChangeParameters(statusDiagnostic)
                layoutDiagnostics.setStartParameters(detailsWithPercent)
            }

            LAYOUT_COLLAPSE_AFTER_VIEW_CAR -> {
                binding.myContainer.setCurrentLayout(LAYOUT_COLLAPSE)
                binding.headerBackToMainMenu.visibility = View.VISIBLE
            }
        }
    }

    fun backToSubmenu() {
        if (thisSubmenu != -1) {
            activeNode = parentNodeFor(activeNode)
            showSubmenu()
        } else {
            listenerLayout(LAYOUT_MAIN)
        }

        clearStatusForAllMenu()
    }

    fun listenerButtonBackFromViewCar() {
        if (thisSubmenu == -1) {
            listenerLayout(LAYOUT_MAIN)
            return
        }

        when (savedLayoutForViewCar) {
            LAYOUT_DETAILS_LIST -> listenerLayout(LAYOUT_DETAILS_LIST)
            LAYOUT_COLORS_LIST -> listenerLayout(LAYOUT_COLORS_LIST_AFTER_BACK)
            LAYOUT_RGB_COLOR -> listenerLayout(LAYOUT_RGB_COLOR)
            LAYOUT_COLLAPSE_AFTER_VIEW_CAR -> listenerLayout(LAYOUT_COLLAPSE_AFTER_VIEW_CAR)
        }
    }

    fun saveLayoutForViewCar(layout: Int) {
        savedLayoutForViewCar = layout
    }

    fun setEmptyThisSubmenu() {
        thisSubmenu = -1
    }

    fun setDataInNewSubmenu(nodes: List<TuneGuiNode>) {
        submenuTunings.clear()
        submenuTunings += nodes
    }

    private fun clearStatusForAllMenu() {
        sendAction(TuningConstants.OPERATION_DELETE_DETAILS)
        layoutDetailsList.setEmptyObjectForDetail()
        layoutColorsList.setEmptyObject()
        layoutCollapse.setEmptyValueCorner()
        layoutColorsList.setEmptyTransparency()
        layoutRgbColor.setEmptyRGBColor()
        layoutDetailsList.setEmptyObjectForVinyl()
    }

    fun setClickSubmenu(typeList: Int) {
        when (typeList) {
            TuningConstants.RGB_TYPE_MENU -> listenerLayout(LAYOUT_COLORS_LIST)
            TuningConstants.READY_COLOR_TYPE_MENU -> listenerLayout(LAYOUT_DETAILS_LIST_WITH_DATA_CLEANING)
            TuningConstants.VINYLS_LIST_TYPE_MENU -> listenerLayout(LAYOUT_COLLAPSE)
        }

        hideOverlay(submenuOverlay)
    }

    fun setClickMainMenu(typeList: Int) {
        if (typeList != TuningConstants.DETAILS_LIST_TYPE_MENU) return

        setEmptyThisSubmenu()
        listenerLayout(LAYOUT_DIAGNOSTIC)
    }

    fun onSubmenuItemClick(position: Int, view: View) {
        view.startAnimation(anim)

        val node = layoutMain.selectedNodesList.getOrNull(position) ?: return

        activeNode = node
        activeSelectorId = node.selectorId
        thisSubmenu = node.opensType
        nameThisCheckSubmenu = node.name

        sendSelector(TuningConstants.OPERATION_SET_CAMERA_FOR_SELECTOR, activeSelectorId)

        handler.postDelayed({ setClickSubmenu(thisSubmenu) }, CLICK_DELAY)
    }

    private fun parentNodeFor(child: TuneGuiNode?): TuneGuiNode? {
        val screen = activeScreen ?: return null
        if (child == null) return null

        for (node in screen.footer) {
            parentNodeIn(node, child)?.let { return it }
        }

        return null
    }

    private fun parentNodeIn(root: TuneGuiNode, child: TuneGuiNode): TuneGuiNode? {
        if (root.names.contains(child)) return root

        for (node in root.names) {
            parentNodeIn(node, child)?.let { return it }
        }

        return null
    }

    // ---- наложения ------------------------------------------------------

    private fun inflater(): LayoutInflater = LayoutInflater.from(Multiplayer.getInstance())

    private fun addOverlay(view: View) {
        val root = binding.root as ViewGroup

        if (view.parent == null) {
            root.addView(view, RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.MATCH_PARENT,
                RelativeLayout.LayoutParams.MATCH_PARENT))
        }

        view.visibility = View.VISIBLE
        view.bringToFront()
    }

    private fun hideOverlay(view: View?) {
        view?.visibility = View.GONE
    }

    private fun showSubmenu() {
        listenerLayout(LAYOUT_MAIN)
        layoutMain.setVisibleMainView(false)

        val view = submenuOverlay ?: inflater()
            .inflate(R.layout.tuning_submenu_view, binding.root as ViewGroup, false)
            .also { overlay ->
                submenuOverlay = overlay

                overlay.findViewById<View>(R.id.tuning_submenu_close).setOnClickListener { button ->
                    button.startAnimation(anim)
                    handler.postDelayed({
                        hideOverlay(submenuOverlay)
                        layoutMain.setVisibleMainView(true)
                    }, CLICK_DELAY)
                }
            }

        addOverlay(view)

        val recycler = view.findViewById<androidx.recyclerview.widget.RecyclerView>(R.id.recv_submenu_items)
        submenuAdapter = TuningSubmenuItemsAdapter(this, submenuTunings)
        recycler.layoutManager = GridLayoutManager(view.context, SUBMENU_COLUMNS)
        recycler.adapter = submenuAdapter
    }

    fun applyBuyNewItem(action: Int, cost: Int, currency: String, selectorId: Int, typeMenu: Int) {
        val view = applyOverlay ?: inflater()
            .inflate(R.layout.tuning_submenu_apply, binding.root as ViewGroup, false)
            .also { applyOverlay = it }

        addOverlay(view)

        applyLogic = TuningLogicPopupWindowApply(this, view, typeMenu).apply {
            this.selectorId = selectorId
            setDataInPopupWindow(action, cost, currency)
            startWindow()
        }
    }

    fun closeApplyWindow() {
        hideOverlay(applyOverlay)
    }

    fun applyResetDetails(selectorId: Int, typeLayout: Int, established: Int) {
        val view = resetOverlay ?: inflater()
            .inflate(R.layout.tuning_reset_apply, binding.root as ViewGroup, false)
            .also { resetOverlay = it }

        addOverlay(view)

        TuningLogicPopupWindowResetDetails(this, view, selectorId, typeLayout, established).start()
    }

    fun dismissPopupWindowResetDetails() {
        hideOverlay(resetOverlay)
    }

    private fun showBoxView() {
        addDataForBox = AddDataForBox(this).also { data ->
            data.build()
            boxNames.clear()
            boxNames += data.allNameInBox
        }

        if (boxOverlay != null && statusBox != 0) {
            (boxOverlay?.parent as? ViewGroup)?.removeView(boxOverlay)
            boxOverlay = null
        }

        val view = boxOverlay ?: inflater()
            .inflate(R.layout.tuning_submenu_box, binding.root as ViewGroup, false)
            .also { boxOverlay = it }

        addOverlay(view)

        val detail = layoutDetailsList.buyThisDetail

        openingBox = TuningOpeningBox(this, view).apply {
            startOpeningBox(detail?.cost ?: 0, detail?.currency ?: -1)
        }

        binding.headerTuning.visibility = View.INVISIBLE
    }

    fun closeBoxCardView() {
        hideOverlay(boxOverlay)

        binding.headerTuning.visibility = View.VISIBLE
        layoutInfoParameters.setVisibleThisWindow(true)
        layoutDetailsList.setVisibleBody(true)
    }

    val namesForBox: List<TuningBoxItem> get() = boxNames

    val statusBox: Int get() = openingBox?.statusOpeningBox ?: 0

    // ---- диагностика ----------------------------------------------------

    fun startDiagnostic() {
        binding.processDiagnostic.visibility = View.VISIBLE

        val progressBar = binding.tuningProgressBar
        val valuePercent = binding.progressValuePercent

        progressStatus = 0
        stepDiagnostic(progressBar, valuePercent)
    }

    private fun stepDiagnostic(progressBar: ProgressBar, valuePercent: TextView) {
        if (progressStatus >= DIAGNOSTIC_STEPS) {
            handler.postDelayed({
                binding.processDiagnostic.visibility = View.INVISIBLE
                progressStatus = 0
            }, DIAGNOSTIC_TAIL)
            return
        }

        progressStatus++
        progressBar.progress = progressStatus
        valuePercent.text = "$progressStatus%"

        handler.postDelayed({ stepDiagnostic(progressBar, valuePercent) }, DIAGNOSTIC_STEP)
    }

    // ---- отправка -------------------------------------------------------

    private fun send(json: JSONObject) = sendPacket(0, json.toString())

    fun sendAction(type: Int) =
        send(JSONObject().put(TuningConstants.KEY_ACTION_TYPE, type))

    fun sendIntegerData(type: Int, data: Int) = send(JSONObject()
        .put(TuningConstants.KEY_ACTION_TYPE, type)
        .put(TuningConstants.KEY_SEND_DETAIL_ID, data))

    fun sendIntegerDataWithSelector(type: Int, id: Int, selector: Int) = send(JSONObject()
        .put(TuningConstants.KEY_ACTION_TYPE, type)
        .put(TuningConstants.KEY_SEND_DETAIL_ID, id)
        .put(TuningConstants.KEY_SEND_SELECTOR_ID, selector))

    fun sendColorWithSelector(type: Int, color: String, selector: Int) = send(JSONObject()
        .put(TuningConstants.KEY_ACTION_TYPE, type)
        .put(TuningConstants.KEY_SEND_COLOR_HEX, color)
        .put(TuningConstants.KEY_SEND_SELECTOR_ID, selector))

    fun sendColorWithTransparency(type: Int, color: String, alpha: Int, selector: Int) = send(JSONObject()
        .put(TuningConstants.KEY_ACTION_TYPE, type)
        .put(TuningConstants.KEY_SEND_COLOR_HEX, color)
        .put(TuningConstants.KEY_SEND_COLOR_TRANSPARENCY, alpha)
        .put(TuningConstants.KEY_SEND_SELECTOR_ID, selector))

    fun sendResetDetails(type: Int, selectorId: Int, established: Int) = send(JSONObject()
        .put(TuningConstants.KEY_ACTION_TYPE, type)
        .put(TuningConstants.KEY_SEND_SELECTOR_ID, selectorId)
        .put(TuningConstants.KEY_SEND_DETAILS_TYPE, established))

    fun sendSelector(type: Int, selector: Int) = send(JSONObject()
        .put(TuningConstants.KEY_ACTION_TYPE, type)
        .put(TuningConstants.KEY_GET_CAR_ID, selector))

    fun sendDetailOrCollapse(type: Int, value: Int) = send(JSONObject()
        .put(TuningConstants.KEY_ACTION_TYPE, type)
        .put(TuningConstants.KEY_SEND_DETAIL_ID, value))

    fun sendColorPreview(type: Int, color: String) = send(JSONObject()
        .put(TuningConstants.KEY_ACTION_TYPE, type)
        .put(TuningConstants.KEY_SEND_COLOR_FOR_PREVIEW, color))

    fun sendVinyl(type: Int, nameVinyl: String) = send(JSONObject()
        .put(TuningConstants.KEY_ACTION_TYPE, type)
        .put(TuningConstants.KEY_SEND_VINYL_NAME, nameVinyl))

    fun sendSound(type: Int, idSound: Int) = send(JSONObject()
        .put(TuningConstants.KEY_ACTION_TYPE, type)
        .put(TuningConstants.KEY_SEND_ID_SOUND, idSound))

    // ---- мелочи ---------------------------------------------------------

    private fun setMoneyInTitle(value: Int) {
        binding.valuePlayersMoney.text = priceWithSpaces(value)
    }

    fun animation(): Animation = anim

    fun drawableByName(name: String): Drawable? {
        val context = Multiplayer.getInstance()
        val id = context.resources.getIdentifier(name, "drawable", context.packageName)

        if (id == 0) return ResourcesCompat.getDrawable(context.resources, R.drawable.tuning_back, null)

        return ResourcesCompat.getDrawable(context.resources, id, null)
    }

    // Текстуру винила рисует движок в свой кадр, ответ приходит на UI-потоке.
    // Готовые картинки держим по номеру ячейки: список пересобирает плитки при
    // каждой прокрутке, а повторно ждать движок незачем.
    fun requestVinylTexture(nameVinyl: String, position: Int, onReady: (Bitmap) -> Unit) {
        EntitySnaps.loadVinyl(nameVinyl) { bitmap ->
            savedVinyls[position] = bitmap
            onReady(bitmap)
        }
    }

    fun vinylBitmap(position: Int): Bitmap? = savedVinyls[position]

    override fun destroy() {
        clearStatusForAllMenu()
        sendAction(TuningConstants.OPERATION_CLOSE_MENU)
        super.destroy()
    }

    companion object {
        const val LAYOUT_MAIN = 0
        const val LAYOUT_VIEW_CAR = 1
        const val LAYOUT_SUBMENU = 2
        const val LAYOUT_DETAILS_LIST_WITH_DATA_CLEANING = 3
        const val LAYOUT_DETAILS_LIST = 4
        const val LAYOUT_COLORS_LIST = 5
        const val LAYOUT_COLORS_LIST_AFTER_BACK = 6
        const val LAYOUT_RGB_COLOR = 8
        const val LAYOUT_COLLAPSE = 9
        const val LAYOUT_VIEW_OPTIONS = 10
        const val LAYOUT_OPENING_BOX = 11
        const val LAYOUT_DIAGNOSTIC = 12
        const val LAYOUT_COLLAPSE_AFTER_VIEW_CAR = 14

        private const val SUBMENU_COLUMNS = 3

        private const val CLICK_DELAY = 250L
        private const val START_DELAY = 500L
        private const val RESET_DELAY = 500L
        private const val DIAGNOSTIC_DELAY = 2100L
        private const val DIAGNOSTIC_STEP = 20L
        private const val DIAGNOSTIC_TAIL = 200L
        private const val DIAGNOSTIC_STEPS = 100

        // Пробел через каждые три разряда, как в остальном интерфейсе
        fun priceWithSpaces(value: Int): String {
            val text = value.toString()
            val out = StringBuilder()

            for ((index, ch) in text.withIndex()) {
                if (index > 0 && (text.length - index) % 3 == 0) out.append(' ')
                out.append(ch)
            }

            return out.toString()
        }
    }
}
