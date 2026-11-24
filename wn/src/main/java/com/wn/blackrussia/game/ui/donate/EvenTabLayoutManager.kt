package com.wn.blackrussia.game.ui.donate

import android.content.Context
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView

// Вкладки доната делят ширину полосы поровну: кнопка объявлена во всю ширину,
// и без этой поправки на экран влезала бы ровно одна.
class EvenTabLayoutManager(context: Context) :
    LinearLayoutManager(context, HORIZONTAL, false) {

    override fun checkLayoutParams(lp: RecyclerView.LayoutParams?): Boolean {
        val count = itemCount

        if (lp != null && count > 0 && width > 0) {
            lp.width = width / count
        }

        return super.checkLayoutParams(lp)
    }
}
