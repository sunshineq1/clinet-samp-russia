package com.wn.blackrussia.game.ui.donate

import android.graphics.Paint
import android.view.Gravity
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.RelativeLayout
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.wn.blackrussia.R
import com.wn.blackrussia.game.Multiplayer
import com.wn.blackrussia.game.ui.util.EntitySnaps
import com.wn.blackrussia.game.ui.util.SnapShot

class DonateTileAdapter(
    private val screen: BrDonate,
    private val onItemClick: (DonateItem) -> Unit,
    private val onGiftClick: () -> Unit
) : RecyclerView.Adapter<DonateTileAdapter.Holder>() {

    private var items: List<DonateItem> = emptyList()

    // Подарок — первая плитка «Акций» со своим видом, товаром он не считается
    var giftSeconds = -1

    fun submit(newItems: List<DonateItem>) {
        items = newItems
        notifyDataSetChanged()
    }

    fun itemAt(position: Int): DonateItem? = items.getOrNull(position)

    fun refreshItem(item: DonateItem) {
        val index = items.indexOf(item)
        if (index >= 0) {
            notifyItemChanged(index + if (hasGift) 1 else 0)
        }
    }

    class Holder(view: View) : RecyclerView.ViewHolder(view) {
        val frame: View = view.findViewById(R.id.tileDonateFrame)
        val button: Button = view.findViewById(R.id.donate_button)

        val layoutSurprise: RelativeLayout = view.findViewById(R.id.linear_layout_surprise)
        val surpriseStatus: TextView = view.findViewById(R.id.surpriseStatus)
        val surpriseTime: TextView = view.findViewById(R.id.donateSupriseTime)
        val layoutDefault: RelativeLayout = view.findViewById(R.id.linear_layout_default)

        val header: TextView = view.findViewById(R.id.brDonateTileHeader)
        val subHeader: TextView = view.findViewById(R.id.brDonateTileSubheader)
        val additionalHeader: TextView = view.findViewById(R.id.brAdditionalDonate)
        val imageView: ImageView = view.findViewById(R.id.brDonateTileImage)

        val price: TextView = view.findViewById(R.id.donatePrice)
        val priceStroked: TextView = view.findViewById(R.id.donatePriceStroked)

        val newBadge: TextView = view.findViewById(R.id.donateBadgeNew)
        val badgeText: TextView = view.findViewById(R.id.donateBadgeText)
        val badgeTime: TextView = view.findViewById(R.id.donateBadgeTime)
        val badgePercent: TextView = view.findViewById(R.id.donateBadgePercent)

        val limitLayout: LinearLayout = view.findViewById(R.id.donateLimitLL)
        val limitCount: TextView = view.findViewById(R.id.donateLimitCount)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Holder =
        Holder(LayoutInflater.from(parent.context).inflate(R.layout.donate_tile, parent, false))

    private val hasGift: Boolean
        get() = giftSeconds >= 0

    override fun getItemCount() = items.size + if (hasGift) 1 else 0

    override fun onBindViewHolder(holder: Holder, position: Int) {
        if (hasGift && position == 0) {
            bindSurprise(holder)
        } else {
            bindItem(holder, items[position - if (hasGift) 1 else 0], position)
        }

        holder.frame.layoutParams = holder.frame.layoutParams.apply { width = screen.itemWidth }
    }

    private fun bindSurprise(holder: Holder) {
        holder.layoutDefault.visibility = View.GONE
        holder.layoutSurprise.visibility = View.VISIBLE

        holder.badgeText.visibility = View.GONE
        holder.badgeTime.visibility = View.GONE
        holder.badgePercent.visibility = View.GONE
        holder.newBadge.visibility = View.GONE
        holder.limitLayout.visibility = View.GONE

        if (giftSeconds <= 0) {
            holder.surpriseTime.text = ""
            holder.surpriseStatus.visibility = View.INVISIBLE

            holder.button.visibility = View.VISIBLE
            holder.button.text = "Получить"
            holder.button.setOnClickListener { onGiftClick() }
            return
        }

        val hours = giftSeconds / 3600
        val minutes = (giftSeconds - hours * 3600) / 60

        holder.surpriseStatus.visibility = View.VISIBLE
        holder.surpriseTime.text = "%02d ч. %02d мин.".format(hours, minutes)

        holder.button.visibility = View.GONE
    }

    // Предпросмотр услуги — это та же плитка витрины, поэтому её собирает
    // ровно эта функция, а не отдельная разметка.
    fun bindItem(holder: Holder, item: DonateItem, position: Int) {
        holder.limitLayout.visibility = View.GONE
        holder.button.alpha = 1.0f

        when (item.isNew) {
            1 -> {
                holder.newBadge.visibility = View.VISIBLE
                holder.newBadge.text = "NEW"
            }

            2 -> holder.newBadge.text = "EVENT"
            else -> holder.newBadge.visibility = View.GONE
        }

        holder.layoutDefault.visibility = View.VISIBLE
        holder.layoutSurprise.visibility = View.GONE

        if (holder.button.visibility == View.GONE) {
            holder.button.visibility = View.VISIBLE
        }

        holder.price.paintFlags = holder.priceStroked.paintFlags and Paint.STRIKE_THRU_TEXT_FLAG.inv()
        holder.price.visibility = View.VISIBLE

        holder.header.text = item.header
        holder.subHeader.text =
            if (item.type == DonateItem.TYPE_SKIN || item.type == DonateItem.TYPE_CAR) item.subheader else ""

        // На полке акций место плитки и задаёт её заголовок, а имя товара
        // уходит в подпись
        if (screen.currentPage == DonateCatalog.PAGE_SALES) {
            SALE_HEADERS[position]?.let { title ->
                holder.header.text = title
                holder.subHeader.text = item.header
            }
        }

        if (item.upTag != 0) {
            holder.additionalHeader.visibility = View.VISIBLE
            holder.additionalHeader.text = if (item.upTag == UP_TAG_POPULAR) "Популярно" else "Лидер продаж"
        } else {
            holder.additionalHeader.visibility = View.GONE
        }

        if (item.saleTime != 0) {
            val hours = item.saleTime / 3600
            val minutes = (item.saleTime - hours * 3600) / 60

            holder.badgeText.visibility = View.VISIBLE
            holder.badgeTime.visibility = View.VISIBLE
            holder.badgeTime.text = "%02d:%02d".format(hours, minutes)
        } else {
            holder.badgeText.visibility = View.GONE
            holder.badgeTime.visibility = View.GONE
        }

        val currency = if (item.rubles) RUBLES_TEXT else COINS_TEXT

        var shownPrice = item.basePrice

        if (item.salePercent != 0) {
            if (item.type == DonateItem.TYPE_SKIN || item.type == DonateItem.TYPE_MONEY ||
                item.type == DonateItem.TYPE_SERVICE || item.type == DonateItem.TYPE_ACCESSORY
            ) {
                holder.priceStroked.gravity = Gravity.CENTER
                holder.price.visibility = View.GONE
            } else {
                holder.priceStroked.gravity = Gravity.NO_GRAVITY
                holder.price.visibility = View.VISIBLE
            }

            holder.badgePercent.visibility = View.VISIBLE
            holder.priceStroked.visibility = View.VISIBLE
            holder.priceStroked.paintFlags = holder.priceStroked.paintFlags or Paint.STRIKE_THRU_TEXT_FLAG
            holder.price.gravity = Gravity.RIGHT

            holder.badgePercent.text = "-${item.salePercent}%"
            holder.priceStroked.text = item.basePrice.toString() + currency

            shownPrice = item.price
            holder.price.text = shownPrice.toString() + currency
        } else {
            holder.badgePercent.visibility = View.GONE
            holder.priceStroked.visibility = View.GONE
            holder.price.gravity = Gravity.CENTER
            holder.price.text = item.basePrice.toString() + currency

            if (item.type == DonateItem.TYPE_SKIN || item.type == DonateItem.TYPE_MONEY ||
                item.type == DonateItem.TYPE_ACCESSORY
            ) {
                holder.priceStroked.visibility = View.GONE
                holder.price.visibility = View.GONE
            }
        }

        holder.button.text = "Смотреть"
        holder.button.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0)

        if (item.type == DonateItem.TYPE_SKIN || item.type == DonateItem.TYPE_MONEY ||
            item.type == DonateItem.TYPE_ACCESSORY
        ) {
            holder.button.text = BrDonate.formatPrice(shownPrice) + currency
            holder.button.setCompoundDrawablesWithIntrinsicBounds(R.drawable.basket_icon_small, 0, 0, 0)
        }

        if (item.type == DonateItem.TYPE_MONEY) {
            holder.subHeader.text = item.subheader
        }

        if (item.type == DonateItem.TYPE_SERVICE) {
            if (item.salePercent != 0) {
                holder.price.text = shownPrice.toString()
                holder.price.paintFlags = holder.priceStroked.paintFlags or Paint.STRIKE_THRU_TEXT_FLAG
            } else {
                holder.price.text = ""
            }

            holder.button.text = BrDonate.formatPrice(item.price) + currency
            holder.button.setCompoundDrawablesWithIntrinsicBounds(R.drawable.basket_icon_small, 0, 0, 0)
        }

        if (item.type == DonateItem.TYPE_CATEGORY) {
            holder.priceStroked.visibility = View.GONE
            holder.price.text = ""
        }

        if (item.type == DonateItem.TYPE_VIP_PACK) {
            holder.priceStroked.visibility = View.GONE
            holder.price.gravity = Gravity.CENTER
            holder.price.visibility = View.VISIBLE
            holder.price.text = "Посмотри что включает в себя набор " + item.header
        }

        bindImage(holder, item)

        val click = View.OnClickListener { view ->
            if (view.alpha < 1.0f) {
                return@OnClickListener
            }

            onItemClick(item)
        }

        holder.button.setOnClickListener(click)
        holder.frame.setOnClickListener { if (holder.button.alpha >= 1.0f) onItemClick(item) }

        if (item.limit != DonateItem.NO_LIMIT) {
            holder.limitLayout.visibility = View.VISIBLE
            holder.limitCount.text = "${item.limit} шт."

            if (item.limit == 0) {
                holder.button.alpha = 0.5f
            }
        }
    }

    private fun bindImage(holder: Holder, item: DonateItem) {
        EntitySnaps.clear(holder.imageView)

        holder.imageView.setImageResource(drawableFor(item.imageId))

        when (item.type) {
            DonateItem.TYPE_ACCESSORY -> EntitySnaps.load(
                SnapShot(
                    type = EntitySnaps.TYPE_OBJECT,
                    modelId = item.modelId,
                    rotX = item.rotation.getOrElse(0) { 0.0f },
                    rotY = item.rotation.getOrElse(1) { 0.0f },
                    rotZ = item.rotation.getOrElse(2) { 0.0f },
                    zoom = item.modelZoom
                ),
                holder.imageView
            )

            DonateItem.TYPE_CAR -> EntitySnaps.load(
                SnapShot(
                    type = EntitySnaps.TYPE_VEHICLE,
                    modelId = item.gameId,
                    rotX = SNAP_ROT_X, rotY = SNAP_ROT_Y, rotZ = SNAP_ROT_Z, zoom = SNAP_ZOOM,
                    color1 = BrDonate.vehicleColor(item.colors.getOrElse(0) { 0 }),
                    color2 = BrDonate.vehicleColor(item.colors.getOrElse(1) { 0 })
                ),
                holder.imageView
            )

            DonateItem.TYPE_SKIN -> EntitySnaps.load(
                SnapShot(
                    type = EntitySnaps.TYPE_PED,
                    modelId = item.gameId,
                    rotX = SNAP_ROT_X, rotY = SNAP_ROT_Y, rotZ = SNAP_ROT_Z, zoom = SNAP_ZOOM
                ),
                holder.imageView
            )
        }
    }

    private fun drawableFor(imageId: String): Int {
        if (imageId.isEmpty()) return R.drawable.back2

        val context = Multiplayer.getInstance()
        val id = context.resources.getIdentifier(imageId, "drawable", context.packageName)

        return if (id != 0) id else R.drawable.back2
    }

    private companion object {
        const val COINS_TEXT = " BC"
        const val RUBLES_TEXT = " ₽"

        const val UP_TAG_POPULAR = 2

        // Витрина всегда снимает модель под одним и тем же ракурсом
        const val SNAP_ROT_X = 20.0f
        const val SNAP_ROT_Y = 180.0f
        const val SNAP_ROT_Z = 45.0f
        const val SNAP_ZOOM = 0.78f

        val SALE_HEADERS = mapOf(1 to "Для вас", 2 to "Акция дня", 3 to "Товар дня")
    }
}
