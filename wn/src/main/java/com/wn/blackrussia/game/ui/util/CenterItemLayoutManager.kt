package com.wn.blackrussia.game.ui.util

import android.content.Context
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView

// Пока плиток мало, они прижимаются к левому краю и полка выглядит пустой —
// менеджер добавляет отступы и держит короткий ряд по центру.
class CenterItemLayoutManager(
    context: Context,
    orientation: Int,
    private val itemWidth: Int,
    private var parentWidth: Int,
    private var itemCount: Int
) : LinearLayoutManager(context, orientation, false) {

    override fun onItemsChanged(recyclerView: RecyclerView) {
        super.onItemsChanged(recyclerView)

        itemCount = recyclerView.adapter?.itemCount ?: 0
        parentWidth = recyclerView.width
    }

    override fun getPaddingLeft(): Int {
        val rowWidth = itemWidth * itemCount

        if (rowWidth >= parentWidth) {
            return super.getPaddingLeft()
        }

        return Math.round(parentWidth / 2.0f - rowWidth / 2.0f)
    }

    override fun getPaddingRight(): Int = paddingLeft
}
