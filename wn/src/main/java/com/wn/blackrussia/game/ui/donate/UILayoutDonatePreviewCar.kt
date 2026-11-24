package com.wn.blackrussia.game.ui.donate

import android.graphics.Paint
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.ImageView
import android.widget.ProgressBar
import android.widget.TextView
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.wn.blackrussia.R
import com.wn.blackrussia.game.Multiplayer
import com.wn.blackrussia.game.ui.util.EntitySnaps
import com.wn.blackrussia.game.ui.util.SnapShot
import com.wn.blackrussia.game.ui.util.UILayout

class UILayoutDonatePreviewCar(private val screen: BrDonate) : UILayout() {

    private var root: View? = null

    private lateinit var name: TextView
    private lateinit var image: ImageView
    private lateinit var priceStroked: TextView
    private lateinit var buyButton: Button
    private lateinit var colors: RecyclerView

    private lateinit var specs: List<SpecView>

    private val adapter = ColorAdapter()

    private var item: DonateItem? = null

    private class SpecView(val text: TextView, val progress: ProgressBar, val suffix: String)

    override fun getView(): View? = root

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup): View {
        val view = inflater.inflate(R.layout.donate_car_preview, null, false)
        root = view

        CarColorCatalog.load()

        name = view.findViewById(R.id.donateCarPreviewName)
        image = view.findViewById(R.id.donateCarPreviewImage)
        priceStroked = view.findViewById(R.id.priceStroked)
        buyButton = view.findViewById(R.id.button)

        view.findViewById<Button>(R.id.donateButtonBack).setOnClickListener { screen.goBackFromDetail() }

        specs = listOf(
            SpecView(view.findViewById(R.id.text_max), view.findViewById(R.id.progress_max), " км/ч"),
            SpecView(view.findViewById(R.id.text_0100), view.findViewById(R.id.progress_0100), " сек"),
            SpecView(view.findViewById(R.id.text_capacity), view.findViewById(R.id.progress_capacity), " чел."),
            SpecView(view.findViewById(R.id.text_boot), view.findViewById(R.id.progress_boot), " кг")
        )

        colors = view.findViewById(R.id.colorSelectRV)
        colors.layoutManager = LinearLayoutManager(Multiplayer.getInstance())
        colors.adapter = adapter

        buyButton.setOnClickListener {
            item?.let {
                screen.requestPurchase(
                    it,
                    payWithRubles = false,
                    extra = adapter.selectedGameColor,
                    price = it.price + adapter.colorPrice
                )
            }
        }

        return view
    }

    override fun onLayoutShown() {
        bind(screen.selectedItem)
    }

    override fun onLayoutClose() {}

    fun bind(target: DonateItem?) {
        item = target

        if (root == null || target == null) {
            return
        }

        adapter.resetColor()
        updatePrice()

        name.text = target.header + " " + target.subheader

        for ((index, spec) in specs.withIndex()) {
            val value = target.specs.getOrElse(index) { 0 }

            spec.progress.progress = value
            spec.text.text = value.toString() + spec.suffix
        }

        loadSnapshot(adapter.selectedGameColor)
    }

    // Цена складывается из цены машины и надбавки за цвет
    private fun updatePrice() {
        val target = item ?: return

        val extra = adapter.colorPrice
        val full = BrDonate.formatPrice(target.basePrice + extra)

        if (target.salePercent != 0) {
            priceStroked.paintFlags = priceStroked.paintFlags or Paint.STRIKE_THRU_TEXT_FLAG
            priceStroked.text = full + " BC"

            buyButton.text = BrDonate.formatPrice(target.price + extra) + " BC"
            return
        }

        buyButton.text = full + " BC"
        priceStroked.text = ""
    }

    private fun loadSnapshot(gameColor: Int) {
        val target = item ?: return

        EntitySnaps.load(
            SnapShot(
                type = EntitySnaps.TYPE_VEHICLE,
                modelId = target.gameId,
                rotX = 20.0f, rotY = 180.0f, rotZ = 45.0f, zoom = 0.78f,
                color1 = BrDonate.vehicleColor(gameColor),
                color2 = BrDonate.vehicleColor(gameColor)
            ),
            image
        )
    }

    private inner class ColorAdapter : RecyclerView.Adapter<ColorAdapter.Holder>() {

        private var selected = 0

        inner class Holder(view: View) : RecyclerView.ViewHolder(view) {
            val swatch: View = view.findViewById(R.id.color)
            val border: Button = view.findViewById(R.id.border)
            val paid: TextView = view.findViewById(R.id.paid)
            val free: TextView = view.findViewById(R.id.free)
        }

        val selectedGameColor: Int
            get() = CarColorCatalog.colors.getOrNull(selected)?.gameColor ?: 0

        // Первый цвет идёт даром, остальные — процент от цены машины плюс пятёрка
        val colorPrice: Int
            get() = if (selected == 0) 0 else ((item?.basePrice ?: 0) * 0.01f + 5.0f).toInt()

        fun resetColor() {
            CarColorCatalog.colors.getOrNull(selected)?.selected = false
            notifyItemChanged(selected)

            selected = 0

            CarColorCatalog.colors.firstOrNull()?.selected = true
            notifyItemChanged(selected)
        }

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Holder {
            val view = LayoutInflater.from(parent.context)
                .inflate(R.layout.br_donate_color_recycler, parent, false)
            return Holder(view)
        }

        override fun getItemCount() = CarColorCatalog.colors.size

        override fun onBindViewHolder(holder: Holder, position: Int) {
            val color = CarColorCatalog.colors[position]

            if (color.selected) {
                if (position == 0) {
                    holder.free.visibility = View.VISIBLE
                } else {
                    holder.free.visibility = View.INVISIBLE
                    holder.paid.visibility = View.VISIBLE
                }

                holder.border.setBackgroundResource(R.drawable.border_button)
            } else {
                holder.paid.visibility = View.INVISIBLE
                holder.free.visibility = View.INVISIBLE

                holder.border.setBackgroundResource(R.drawable.border_button_gray)
            }

            holder.paid.text = "+ $colorPrice BC"
            holder.swatch.setBackgroundColor(color.color)

            holder.border.setOnClickListener {
                CarColorCatalog.colors[selected].selected = false
                notifyItemChanged(selected)

                color.selected = true
                selected = position
                notifyItemChanged(position)

                loadSnapshot(color.gameColor)
                updatePrice()
            }
        }
    }
}
