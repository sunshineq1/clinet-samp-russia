package com.wn.blackrussia.game.ui.util;

import android.app.Activity;
import android.content.Context;
import android.os.Build;
import android.view.Display;
import android.view.Window;
import android.view.WindowManager;

public class FrameRate {

    private final Context context;

    public FrameRate(Context context) {
        this.context = context;
    }

    private Display getDisplay() {
        WindowManager windowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        if (windowManager == null) {
            return null;
        }
        return windowManager.getDefaultDisplay();
    }

    /**
     * Максимальная частота, которую вообще умеет экран, а не текущая.
     * getRefreshRate() возвращает режим, активный прямо сейчас — на многих
     * устройствах это 60 даже при 120-герцовой матрице.
     */
    public int getMaxRefreshRate() {
        Display display = getDisplay();
        if (display == null) {
            return 60;
        }

        float best = display.getRefreshRate();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            Display.Mode[] modes = display.getSupportedModes();
            if (modes != null) {
                Display.Mode current = display.getMode();

                for (Display.Mode mode : modes) {
                    // Только режимы с текущим разрешением: смена разрешения на лету
                    // ломает разметку интерфейса.
                    if (current != null
                            && (mode.getPhysicalWidth() != current.getPhysicalWidth()
                            || mode.getPhysicalHeight() != current.getPhysicalHeight())) {
                        continue;
                    }

                    if (mode.getRefreshRate() > best) {
                        best = mode.getRefreshRate();
                    }
                }
            }
        }

        int result = Math.round(best);
        if (result < 30) {
            result = 60;
        }

        return result;
    }

    /**
     * Просит систему переключиться на самый быстрый режим экрана.
     * Без этого Android держит 60 Гц, даже если панель умеет больше.
     */
    public void requestMaxRefreshRate(Activity activity) {
        if (activity == null || Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            return;
        }

        Display display = getDisplay();
        if (display == null) {
            return;
        }

        Display.Mode current = display.getMode();
        Display.Mode[] modes = display.getSupportedModes();
        if (current == null || modes == null) {
            return;
        }

        Display.Mode best = current;
        for (Display.Mode mode : modes) {
            if (mode.getPhysicalWidth() != current.getPhysicalWidth()
                    || mode.getPhysicalHeight() != current.getPhysicalHeight()) {
                continue;
            }
            if (mode.getRefreshRate() > best.getRefreshRate()) {
                best = mode;
            }
        }

        if (best.getModeId() == current.getModeId()) {
            return;
        }

        Window window = activity.getWindow();
        if (window == null) {
            return;
        }

        WindowManager.LayoutParams params = window.getAttributes();
        params.preferredDisplayModeId = best.getModeId();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            params.preferredRefreshRate = best.getRefreshRate();
        }

        window.setAttributes(params);
    }
}
