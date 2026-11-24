package com.wn.blackrussia.game.common;

import android.app.Dialog;
import android.content.Context;
import androidx.core.view.InputDeviceCompat;

public class DialogNoNavBar extends Dialog {
    public DialogNoNavBar(Context context, int i) {
        super(context, i);
    }

    public DialogNoNavBar(Context context) {
        super(context);
    }

    public void show() {
        getWindow().setFlags(8, 8);
        super.show();
        super.getWindow().getDecorView().setSystemUiVisibility(InputDeviceCompat.SOURCE_TOUCHSCREEN);
        getWindow().clearFlags(8);
    }
}
