package com.wn.blackrussia.game.ui;

import android.app.Activity;
import android.content.Context;
import android.text.Layout;
import android.text.StaticLayout;
import android.text.TextPaint;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.WindowManager;
import android.view.ViewGroup;
import android.view.animation.AnimationUtils;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.PopupWindow;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.core.content.res.ResourcesCompat;

import com.rockstargames.gtacr.common.CustomEditTextWithBackPressEvent;
import com.wn.blackrussia.R;
import com.wn.blackrussia.game.Multiplayer;
import com.wn.blackrussia.game.ui.util.Util;
import com.wn.blackrussia.game.ui.util.Utils;

import java.io.UnsupportedEncodingException;
import java.util.regex.Pattern;
import org.json.JSONObject;

public class BrDialogWindow {
    private Multiplayer mActivity = Multiplayer.getInstance();
    private View mSelectedButton = null;
    private TextView mOrigButton = null;
    private int mClickedButton = -1;
    private CustomEditTextWithBackPressEvent mEditText = null;
    private int mListitemToSend = -1;
    private boolean mIsShow = false;
    private boolean ifOtherMethodClosed = false;
    String okText;
    String contentText;
    String captionText;
    int dialogStyle;
    String cancelText;
    Button button;
    boolean withMargin;
    boolean tabbed;
    boolean hasHeaderRow;
    String[] itemLines;
    String[] headerColumns;
    String selectedItemText;
    boolean captionWider;
    TextView listTemplate;
    TextView headerCell;
    Activity activity;
    int mCurrentDialogId;
    LinearLayout rootView;
    View mView;
    PopupWindow mWindow;

    public BrDialogWindow (Activity hostActivity) {
        activity = hostActivity;
    }

    private void createWindow() {
        if (mWindow != null) {
            return;
        }
        mView = LayoutInflater.from(activity).inflate(R.layout.window_dialog, null, false);
        rootView = mView.findViewById(R.id.dw_root);

        mWindow = new PopupWindow(mView, -2, -2, true);
        mWindow.setAnimationStyle(R.style.PopupWindowAnimation);
        mWindow.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        mWindow.setInputMethodMode(PopupWindow.INPUT_METHOD_NEEDED);
        mWindow.setFocusable(true);
        mWindow.setOnDismissListener(() -> {
            detachKeyboardShift();
            ifOtherMethodClosed = false;
            mIsShow = false;
            mWindow.setFocusable(false);
        });
    }

    private String[] getSplittedStrings(String text) {
        return text.split(Pattern.quote("\n"));
    }

    private String[] getSplittedTabs(String text) {
        return text.split(Pattern.quote("\t"));
    }

    public float[] calcuteStringsLength(TextView template, String[] lines) {
        float[] widths = new float[lines.length];
        TextPaint textPaint = new TextPaint();
        textPaint.setTextSize(template.getTextSize());
        textPaint.setTypeface(template.getTypeface());
        for (int i = 0; i < lines.length; i++) {
            widths[i] = new StaticLayout(Utils.transfromColors(lines[i]), textPaint, 10000, Layout.Alignment.ALIGN_NORMAL, 1.0f, 0.0f, false).getLineWidth(0) + 5.0f;
        }
        return widths;
    }

    private TextView createButtonFromOrig(TextView template, boolean withMargin, boolean tabbed) {
        TextView cell = new TextView(this.mActivity);
        LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(-1, -2);
        if (withMargin) {
            layoutParams.topMargin = Utils.dpToPx(6.0f, this.mActivity);
        }
        cell.setLayoutParams(layoutParams);
        cell.setBackground(template.getBackground());
        cell.setTypeface(template.getTypeface());
        cell.setGravity(Gravity.START | Gravity.CENTER_VERTICAL);
        if (!tabbed) {
            cell.setPadding(template.getPaddingLeft(), template.getPaddingTop(), template.getPaddingRight(), template.getPaddingBottom());
        }
        cell.setAllCaps(false);
        cell.setTextColor(template.getTextColors().getDefaultColor());
        cell.setTextSize(0, template.getTextSize());
        return cell;
    }

    private float[][] getColumnsWidth(TextView template, String[] lines) {
        float[][] widths = new float[lines.length][];
        for (int i = 0; i < lines.length; i++) {
            String[] splittedTabs = getSplittedTabs(lines[i]);
            widths[i] = new float[splittedTabs.length];
            for (int i2 = 0; i2 < splittedTabs.length; i2++) {
                template.setText(Utils.transfromColors(splittedTabs[i2]));
                widths[i][i2] = (float) (Utils.getTextLength(template) + Utils.dpToPx(54.0f, this.mActivity));
            }
        }
        return widths;
    }

