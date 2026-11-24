package com.wn.blackrussia.game.ui.donate

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.RelativeLayout
import android.widget.TextView
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.wn.blackrussia.R
import com.wn.blackrussia.game.Multiplayer
import com.wn.blackrussia.game.ui.util.UILayout

// Услуга в предпросмотре собирается тем же кодом, что и плитка витрины
private const val PREVIEW_POSITION = 4
private const val HEADER_TOP_MARGIN_DP = 20

class UILayoutDonateServices(private val screen: BrDonate) : UILayout() {

    private var root: View? = null

    private lateinit var list: RecyclerView

    private lateinit var tileHolder: DonateTileAdapter.Holder
    private lateinit var tileBinder: DonateTileAdapter

    private val adapter = ServiceAdapter()

    private var services: List<DonateItem> = emptyList()
    private var selected: DonateItem? = null

    override fun getView(): View? = root

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup): View {
        val view = inflater.inflate(R.layout.donate_services, null, false)
        root = view

        val tile = view.findViewById<View>(R.id.tileDonateFrame)

        tileHolder = DonateTileAdapter.Holder(tile)
        tileBinder = DonateTileAdapter(screen, { screen.onTileClicked(it) }, {})

        tileHolder.subHeader.visibility = View.GONE
        tileHolder.button.setCompoundDrawablesWithIntrinsicBounds(R.drawable.basket_icon, 0, 0, 0)

        // Заголовок в предпросмотре опущен ниже, чем на полке
        val density = view.resources.displayMetrics.density

        tileHolder.header.layoutParams =
            (tileHolder.header.layoutParams as RelativeLayout.LayoutParams).apply {
                topMargin = (HEADER_TOP_MARGIN_DP * density).toInt()
            }

        list = view.findViewById(R.id.donateServicesRV)
        list.layoutManager = LinearLayoutManager(Multiplayer.getInstance())
        list.adapter = adapter

        return view
    }

    override fun onLayoutShown() {
        services = DonateCatalog.page(DonateCatalog.PAGE_SERVICES)
        adapter.notifyDataSetChanged()

        showPreview(selected ?: services.firstOrNull())
    }

    // цены услуг приходят с сервера уже после открытия витрины
    fun refresh() {
        if (root == null) {
            return
        }

        adapter.notifyDataSetChanged()
        showPreview(selected)
    }

    override fun onLayoutClose() {}

    private fun showPreview(item: DonateItem?) {
        selected = item

        if (item == null) {
            return
        }

        tileBinder.bindItem(tileHolder, item, PREVIEW_POSITION)
        tileHolder.subHeader.visibility = View.GONE
    }

    // Услуга занимает строку из двух кнопок: слева название, справа цена.
    // Выбранная подсвечивается жёлтым, остальные серые.
    private inner class ServiceAdapter : RecyclerView.Adapter<ServiceAdapter.Holder>() {

        private var selected = -1

        inner class Holder(view: View) : RecyclerView.ViewHolder(view) {
            val left: Button = view.findViewById(R.id.button_left)
            val right: Button = view.findViewById(R.id.button_right)
            val sale: Button = view.findViewById(R.id.sales)
        }

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Holder {
            val view = LayoutInflater.from(parent.context)
                .inflate(R.layout.donate_services_item, parent, false)
            return Holder(view)
        }

        override fun getItemCount() = services.size

        private fun choose(position: Int) {
            val previous = selected
            selected = position

            if (previous >= 0) notifyItemChanged(previous)
            notifyItemChanged(position)

            showPreview(services.getOrNull(position))
        }

        // Первую строку выделяем без оповещения списка: перерисовка изнутри
        // разметки роняет RecyclerView. В оригинале ровно поэтому обработчик
        // вызывают с пустым видом - он пропускает ветку с notifyItemChanged.
        private fun selectFirst() {
            selected = 0
            showPreview(services.firstOrNull())
        }

        override fun onBindViewHolder(holder: Holder, position: Int) {
            val item = services[position]

            holder.left.text = item.header
            holder.right.text = "${item.basePrice} BC"

            if (item.salePercent != 0) {
                holder.sale.visibility = View.VISIBLE
                holder.sale.text = "-${item.salePercent}%"
            } else {
                holder.sale.visibility = View.INVISIBLE
            }

            if (selected == -1) {
                selectFirst()
            }

            val active = position == selected
            val background = if (active) R.drawable.button_yellow_gradient else R.drawable.button_gray
            val color = if (active) COLOR_SELECTED else COLOR_PLAIN

            holder.left.setBackgroundResource(background)
            holder.right.setBackgroundResource(background)
            holder.left.setTextColor(color)
            holder.right.setTextColor(color)

            val select = View.OnClickListener { choose(position) }

            holder.left.setOnClickListener(select)
            holder.right.setOnClickListener(select)
        }
    }

    private companion object {
        val COLOR_SELECTED = android.graphics.Color.parseColor("#20212A")
        val COLOR_PLAIN = android.graphics.Color.parseColor("#FFFFFF")
    }

}
