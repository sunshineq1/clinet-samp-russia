package com.wn.blackrussia.game.ui.util

import android.content.Context
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup

abstract class UILayout {
    private var parent: UIContainer? = null

    abstract fun getView(): View?

    abstract fun onCreateView(inflater: LayoutInflater, container: ViewGroup): View?

    abstract fun onLayoutShown()

    abstract fun onLayoutClose()

    fun setParent(parent: UIContainer) {
        this.parent = parent
    }

    val context: Context
        get() = parent!!.context
}
