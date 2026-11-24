package com.wn.blackrussia.game.ui.donate

import android.graphics.Color
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import androidx.core.content.res.ResourcesCompat
import androidx.recyclerview.widget.RecyclerView
import com.wn.blackrussia.R
import com.wn.blackrussia.game.Multiplayer

class DonateTab(val title: String, val id: Int)

// Ряды вкладок сверху и снизу устроены одинаково: подсвеченная кнопка и вызов
// наружу при смене. Разница только в наборе вкладок.
class DonateTabAdapter(
    private val tabs: List<DonateTab>,
    private val textSizeSp: Float,
    private val onSelected: (Int) -> Unit
) : RecyclerView.Adapter<DonateTabAdapter.Holder>() {

    private var selected = 0

    class Holder(val button: Button) : RecyclerView.ViewHolder(button)

    val selectedTabId: Int
        get() = tabs.getOrNull(selected)?.id ?: -1

    fun select(index: Int) {
        if (index !in tabs.indices || index == selected) {
            return
        }

        val previous = selected
        selected = index

        if (previous in tabs.indices) {
            notifyItemChanged(previous)
        }
        notifyItemChanged(selected)

        onSelected(tabs[selected].id)
    }

    fun selectById(id: Int) {
        val index = tabs.indexOfFirst { it.id == id }
        if (index >= 0) {
            select(index)
        }
    }

    fun deselect() {
        val previous = selected
        selected = -1

        if (previous in tabs.indices) {
            notifyItemChanged(previous)
        }
    }

    fun next() {
        if (selected < 0) return
        select(if (selected + 1 >= tabs.size) 0 else selected + 1)
    }

    fun previous() {
        if (selected < 0) return
        select(if (selected - 1 < 0) tabs.size - 1 else selected - 1)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Holder {
        val button = Button(Multiplayer.getInstance())

        button.setBackgroundColor(Color.TRANSPARENT)
        button.typeface = ResourcesCompat.getFont(Multiplayer.getInstance(), R.font.muller_bold)
        button.setTextSize(android.util.TypedValue.COMPLEX_UNIT_SP, textSizeSp)
        button.layoutParams = ViewGroup.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT
        )

        return Holder(button)
    }

    override fun getItemCount() = tabs.size

    override fun onBindViewHolder(holder: Holder, position: Int) {
        val tab = tabs[position]

        holder.button.text = tab.title
        holder.button.setOnClickListener { select(position) }

        if (position == selected) {
            holder.button.setTextColor(COLOR_SELECTED_TEXT)
            holder.button.setBackgroundColor(COLOR_SELECTED_BACKGROUND)
        } else {
            holder.button.setTextColor(COLOR_TEXT)
            holder.button.setBackgroundColor(Color.TRANSPARENT)
        }
    }

    private companion object {
        val COLOR_SELECTED_TEXT = Color.parseColor("#FFFFFF")
        val COLOR_SELECTED_BACKGROUND = Color.parseColor("#E2310C")
        val COLOR_TEXT = Color.parseColor("#6D6D6D")
    }
}
