package com.wn.blackrussia.game.ui.brguifix;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.text.Html;
import android.text.Layout;
import android.text.Spanned;
import android.text.StaticLayout;
import android.text.TextPaint;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.PopupWindow;
import android.widget.TextView;

import com.google.firebase.crashlytics.FirebaseCrashlytics;
import com.nvidia.devtech.NvEventQueueActivity;
import com.wn.blackrussia.game.Multiplayer;

import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.Locale;
import java.util.regex.Pattern;

import org.json.JSONObject;

public class GUIManager {
    Activity mActivity;
    private LinkedList<ISAMPGUI> mGUIStack;
    int[] mGUITypes;
    ISAMPGUI[] mGUIs;

    public static String getBCText() {
        return " BC";
    }

    public static String getRublesText() {
        return " ₽";
    }

    private void setGuiType(int id, int type) {
        this.mGUITypes[id] = type;
    }
    public Boolean getGUIs(Integer id) {
        return Boolean.valueOf(this.mGUIs[id.intValue()] != null ? this.mGUIs[id.intValue()].isShowingGui() : false);
    }

    public ISAMPGUI createGuiFromId(int id) {
        switch (id) {
            default:
                return null;
        }
    }
    public GUIManager(Activity activity) {
        this.mActivity = null;
        this.mGUIs = null;
        this.mGUITypes = null;
        this.mGUIStack = null;
        this.mActivity = activity;
        this.mGUIs = new ISAMPGUI[256];
        this.mGUITypes = new int[256];
        this.mGUIStack = new LinkedList<>();
        setGuiType(0, 2);
        setGuiType(1, 2);
        setGuiType(2, 2);
        setGuiType(3, 2);
        setGuiType(4, 2);
        setGuiType(5, 2);
        setGuiType(6, 2);
        setGuiType(8, 2);
        setGuiType(7, 2);
        setGuiType(9, 2);
        setGuiType(12, 2);
        setGuiType(13, 3);
        setGuiType(14, 1);
        setGuiType(15, 2);
        setGuiType(16, 2);
        setGuiType(17, 2);
        setGuiType(18, 2);
        setGuiType(20, 2);
        setGuiType(21, 2);
        setGuiType(23, 2);
        setGuiType(26, 2);
        setGuiType(10, 1);
        setGuiType(11, 2);
        setGuiType(19, 2);
        setGuiType(30, 2);
        setGuiType(31, 2);
        setGuiType(22, 2);
        setGuiType(28, 2);
        setGuiType(25, 2);
        setGuiType(27, 2);
        setGuiType(24, 2);
        setGuiType(33, 2);
        setGuiType(34, 2);
        setGuiType(35, 2);
        setGuiType(36, 2);
        setGuiType(37, 2);
        setGuiType(38, 2);
        setGuiType(39, 2);
        setGuiType(40, 2);
        setGuiType(41, 2);
        setGuiType(42, 2);
        setGuiType(43, 2);
        setGuiType(45, 2);
        setGuiType(46, 2);
        setGuiType(52, 2);
        setGuiType(50, 2);

        setGuiType(53, 2); // .. test
    }

    public void onPacketIncoming(int guiid, JSONObject data) {
        if (guiid >= 256 || guiid < 0) {
            return;
        }

        if (data.optInt("o") == 1) {
            showGUI(guiid, data);
            return;
        }

        if (this.mGUIs[guiid] == null) {
            ISAMPGUI[] isampguiArr = this.mGUIs;
            if (isampguiArr[guiid] != null) {
                isampguiArr[guiid].onPacketIncoming(data);
                return;
            }
            return;
        }

        if (guiid != 10) {
            closeGUI(data, guiid);
        } else {
            closeSAMPWindow(data);
        }
    }

    public boolean isShowingGui() {
        //if (BrNotification.mActiveNotifications > 0) {
        //    return true;
       // }
        if (this.mGUIStack.size() == 0) {
            return false;
        }
        return this.mGUIStack.peek().isShowingGui();
    }

    public void showGUI(int guiid, JSONObject json) {
        if (guiid == 51) {
            this.mActivity.startActivity(new Intent("android.intent.action.VIEW", Uri.parse(json.optString("s"))));
            return;
        }
        if (this.mGUITypes[guiid] == 3) {
            createGuiFromId(guiid).show(json, this, this.mActivity);
            return;
        }
        ISAMPGUI[] isampguiArr = this.mGUIs;
        if (isampguiArr[guiid] == null) {
            this.mGUIs[guiid] = createGuiFromId(guiid);
        } else {
            isampguiArr[guiid].close(null);
            if (this.mGUITypes[guiid] == 2) {
                this.mGUIs[guiid] = null;
                this.mGUIs[guiid] = createGuiFromId(guiid);
            }
        }
        this.mGUIStack.remove(this.mGUIs[guiid]);
        this.mGUIStack.push(this.mGUIs[guiid]);
        this.mGUIs[guiid].show(json, this, this.mActivity);
    }

