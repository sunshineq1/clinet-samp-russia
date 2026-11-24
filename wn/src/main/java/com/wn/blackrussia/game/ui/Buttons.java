package com.wn.blackrussia.game.ui;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.view.View;
import android.view.animation.AnimationUtils;

import androidx.constraintlayout.widget.ConstraintLayout;

import com.wn.blackrussia.R;
import com.wn.blackrussia.game.Multiplayer;
import com.wn.blackrussia.game.ui.util.Util;

public class Buttons {
    private Activity activity;
    private ConstraintLayout buttonsLayout;
    private View changer, alt, tab, y, n, f;
    private Boolean m_bSpecialKeysState = false;
    private Boolean m_bLastState;

    @SuppressLint("WrongViewCast")
    public Buttons(Activity activity) {
        this.activity = activity;
        buttonsLayout = (ConstraintLayout) activity.getLayoutInflater().inflate(R.layout.wn_buttons, null);
        activity.addContentView(buttonsLayout, new ConstraintLayout.LayoutParams(-1, -1));

        changer = activity.findViewById(R.id.btn_sl);
        alt = activity.findViewById(R.id.btn_alt);
        tab = activity.findViewById(R.id.btn_tab);
        y = activity.findViewById(R.id.btn_y);
        n = activity.findViewById(R.id.btn_n);
        f = activity.findViewById(R.id.btn_f);

        initClickers();
        toggleVisibility(false);
    }

    public void toggleVisibility(Boolean state) {
        if(state)
            Util.ShowLayout(buttonsLayout, true);
        else
            Util.HideLayout(buttonsLayout, true);
    }

    public void toggleSpecial(Boolean state) {
        y.setVisibility(state? View.VISIBLE : View.GONE);
        n.setVisibility(state? View.VISIBLE : View.GONE);
        f.setVisibility(state? View.VISIBLE : View.GONE);

        changer.setBackground(state? Util.getRes(activity, R.drawable.ic_hud_btn_sl) : Util.getRes(activity, R.drawable.ic_hud_btn_sr));

        m_bSpecialKeysState = !m_bSpecialKeysState;
    }

    private void initClickers() {
        changer.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                changer.startAnimation(AnimationUtils.loadAnimation(activity, R.anim.click));
                toggleSpecial(!m_bSpecialKeysState);
            }
        });
        alt.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                alt.startAnimation(AnimationUtils.loadAnimation(activity, R.anim.click));
                Multiplayer.getInstance().specialCall(0);
            }
        });
        tab.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                tab.startAnimation(AnimationUtils.loadAnimation(activity, R.anim.click));
                Multiplayer.getInstance().specialCall(1);
            }
        });
        y.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                y.startAnimation(AnimationUtils.loadAnimation(activity, R.anim.click));
                Multiplayer.getInstance().specialCall(2);
            }
        });
        n.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                n.startAnimation(AnimationUtils.loadAnimation(activity, R.anim.click));
                Multiplayer.getInstance().specialCall(3);
            }
        });
        f.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                f.startAnimation(AnimationUtils.loadAnimation(activity, R.anim.click));
                Multiplayer.getInstance().specialCall(4);
            }
        });
    }
}
