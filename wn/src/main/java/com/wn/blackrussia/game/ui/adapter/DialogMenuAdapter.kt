package com.wn.blackrussia.game.ui.adapter

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.wn.blackrussia.R
import com.wn.blackrussia.game.ui.models.DataDialogMenu

class DialogMenuAdapter(
    private val dialogMenuData: List<DataDialogMenu>,
    private val onUserClickListener: OnUserClickListener
) : RecyclerView.Adapter<DialogMenuAdapter.DialogMenuHolder>() {

    interface OnUserClickListener {
        fun click(dataDialogMenu: DataDialogMenu, view: View)
    }

    override fun onCreateViewHolder(viewGroup: ViewGroup, i: Int): DialogMenuHolder {
        return DialogMenuHolder(LayoutInflater.from(viewGroup.context).inflate(R.layout.menu_action_item, viewGroup, false))
    }

    override fun onBindViewHolder(holder: DialogMenuHolder, position: Int) {
        val dataDialogMenu = dialogMenuData[position]
        holder.nameBlockButton.text = dataDialogMenu.nameButton
        holder.imageViewBlockButton.setImageResource(dataDialogMenu.imgDrawableButton)
    }

    override fun getItemCount(): Int = dialogMenuData.size

    inner class DialogMenuHolder(view: View) : RecyclerView.ViewHolder(view) {
        val nameBlockButton: TextView = view.findViewById(R.id.item_menu_name_button)
        val imageViewBlockButton: ImageView = view.findViewById(R.id.item_menu_image)

        init {
            view.setOnClickListener {
                onUserClickListener.click(dialogMenuData[adapterPosition], it)
            }
        }
    }
}