    public void closeGUI(JSONObject json, int guiId) {
        if (json != null && json.optInt("not") == 1) {
            // BrNotification.closeNotificationById(json.optInt("b"));
            return;
        }
        if (this.mGUIStack.size() != 0) {
            ISAMPGUI[] isampguiArr = this.mGUIs;
            if (isampguiArr[guiId] != null && this.mGUIStack.contains(isampguiArr[guiId]) && this.mGUITypes[guiId] == 2) {
                this.mGUIs[guiId].close(json);
                this.mGUIStack.remove(this.mGUIs[guiId]);
                this.mGUIs[guiId] = null;
            }
        }
    }

    public void sendJsonData(int guiid, JSONObject object) {
        try {
            byte[] bytes = object.toString().getBytes("windows-1251");
            if (bytes == null) {
                return;
            }
            if (this.mActivity instanceof com.wn.blackrussia.game.Multiplayer) {
                ((com.wn.blackrussia.game.Multiplayer) this.mActivity).sendJsonData(guiid, bytes);
            }
        } catch (Exception e) {
            FirebaseCrashlytics.getInstance().recordException(e);
        }
    }

    public void onGUIClosed(int i) {
        /* code */
    }

    public void closeSAMPWindow(JSONObject json) {
        if (this.mGUIStack.size() == 0) {
            return;
        }
        for (int i = 0; i < this.mGUIStack.size(); i++) {
            ISAMPGUI isampgui = this.mGUIStack.get(i);
            if (this.mGUIs[10] == isampgui) {
                isampgui.close(json);
                this.mGUIStack.remove(isampgui);
                return;
            }
        }
    }

    public void closeAllWindows() {
        if (this.mGUIStack.size() == 0) {
            return;
        }
        this.mActivity.runOnUiThread(new Runnable() {
            @Override // java.lang.Runnable
            public void run() {
                JSONObject jSONObject = new JSONObject();
                for (int i = 0; i < GUIManager.this.mGUIStack.size(); i++) {
                    ((ISAMPGUI) GUIManager.this.mGUIStack.get(i)).close(jSONObject);
                }
                GUIManager.this.mGUIStack.clear();
            }
        });
    }

    public void closeAllWindowsExSAMP() {
        if (this.mGUIStack.size() == 0) {
            return;
        }
        this.mActivity.runOnUiThread(new Runnable() {
            @Override // java.lang.Runnable
            public void run() {
                JSONObject jSONObject = new JSONObject();
                for (int i = 0; i < GUIManager.this.mGUIStack.size(); i++) {
                    ISAMPGUI isampgui = (ISAMPGUI) GUIManager.this.mGUIStack.get(i);
                    if (isampgui != GUIManager.this.mGUIs[10]) {
                        isampgui.close(jSONObject);
                    }
                }
                GUIManager.this.mGUIStack.clear();
            }
        });
    }

    public static String getPriceFromInt(float f) {
        DecimalFormatSymbols decimalFormatSymbols = new DecimalFormatSymbols(Locale.getDefault());
        decimalFormatSymbols.setDecimalSeparator(',');
        decimalFormatSymbols.setGroupingSeparator('.');
        return new DecimalFormat("#,###,###", decimalFormatSymbols).format((double) f);
    }

    public static String getPriceFromInt(int i) {
        DecimalFormatSymbols decimalFormatSymbols = new DecimalFormatSymbols(Locale.getDefault());
        decimalFormatSymbols.setDecimalSeparator(',');
        decimalFormatSymbols.setGroupingSeparator('.');
        return new DecimalFormat("#,###,###", decimalFormatSymbols).format((long) i);
    }

    public static String getPriceWithSpaces(int i) {
        return new DecimalFormat("#,###,###", new DecimalFormatSymbols(Locale.getDefault())).format((long) i);
    }

    public static String getPriceWithSpacesForLong(long j) {
        return new DecimalFormat("#,###,###", new DecimalFormatSymbols(Locale.getDefault())).format(j);
    }

