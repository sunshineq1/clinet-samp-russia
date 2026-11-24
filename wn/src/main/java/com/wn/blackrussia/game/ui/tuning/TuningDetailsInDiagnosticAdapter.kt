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

class TuningDetailsInDiagnosticAdapter(
    private val mainRoot: BrTuning,
    private val context: Context,
    private val details: List<TuningDetailDiagnosticItem>,
    private val onClickDetailItem: OnClickDetailInDiagnosticItem,
    private val statusDiagnostic: Int
) : RecyclerView.Adapter<TuningDetailsInDiagnosticAdapter.DiagnosticHolder>() {

    fun interface OnClickDetailInDiagnosticItem {
        fun clickInDiagnostic(detailsItem: TuningDetailDiagnosticItem, position: Int, view: View)
    }

    inner class DiagnosticHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        val bgViewDetail: View = itemView.findViewById(R.id.bg_view_detail_in_diagnostic)
        val bgHeaderDetail: View = itemView.findViewById(R.id.bg_header_detail_in_diagnostic)
        val nameViewDetail: TextView = itemView.findViewById(R.id.name_detail_in_diagnostic)
        val valueViewState: TextView = itemView.findViewById(R.id.value_state_detail_in_diagnostic)
        val valueViewCost: TextView = itemView.findViewById(R.id.value_cost_detail_in_diagnostic)
        val iconState: ImageView = itemView.findViewById(R.id.icon_state_detail_in_diagnostic)
        val iconCost: ImageView = itemView.findViewById(R.id.icon_detail_in_diagnostic)
        val iconDetail: ImageView = itemView.findViewById(R.id.icon_details_in_diagnostic)

        init {
            itemView.setOnClickListener { view ->
                val position = layoutPosition
                val item = details[position]

                item.checked = true
                notifyItemChanged(position)
                onClickDetailItem.clickInDiagnostic(item, position, view)
            }
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): DiagnosticHolder =
        DiagnosticHolder(LayoutInflater.from(parent.context)
            .inflate(R.layout.tuning_diagnostic_item, parent, false))

    override fun onBindViewHolder(holder: DiagnosticHolder, position: Int) {
        val item = details[position]

        holder.iconDetail.setImageDrawable(mainRoot.drawableByName(item.resName))

        if (item.checked) {
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

        holder.nameViewDetail.text = item.nameDetail
        holder.valueViewState.text = "${item.valueState}%"
        holder.valueViewCost.text = item.cost.toString()
        holder.iconCost.setImageResource(R.drawable.tuning_icon_gold_in_details)

        if (statusDiagnostic == TuningConstants.STATUS_NOT_ACTUAL_DIAGNOSTIC) {
            holder.iconState.setImageResource(R.drawable.tuning_icon_state_diagnostic_noactual)
            holder.valueViewState.setTextColor(ContextCompat.getColor(context, R.color.red_diagnostics))
        } else {
            holder.iconState.setImageResource(R.drawable.tuning_icon_state_diagnostic_actual)
            holder.valueViewState.setTextColor(ContextCompat.getColor(context, R.color.green_check_element))
        }
    }

    override fun getItemCount(): Int = details.size

    fun setOnlyItemClickable(position: Int) {
        for (i in details.indices) {
            if (details[i].checked && i != position) {
                details[i].checked = false
                notifyItemChanged(i)
            }
        }
    }
}
