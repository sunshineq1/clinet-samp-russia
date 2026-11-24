package com.wn.blackrussia.game.common;

import android.content.Context;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

public class CenterItemLayoutManager extends LinearLayoutManager {
    private int itemWidth;
    private int numItems;
    private int parentWidth;

    public CenterItemLayoutManager(Context context, int i, int i2, int i3, int i4) {
        super(context, i, false);
        this.itemWidth = i2;
        this.parentWidth = i3;
        this.numItems = i4;
    }

    public void onItemsChanged(RecyclerView recyclerView) {
        super.onItemsChanged(recyclerView);
        this.numItems = recyclerView.getAdapter().getItemCount();
        this.parentWidth = recyclerView.getWidth();
    }

    public int getPaddingLeft() {
        int i = this.itemWidth * this.numItems;
        int i2 = this.parentWidth;
        if (i >= i2) {
            return super.getPaddingLeft();
        }
        return Math.round((((float) i2) / 2.0f) - (((float) i) / 2.0f));
    }

    public int getPaddingRight() {
        return getPaddingLeft();
    }
}
