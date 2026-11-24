package com.wn.blackrussia.game.ui.donate

import android.graphics.Paint
import android.text.Html
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.ImageView
import android.widget.TextView
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.wn.blackrussia.R
import com.wn.blackrussia.game.Multiplayer
import com.wn.blackrussia.game.ui.util.KUtils
import com.wn.blackrussia.game.ui.util.UILayout

class UILayoutDonatePreviewPack(private val screen: BrDonate) : UILayout() {

    private var root: View? = null

    private lateinit var header: TextView
    private lateinit var image: ImageView
    private lateinit var strokedPrice: TextView
    private lateinit var buyButton: Button
    private lateinit var list: RecyclerView

    private var item: DonateItem? = null
    private var contents: List<DonateItem> = emptyList()

    override fun getView(): View? = root

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup): View {
        val view = inflater.inflate(R.layout.donate_pack_preview, null, false)
        root = view

        header = view.findViewById(R.id.header)
        image = view.findViewById(R.id.image)
        strokedPrice = view.findViewById(R.id.strokedPrice)
        buyButton = view.findViewById(R.id.button)

        view.findViewById<Button>(R.id.donateButtonBack).setOnClickListener { screen.goBackFromDetail() }

        list = view.findViewById(R.id.donatePackRV)
        list.layoutManager = LinearLayoutManager(Multiplayer.getInstance())
        list.adapter = ContentsAdapter()

        buyButton.setOnClickListener {
            item?.let { screen.requestPurchase(it, payWithRubles = false) }
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

        if (target.salePercent != 0) {
            strokedPrice.visibility = View.VISIBLE
            strokedPrice.text = BrDonate.formatPrice(target.basePrice) + " BC"
            strokedPrice.paintFlags = strokedPrice.paintFlags or Paint.STRIKE_THRU_TEXT_FLAG

            buyButton.text = BrDonate.formatPrice(target.price) + " BC"
        } else {
            strokedPrice.visibility = View.GONE
            buyButton.text = BrDonate.formatPrice(target.basePrice) + " BC"
        }

        // У набора заголовок собирается из двух цветов, у VIP берётся описание
        if (target.type == DonateItem.TYPE_PACK) {
            header.text = Html.fromHtml(
                "<font color='#FFFFFF'>Набор</font> <font color='#E2310C'>${target.header}</font>"
            )
        }

        if (target.type == DonateItem.TYPE_VIP_DURATION && target.description.isNotEmpty()) {
            header.text = KUtils.transfromColors(target.description)
        }

        val context = Multiplayer.getInstance()
        val drawable = context.resources.getIdentifier(target.imageId, "drawable", context.packageName)
        image.setImageResource(if (drawable != 0) drawable else R.drawable.back2)

        contents = DonateCatalog.contentsOf(target)
        list.adapter?.notifyDataSetChanged()
    }

    private inner class ContentsAdapter : RecyclerView.Adapter<ContentsAdapter.Holder>() {

        inner class Holder(view: View) : RecyclerView.ViewHolder(view) {
            val left: Button = view.findViewById(R.id.button_left)
            val right: Button = view.findViewById(R.id.button_right)

            init {
                // В составе набора только названия: правая колонка и рамка убраны
                view.findViewById<View>(R.id.frame).visibility = View.GONE
            }
        }

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Holder {
            val view = LayoutInflater.from(parent.context)
                .inflate(R.layout.donate_services_item, parent, false)
            return Holder(view)
        }

        override fun getItemCount() = contents.size

        override fun onBindViewHolder(holder: Holder, position: Int) {
            val entry = contents[position]

            holder.right.visibility = View.GONE
            holder.left.text = if (entry.isCar) "${entry.header} ${entry.subheader}" else entry.header
        }
    }
}
