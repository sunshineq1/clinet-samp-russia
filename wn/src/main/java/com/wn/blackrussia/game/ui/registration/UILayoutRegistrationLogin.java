package com.wn.blackrussia.game.ui.registration;

import android.annotation.SuppressLint;
import android.text.Html;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.Switch;
import android.widget.TextView;

import com.nvidia.devtech.CustomEditText;
import com.nvidia.devtech.NvEventQueueActivity;
import com.wn.blackrussia.R;
import com.wn.blackrussia.game.Multiplayer;
import com.wn.blackrussia.game.common.CustomEditTextWithBackPressEvent;
import com.wn.blackrussia.game.common.UIContainer;
import com.wn.blackrussia.game.common.UILayout;
import com.wn.blackrussia.game.ui.tuning.TuningConstants;

import org.json.JSONObject;

import java.io.UnsupportedEncodingException;

public class UILayoutRegistrationLogin extends UILayout {
    LinearLayout butBack;
    LinearLayout butFinger;
    Button butPlay;
    com.wn.blackrussia.game.ui.util.CustomEditText editPassword;
    ImageView imgAutoHint;
    private GUIRegistration mRoot;
    private UILayoutRegistrationGeneral mUIParent;
    private View mViewRoot = null;
    Switch switchAuto;
    TextView textPassword;

    @Override
    public void onLayoutClose() {
    }

    public UILayoutRegistrationLogin(GUIRegistration root) {
        this.mRoot = root;
    }

    @Override
    public View getView() {
        return this.mViewRoot;
    }