    public static void makeAllViewsVisible(ViewGroup viewGroup) {
        viewGroup.setVisibility(View.VISIBLE);
        for (int i = 0; i < viewGroup.getChildCount(); i++) {
            if (viewGroup.getChildAt(i) instanceof ViewGroup) {
                makeAllViewsVisible((ViewGroup) viewGroup.getChildAt(i));
            } else {
                viewGroup.getChildAt(i).setVisibility(View.VISIBLE);
            }
        }
    }

    public static int getTextLength(TextView textView) {
        TextPaint textPaint = new TextPaint();
        textPaint.setTextSize(textView.getTextSize());
        textPaint.setTypeface(textView.getTypeface());
        return (int) textPaint.measureText(textView.getText().toString());
    }

    public static void changeTextViewWidth(final TextView textView) {
        textView.post(new Runnable() {
            public void run() {
                TextPaint textPaint = new TextPaint();
                textPaint.setTextSize(textView.getTextSize());
                textPaint.setTypeface(textView.getTypeface());
                textPaint.measureText(textView.getText().toString());
                StaticLayout staticLayout = new StaticLayout(textView.getText(), textPaint, 10000, Layout.Alignment.ALIGN_NORMAL, 1.0f, 0.0f, false);
                int lineCount = staticLayout.getLineCount();
                float lineWidth = staticLayout.getLineWidth(0);
                for (int i = 0; i < lineCount; i++) {
                    if (staticLayout.getLineWidth(i) >= lineWidth) {
                        lineWidth = staticLayout.getLineWidth(i) + 5.0f;
                    }
                }
                ViewGroup.LayoutParams layoutParams = textView.getLayoutParams();
                layoutParams.width = (int) lineWidth;
                textView.setLayoutParams(layoutParams);
            }
        });
    }

    public GUIManager(Multiplayer nvEventQueueActivity) {
        this.mActivity = nvEventQueueActivity;
        this.mGUITypes = new int[256];
    }

    public static void hideSystemUI(Window window) {
        hideSystemUI(window.getDecorView());
    }

    public static void hideSystemUI(View view) {
        view.setSystemUiVisibility(7942);
    }

    public static void hideSystemUIver2(View view) {
        if (Build.VERSION.SDK_INT > 29) {
            view.setSystemUiVisibility(6150);
        } else {
            view.setSystemUiVisibility(6662);
        }
    }

    public static Spanned transfromColors(String str) {
        int i;
        if (str == null) {
            str = "";
        }
        String r9 = str;
        LinkedList linkedList = new LinkedList();
        int i2 = 0;
        int i3 = 0;
        for (int i4 = 0; i4 < r9.length(); i4++) {
            if (r9.charAt(i4) == '{' && (i = i4 + 7) < r9.length()) {
                StringBuilder sb = new StringBuilder();
                sb.append(new String() + "#");
                int i5 = i4 + 1;
                sb.append(r9.substring(i5, i));
                linkedList.addLast(sb.toString());
                r9 = r9.substring(0, i5) + "repl" + i3 + r9.substring(i, r9.length());
                i3++;
            }
        }
        Iterator it = linkedList.iterator();
        while (it.hasNext()) {
            String str2 = (String) it.next();
            if (i2 == 0) {
                String quote = Pattern.quote(new String() + "{repl" + i2 + "}");
                StringBuilder sb2 = new StringBuilder();
                sb2.append("<font color='");
                sb2.append(str2);
                sb2.append("'>");
                r9 = r9.replaceAll(quote, sb2.toString());
            } else {
                String quote2 = Pattern.quote(new String() + "{repl" + i2 + "}");
                StringBuilder sb3 = new StringBuilder();
                sb3.append("</font><font color='");
                sb3.append(str2);
                sb3.append("'>");
                r9 = r9.replaceAll(quote2, sb3.toString());
            }
            i2++;
        }
        if (linkedList.size() >= 1) {
            r9 = r9 + "</font>";
        }
        return Html.fromHtml(r9.replaceAll(Pattern.quote("\n"), "<br>"));
    }

    public void setCustomTouchInterceptor(PopupWindow popupWindow) {
        popupWindow.setTouchInterceptor(new View.OnTouchListener() {
            public boolean onTouch(View view, MotionEvent motionEvent) {
                if (motionEvent.getAction() != 0) {
                    return false;
                }
                if (motionEvent.getX() < 0.0f || motionEvent.getX() > ((float) view.getWidth()) || motionEvent.getY() < 0.0f || motionEvent.getY() > ((float) view.getHeight())) {
                    return true;
                }
                return false;
            }
        });
    }
}
