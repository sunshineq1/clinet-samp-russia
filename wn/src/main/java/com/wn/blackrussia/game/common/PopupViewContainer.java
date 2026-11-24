package com.wn.blackrussia.game.common;

import android.content.Context;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.widget.FrameLayout;

public class PopupViewContainer extends FrameLayout {
    public PopupViewContainer(Context context) {
        super(context);
    }

    public PopupViewContainer(Context context, AttributeSet attributeSet) {
        super(context, attributeSet);
    }

    public PopupViewContainer(Context context, AttributeSet attributeSet, int i) {
        super(context, attributeSet, i);
    }

    public boolean onKeyPreIme(int i, KeyEvent keyEvent) {
        if (i == 4) {
            return true;
        }
        return super.onKeyPreIme(i, keyEvent);
    }
}
