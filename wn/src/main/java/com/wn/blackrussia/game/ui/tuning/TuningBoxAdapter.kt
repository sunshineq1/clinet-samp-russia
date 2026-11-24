package com.wn.blackrussia.game.ui.tuning

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.wn.blackrussia.R

class TuningBoxAdapter(private val items: List<TuningBoxItem>) :
    RecyclerView.Adapter<TuningBoxAdapter.BoxHolder>() {

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int) =
        BoxHolder(LayoutInflater.from(parent.context).inflate(R.layout.tuning_submenu_box_item, parent, false))

    override fun onBindViewHolder(holder: BoxHolder, position: Int) {
        holder.name.text = items[position].name
    }

    override fun getItemCount() = items.size

    class BoxHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        val name: TextView = itemView.findViewById(R.id.text_box_item)
    }
}
