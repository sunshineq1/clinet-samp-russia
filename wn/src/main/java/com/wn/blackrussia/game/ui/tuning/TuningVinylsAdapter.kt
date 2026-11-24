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

class TuningVinylsAdapter(
    private val mainRoot: BrTuning,
    private val context: Context,
    private val vinyls: List<TuneVinyls>,
    private val onClickVinylItem: OnClickVinylItem,
    private val costVinyl: Int
) : RecyclerView.Adapter<TuningVinylsAdapter.VinylHolder>() {

    fun interface OnClickVinylItem {
        fun click(tuneVinyl: TuneVinyls, position: Int, view: View)
    }

    inner class VinylHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        val bgViewDetail: View = itemView.findViewById(R.id.bg_view_detail)
        val bgHeaderDetail: View = itemView.findViewById(R.id.bg_header_detail)
        val nameDetail: TextView = itemView.findViewById(R.id.name_detail)
        val iconDetail: ImageView = itemView.findViewById(R.id.icon_details)
        val costDetail: TextView = itemView.findViewById(R.id.value_cost)
        val iconCost: ImageView = itemView.findViewById(R.id.icon_value_cost)

        init {
            itemView.setOnClickListener { view ->
                val position = layoutPosition
                val vinyl = vinyls[position]

                vinyl.selected = true
                notifyItemChanged(position)
                onClickVinylItem.click(vinyl, position, view)
            }
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): VinylHolder =
        VinylHolder(LayoutInflater.from(parent.context)
            .inflate(R.layout.tuning_detail_item, parent, false))

    override fun onBindViewHolder(holder: VinylHolder, position: Int) {
        val vinyl = vinyls[position]

        holder.nameDetail.text = "Винил №${position + 1}"

        // Текстуру винила рисует движок. Готовая картинка ставится сразу,
        // остальные проявляются по мере отрисовки.
        val ready = mainRoot.vinylBitmap(position)

        holder.iconDetail.setTag(R.id.snap_model_tag, position)

        if (ready != null && !vinyl.firstRender) {
            holder.iconDetail.setImageBitmap(ready)
            holder.iconDetail.alpha = 1.0f
        } else {
            holder.iconDetail.alpha = 0.0f

            mainRoot.requestVinylTexture(vinyl.nameVinyl, position) { bitmap ->
                // ячейку могли пролистать и переиспользовать под другой винил
                if (holder.iconDetail.getTag(R.id.snap_model_tag) == position) {
                    holder.iconDetail.setImageBitmap(bitmap)
                    holder.iconDetail.alpha = 1.0f
                }
            }

            vinyl.firstRender = false
        }

        if (vinyl.selected) {
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

        if (vinyl.startVinyl) {
            holder.costDetail.setText(R.string.tuning_set_detail)
            holder.iconCost.setImageResource(R.drawable.tuning_icon_check_in_details)
        } else {
            holder.costDetail.text = BrTuning.priceWithSpaces(costVinyl)
            holder.iconCost.setImageResource(R.drawable.tuning_icon_gold_in_details)
        }
    }

    override fun getItemCount(): Int = vinyls.size

    fun setOnlyVinylClickable(position: Int) {
        for (i in vinyls.indices) {
            if (vinyls[i].selected && i != position) {
                vinyls[i].selected = false
                notifyItemChanged(i)
            }
        }
    }
}
