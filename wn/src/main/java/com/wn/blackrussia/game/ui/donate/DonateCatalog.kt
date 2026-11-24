package com.wn.blackrussia.game.ui.donate

import com.wn.blackrussia.game.ui.util.AssetsJson
import com.wn.blackrussia.game.ui.util.EntitySnaps
import org.json.JSONObject

class DonateItem(json: JSONObject) {
    val internalId = json.optInt("internalId", -1)
    val gameId = json.optInt("gameId", 0)
    val modelId = json.optInt("modelId", 0)

    val header: String = json.optString("header")
    val subheader: String = json.optString("subheader")
    val description: String = json.optString("desc")

    val imageId: String = json.optString("imageId")

    val basePrice = json.optInt("basePrice")
    val count = json.optInt("count", 1)
    val type = json.optInt("type", TYPE_UNKNOWN)

    val isNew = json.optInt("isNew")
    val money = json.optInt("money")

    val modelZoom = json.optDouble("modelZoom", 1.0).toFloat()

    // Признак валюты, а не цена: помеченный товар стоит донат-рубли, остальные —
    // Black Coins. Сумма в обоих случаях лежит в basePrice.
    val rubles = json.optInt("rubles") == 1

    val contains: List<Int> = json.optJSONArray("contains").toIntList()
    val values: List<Int> = json.optJSONArray("values").toIntList()
    val colors: List<Int> = json.optJSONArray("colors").toIntList()
    val specs: List<Int> = json.optJSONArray("specs").toIntList()
    val rotation: List<Float> = json.optJSONArray("rot").toFloatList()

    var category = -1
        internal set

    var salePercent = 0
    var saleTime = 0

    // метка над плиткой: 1 — лидер продаж, 2 — популярно
    var upTag = 0

    var limit = NO_LIMIT

    val price: Int
        get() = if (salePercent != 0) (basePrice * (1.0f - salePercent / 100.0f)).toInt() else basePrice

    val isCar: Boolean
        get() = type == TYPE_CAR

    val isAccessory: Boolean
        get() = type == TYPE_ACCESSORY

    val isPack: Boolean
        get() = type == TYPE_PACK || type == TYPE_VIP_PACK

    val previewName: String
        get() = if (subheader.isEmpty()) header else "$header $subheader"

    companion object {
        const val NO_LIMIT = -1

        const val TYPE_UNKNOWN = -1
        const val TYPE_CATEGORY = 1
        const val TYPE_SKIN = 2
        const val TYPE_PACK = 3
        const val TYPE_VIP_PACK = 4
        const val TYPE_SERVICE = 5
        const val TYPE_MONEY = 6
        const val TYPE_CAR = 7
        const val TYPE_VIP_DURATION = 9
        const val TYPE_BANNER = 10
        const val TYPE_ACCESSORY = 11
    }
}

private fun org.json.JSONArray?.toIntList(): List<Int> {
    if (this == null) return emptyList()
    return (0 until length()).map { optInt(it) }
}

private fun org.json.JSONArray?.toFloatList(): List<Float> {
    if (this == null) return emptyList()
    return (0 until length()).map { optDouble(it).toFloat() }
}

// Витрина доната целиком лежит в assets: сервер шлёт только цены, остатки и
// баланс, а тексты с картинками одинаковы для всех и грузятся один раз.
object DonateCatalog {
    private const val ASSET = "json/donate-items.json"

    private val byId = LinkedHashMap<Int, DonateItem>()

    private val pages = HashMap<Int, MutableList<DonateItem>>()

    val categories = ArrayList<DonateItem>()

    private val missing = HashSet<Int>()

    // Порядок акций задаёт сервер: от места на полке зависит подпись плитки
    private val salesOrder = ArrayList<DonateItem>()

    var loaded = false
        private set

    fun load() {
        if (loaded) return

        val root = AssetsJson.read(ASSET) ?: return
        val array = root.optJSONArray("array") ?: return

        for (i in 0 until array.length()) {
            val json = array.optJSONObject(i) ?: continue
            val item = DonateItem(json)

            if (item.internalId < 0) continue

            byId[item.internalId] = item

            if (item.type == DonateItem.TYPE_CATEGORY) {
                categories += item
            }
        }

        for (category in categories) {
            for (id in category.contains) {
                byId[id]?.category = category.internalId
            }
        }

        buildPages()

        loaded = true
    }

    private fun buildPages() {
        pages.clear()

        for (page in ALL_PAGES) {
            pages[page] = ArrayList()
        }

        for (item in byId.values) {
            val page = when (item.type) {
                DonateItem.TYPE_CAR -> PAGE_CARS
                DonateItem.TYPE_SKIN -> PAGE_SKINS
                DonateItem.TYPE_PACK -> PAGE_PACKS
                DonateItem.TYPE_VIP_PACK -> PAGE_VIP
                DonateItem.TYPE_MONEY -> PAGE_MONEY
                DonateItem.TYPE_ACCESSORY -> PAGE_ACCESSORIES
                DonateItem.TYPE_SERVICE -> PAGE_SERVICES
                DonateItem.TYPE_BANNER -> PAGE_ADVANTAGES
                else -> continue
            }

            pages[page]?.add(item)
        }
    }

    fun get(internalId: Int): DonateItem? = byId[internalId]

    fun page(page: Int): List<DonateItem> = pages[page].orEmpty()

    fun sales(): List<DonateItem> = salesOrder

    fun addSale(item: DonateItem) {
        if (item !in salesOrder) {
            salesOrder += item
        }
    }

    fun contentsOf(item: DonateItem): List<DonateItem> =
        item.contains.mapNotNull { byId[it] }.filter { it.internalId !in missing }

    // Транспорт сначала показывает классы (Эконом, Средний, Премиум и прочие),
    // и только внутри класса — сами машины.
    fun carClasses(): List<DonateItem> = categories

    fun resetOffers() {
        salesOrder.clear()

        for (item in byId.values) {
            item.salePercent = 0
            item.saleTime = 0
            item.upTag = 0
            item.limit = DonateItem.NO_LIMIT
        }
    }

    const val PAGE_SALES = 0
    const val PAGE_CARS = 1
    const val PAGE_SKINS = 2
    const val PAGE_PACKS = 3
    const val PAGE_VIP = 4
    const val PAGE_SERVICES = 5
    const val PAGE_MONEY = 6
    const val PAGE_CAR_DETAIL = 7
    const val PAGE_PACK_DETAIL = 8
    const val PAGE_VIP_DETAIL = 9
    const val PAGE_ACCESSORIES = 11

    // Преимущества пополнения, список на экране покупки коинов
    const val PAGE_ADVANTAGES = 10

    private val ALL_PAGES = listOf(
        PAGE_SALES, PAGE_CARS, PAGE_SKINS, PAGE_PACKS,
        PAGE_VIP, PAGE_SERVICES, PAGE_MONEY, PAGE_ACCESSORIES, PAGE_ADVANTAGES
    )
}