    @SuppressLint("WrongConstant")
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container) {
        this.mViewRoot = inflater.inflate(R.layout.reg_login, (ViewGroup) null, false);
        this.mUIParent = (UILayoutRegistrationGeneral) this.mRoot.mContainer.getLayout(0);
        this.butBack = (LinearLayout) this.mViewRoot.findViewById(R.id.back_but);
        this.butFinger = (LinearLayout) this.mViewRoot.findViewById(R.id.fingerprint_but);
        this.editPassword = (com.wn.blackrussia.game.ui.util.CustomEditText) this.mViewRoot.findViewById(R.id.password_enter);
        this.textPassword = (TextView) this.mViewRoot.findViewById(R.id.password_text);
        this.butPlay = (Button) this.mViewRoot.findViewById(R.id.play_but);
        this.switchAuto = (Switch) this.mViewRoot.findViewById(R.id.auto_switch);
        this.imgAutoHint = (ImageView) this.mViewRoot.findViewById(R.id.auto_img);
        this.butPlay.setAlpha(0.5f);
        if (!this.mRoot.mHasFingerprint || !this.mRoot.mActivity.isSupportFingerPrint) {
            this.butFinger.setAlpha(0.5f);
        }
        this.editPassword.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                UILayoutRegistrationLogin.this.editPassword.getViewTreeObserver().removeGlobalOnLayoutListener(this);
                LinearLayout.LayoutParams layoutParams = (LinearLayout.LayoutParams) UILayoutRegistrationLogin.this.editPassword.getLayoutParams();
                int width = (int) (UILayoutRegistrationLogin.this.editPassword.getWidth() * 0.16f);
                layoutParams.height = width;
                UILayoutRegistrationLogin.this.editPassword.setLayoutParams(layoutParams);
                LinearLayout.LayoutParams layoutParams2 = (LinearLayout.LayoutParams) UILayoutRegistrationLogin.this.butFinger.getLayoutParams();
                layoutParams2.height = width;
                UILayoutRegistrationLogin.this.butFinger.setLayoutParams(layoutParams2);
                LinearLayout.LayoutParams layoutParams3 = (LinearLayout.LayoutParams) UILayoutRegistrationLogin.this.butPlay.getLayoutParams();
                layoutParams3.height = width;
                UILayoutRegistrationLogin.this.butPlay.setLayoutParams(layoutParams3);
            }
        });
        if (this.mRoot.mHasMail) {
            this.textPassword.setText(Html.fromHtml("<u>Восстановить пароль</u>"));
            this.textPassword.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    JSONObject jSONObject = new JSONObject();
                    try {
                        jSONObject.put("t", 7);
                        jSONObject.put(TuningConstants.KEY_GET_OLD_INSTALL_DETAIL, 0);
                    } catch (Exception unused) {
                    }
                }
            });
        } else {
            this.textPassword.setVisibility(4);
        }
        this.editPassword.setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View view, int i, KeyEvent keyEvent) {
                if (UILayoutRegistrationLogin.this.editPassword.getText().length() >= 6) {
                    UILayoutRegistrationLogin.this.butPlay.setAlpha(1.0f);
                } else {
                    UILayoutRegistrationLogin.this.butPlay.setAlpha(0.5f);
                }
                if (i == 66) {
                    Multiplayer nvEventQueueActivity = UILayoutRegistrationLogin.this.mRoot.mActivity;
                    Multiplayer nvEventQueueActivity2 = UILayoutRegistrationLogin.this.mRoot.mActivity;
                    ((InputMethodManager) nvEventQueueActivity.getSystemService("input_method")).hideSoftInputFromWindow(UILayoutRegistrationLogin.this.mViewRoot.getWindowToken(), 0);
                    view.setFocusable(false);
                    view.setFocusableInTouchMode(true);
                    return true;
                }
                return false;
            }
        });
        this.butPlay.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (UILayoutRegistrationLogin.this.editPassword.getText().toString().length() >= 6) {
                    UILayoutRegistrationLogin.this.butPlay.setAlpha(1.0f);
                } else {
                    UILayoutRegistrationLogin.this.butPlay.setAlpha(0.5f);
                }
                if (UILayoutRegistrationLogin.this.butPlay.getAlpha() >= 1.0f) {
                    view.startAnimation(UILayoutRegistrationLogin.this.mRoot.mAnim);
                    JSONObject jSONObject = new JSONObject();
                    try {
                        jSONObject.put("t", 6);
                        jSONObject.put("s", UILayoutRegistrationLogin.this.editPassword.getText().toString());
                        jSONObject.put(TuningConstants.KEY_GET_OLD_INSTALL_DETAIL, UILayoutRegistrationLogin.this.switchAuto.isChecked() ? 1 : 0);

                        try {
                            Multiplayer.getInstance().sendDialogResponse(1, 1, 0, UILayoutRegistrationLogin.this.editPassword.getText().toString().getBytes("windows-1251"));
                            Multiplayer.getInstance().closeGUIRegistration();
                        } catch (UnsupportedEncodingException e) {
                            e.printStackTrace();
                        }
                        return;
                    } catch (Exception unused) {
                        return;
                    }
                }
                JSONObject jSONObject2 = new JSONObject();
                try {
                    jSONObject2.put("t", 2);
                    jSONObject2.put("d", 3);
                    jSONObject2.put("k", "");
                    jSONObject2.put("a", "");
                    jSONObject2.put("i", "");

                    if (UILayoutRegistrationLogin.this.editPassword.getText().length() == 0) {
                        jSONObject2.put("i", "Введите пароль");
                    } else {
                        jSONObject2.put("i", "Пароль слишком короткий");
                    }
                    UILayoutRegistrationLogin.this.mRoot.mActivity.localShowNotification(jSONObject2);
                } catch (Exception unused2) {
                }
            }
        });
        this.butFinger.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (view.getAlpha() < 1.0f) {
                    if (UILayoutRegistrationLogin.this.mRoot.mNeedPasswordNow) {
                        JSONObject jSONObject = new JSONObject();
                        try {
                            jSONObject.put("t", 2);
                            jSONObject.put("d", 3);
                            jSONObject.put("k", "");
                            jSONObject.put("a", "");
                            jSONObject.put("i", "Авторизуйтесь при помощи пароля, а не отпечатка");

                            UILayoutRegistrationLogin.this.mRoot.mActivity.localShowNotification(jSONObject);
                            return;
                        } catch (Exception unused) {
                            return;
                        }
                    }
                    return;
                }
                view.startAnimation(UILayoutRegistrationLogin.this.mRoot.mAnim);
                UIContainer uIContainer = UILayoutRegistrationLogin.this.mUIParent.mContainer;
                UILayoutRegistrationGeneral unused2 = UILayoutRegistrationLogin.this.mUIParent;
                uIContainer.setCurrentLayout(2);
            }
        });
        this.switchAuto.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                if (b) {
                    UILayoutRegistrationLogin.this.switchAuto.setThumbResource(R.drawable.switch_circle);
                } else {
                    UILayoutRegistrationLogin.this.switchAuto.setThumbResource(R.drawable.switch_off_circle);
                }
            }
        });
        this.imgAutoHint.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, MotionEvent motionEvent) {
                view.startAnimation(UILayoutRegistrationLogin.this.mRoot.mAnim);
                UILayoutRegistrationLogin.this.mRoot.showPopup(
                        "Автовход",
                        "При активации переключателя автовхода, система запомнит ваши данные и авторизует вас автоматически, при следующей авторизации без ввода данных. Активация автовхода работает в течении 15 минут после последнего отключения от сервера. По истечению времени после отключения игрока с сервера, автовход автоматически отключается.",
                        "Ок",
                        null,
                        null
                );
                return false;
            }
        });
        this.butBack.setVisibility(4);
        this.butBack.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, MotionEvent motionEvent) {
                view.startAnimation(UILayoutRegistrationLogin.this.mRoot.mAnim);
                Multiplayer nvEventQueueActivity = UILayoutRegistrationLogin.this.mRoot.mActivity;
                Multiplayer nvEventQueueActivity2 = UILayoutRegistrationLogin.this.mRoot.mActivity;
                ((InputMethodManager) nvEventQueueActivity.getSystemService("input_method")).hideSoftInputFromWindow(UILayoutRegistrationLogin.this.mViewRoot.getWindowToken(), 0);
                UILayoutRegistrationLogin.this.editPassword.setFocusable(false);
                UILayoutRegistrationLogin.this.editPassword.setFocusableInTouchMode(true);
                return false;
            }
        });
        return this.mViewRoot;
    }

    @Override
    public void onLayoutShown() {
        this.mUIParent.textHint.setText("Введите пароль, чтобы войти в игру.\n\nИнформация: если данный аккаунт является не вашим, то выйдите из игры и смените игровое имя на новое!");
    }
}
