package com.wn.blackrussia.game.common;

import android.view.View;
import android.widget.PopupWindow;

public class PopupWindowNavBar extends PopupWindow {
    public PopupWindowNavBar(View view, int i, int i2, boolean z) {
        super(view, i, i2, z);
    }

    public void showAtLocation(View view, int i, int i2, int i3) {
        setFocusable(false);
        super.showAtLocation(view, i, i2, i3);
        setFocusable(true);
        update();
    }
}
