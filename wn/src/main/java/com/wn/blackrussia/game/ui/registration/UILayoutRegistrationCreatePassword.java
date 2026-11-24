package com.wn.blackrussia.game.ui.registration;

import android.annotation.SuppressLint;
import android.text.Html;
import android.text.InputFilter;
import android.text.method.HideReturnsTransformationMethod;
import android.text.method.PasswordTransformationMethod;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.nvidia.devtech.CustomEditText;
import com.nvidia.devtech.NvEventQueueActivity;
import com.wn.blackrussia.R;
import com.wn.blackrussia.game.Multiplayer;
import com.wn.blackrussia.game.common.CustomEditTextWithBackPressEvent;
import com.wn.blackrussia.game.common.UILayout;
import com.wn.blackrussia.game.ui.tuning.TuningConstants;

import org.json.JSONObject;

public class UILayoutRegistrationCreatePassword extends UILayout {
    LinearLayout butBack;
    Button butt;
    com.wn.blackrussia.game.ui.util.CustomEditText edit1;
    com.wn.blackrussia.game.ui.util.CustomEditText edit2;
    com.wn.blackrussia.game.ui.util.CustomEditText edit3;
    private GUIRegistration mRoot;
    private UILayoutRegistrationGeneral mUIParent;
    private View mViewRoot = null;
    TextView textHeader;
    TextView textRestore;

    @Override
    public void onLayoutClose() {
    }

    public UILayoutRegistrationCreatePassword(GUIRegistration root) {
        this.mRoot = root;
    }

    @Override
    public View getView() {
        return this.mViewRoot;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container) {
        this.mViewRoot = inflater.inflate(R.layout.registration_create_password, (ViewGroup) null, false);
        this.mUIParent = (UILayoutRegistrationGeneral) this.mRoot.mContainer.getLayout(0);
        this.butBack = (LinearLayout) this.mViewRoot.findViewById(R.id.back_butt);
        this.textHeader = (TextView) this.mViewRoot.findViewById(R.id.header_text);
        this.textRestore = (TextView) this.mViewRoot.findViewById(R.id.restore_text);
        this.butt = (Button) this.mViewRoot.findViewById(R.id.reg_butt);
        this.edit1 = (com.wn.blackrussia.game.ui.util.CustomEditText) this.mViewRoot.findViewById(R.id.edit1);
        this.edit2 = (com.wn.blackrussia.game.ui.util.CustomEditText) this.mViewRoot.findViewById(R.id.edit2);
        this.edit3 = (com.wn.blackrussia.game.ui.util.CustomEditText) this.mViewRoot.findViewById(R.id.edit3);
        return this.mViewRoot;
    }

