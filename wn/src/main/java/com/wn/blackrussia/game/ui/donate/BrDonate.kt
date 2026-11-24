package com.wn.blackrussia.game.ui.donate

import android.graphics.Color
import android.graphics.Outline
import android.view.LayoutInflater
import android.view.View
import android.view.ViewOutlineProvider
import android.widget.Button
import android.widget.RelativeLayout
import android.widget.TextView
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.wn.blackrussia.R
import com.wn.blackrussia.databinding.DonateBinding
import com.wn.blackrussia.game.GUIManager
import com.wn.blackrussia.game.Multiplayer
import com.wn.blackrussia.game.ui.NativeGui
import com.wn.blackrussia.game.ui.blackpass.BrBlackPass
import com.wn.blackrussia.game.ui.util.KUtils
import com.wn.blackrussia.game.ui.util.Notify
import com.wn.blackrussia.game.ui.util.UIContainer
import org.json.JSONObject

class BrDonate : NativeGui<DonateBinding>(
    DonateBinding::class,
    hud = false,
    chat = false
) {
    private lateinit var container: UIContainer

    private lateinit var headerAdapter: DonateTabAdapter
    private lateinit var footerAdapter: DonateTabAdapter

    private lateinit var tileLayout: UILayoutDonateTile
    private lateinit var servicesLayout: UILayoutDonateServices
    private lateinit var depositLayout: UILayoutDonateDepositCoins
    private lateinit var carLayout: UILayoutDonatePreviewCar
    private lateinit var packLayout: UILayoutDonatePreviewPack
    private lateinit var blackPassLayout: BrBlackPass

    var currentPage = DonateCatalog.PAGE_SALES
        private set

    var selectedItem: DonateItem? = null
        private set

    private var previousPage = DonateCatalog.PAGE_SALES

    var giftSeconds = -1
        private set

    private var coins = 0
    private var rubles = 0

    // Плитка считается от ширины экрана, а не от плотности: на полку должно
    // помещаться четыре с половиной штуки, как в оригинале.
    val itemWidth: Int
        get() = (Multiplayer.getInstance().resources.displayMetrics.widthPixels / TILES_ON_SHELF).toInt()

    init {
        Multiplayer.getInstance().runOnUiThread { initUI() }
    }

    private fun initUI() {
        DonateCatalog.load()

        container = binding.container

        container.addLayout(LAYOUT_TILE, UILayoutDonateTile(this).also { tileLayout = it })
        container.addLayout(LAYOUT_SERVICES, UILayoutDonateServices(this).also { servicesLayout = it })
        container.addLayout(LAYOUT_BLACKPASS, BrBlackPass(this).also { blackPassLayout = it })
        container.addLayout(LAYOUT_DEPOSIT, UILayoutDonateDepositCoins(this).also { depositLayout = it })
        container.addLayout(LAYOUT_PREVIEW_CAR, UILayoutDonatePreviewCar(this).also { carLayout = it })
        container.addLayout(LAYOUT_PREVIEW_PACK, UILayoutDonatePreviewPack(this).also { packLayout = it })

        setupHeader()
        setupFooter()

        binding.headerExit.setOnClickListener { closeWindow() }

        binding.donateButtonBCPlus.setOnClickListener { openDeposit(0) }
        binding.donateButtonMoneyPlus.setOnClickListener { openDeposit(0) }

        container.setCurrentLayout(LAYOUT_TILE)
        onPageChanged(DonateCatalog.PAGE_SALES)

        updateBalance()

        binding.root.bringToFront()

        sendPacket(0, JSONObject().put(KEY_TYPE, ACTION_REQUEST_STATE).toString())
    }

    private fun setupHeader() {
        headerAdapter = DonateTabAdapter(
            listOf(
                DonateTab("Товары", LAYOUT_TILE),
                DonateTab("Услуги", LAYOUT_SERVICES),
                DonateTab("Black Pass", LAYOUT_BLACKPASS),
                DonateTab("Кейсы", LAYOUT_CASES)
            ),
            textSizeSp = 9.0f
        ) { onLayoutChanged(it) }

        binding.headerRV.adapter = headerAdapter
        binding.headerRV.layoutManager = EvenTabLayoutManager(Multiplayer.getInstance())

        roundCorners(binding.headerRV, top = false)

        binding.headerLeft.setOnClickListener { headerAdapter.previous() }
        binding.headerRight.setOnClickListener { headerAdapter.next() }
    }

    private fun setupFooter() {
        footerAdapter = DonateTabAdapter(
            listOf(
                DonateTab("Акции", DonateCatalog.PAGE_SALES),
                DonateTab("Транспорт", DonateCatalog.PAGE_CARS),
                DonateTab("Скины", DonateCatalog.PAGE_SKINS),
                DonateTab("Наборы", DonateCatalog.PAGE_PACKS),
                DonateTab("VIP", DonateCatalog.PAGE_VIP),
                DonateTab("Аксессуары", DonateCatalog.PAGE_ACCESSORIES)
            ),
            textSizeSp = 11.0f
        ) { onPageChanged(it) }

        binding.footerRV.adapter = footerAdapter
        binding.footerRV.layoutManager = EvenTabLayoutManager(Multiplayer.getInstance())

        roundCorners(binding.footerRV, top = true)

        binding.footerLeft.setOnClickListener { footerAdapter.previous() }
        binding.footerRight.setOnClickListener { footerAdapter.next() }
    }

    // Полоса вкладок примыкает к краю экрана: скругляются только внешние углы,
    // поэтому прямоугольник выносится за границу вида на радиус.
    private fun roundCorners(view: View, top: Boolean) {
        val radius = CORNER_RADIUS_DP * Multiplayer.getInstance().resources.displayMetrics.density

        view.outlineProvider = object : ViewOutlineProvider() {
            override fun getOutline(target: View, outline: Outline) {
                if (top) {
                    outline.setRoundRect(0, 0, target.width, (target.height + radius).toInt(), radius)
                } else {
                    outline.setRoundRect(0, (-radius).toInt(), target.width, target.height, radius)
                }
            }
        }

        view.clipToOutline = true
    }

    private fun onLayoutChanged(newLayout: Int) {
        // Полка страниц не нужна только услугам и Black Pass
        showFooter(newLayout != LAYOUT_SERVICES && newLayout != LAYOUT_BLACKPASS)

        // У кейсов нет своего экрана: вкладка лишь просит сервер открыть их
        // и закрывает донат за собой.
        if (newLayout == LAYOUT_CASES) {
            sendPacket(0, JSONObject().put(KEY_TYPE, ACTION_CASES).toString())
            destroy()
            return
        }

        if (newLayout == LAYOUT_DEPOSIT) {
            openDeposit(0)
            return
        }

        container.setCurrentLayout(newLayout)
    }

    private fun showFooter(show: Boolean) {
        val visibility = if (show) View.VISIBLE else View.GONE

        binding.footerRV.visibility = visibility
        binding.footerLeft.visibility = visibility
        binding.footerRight.visibility = visibility
    }

    // Страница витрины всегда показывается на экране плиток
    fun onPageChanged(page: Int) {
        previousPage = if (isDetailPage(page)) currentPage else page
        currentPage = page

        container.setCurrentLayout(LAYOUT_TILE)
        tileLayout.applyPage(page)
    }

    fun isDetailPage(page: Int) =
        page == DonateCatalog.PAGE_CAR_DETAIL || page == DonateCatalog.PAGE_VIP_DETAIL

    fun itemsForPage(page: Int): List<DonateItem> = when (page) {
        DonateCatalog.PAGE_SALES -> DonateCatalog.sales()
        DonateCatalog.PAGE_CARS -> DonateCatalog.carClasses()
        DonateCatalog.PAGE_CAR_DETAIL, DonateCatalog.PAGE_VIP_DETAIL ->
            selectedItem?.let { DonateCatalog.contentsOf(it) }.orEmpty()
        else -> DonateCatalog.page(page)
    }

    fun onTileClicked(item: DonateItem) {
        selectedItem = item

        when {
            // класс транспорта — это витрина, а не товар
            item.type == DonateItem.TYPE_CATEGORY -> {
                onPageChanged(DonateCatalog.PAGE_CAR_DETAIL)
                footerAdapter.deselect()
            }

            item.isCar -> {
                container.setCurrentLayout(LAYOUT_PREVIEW_CAR)
                carLayout.bind(item)
                footerAdapter.deselect()
            }

            item.isPack -> {
                if (item.type == DonateItem.TYPE_VIP_PACK) {
                    onPageChanged(DonateCatalog.PAGE_VIP_DETAIL)
                } else {
                    container.setCurrentLayout(LAYOUT_PREVIEW_PACK)
                    packLayout.bind(item)
                }
                footerAdapter.deselect()
            }

            else -> requestPurchase(item, payWithRubles = item.rubles)
        }
    }

    // Оригинал возвращает не на прошлый экран, а в категорию самого товара:
    // из обзора машины - в её класс, а не в список классов.
    fun goBackFromDetail() {
        val category = DonateCatalog.get(selectedItem?.category ?: -1)

        selectedItem = category

        if (category != null) {
            onPageChanged(DonateCatalog.PAGE_CAR_DETAIL)
            footerAdapter.deselect()
            return
        }

        onPageChanged(previousPage)
        footerAdapter.selectById(previousPage)
    }

    // Разметка диалогов задана по содержимому, а корень доната - RelativeLayout,
    // и без своего правила окно прилипает к верхнему левому углу.
    private fun addCentered(view: View) {
        val params = RelativeLayout.LayoutParams(
            RelativeLayout.LayoutParams.WRAP_CONTENT,
            RelativeLayout.LayoutParams.WRAP_CONTENT
        )

        params.addRule(RelativeLayout.CENTER_IN_PARENT)

        binding.root.addView(view, params)
        view.bringToFront()
    }

    private fun openDeposit(coins: Int) {
        showFooter(false)

        container.setCurrentLayout(LAYOUT_DEPOSIT)
        depositLayout.setInputCoins(coins)
    }

    // Black Pass шлёт свои пакеты через донат: снаружи t = -1
    fun sendBlackPass(json: JSONObject) {
        sendPacket(0, json.toString())
    }

    fun closeFromBlackPass() {
        headerAdapter.selectById(LAYOUT_TILE)
    }

    fun showBlackPassTab() {
        headerAdapter.selectById(LAYOUT_BLACKPASS)
    }

    // extra — это доплата в товаре, а не в цене: для машины там номер цвета,
    // поэтому сумму к подтверждению задаёт вызывающий.
    fun requestPurchase(item: DonateItem, payWithRubles: Boolean, extra: Int = 0, price: Int = item.price) {
        val currency = if (item.rubles) " ₽" else " BC"

        showConfirmationDialog(price, currency, "Подтверждение", "Да", "Нет") {
            val json = JSONObject()
                .put(KEY_TYPE, ACTION_BUY)
                .put("id", item.internalId)
                .put("pay", if (payWithRubles) PAY_RUBLES else PAY_COINS)
                .put("extra", extra)

            sendPacket(0, json.toString())
        }
    }

    fun openGift() {
        sendPacket(0, JSONObject().put(KEY_TYPE, ACTION_GIFT).toString())
    }

    // Сколько коинов даст сумма, знает сервер: там живут акции на пополнение
    fun requestQuote(amount: Int) {
        sendPacket(0, JSONObject().put(KEY_TYPE, ACTION_QUOTE).put("amount", amount).toString())
    }

    fun requestDeposit(coins: Int) {
        val json = JSONObject()
            .put(KEY_TYPE, ACTION_DEPOSIT)
            .put("amount", coins)

        sendPacket(0, json.toString())
    }

    private fun notImplemented() {
        Notify.notImplemented()
    }

    // Подтверждение покупки и её итог показываются своими окнами, а не строкой
    // уведомления: так это устроено в оригинале.
    private val inflater = LayoutInflater.from(Multiplayer.getInstance())

    private var dialog: View? = null

    fun closeDialog() {
        dialog?.let { binding.root.removeView(it) }
        dialog = null
    }

    private fun showConfirmationDialog(
        price: Int,
        currency: String,
        caption: String,
        buttonOk: String,
        buttonCancel: String,
        onConfirmed: () -> Unit
    ) {
        closeDialog()

        val view = inflater.inflate(R.layout.br_popup, binding.root, false)

        view.findViewById<TextView>(R.id.dw_caption).text = caption
        view.findViewById<TextView>(R.id.dw_info).text = KUtils.transfromColors(
            "Вы действительно желаете\n{f4b939}приобрести данный товар за $price$currency?"
        )

        view.findViewById<Button>(R.id.dw_button_ok).apply {
            text = buttonOk
            setOnClickListener {
                closeDialog()
                onConfirmed()
            }
        }

        view.findViewById<Button>(R.id.dw_button_cancel).apply {
            text = buttonCancel
            setOnClickListener { closeDialog() }
        }

        addCentered(view)

        dialog = view
    }

    // Black Pass отвечает своим пакетом, но окна итога у них общие
    fun showBlackPassResult(success: Boolean, text: String, need: Int) {
        if (success) {
            showResultDialog(true, text, "", "ВЫПОЛНЕНО", "ВЕРНУТЬСЯ") {}
            return
        }

        if (need > 0) {
            showResultDialog(false, text, "", "ОШИБКА", "ПОПОЛНИТЬ") { openDeposit(need) }
        } else {
            showResultDialog(false, text, "", "ОШИБКА", "ВЕРНУТЬСЯ") {}
        }
    }

    private fun showResultDialog(
        success: Boolean,
        message: String,
        boldMessage: String,
        caption: String,
        buttonOk: String,
        onOk: () -> Unit
    ) {
        closeDialog()

        val view = inflater.inflate(R.layout.br_dialog_error_success, binding.root, false)

        view.findViewById<TextView>(R.id.dw_default).text = KUtils.transfromColors(message)
        view.findViewById<TextView>(R.id.dw_bold).text = KUtils.transfromColors(boldMessage)
        view.findViewById<TextView>(R.id.dw_caption).text = caption

        if (!success) {
            view.findViewById<View>(R.id.dw_icon).setBackgroundResource(R.drawable.checkbox_red_icon)
        }

        view.findViewById<Button>(R.id.dw_button_ok).apply {
            text = buttonOk
            setOnClickListener {
                closeDialog()
                onOk()
            }
        }

        view.findViewById<View>(R.id.close).setOnClickListener { closeDialog() }

        addCentered(view)

        dialog = view
    }

    private fun closeWindow() {
        sendPacket(0, JSONObject().put(KEY_TYPE, ACTION_CLOSED).toString())
        destroy()
    }

    private fun updateBalance() {
        binding.headerCoins.text = formatPrice(coins)
        binding.headerRubles.text = formatPrice(rubles)
    }

    override fun receivePacket(actionId: Int, json: String) {
        if (json.isEmpty()) {
            return
        }

        val packet = runCatching { JSONObject(json) }.getOrNull() ?: return

        // пакеты Black Pass приходят тем же каналом и помечены t = -1
        if (packet.optInt(KEY_TYPE, 0) == BLACKPASS_PACKET) {
            blackPassLayout.onBlackPassPacket(packet)
            return
        }

        when (packet.optInt(KEY_TYPE, -1)) {
            ACTION_REQUEST_STATE -> applyState(packet)
            ACTION_BUY -> applyPurchaseResult(packet)
            ACTION_QUOTE -> depositLayout.setCoins(packet.optInt("coins"))
            ACTION_CLOSED -> destroy()
            ACTION_OPEN_BLACKPASS -> showBlackPassTab()
        }
    }

    private fun applyState(packet: JSONObject) {
        coins = packet.optInt("coins", coins)
        rubles = packet.optInt("rubles", rubles)
        giftSeconds = packet.optInt("gift", giftSeconds)

        DonateCatalog.resetOffers()

        packet.optJSONArray("sales")?.let { sales ->
            for (i in 0 until sales.length()) {
                val entry = sales.optJSONObject(i) ?: continue
                val item = DonateCatalog.get(entry.optInt("id")) ?: continue

                item.salePercent = entry.optInt("p")
                item.saleTime = entry.optInt("time")
                item.upTag = entry.optInt("u")

                DonateCatalog.addSale(item)
            }
        }

        packet.optJSONArray("limits")?.let { limits ->
            for (i in 0 until limits.length()) {
                val entry = limits.optJSONObject(i) ?: continue
                val item = DonateCatalog.get(entry.optInt("id")) ?: continue

                item.limit = entry.optInt("n", DonateItem.NO_LIMIT)
            }
        }

        updateBalance()

        tileLayout.applyPage(currentPage)
        servicesLayout.refresh()
    }

    private fun applyPurchaseResult(packet: JSONObject) {
        coins = packet.optInt("coins", coins)
        rubles = packet.optInt("rubles", rubles)

        updateBalance()

        val success = packet.optInt("ok") == 1
        val title = packet.optString("title")
        val text = packet.optString("text")

        if (success) {
            showResultDialog(true, title, text, "ВЫПОЛНЕНО", "ВЕРНУТЬСЯ") {}
        } else {
            // не хватило коинов — вместо возврата предлагаем пополнить счёт
            val short = packet.optInt("need")

            if (short > 0) {
                showResultDialog(false, title, text, "ОШИБКА", "ПОПОЛНИТЬ") { openDeposit(short) }
            } else {
                showResultDialog(false, title, text, "ОШИБКА", "ВЕРНУТЬСЯ") {}
            }
        }

        // остаток штучного товара мог измениться — обновляем плитку
        DonateCatalog.get(packet.optInt("id", -1))?.let { item ->
            item.limit = packet.optInt("n", item.limit)
            tileLayout.refreshItem(item)
        }
    }

    companion object {
        const val LAYOUT_TILE = 0
        const val LAYOUT_SERVICES = 1
        const val LAYOUT_BLACKPASS = 2
        const val LAYOUT_CASES = 3
        const val LAYOUT_DEPOSIT = 4
        const val LAYOUT_PREVIEW_CAR = 5
        const val LAYOUT_PREVIEW_PACK = 6

        const val KEY_TYPE = "t"
        const val BLACKPASS_PACKET = -1

        const val ACTION_REQUEST_STATE = 0
        const val ACTION_BUY = 1
        const val ACTION_DEPOSIT = 2
        const val ACTION_GIFT = 4
        const val ACTION_OPEN_BLACKPASS = 3
        const val ACTION_CASES = 5
        const val ACTION_QUOTE = 6
        const val ACTION_CLOSED = 9

        const val PAY_COINS = 0
        const val PAY_RUBLES = 1

        private const val TILES_ON_SHELF = 4.5f
        private const val CORNER_RADIUS_DP = 7.0f

        fun formatPrice(value: Int): String {
            val text = value.toString()
            val builder = StringBuilder()

            for ((index, ch) in text.withIndex()) {
                if (index > 0 && (text.length - index) % 3 == 0) {
                    builder.append(' ')
                }
                builder.append(ch)
            }

            return builder.toString()
        }

        fun vehicleColor(colorId: Int): Int {
            val rgb = VEHICLE_COLOURS.getOrElse(colorId) { 0x000000 }

            return Color.rgb((rgb shr 16) and 0xFF, (rgb shr 8) and 0xFF, rgb and 0xFF)
        }

        // цвета кузова San Andreas, 0xRRGGBB
        private val VEHICLE_COLOURS = intArrayOf(
            0x000000, 0xF5F5F5, 0x2A77A1, 0x840410, 0x263739, 0x86446E, 0xD78E10, 0x4C75B7,
            0xBDBEC6, 0x5E7072, 0x46597A, 0x656A79, 0x5D7E8D, 0x58595A, 0xD6DAD6, 0x9CA1A3,
            0x335F3F, 0x730E1A, 0x7B0A2A, 0x9F9D94, 0x3B4E78, 0x732E3E, 0x691E3B, 0x96918C,
            0x515459, 0x3F3E45, 0xA5A9A7, 0x635C5A, 0x3D4A68, 0x979592, 0x421F21, 0x5F272B,
            0x8494AB, 0x767B7C, 0x646464, 0x5A5752, 0x252527, 0x2D3A35, 0x93A396, 0x6D7A88,
            0x221918, 0x6F675F, 0x7C1C2A, 0x5F0A15, 0x193826, 0x5D1B20, 0x9D9872, 0x7A7560,
            0x989586, 0xADB0B0, 0x848988, 0x304F45, 0x4D6268, 0x162248, 0x272F4B, 0x7D6256,
            0x9EA4AB, 0x9C8D71, 0x6D1822, 0x4E6881, 0x9C9C98, 0x917347, 0x661C26, 0x949D9F,
            0xA4A7A5, 0x8E8C46, 0x341A1E, 0x6A7A8C, 0xAAAD8E, 0xAB988F, 0x851F2E, 0x6F8297,
            0x585853, 0x9AA790, 0x601A23, 0x20202C, 0xA4A096, 0xAA9D84, 0x78222B, 0x0E316D,
            0x722A3F, 0x7B715E, 0x741D28, 0x1E2E32, 0x4D322F, 0x7C1B44, 0x2E5B20, 0x395A83,
            0x6D2837, 0xA7A28F, 0xAFB1B1, 0x364155, 0x6D6C6E, 0x0F6A89, 0x204B6B, 0x2B3E57,
            0x9B9F9D, 0x6C8495, 0x4D8495, 0xAE9B7F, 0x406C8F, 0x1F253B, 0xAB9276, 0x134573,
            0x96816C, 0x64686A, 0x105082, 0xA19983, 0x385694, 0x525661, 0x7F6956, 0x8C929A,
            0x596E87, 0x473532, 0x44624F, 0x730A27, 0x223457, 0x640D1B, 0xA3ADC6, 0x695853,
            0x9B8B80, 0x620B1C, 0x5B5D5E, 0x624428, 0x731827, 0x1B376D, 0xEC6AAE, 0x000000
        )
    }
}
