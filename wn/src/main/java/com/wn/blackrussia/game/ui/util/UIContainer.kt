package com.wn.blackrussia.game.ui.util

import android.content.Context
import android.util.AttributeSet
import android.view.LayoutInflater
import android.view.ViewGroup
import android.widget.FrameLayout

class UIContainer @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : FrameLayout(context, attrs, defStyleAttr) {

    private val layouts = ArrayList<UILayout?>()
    private val backStack = ArrayList<Int>()

    var currentLayoutId = NO_LAYOUT
        private set

    // Экран мог быть заявлен отложенным: тогда разметки ещё нет и показывать
    // нечего, пока её не создали.
    private fun ensureView(layout: UILayout) {
        if (layout.getView() == null) {
            layout.onCreateView(
                context.getSystemService(Context.LAYOUT_INFLATER_SERVICE) as LayoutInflater,
                this
            )
        }
    }

    private fun showLayout(id: Int) {
        if (currentLayoutId == id) {
            layouts.getOrNull(id)?.let {
                ensureView(it)
                it.onLayoutShown()
            }
            return
        }

        if (currentLayoutId != NO_LAYOUT) {
            layouts.getOrNull(currentLayoutId)?.onLayoutClose()
        }

        removeAllViews()

        if (id == NO_LAYOUT) {
            return
        }

        val layout = layouts.getOrNull(id) ?: return

        ensureView(layout)

        layout.getView()?.let { view ->
            (view.parent as? ViewGroup)?.removeView(view)
            addView(view)
        }

        layout.onLayoutShown()
    }

    private fun place(id: Int, layout: UILayout) {
        while (layouts.size <= id) {
            layouts.add(null)
        }

        layouts[id] = layout
        layout.setParent(this)
    }

    fun getLayout(id: Int): UILayout? = layouts.getOrNull(id)

    fun addLayout(id: Int, layout: UILayout) {
        place(id, layout)
        ensureView(layout)
    }

    // экран создастся при первом показе — так открытие доната не тормозит
    fun addLazyLayout(id: Int, layout: UILayout) {
        place(id, layout)
    }

    fun setCurrentLayout(id: Int) {
        if (currentLayoutId != NO_LAYOUT) {
            backStack.add(currentLayoutId)
        }

        showLayout(id)
        currentLayoutId = id
    }

    fun replaceCurrentLayout(id: Int) {
        showLayout(id)
        currentLayoutId = id
    }

    fun closeCurrentLayout(): Boolean {
        if (currentLayoutId == NO_LAYOUT || backStack.isEmpty()) {
            return false
        }

        val previous = backStack.removeAt(backStack.lastIndex)
        currentLayoutId = previous
        showLayout(previous)

        return true
    }

    fun clearBackStack() {
        backStack.clear()
    }

    override fun onDetachedFromWindow() {
        super.onDetachedFromWindow()

        if (currentLayoutId != NO_LAYOUT) {
            layouts.getOrNull(currentLayoutId)?.onLayoutClose()
        }
    }

    companion object {
        const val NO_LAYOUT = -1
    }
}
