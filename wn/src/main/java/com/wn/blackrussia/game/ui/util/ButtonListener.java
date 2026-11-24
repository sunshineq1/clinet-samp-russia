package com.wn.blackrussia.game.ui.util;

import android.animation.AnimatorInflater;
import android.animation.AnimatorSet;
import android.content.Context;
import android.view.MotionEvent;
import android.view.View;

import com.wn.blackrussia.R;

public class ButtonListener implements View.OnTouchListener {
    private final Context mContext;
    private final View mViewToReduce;
    private final AnimatorSet reducer;
    private final AnimatorSet regainer;

    public ButtonListener(Context context, View view) {
        //
        // Created by weikton on 21.03.2025
        //
        mContext = context;
        mViewToReduce = view;

        mViewToReduce.setClickable(true);

        reducer = (AnimatorSet) AnimatorInflater.loadAnimator(context, R.animator.reduce_size);
        regainer = (AnimatorSet) AnimatorInflater.loadAnimator(context, R.animator.regain_size);
    }

    @Override
    public boolean onTouch(View view, MotionEvent motionEvent) {
        int action = motionEvent.getAction() & 255;
        if (action == 0) {
            if (regainer.isRunning())
                regainer.end();

            reducer.setTarget(mViewToReduce);
            reducer.start();

            return false;
        } else if (action != 1 && action != 3) {
            return false;
        } else {
            if (reducer.isRunning())
                reducer.end();

            regainer.setTarget(mViewToReduce);
            regainer.start();

            return false;
        }
    }
}
