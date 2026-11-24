package com.wn.blackrussia

import android.content.Context
import android.graphics.Rect
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import androidx.recyclerview.widget.RecyclerView

class PercentRecyclerView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : RecyclerView(context, attrs, defStyleAttr) {

    private val itemAttributes: PercentAttributes
    var isTouchable = true

    override fun onTouchEvent(event: MotionEvent): Boolean {
        return if (isTouchable) super.onTouchEvent(event) else false
    }
    override fun onInterceptTouchEvent(event: MotionEvent): Boolean {
        return !isTouchable || super.onInterceptTouchEvent(event)
    }

    init {
        val typedArray = context.obtainStyledAttributes(attrs, R.styleable.PercentRecyclerView)
        itemAttributes = PercentAttributes(
            itemWidthPercent = typedArray.getFloat(R.styleable.PercentRecyclerView_itemWidthPercent, 0f),
            itemHeightPercent = typedArray.getFloat(R.styleable.PercentRecyclerView_itemHeightPercent, 0f),
            itemHeightMultiplier = typedArray.getFloat(R.styleable.PercentRecyclerView_itemHeightMultiplier, 0f),
            marginTopPercent = typedArray.getFloat(R.styleable.PercentRecyclerView_marginTopPercent, 0f),
            marginBottomPercent = typedArray.getFloat(R.styleable.PercentRecyclerView_marginBottomPercent, 0f),
            marginLeftPercent = typedArray.getFloat(R.styleable.PercentRecyclerView_marginLeftPercent, 0f),
            marginRightPercent = typedArray.getFloat(R.styleable.PercentRecyclerView_marginRightPercent, 0f)
        )
        typedArray.recycle()

        addItemDecoration(PercentItemDecoration(itemAttributes))
    }

    data class PercentAttributes(
        val itemWidthPercent: Float,
        val itemHeightPercent: Float,
        val itemHeightMultiplier: Float,
        val marginTopPercent: Float,
        val marginBottomPercent: Float,
        val marginLeftPercent: Float,
        val marginRightPercent: Float
    )

    class PercentItemDecoration(
        private val attributes: PercentAttributes
    ) : ItemDecoration() {

        override fun getItemOffsets(outRect: Rect, view: View, parent: RecyclerView, state: RecyclerView.State) {
            val totalWidth = parent.width
            val totalHeight = parent.height

            val itemWidth = (totalWidth * attributes.itemWidthPercent).toInt()
            val itemHeight = if (attributes.itemHeightMultiplier != 0f) {
                (itemWidth * attributes.itemHeightMultiplier).toInt()
            } else {
                (totalHeight * attributes.itemHeightPercent).toInt()
            }

            val marginTop = (totalHeight * attributes.marginTopPercent).toInt()
            val marginBottom = (totalHeight * attributes.marginBottomPercent).toInt()
            val marginLeft = (totalWidth * attributes.marginLeftPercent).toInt()
            val marginRight = (totalWidth * attributes.marginRightPercent).toInt()

            view.layoutParams = view.layoutParams.apply {
                width = itemWidth
                height = itemHeight
            }

            outRect.set(marginLeft, marginTop, marginRight, marginBottom)
        }
    }
}

