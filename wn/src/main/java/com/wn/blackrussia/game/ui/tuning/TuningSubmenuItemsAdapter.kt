package com.wn.blackrussia.game.ui.tuning

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.wn.blackrussia.R

class TuningSubmenuItemsAdapter(
    private val root: BrTuning,
    private val items: List<TuneGuiNode>
) : RecyclerView.Adapter<TuningSubmenuItemsAdapter.SubmenuHolder>() {

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int) =
        SubmenuHolder(LayoutInflater.from(parent.context).inflate(R.layout.tuning_submenu_item, parent, false))

    override fun onBindViewHolder(holder: SubmenuHolder, position: Int) {
        val node = items[position]

        holder.name.text = node.name
        holder.icon.setImageDrawable(root.drawableByName(node.imageId))
    }

    override fun getItemCount() = items.size

    inner class SubmenuHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        val icon: ImageView = itemView.findViewById(R.id.icon_submenu_item)
        val name: TextView = itemView.findViewById(R.id.name_submenu_item)

        init {
            itemView.setOnClickListener { view -> root.onSubmenuItemClick(layoutPosition, view) }
        }
    }
}