    private float[] getMaxWidths(float[][] widths) {
        float[] maxWidths = new float[5];
        for (int i = 0; i < 5; i++) {
            maxWidths[i] = -1.0f;
        }
        for (int i2 = 0; i2 < 5; i2++) {
            for (int i3 = 0; i3 < widths.length; i3++) {
                if (i2 < widths[i3].length && widths[i3][i2] >= maxWidths[i2]) {
                    maxWidths[i2] = widths[i3][i2];
                }
            }
        }
        return maxWidths;
    }

    public void show(int dialogId, int dialogTypeId, String caption, String content, String leftBtnText, String rightBtnText) {
        mCurrentDialogId = dialogId;
        JSONObject jsonObject = new JSONObject();
        try {
            jsonObject.put("i", dialogTypeId);
            jsonObject.put("l", leftBtnText);
            jsonObject.put("r", rightBtnText);
            jsonObject.put("c", caption);
            jsonObject.put("s", content);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        show(jsonObject);
    }

    public void show(JSONObject jSONObject) {
        if(mIsShow) return;

        createWindow();

        this.mClickedButton = -1;
        this.mListitemToSend = -1;
        Utils.makeAllViewsVisible(rootView);
        mIsShow = true;
        try {
            dialogStyle = jSONObject.getInt("i");
            try {
                captionText = jSONObject.getString("c");
            } catch (Exception ignored) {
            }
        } catch (Exception ignored) {
        }
        try {
            contentText = jSONObject.getString("s");
            try {
                okText = jSONObject.getString("l");
                try {
                    cancelText = jSONObject.getString("r");
                } catch (Exception ignored) {
                }
            } catch (Exception ignored) {
            }
        } catch (Exception ignored) {
        }
        Button buttonOk = mView.findViewById(R.id.dw_button_ok);
        button = mView.findViewById(R.id.dw_button_cancel);
        TextView captionView = mView.findViewById(R.id.dw_caption);
        CustomEditTextWithBackPressEvent editText = mView.findViewById(R.id.dw_edittext);
        this.mEditText = editText;
        Utils.fixEditTextForAndroid10Xiaomi(editText);
        buttonOk.setText(Utils.transfromColors(okText));
        button.setText(Utils.transfromColors(cancelText));
        captionView.setText(Utils.transfromColors(captionText));
        if (cancelText != null && cancelText.length() == 0) {
            button.setVisibility(View.GONE);
        }
        if (dialogStyle == 3) {
            dialogStyle = 1;
        }
        if (dialogStyle == 4) {
            hasHeaderRow = false;
            tabbed = true;
            dialogStyle = 2;
        }
        if (dialogStyle == 5) {
            hasHeaderRow = true;
            dialogStyle = 2;
            tabbed = true;
        }
        buttonOk.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ifOtherMethodClosed = true;
                sendResponse(1);
            }
        });
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ifOtherMethodClosed = true;
                sendResponse(0);
            }
        });
        Utils.changeTextViewWidth(captionView);
        if (dialogStyle == 0) {
            this.mEditText.setVisibility(View.GONE);
            mView.findViewById(R.id.dw_scroll_root).setVisibility(View.GONE);
            TextView infoView = (TextView) mView.findViewById(R.id.dw_info);
            infoView.setText(Utils.transfromColors(contentText));
            Utils.changeTextViewWidth(infoView);
        } else if (dialogStyle == 1) {
            mView.findViewById(R.id.dw_scroll_root).setVisibility(View.GONE);
            TextView infoView = (TextView) mView.findViewById(R.id.dw_info);
            infoView.setText(Utils.transfromColors(contentText));
            Utils.changeTextViewWidth(infoView);
            this.mEditText.getEditableText().clear();
        } else if (dialogStyle == 2) {
            LinearLayout linearLayout = mView.findViewById(R.id.dw_header);
            mView.findViewById(R.id.dw_scroll_info).setVisibility(View.GONE);
            this.mEditText.setVisibility(View.GONE);
            if (!hasHeaderRow) {
                mView.findViewById(R.id.dw_divider).setVisibility(View.GONE);
                linearLayout.setVisibility(View.GONE);
            }
            if (this.mOrigButton == null) {
                this.mOrigButton = mView.findViewById(R.id.dw_list_button);
            }
            TextView listTemplateLocal = this.mOrigButton;
            String[] splittedStrings = getSplittedStrings(contentText);
            final float[] maxWidths = tabbed ? getMaxWidths(getColumnsWidth(listTemplateLocal, splittedStrings)) : null;
            if (hasHeaderRow) {
                selectedItemText = splittedStrings[0];
                String[] itemColumns = new String[splittedStrings.length - 1];
                System.arraycopy(splittedStrings, 1, itemColumns, 0, splittedStrings.length - 1);
                String[] splittedTabs = getSplittedTabs(selectedItemText);
                for (int i3 = 0; i3 < linearLayout.getChildCount(); i3++) {
                    linearLayout.getChildAt(i3).setVisibility(View.GONE);
                }
                for (int i4 = 0; i4 < splittedTabs.length; i4++) {
                    TextView headerCellView = (TextView) linearLayout.getChildAt(i4);
                    headerCellView.setText(Utils.transfromColors(splittedTabs[i4]));
                    headerCellView.setVisibility(View.VISIBLE);
                }
                itemLines = splittedTabs;
                headerColumns = itemColumns;
            } else {
                headerColumns = splittedStrings;
                selectedItemText = null;
                itemLines = null;
            }
            float[] calcuteStringsLength = calcuteStringsLength(listTemplateLocal, headerColumns);
            if (calcuteStringsLength != null && calcuteStringsLength.length != 0) {
                float f = calcuteStringsLength[0];
                for (float v : calcuteStringsLength) {
                    if (v > f) {
                        f = v;
                    }
                }
                final View findViewById = mView.findViewById(R.id.dw_scroll_root);
                LinearLayout.LayoutParams layoutParams = (LinearLayout.LayoutParams) findViewById.getLayoutParams();
                layoutParams.width = ((int) f) + Utils.dpToPx(54.0f, this.mActivity);
                if (tabbed) {
                    layoutParams.width = 0;
                    for (float f2 : maxWidths) {
                        layoutParams.width = (int) (((float) layoutParams.width) + f2);
                    }
                }
                int textLength = Utils.getTextLength(captionView);
                if (layoutParams.width < textLength) {
                    layoutParams.width = textLength;
                    captionWider = true;
                } else {
                    captionWider = false;
                }
                findViewById.setLayoutParams(layoutParams);
                ViewGroup viewGroup = (ViewGroup) mView.findViewById(R.id.dw_root);
                LinearLayout linearLayout2 = (LinearLayout) mView.findViewById(R.id.dw_scroll_layout);
                TextView itemView = listTemplateLocal;
                viewGroup.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
                    @Override
                    public void onGlobalLayout() {
                        viewGroup.getViewTreeObserver().removeOnGlobalLayoutListener(this);
                        if (viewGroup.getWidth() > findViewById.getWidth() && captionWider) {
                            ViewGroup.LayoutParams layoutParams2 = findViewById.getLayoutParams();
                            layoutParams2.width = -1;
                            findViewById.setLayoutParams(layoutParams2);
                        }
                        if (viewGroup.findViewById(R.id.dw_root_buttons).getWidth() > findViewById.getWidth()) {
                            ViewGroup.LayoutParams layoutParams3 = findViewById.getLayoutParams();
                            layoutParams3.width = -1;
                            findViewById.setLayoutParams(layoutParams3);
                        }
                        float[] widths = maxWidths;
                        if (widths != null) {
                            int length = widths.length;
                            float[] maxWidths = new float[length];
                            for (int i6 = 0; i6 < length; i6++) {
                                maxWidths[i6] = maxWidths[i6] / ((float) findViewById.getWidth());
                            }
                            if (itemLines != null) {
                                for (int i7 = 0; i7 < itemLines.length; i7++) {
                                    TextView headerCellView = (TextView) linearLayout.getChildAt(i7);
                                    LinearLayout.LayoutParams layoutParams4 = (LinearLayout.LayoutParams) headerCellView.getLayoutParams();
                                    layoutParams4.weight = 1.0f - maxWidths[i7];
                                    headerCellView.setLayoutParams(layoutParams4);
                                }
                            }
                            for (int i8 = 0; i8 < linearLayout2.getChildCount(); i8++) {
                                if (linearLayout2.getChildAt(i8) instanceof LinearLayout) {
                                    LinearLayout linearLayout3 = (LinearLayout) linearLayout2.getChildAt(i8);
                                    for (int i9 = 0; i9 < linearLayout3.getChildCount(); i9++) {
                                        LinearLayout.LayoutParams layoutParams5 = (LinearLayout.LayoutParams) linearLayout3.getChildAt(i9).getLayoutParams();
                                        layoutParams5.weight = 1.0f - maxWidths[i9];
                                        linearLayout3.getChildAt(i9).setLayoutParams(layoutParams5);
                                    }
                                }
                            }
                        }
                    }
                });
                linearLayout2.removeAllViews();
                int i6 = 0;
                while (i6 < headerColumns.length) {
                    if (tabbed) {
                        LinearLayout linearLayout3 = new LinearLayout(this.mActivity);
                        LinearLayout.LayoutParams layoutParams2 = new LinearLayout.LayoutParams(-1, -2);
                        if (i6 >= 1) {
                            layoutParams2.topMargin = Utils.dpToPx(6.0f, this.mActivity);
                        }
                        linearLayout3.setLayoutParams(layoutParams2);
                        linearLayout3.setOrientation(LinearLayout.HORIZONTAL);
                        linearLayout3.setPadding(itemView.getPaddingLeft(), itemView.getPaddingTop(), itemView.getPaddingRight(), itemView.getPaddingBottom());
                        linearLayout3.setBackground(ResourcesCompat.getDrawable(this.mActivity.getResources(), R.drawable.br_list_inactive, null));
                        linearLayout2.addView(linearLayout3);
                        for (String column : getSplittedTabs(headerColumns[i6])) {
                            TextView columnView = createButtonFromOrig(listTemplate, true, false);
                            TextView listTemplate = new TextView(mActivity);
                            listTemplate.setText(Utils.transfromColors(column));
                            listTemplate.setTypeface(columnView.getTypeface());
                            listTemplate.setTextSize(13);
                            linearLayout3.addView(listTemplate);
                        }
                        if(i6 == 0) {
                            mSelectedButton = linearLayout3;
                            mClickedButton = 1;
                            mListitemToSend = i6;
                            mSelectedButton.setBackground(ResourcesCompat.getDrawable(BrDialogWindow.this.mActivity.getResources(), R.drawable.br_list_active, null));
                        }
                        int finalI1 = i6;
                        linearLayout3.setOnClickListener(new View.OnClickListener() {
                            @Override
                            public void onClick(View view) {
                                if (mSelectedButton != view) {
                                    if (mSelectedButton != null) {
                                        mSelectedButton.setBackground(ResourcesCompat.getDrawable(BrDialogWindow.this.mActivity.getResources(), R.drawable.br_list_inactive, null));
                                    }
                                    mSelectedButton = view;
                                    mClickedButton = 1;
                                    mListitemToSend = finalI1;
                                    mSelectedButton.setBackground(ResourcesCompat.getDrawable(BrDialogWindow.this.mActivity.getResources(), R.drawable.br_list_active, null));
                                    mSelectedButton.startAnimation(AnimationUtils.loadAnimation(BrDialogWindow.this.mActivity, R.anim.button_click));
                                    return;
                                }
                                mClickedButton = 1;
                                ifOtherMethodClosed = true;
                                sendResponse(1);
                                mSelectedButton = null;
                            }
                        });
                        listTemplate = itemView;
                    } else {
                        listTemplate = itemView;
                        if (i6 >= 1) {
                            headerCell = createButtonFromOrig(listTemplate, true, false);
                        }
                        if (i6 == 0) {
                            headerCell = createButtonFromOrig(listTemplate, false, false);
                        }
                    }
                        int finalI = i6;
                    if(!tabbed) {
                        headerCell.setOnClickListener(new View.OnClickListener() {
                            @Override
                            public void onClick(View view) {
                                if (mSelectedButton != view) {
                                    if (mSelectedButton != null) {
                                        mSelectedButton.setBackground(ResourcesCompat.getDrawable(BrDialogWindow.this.mActivity.getResources(), R.drawable.br_list_inactive, null));
                                    }
                                    mSelectedButton = view;
                                    mClickedButton = 1;
                                    mListitemToSend = finalI;
                                    mSelectedButton.setBackground(ResourcesCompat.getDrawable(BrDialogWindow.this.mActivity.getResources(), R.drawable.br_list_active, null));
                                    mSelectedButton.startAnimation(AnimationUtils.loadAnimation(BrDialogWindow.this.mActivity, R.anim.button_click));
                                    return;
                                }
                                mClickedButton = 1;
                                ifOtherMethodClosed = true;
                                sendResponse(1);
                                mSelectedButton = null;
                            }
                        });
                    }
                    if (!tabbed) {
                        headerCell.setText(Utils.transfromColors(headerColumns[i6]));
                        linearLayout2.addView(headerCell);
                        if(i6 == 0) {
                            mSelectedButton = headerCell;
                            mClickedButton = 1;
                            mListitemToSend = i6;
                            mSelectedButton.setBackground(ResourcesCompat.getDrawable(BrDialogWindow.this.mActivity.getResources(), R.drawable.br_list_active, null));
                        }
                    }
                    i6++;
                    itemView = listTemplate;
                }
                itemView.setVisibility(View.GONE);
            } else if (selectedItemText != null) {
                return;
            } else {
                return;
            }
        }
        this.mEditText.setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View view, int i22, KeyEvent keyEvent) {
                if (i22 != 66) {
                    return false;
                };
                ((InputMethodManager) mActivity.getSystemService(Context.INPUT_METHOD_SERVICE)).hideSoftInputFromWindow(mEditText.getWindowToken(), 0);
                mEditText.clearFocus();
                mEditText.setFocusable(true);
                mEditText.setFocusableInTouchMode(true);
                return true;
            }
        });
        tabbed = false;
        hasHeaderRow = false;
        mView.setFocusableInTouchMode(true);
        mWindow.setFocusable(true);
        mWindow.setInputMethodMode(PopupWindow.INPUT_METHOD_NEEDED);

        mWindow.showAtLocation(Multiplayer.getInstance().getParentLayout(), Gravity.CENTER, 0, 0);
        attachKeyboardShift();
        restoreEditTextFocus();
    }

    private void restoreEditTextFocus() {
        if (mEditText == null || mView == null) {
            return;
        }

        mEditText.setEnabled(true);
        mEditText.setFocusable(true);
        mEditText.setFocusableInTouchMode(true);
        mEditText.setSelected(false);
        mEditText.clearFocus();

        mView.post(() -> {
            if (mEditText == null || mEditText.getVisibility() != View.VISIBLE) {
                return;
            }
            mEditText.setFocusable(true);
            mEditText.setFocusableInTouchMode(true);
        });
    }

    private ViewTreeObserver.OnGlobalLayoutListener mKeyboardListener = null;
    private View mKeyboardHost = null;

    private void attachKeyboardShift() {
        if (mKeyboardListener != null || mView == null) {
            return;
        }

        mKeyboardHost = Multiplayer.getInstance().getParentLayout();
        if (mKeyboardHost == null) {
            return;
        }

        mKeyboardListener = () -> {
            if (mView == null || mEditText == null || mKeyboardHost == null) {
                return;
            }

            final android.graphics.Rect visible = new android.graphics.Rect();
            mKeyboardHost.getWindowVisibleDisplayFrame(visible);

            final int hostHeight = mKeyboardHost.getRootView().getHeight();
            final int keyboardTop = visible.bottom;

            if (hostHeight - keyboardTop < hostHeight / 5 || mEditText.getVisibility() != View.VISIBLE) {
                mView.setTranslationY(0.0f);
                return;
            }

            final int[] location = new int[2];
            mEditText.getLocationOnScreen(location);

            final int overlap = location[1] + mEditText.getHeight() - keyboardTop;
            if (overlap <= 0) {
                return;
            }

            mView.setTranslationY(mView.getTranslationY() - (overlap + mEditText.getHeight() / 2));
        };

        mKeyboardHost.getViewTreeObserver().addOnGlobalLayoutListener(mKeyboardListener);
    }

    private void detachKeyboardShift() {
        if (mKeyboardListener == null) {
            return;
        }

        if (mKeyboardHost != null) {
            mKeyboardHost.getViewTreeObserver().removeOnGlobalLayoutListener(mKeyboardListener);
            mKeyboardHost = null;
        }

        mKeyboardListener = null;

        if (mView != null) {
            mView.setTranslationY(0.0f);
        }
    }


    public void sendResponse(int i) {
        try {
            Multiplayer.getInstance().sendDialogResponse(i, mCurrentDialogId, mListitemToSend, mEditText.getText().toString().getBytes("windows-1251"));
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }
        this.mClickedButton = -1;
        mIsShow = false;
        close();
    }

    public void close() {
        mIsShow = false;
        if (mWindow != null) {
            mWindow.dismiss();
        }
    }

    public boolean getVis() {
        return mWindow != null && mWindow.isShowing();
    }

    public boolean m_bIsTempToggled = false;
    public void tempToggle(boolean state) {
        m_bIsTempToggled = state;
        if (mView != null) {
            mView.setVisibility(state ? View.VISIBLE : View.GONE);
        }
    }

    public void onHeightChanged(int height) {
    }
}
