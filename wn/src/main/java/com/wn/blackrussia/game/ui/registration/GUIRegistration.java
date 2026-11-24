package com.wn.blackrussia.game.ui.registration;

import android.content.Context;
import android.graphics.Bitmap;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.Button;
import android.widget.PopupWindow;
import android.widget.TextView;

import androidx.constraintlayout.widget.ConstraintLayout;

import com.nvidia.devtech.NvEventQueueActivity;
import com.wn.blackrussia.R;
import com.wn.blackrussia.game.Multiplayer;
import com.wn.blackrussia.game.common.GUIPopupWindow;
import com.wn.blackrussia.game.common.UIContainer;
import com.wn.blackrussia.game.ui.brguifix.GUIManager;
import com.wn.blackrussia.game.ui.tuning.TuningConstants;

import org.json.JSONObject;

public class GUIRegistration {
    public static final int REGISTATION_LAYOUT_CHOOSE_SEX = 1;
    public static final int REGISTATION_LAYOUT_GENERAL = 0;
    public static final int REGISTATION_LAYOUT_PERSON = 2;
    public static final int REGISTRATION_RESTORE = 0;
    public static final int REGISTRATION_RESTORE_NEW = 1;
    public static final int REGISTRATION_RESTORE_PIN = 2;
    public static final int REGISTRATION_RESTORE_PIN_AUTH = 4;
    public static final int REGISTRATION_RESTORE_PIN_NEW = 3;

    public static final int REGISTRATION_SUBLAYOUT_CREATE_PASSWORD = 1;
    public static final int REGISTRATION_SUBLAYOUT_ENTER = 0;
    public static final int REGISTRATION_SUBLAYOUT_FINGERPRINT = 2;
    public static final int REGISTRATION_SUBLAYOUT_INVITE = 3;
    public static final int REGISTRATION_SUBLAYOUT_LOGIN = 4;

    public static UILayoutRegistrationGeneral uiGeneral;
    public static Multiplayer mActivity = null;
    public GUIManager mGUIManager = null;
    private static GUIPopupWindow mWindow = null;
    private GUIPopupWindow mPopup = null;
    public static UIContainer mContainer = null;
    public static View mViewRoot = null;
    public Bitmap mLogoBitmap = null;
    public Animation mAnim = null;
    public static boolean mIsRegistration = false;
    public boolean mHasFingerprint = false;
    public boolean mNeedPasswordNow = false;
    public boolean mHasMail = false;
    public static int mRestoreSet = -1;
    public boolean isMale = true;
    public int mRestoreMethod = 0;

    public int getGuiId() {
        return 38;
    }

    public void show(JSONObject json, Multiplayer activity) {
        this.mActivity = activity;
        if (this.mWindow == null) {
            this.mViewRoot = ((LayoutInflater) activity.getSystemService(Context.LAYOUT_INFLATER_SERVICE)).inflate(R.layout.registration_mainframe, (ViewGroup) null, false);
            GUIPopupWindow gUIPopupWindow = new GUIPopupWindow(this.mViewRoot, -1, -1, true);
            this.mWindow = gUIPopupWindow;
        }

        NvEventQueueActivity.getInstance().hideSystemUI();
        this.mAnim = AnimationUtils.loadAnimation(activity, R.anim.button_click);

        mIsRegistration = json.optBoolean("mIsRegistration");
        mHasFingerprint = json.optBoolean("mHasFingerprint");
        mNeedPasswordNow = json.optBoolean("mNeedPasswordNow");
        mHasMail = json.optBoolean("mHasMail");

        UIContainer uIContainer = (UIContainer) this.mViewRoot.findViewById(R.id.container);
        this.mContainer = uIContainer;

        uIContainer.addLayout(0, new UILayoutRegistrationGeneral(this));
        this.mContainer.addLayout(1, new UILayoutRegistrationSex(this));
        this.mContainer.addLayout(2, new UILayoutRegistrationPerson(this));
        this.uiGeneral = (UILayoutRegistrationGeneral) this.mContainer.getLayout(0);
        this.mContainer.setCurrentLayout(0);

        this.mWindow.showAtLocation(this.mActivity.getParentLayout(), 17, 0, 0);
    }

