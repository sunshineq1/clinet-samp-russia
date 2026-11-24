package com.wn.blackrussia.game.common;

import android.annotation.SuppressLint;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.PopupWindow;

import com.wn.blackrussia.game.ui.brguifix.GUIManager;

public class GUIPopupWindow extends PopupWindow {
    GUIManager mGUIManager = null;

    public GUIPopupWindow(View view, int i, int i2, boolean z) {
        super(view, i, i2, z);
    }

    @SuppressLint("WrongConstant")
    public void setDimming(float f) {
        View view;
        if (Build.VERSION.SDK_INT >= 23) {
            view = (View) getContentView().getParent();
        } else {
            view = getContentView();
        }
        if (getBackground() != null) {
            view = (View) view.getParent();
        }
        WindowManager.LayoutParams layoutParams = (WindowManager.LayoutParams) view.getLayoutParams();
        layoutParams.flags |= 2;
        layoutParams.dimAmount = f;
        ((WindowManager) getContentView().getContext().getSystemService("window")).updateViewLayout(view, layoutParams);
    }

    public void showAtLocation(View view, int i, int i2, int i3) {
        View contentView = getContentView();
        ViewGroup.LayoutParams layoutParams = contentView.getLayoutParams();
        int i4 = -2;
        if (layoutParams == null || layoutParams.height != -2) {
            i4 = -1;
        }
        PopupViewContainer popupViewContainer = new PopupViewContainer(contentView.getContext());
        FrameLayout.LayoutParams layoutParams2 = new FrameLayout.LayoutParams(-1, i4);
        popupViewContainer.setBackgroundDrawable((Drawable) null);
        popupViewContainer.addView(contentView, layoutParams2);
        setContentView(popupViewContainer);
        setBackgroundDrawable((Drawable) null);
        getContentView().setFocusableInTouchMode(true);
        setClippingEnabled(false);
        GUIManager.hideSystemUIver2(getContentView());
        getContentView().setOnSystemUiVisibilityChangeListener(new View.OnSystemUiVisibilityChangeListener() {
            public void onSystemUiVisibilityChange(int i) {
                GUIManager.hideSystemUIver2(GUIPopupWindow.this.getContentView());
            }
        });
        super.showAtLocation(view, i, i2, i3);
        extendUnderCutout();
    }

    private void extendUnderCutout() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P) {
            return;
        }
        View root = (View) getContentView().getParent();
        if (root == null || !(root.getLayoutParams() instanceof WindowManager.LayoutParams)) {
            return;
        }
        WindowManager.LayoutParams lp = (WindowManager.LayoutParams) root.getLayoutParams();
        lp.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        ((WindowManager) getContentView().getContext().getSystemService("window"))
                .updateViewLayout(root, lp);
    }
}
