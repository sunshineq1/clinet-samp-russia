package com.wn.blackrussia.game.ui.tuning

import android.content.Context
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.RecyclerView
import com.wn.blackrussia.R

class TuningTypeAdapter(
    private val context: Context,
    private val types: List<TuningMenuType>,
    private val onClickTuningType: OnClickTuningType
) : RecyclerView.Adapter<TuningTypeAdapter.TypeHolder>() {

    fun interface OnClickTuningType {
        fun click(tuningType: TuningMenuType, position: Int, view: View)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int) =
        TypeHolder(LayoutInflater.from(parent.context).inflate(R.layout.tuning_type_item, parent, false))

    override fun onBindViewHolder(holder: TypeHolder, position: Int) {
        val type = types[position]

        holder.title.text = type.titleType

        // Скругление есть только у крайних вкладок, поэтому фон у них свой
        holder.background.background = ContextCompat.getDrawable(
            context,
            when (position) {
                0 -> R.drawable.tuning_bg_first_title_type_tuning
                types.size - 1 -> R.drawable.tuning_bg_last_title_type_tuning
                else -> R.drawable.tuning_bg_title_type_tuning
            }
        )

        holder.clickMark.visibility = if (type.clickable) View.VISIBLE else View.INVISIBLE
    }

    override fun getItemCount() = types.size

    // Подсветка должна остаться ровно на одной вкладке, с остальных её снимаем
    fun setOnlyItemClickable(position: Int) {
        for (i in types.indices) {
            if (types[i].clickable && i != position) {
                types[i].clickable = false
                notifyItemChanged(i)
            }
        }
    }

    inner class TypeHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        val title: TextView = itemView.findViewById(R.id.title_type_tuning)
        val background: View = itemView.findViewById(R.id.bg_title_type_tuning)
        val clickMark: View = itemView.findViewById(R.id.bg_click_title)

        init {
            itemView.setOnClickListener { view ->
                val type = types[layoutPosition]

                type.clickable = true
                notifyItemChanged(layoutPosition)
                onClickTuningType.click(type, layoutPosition, view)
            }
        }
    }
}
