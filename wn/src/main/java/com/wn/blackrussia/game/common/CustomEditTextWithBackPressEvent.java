package com.wn.blackrussia.game.common;

import android.content.Context;
import android.util.AttributeSet;
import android.view.KeyEvent;
import androidx.appcompat.widget.AppCompatEditText;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import java.util.Objects;

public class CustomEditTextWithBackPressEvent extends AppCompatEditText {
    private MyEditTextListener onBackPressListener;
    public interface MyEditTextListener {
        void callback();
    }

    public CustomEditTextWithBackPressEvent(Context context, AttributeSet attributeSet) {
        super(context, attributeSet);
    }

    public void setOnBackPressListener(MyEditTextListener myEditTextListener) {
        this.onBackPressListener = myEditTextListener;
    }

    public boolean onKeyPreIme(int i, KeyEvent keyEvent) {
        if (i != 4 || keyEvent.getAction() != 1) {
            return super.dispatchKeyEvent(keyEvent);
        }
        WindowInsetsCompat rootWindowInsets = ViewCompat.getRootWindowInsets(getRootView());
        Objects.requireNonNull(rootWindowInsets);

        this.onBackPressListener.callback();
        return false;
    }
}
