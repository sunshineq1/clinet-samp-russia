package com.wn.blackrussia.game.ui.tuning

import android.content.Context
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.RecyclerView
import com.wn.blackrussia.R

class TuningDetailsAdapter(
    private val mainRoot: BrTuning,
    private val context: Context,
    private val details: List<Tune>,
    private val onClickDetailItem: OnClickDetailItem
) : RecyclerView.Adapter<TuningDetailsAdapter.TuningDetailsHolder>() {

    fun interface OnClickDetailItem {
        fun click(detailsItem: Tune, position: Int, view: View)
    }

    inner class TuningDetailsHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        val bgViewDetail: View = itemView.findViewById(R.id.bg_view_detail)
        val bgHeaderDetail: View = itemView.findViewById(R.id.bg_header_detail)
        val nameDetail: TextView = itemView.findViewById(R.id.name_detail)
        val iconDetail: ImageView = itemView.findViewById(R.id.icon_details)
        val costDetail: TextView = itemView.findViewById(R.id.value_cost)
        val iconCost: ImageView = itemView.findViewById(R.id.icon_value_cost)

        init {
            itemView.setOnClickListener { view ->
                val position = layoutPosition
                val tune = details[position]

                tune.checked = true
                notifyItemChanged(position)
                onClickDetailItem.click(tune, position, view)
            }
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): TuningDetailsHolder =
        TuningDetailsHolder(LayoutInflater.from(parent.context)
            .inflate(R.layout.tuning_detail_item, parent, false))

    override fun onBindViewHolder(holder: TuningDetailsHolder, position: Int) {
        val tune = details[position]

        holder.nameDetail.text = tune.name
        holder.iconDetail.setImageDrawable(mainRoot.drawableByName(tune.imageId))

        if (tune.checked) {
            holder.bgViewDetail.background =
                ContextCompat.getDrawable(context, R.drawable.tuning_bg_detail_check)
            holder.bgHeaderDetail.background =
                ContextCompat.getDrawable(context, R.drawable.tuning_border_detail_check_item)
        } else {
            holder.bgViewDetail.background =
                ContextCompat.getDrawable(context, R.drawable.tuning_bg_detail_not_check)
            holder.bgHeaderDetail.background =
                ContextCompat.getDrawable(context, R.drawable.tuning_border_detail_not_check_item)
        }

        when (tune.thisLocation) {
            TuningConstants.DETAIL_STATUS_CAN_BUY -> {
                holder.costDetail.text = BrTuning.priceWithSpaces(tune.cost)
                holder.iconCost.setImageResource(
                    if (tune.currency == TuningConstants.CURRENCY_RUB_MONEY) R.drawable.tuning_icon_gold_in_details
                    else R.drawable.tuning_icon_black_coin_in_details)
            }

            TuningConstants.DETAIL_STATUS_BOUGHT -> {
                holder.costDetail.setText(R.string.tuning_bought_detail)
                holder.iconCost.setImageResource(R.drawable.tuning_icon_bought_in_details)
            }

            TuningConstants.DETAIL_STATUS_ESTABLISHED -> {
                holder.costDetail.setText(R.string.tuning_set_detail)
                holder.iconCost.setImageResource(R.drawable.tuning_icon_check_in_details)
            }
        }
    }

    override fun getItemCount(): Int = details.size

    fun setOnlyDetailClickable(position: Int) {
        for (i in details.indices) {
            if (details[i].checked && i != position) {
                details[i].checked = false
                notifyItemChanged(i)
            }
        }
    }
}
