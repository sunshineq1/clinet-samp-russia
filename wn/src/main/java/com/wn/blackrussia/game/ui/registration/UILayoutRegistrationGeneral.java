package com.wn.blackrussia.game.ui.registration;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.ImageView;
import android.widget.TextView;
import androidx.constraintlayout.widget.ConstraintLayout;

import com.wn.blackrussia.R;
import com.wn.blackrussia.game.Multiplayer;
import com.wn.blackrussia.game.common.UIContainer;
import com.wn.blackrussia.game.common.UILayout;
import com.wn.storage.Preferences;

public class UILayoutRegistrationGeneral extends UILayout {
    public static final int REGISTRATION_SUBLAYOUT_CREATE_PASSWORD = 1;
    public static final int REGISTRATION_SUBLAYOUT_ENTER = 0;
    public static final int REGISTRATION_SUBLAYOUT_FINGERPRINT = 2;
    public static final int REGISTRATION_SUBLAYOUT_INVITE = 3;
    public static final int REGISTRATION_SUBLAYOUT_LOGIN = 4;
    
    ImageView imgLogo;
    private GUIRegistration mRoot;
    TextView textHeader;
    TextView textHint;
    TextView textNick;
    private View mViewRoot = null;
    public UIContainer mContainer = null;
    public int mHeightField = 0;

    @Override
    public void onLayoutClose() {
    }

    public UILayoutRegistrationGeneral(GUIRegistration root) {
        this.mRoot = root;
    }

    @Override
    public View getView() {
        return this.mViewRoot;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container) {
        View inflate = inflater.inflate(R.layout.registration_view, container, false);

        ViewGroup.LayoutParams params = new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
        );
        inflate.setLayoutParams(params);

        this.mViewRoot = inflate;

        this.textHeader = (TextView) inflate.findViewById(R.id.reg_header);
        this.imgLogo = (ImageView) this.mViewRoot.findViewById(R.id.brlogo);
        this.textNick = (TextView) this.mViewRoot.findViewById(R.id.reg_nickname);
        this.textHint = (TextView) this.mViewRoot.findViewById(R.id.reg_info);
        UIContainer uIContainer = (UIContainer) this.mViewRoot.findViewById(R.id.reg_container);
        this.mContainer = uIContainer;
        uIContainer.addLayout(0, new UILayoutRegistrationEnter(this.mRoot));
        this.mContainer.addLayout(1, new UILayoutRegistrationCreatePassword(this.mRoot));
        this.mContainer.addLayout(3, new UILayoutRegistrationInvite(this.mRoot));
        this.mContainer.addLayout(4, new UILayoutRegistrationLogin(this.mRoot));
        if (this.mRoot.mIsRegistration) {
            this.mContainer.setCurrentLayout(1);
        } else {
            this.mContainer.setCurrentLayout(4);
        }
        this.textNick.setText(Preferences.getNick() + " [" + Preferences.GetID() + "]");
        UILayoutRegistrationGeneral.this.imgLogo.setImageResource(R.drawable.logobrred);
        UILayoutRegistrationGeneral.this.imgLogo.setVisibility(View.INVISIBLE);

        if (this.mRoot.mIsRegistration) {
            this.textHeader.setText("Регистрация");
        } else {
            this.textHeader.setText("Авторизация");
        }
        return this.mViewRoot;
    }

    @Override
    public void onLayoutShown() {
        if (this.mRoot.mIsRegistration) {
            this.textHeader.setText("Регистрация");
        } else {
            this.textHeader.setText("Авторизация");
        }
        this.mViewRoot.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                UILayoutRegistrationGeneral.this.mViewRoot.getViewTreeObserver().removeGlobalOnLayoutListener(this);
                ImageView imageView = (ImageView) UILayoutRegistrationGeneral.this.mViewRoot.findViewById(R.id.reg_person);
                ConstraintLayout.LayoutParams layoutParams = (ConstraintLayout.LayoutParams) imageView.getLayoutParams();
                int height = (int) (UILayoutRegistrationGeneral.this.mViewRoot.getHeight() * 0.7f);
                layoutParams.width = height;
                layoutParams.height = height;
                imageView.setLayoutParams(layoutParams);
                ConstraintLayout.LayoutParams layoutParams2 = (ConstraintLayout.LayoutParams) UILayoutRegistrationGeneral.this.imgLogo.getLayoutParams();
                layoutParams2.height = (int) (UILayoutRegistrationGeneral.this.mViewRoot.getHeight() * 0.15f);
                layoutParams2.width = layoutParams2.height * 2;
                UILayoutRegistrationGeneral.this.imgLogo.setLayoutParams(layoutParams2);
                ConstraintLayout.LayoutParams layoutParams3 = (ConstraintLayout.LayoutParams) UILayoutRegistrationGeneral.this.textHint.getLayoutParams();
                layoutParams3.leftMargin = (int) (height * (-0.6f));
                UILayoutRegistrationGeneral.this.textHint.setLayoutParams(layoutParams3);
            }
        });
    }
}