    public static void onPacketIncoming(int i) {
        /**
         *     REGISTRATION_SUBLAYOUT_CREATE_PASSWORD = 1;
         *     REGISTRATION_SUBLAYOUT_ENTER = 0;
         *     REGISTRATION_SUBLAYOUT_FINGERPRINT = 2;
         *     REGISTRATION_SUBLAYOUT_INVITE = 3;
         *     REGISTRATION_SUBLAYOUT_LOGIN = 4;
         */

        int optInt = i;
        UILayoutRegistrationGeneral uILayoutRegistrationGeneral = (UILayoutRegistrationGeneral) mContainer.getLayout(0);
        if (optInt != 0) {
            if (optInt == 1) {
                uILayoutRegistrationGeneral.mContainer.setCurrentLayout(1);
                mRestoreSet = 0;
            } else if (optInt == 2) {
                uILayoutRegistrationGeneral.mContainer.setCurrentLayout(1);
                mRestoreSet = 0;
            } else if (optInt == 3) {
                mIsRegistration = true;
                mRestoreSet = 0;
                uiGeneral.mContainer.setCurrentLayout(3);
                mContainer.setCurrentLayout(1);
            } else if (optInt != 4) {

            }
        } else if (mIsRegistration) {
            int currentLayoutId = uiGeneral.mContainer.getCurrentLayoutId();
            UILayoutRegistrationGeneral uILayoutRegistrationGeneral2 = uiGeneral;
            if (currentLayoutId != 3) {
                if (uILayoutRegistrationGeneral2.mContainer.getCurrentLayoutId() == 1 && !mActivity.isSupportFingerPrint) {
                    JSONObject jSONObject = new JSONObject();
                    try {
                        jSONObject.put("t", 2);
                        jSONObject.put("s", "");
                        jSONObject.put(TuningConstants.KEY_GET_OLD_INSTALL_DETAIL, 0);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    uiGeneral.mContainer.setCurrentLayout(3);
                    return;
                }
                uiGeneral.mContainer.setCurrentLayout(uiGeneral.mContainer.getCurrentLayoutId() + 1);
            } else if (mContainer.getCurrentLayoutId() != 1) {
                mContainer.setCurrentLayout(1);
            } else {
                mContainer.setCurrentLayout(2);
                mViewRoot.setBackground(null);
            }
        } else if (uILayoutRegistrationGeneral.mContainer.getCurrentLayoutId() == 1) {
            int i2 = mRestoreSet;
            if (i2 == 0) {
                mRestoreSet = i2 + 1;
                uILayoutRegistrationGeneral.mContainer.setCurrentLayout(1);
                return;
            }
            mWindow.dismiss();
        } else {
            mWindow.dismiss();
        }
    }

    public static void close() {
        GUIPopupWindow gUIPopupWindow = mWindow;
        if (gUIPopupWindow != null) {
            gUIPopupWindow.dismiss();
        }
    }

    public boolean isShowingGui() {
        GUIPopupWindow gUIPopupWindow = this.mWindow;
        if (gUIPopupWindow != null) {
            return gUIPopupWindow.isShowing();
        }
        return false;
    }

    public void showPopup(String header, String content, String button, View.OnClickListener closeListener, View.OnClickListener okListener) {
        GUIPopupWindow gUIPopupWindow = this.mPopup;
        if (gUIPopupWindow != null) {
            gUIPopupWindow.dismiss();
        }
        View inflate = ((LayoutInflater) this.mActivity.getSystemService(Context.LAYOUT_INFLATER_SERVICE)).inflate(R.layout.halloween_popup, (ViewGroup) null, false);
        this.mPopup = new GUIPopupWindow(inflate, -2, -2, true);
        inflate.findViewById(R.id.ok).setOnClickListener(okListener);
        if (closeListener != null) {
            inflate.findViewById(R.id.close).setOnClickListener(closeListener);
        } else {
            inflate.findViewById(R.id.close).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    GUIRegistration.this.mPopup.dismiss();
                }
            });
        }
        if (okListener != null) {
            inflate.findViewById(R.id.ok).setOnClickListener(okListener);
        } else {
            inflate.findViewById(R.id.ok).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    GUIRegistration.this.mPopup.dismiss();
                }
            });
        }
        ((TextView) inflate.findViewById(R.id.header)).setText(header);
        ((TextView) inflate.findViewById(R.id.content)).setText(content);
        ((TextView) inflate.findViewById(R.id.content)).setTextSize(2, 12.0f);
        ((Button) inflate.findViewById(R.id.ok)).setText(button);
        this.mPopup.showAtLocation(this.mActivity.getParentLayout(), 17, 0, 0);
        this.mPopup.setDimming(0.8f);
    }
}