    @Override
    public void onLayoutShown() {
        this.butt.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                UILayoutRegistrationCreatePassword.this.butt.getViewTreeObserver().removeGlobalOnLayoutListener(this);
                LinearLayout.LayoutParams layoutParams = (LinearLayout.LayoutParams) UILayoutRegistrationCreatePassword.this.butt.getLayoutParams();
                int width = (int) (UILayoutRegistrationCreatePassword.this.butt.getWidth() * 0.16f);
                layoutParams.height = width;
                UILayoutRegistrationCreatePassword.this.butt.setLayoutParams(layoutParams);
                LinearLayout.LayoutParams layoutParams2 = (LinearLayout.LayoutParams) UILayoutRegistrationCreatePassword.this.edit1.getLayoutParams();
                layoutParams2.height = width;
                UILayoutRegistrationCreatePassword.this.edit1.setLayoutParams(layoutParams2);
                LinearLayout.LayoutParams layoutParams3 = (LinearLayout.LayoutParams) UILayoutRegistrationCreatePassword.this.edit2.getLayoutParams();
                layoutParams3.height = width;
                UILayoutRegistrationCreatePassword.this.edit2.setLayoutParams(layoutParams3);
                LinearLayout.LayoutParams layoutParams4 = (LinearLayout.LayoutParams) UILayoutRegistrationCreatePassword.this.edit3.getLayoutParams();
                layoutParams4.height = width;
                UILayoutRegistrationCreatePassword.this.edit3.setLayoutParams(layoutParams4);
            }
        });
        if (this.mRoot.mIsRegistration) {
            RegistrationSet();
            return;
        }
    }

    @SuppressLint("WrongConstant")
    private void RegistrationSet() {
        this.butt.setText("Зарегистрироваться");
        this.edit1.setHint("Электронная почта (необязательно)");
        this.edit2.setVisibility(0);
        this.edit2.setHint("Пароль");
        this.edit3.setVisibility(0);
        this.edit3.setHint("Повторите пароль");
        this.textHeader.setVisibility(8);
        this.textRestore.setVisibility(8);
        this.edit1.setInputType(1);
        this.edit2.setInputType(128);
        this.edit2.setTransformationMethod(PasswordTransformationMethod.getInstance());
        this.edit2.setFilters(new InputFilter[]{new InputFilter.LengthFilter(16)});
        this.edit3.setInputType(128);
        this.edit3.setTransformationMethod(PasswordTransformationMethod.getInstance());
        this.edit3.setFilters(new InputFilter[]{new InputFilter.LengthFilter(16)});
        this.butt.setAlpha(0.5f);
        this.edit1.setText("");
        this.edit2.setText("");
        this.edit3.setText("");
        this.edit1.setVisibility(View.GONE); // Ввод почты не нужен, скрываем
        //this.edit1.setOnBackPressListener(new CustomEditTextWithBackPressEvent.MyEditTextListener() {
        //    @Override
        //    public void callback() {
        //        NvEventQueueActivity nvEventQueueActivity = UILayoutRegistrationCreatePassword.this.mRoot.mActivity;
        //        NvEventQueueActivity nvEventQueueActivity2 = UILayoutRegistrationCreatePassword.this.mRoot.mActivity;
        //        ((InputMethodManager) nvEventQueueActivity.getSystemService("input_method")).hideSoftInputFromWindow(UILayoutRegistrationCreatePassword.this.mViewRoot.getWindowToken(), 0);
        //        UILayoutRegistrationCreatePassword.this.edit1.setFocusable(false);
        //        UILayoutRegistrationCreatePassword.this.edit1.setFocusableInTouchMode(true);
        //    }
        //});
        this.edit2.setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View view, int i, KeyEvent keyEvent) {
                if (UILayoutRegistrationCreatePassword.this.edit2.getText().length() >= 6 && UILayoutRegistrationCreatePassword.this.edit2.getText().toString().equals(UILayoutRegistrationCreatePassword.this.edit3.getText().toString())) {
                    UILayoutRegistrationCreatePassword.this.butt.setAlpha(1.0f);
                    return false;
                }
                UILayoutRegistrationCreatePassword.this.butt.setAlpha(0.5f);
                return false;
            }
        });

        this.edit3.setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View view, int i, KeyEvent keyEvent) {
                if (UILayoutRegistrationCreatePassword.this.edit3.getText().length() >= 6 && UILayoutRegistrationCreatePassword.this.edit2.getText().toString().equals(UILayoutRegistrationCreatePassword.this.edit3.getText().toString())) {
                    UILayoutRegistrationCreatePassword.this.butt.setAlpha(1.0f);
                } else {
                    UILayoutRegistrationCreatePassword.this.butt.setAlpha(0.5f);
                }
                if (i == 66) {
                    Multiplayer nvEventQueueActivity = UILayoutRegistrationCreatePassword.this.mRoot.mActivity;
                    Multiplayer nvEventQueueActivity2 = UILayoutRegistrationCreatePassword.this.mRoot.mActivity;
                    ((InputMethodManager) nvEventQueueActivity.getSystemService("input_method")).hideSoftInputFromWindow(UILayoutRegistrationCreatePassword.this.mViewRoot.getWindowToken(), 0);
                    view.setFocusable(false);
                    view.setFocusableInTouchMode(true);
                    return true;
                }
                return false;
            }
        });
        this.butBack.setVisibility(4);
        this.butBack.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, MotionEvent motionEvent) {
                view.startAnimation(UILayoutRegistrationCreatePassword.this.mRoot.mAnim);
                boolean z = UILayoutRegistrationCreatePassword.this.mRoot.mIsRegistration;
                Multiplayer nvEventQueueActivity = UILayoutRegistrationCreatePassword.this.mRoot.mActivity;
                Multiplayer nvEventQueueActivity2 = UILayoutRegistrationCreatePassword.this.mRoot.mActivity;
                ((InputMethodManager) nvEventQueueActivity.getSystemService("input_method")).hideSoftInputFromWindow(UILayoutRegistrationCreatePassword.this.mViewRoot.getWindowToken(), 0);
                UILayoutRegistrationCreatePassword.this.edit1.setFocusable(false);
                UILayoutRegistrationCreatePassword.this.edit1.setFocusableInTouchMode(true);
                UILayoutRegistrationCreatePassword.this.edit2.setFocusable(false);
                UILayoutRegistrationCreatePassword.this.edit2.setFocusableInTouchMode(true);
                UILayoutRegistrationCreatePassword.this.edit3.setFocusable(false);
                UILayoutRegistrationCreatePassword.this.edit3.setFocusableInTouchMode(true);
                return false;
            }
        });
        this.butt.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (UILayoutRegistrationCreatePassword.this.edit3.getText().length() >= 6 && UILayoutRegistrationCreatePassword.this.edit2.getText().toString().equals(UILayoutRegistrationCreatePassword.this.edit3.getText().toString())) {
                    UILayoutRegistrationCreatePassword.this.butt.setAlpha(1.0f);
                } else {
                    UILayoutRegistrationCreatePassword.this.butt.setAlpha(0.5f);
                }
                JSONObject jSONObject2 = new JSONObject();
                try {
                    jSONObject2.put("t", 2);
                    jSONObject2.put("d", 3);
                    jSONObject2.put("k", "");
                    jSONObject2.put("a", "");
                    jSONObject2.put("i", "");

                    if (!UILayoutRegistrationCreatePassword.this.edit2.getText().toString().equals(UILayoutRegistrationCreatePassword.this.edit3.getText().toString())) {
                        jSONObject2.put("i", "Пароли не совпадают");
                        UILayoutRegistrationCreatePassword.this.mRoot.mActivity.localShowNotification(jSONObject2);
                    } else if (UILayoutRegistrationCreatePassword.this.edit2.getText().length() == 0) {
                        jSONObject2.put("i", "Придумайте пароль");
                        UILayoutRegistrationCreatePassword.this.mRoot.mActivity.localShowNotification(jSONObject2);
                    } else if (UILayoutRegistrationCreatePassword.this.edit2.getText().length() < 6) {
                        jSONObject2.put("i", "Пароль слишком короткий");
                        UILayoutRegistrationCreatePassword.this.mRoot.mActivity.localShowNotification(jSONObject2);
                    }

                } catch (Exception unused2) {
                }
                if (view.getAlpha() >= 1.0f) {
                    view.startAnimation(UILayoutRegistrationCreatePassword.this.mRoot.mAnim);
                    if (UILayoutRegistrationCreatePassword.this.mRoot.mIsRegistration) {
                        JSONObject jSONObject = new JSONObject();
                        try {
                            jSONObject.put("t", 1);
                            jSONObject.put("s", UILayoutRegistrationCreatePassword.this.edit1.getText().toString());
                            jSONObject.put(TuningConstants.KEY_GET_ID_DETAIL, UILayoutRegistrationCreatePassword.this.edit2.getText().toString());

                            Multiplayer.getInstance().sendDialogResponse(1, 2, 0, UILayoutRegistrationCreatePassword.this.edit2.getText().toString().getBytes("windows-1251"));
                            GUIRegistration.uiGeneral.mContainer.setCurrentLayout(3);
                            return;
                        } catch (Exception unused) {
                            return;
                        }
                    }
                    return;
                }
            }
        });
        this.mUIParent.textHint.setText(Html.fromHtml("<b>Для начала игры, заполни все поля</b>\n<ul><li>Пароль должен состоять от 6 до 16 символов</li><li>Пароль чувствителен к регистру</li></ul>"));
    }

    @SuppressLint("WrongConstant")
    private void RestoreSet() {

    }

    private void RestoreNewSet() {

    }

    public void PinSet() {

    }

    public void PinRestoreSet() {

    }

    public void PinAuthSet() {

    }
}
