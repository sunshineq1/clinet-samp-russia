package com.wn.blackrussia.game.ui;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.ForegroundColorSpan;
import android.util.Log;
import android.view.View;
import android.widget.*;

import androidx.constraintlayout.widget.ConstraintLayout;

import com.triggertrap.seekarc.SeekArc;
import com.wn.blackrussia.R;
import com.wn.blackrussia.game.ui.util.Util;

public class BrSpeed extends FrameLayout {
    public Activity activity;
    public ConstraintLayout mSpeedometer;

    public TextView mSpeed, mSP_Fuel, mSP_Repair;
    public SeekArc mSpeedLine;
    public ImageView mSPM_EngineOff, mSPM_DoorsOff, mSPM_EngineOn, mSPM_DoorsOn,
            mSPM_Turn, mSPM_TurnCLICK, mSPM_TurnTop,
            mSPM_Left, mSPM_Right;

    static final int TURN_SIGNAL_LEFT = 1;
    static final int TURN_SIGNAL_RIGHT = 2;
    static final int TURN_SIGNAL_ALL = 3;

    public Boolean m_bEnabled = false, m_bUI = false;

    public native void specialSpeedometerCall(int id);

    public BrSpeed(Activity aactivity) {
        super(aactivity);
        activity = aactivity;

        mSpeedometer = (ConstraintLayout) activity.getLayoutInflater().inflate(R.layout.br_speed, null);
        activity.addContentView(mSpeedometer, new ConstraintLayout.LayoutParams(-1, -1));

        mSpeedLine = activity.findViewById(R.id.speed_line);
        mSpeed = activity.findViewById(R.id.speed_text);

        mSP_Fuel = activity.findViewById(R.id.speed_fuel_text2);
        mSP_Repair = activity.findViewById(R.id.speed_car_hp_text);

        mSPM_EngineOff = activity.findViewById(R.id.speed_engine_ico_red);
        mSPM_DoorsOff = activity.findViewById(R.id.speed_lock_ico_green);
        mSPM_EngineOn = activity.findViewById(R.id.speed_engine_ico_green);
        mSPM_DoorsOn = activity.findViewById(R.id.speed_lock_ico_red);

        mSPM_TurnCLICK = activity.findViewById(R.id.imageView5);
        mSPM_Turn = activity.findViewById(R.id.oldBlink);
        mSPM_TurnTop = activity.findViewById(R.id.blinker);
        mSPM_Left = activity.findViewById(R.id.left_nactive);
        mSPM_Right = activity.findViewById(R.id.right_nactive);

        mSPM_TurnCLICK.setOnClickListener(v -> {
            specialSpeedometerCall(0);
        });
        mSPM_Left.setOnClickListener(v -> {
            specialSpeedometerCall(1);
        });
        mSPM_Right.setOnClickListener(v -> {
            specialSpeedometerCall(2);
        });
    }

    public void UpdateSpeedInfo(
            int speed, int fuel, int hp, int engine, int light, int lock, int turnlight) {

        String formattedSpeed = String.format("%03d", speed);
        SpannableString spannableSpeed = new SpannableString(formattedSpeed);

        if (speed == 0) {
            spannableSpeed.setSpan(new ForegroundColorSpan(Color.parseColor("#4DFFFFFF")),
                    0, formattedSpeed.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        } else {
            int leadingZeros = 0;
            if (speed < 100)
                leadingZeros++;
            if (speed < 10)
                leadingZeros++;

            for (int i = 0; i < leadingZeros; i++) {
                spannableSpeed.setSpan(new ForegroundColorSpan(Color.parseColor("#4DFFFFFF")),
                        i, i + 1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            }

            spannableSpeed.setSpan(new ForegroundColorSpan(Color.WHITE),
                    leadingZeros, formattedSpeed.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }

        mSpeed.setText(spannableSpeed);

        hp /= 10;

        if (fuel > 100)
            fuel = 100;
        else if (fuel < 0)
            fuel = 0;

        String formattedFuel = String.format("%03d", fuel);
        SpannableString spannableFuel = new SpannableString(formattedFuel);

        if (fuel == 0) {
            spannableFuel.setSpan(new ForegroundColorSpan(Color.parseColor("#4DFFFFFF")),
                    0, formattedFuel.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        } else {
            int leadingZeros2 = 0;
            if (fuel < 100)
                leadingZeros2++;
            if (fuel < 10)
                leadingZeros2++;

            for (int i = 0; i < leadingZeros2; i++) {
                if (formattedFuel.charAt(i) == '0') {
                    spannableFuel.setSpan(new ForegroundColorSpan(Color.parseColor("#4DFFFFFF")),
                            i, i + 1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                }
            }

            int spanStartIndex2 = leadingZeros2;
            int spanEndIndex2 = formattedFuel.length();
            spannableFuel.setSpan(new ForegroundColorSpan(Color.WHITE),
                    spanStartIndex2, spanEndIndex2,
                    Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }

        mSP_Fuel.setText(spannableFuel);

        mSP_Repair.setText(String.valueOf(hp) + "%");

        if(engine == 1) {
            mSPM_EngineOff.setVisibility(GONE);
            mSPM_EngineOn.setVisibility(VISIBLE);
        } else {
            mSPM_EngineOff.setVisibility(VISIBLE);
            mSPM_EngineOn.setVisibility(GONE);
        }
        if(lock == 1) {
            mSPM_DoorsOn.setVisibility(GONE);
            mSPM_DoorsOff.setVisibility(VISIBLE);
        } else {
            mSPM_DoorsOn.setVisibility(VISIBLE);
            mSPM_DoorsOff.setVisibility(GONE);
        }

        mSpeedLine.setProgress(speed);

        activity.runOnUiThread(() -> {
            boolean all   = (turnlight == TURN_SIGNAL_ALL);
            boolean left  = all || (turnlight == TURN_SIGNAL_LEFT);
            boolean right = all || (turnlight == TURN_SIGNAL_RIGHT);

            mSPM_TurnTop.setBackground(Util.getRes(activity,
                    all ? R.drawable.speedo_ablinker : R.drawable.speedo_blinker));
            mSPM_Turn.setVisibility(all ? VISIBLE : INVISIBLE);
            mSPM_Left.setBackground(Util.getRes(activity,
                    left ? R.drawable.speedo_arrowa_left : R.drawable.speedo_arrow_left));
            mSPM_Right.setBackground(Util.getRes(activity,
                    right ? R.drawable.speedo_arrowa : R.drawable.speedo_arrow));
        });
    }

    public void toggleVisibility(boolean value) {
        if(m_bUI)
            return;
        activity.runOnUiThread(() -> {
            if(value && mSpeedometer.getVisibility() == View.GONE) {
                m_bEnabled = true;
                Util.ShowLayout(mSpeedometer, true);
            }

            if(!value && mSpeedometer.getVisibility() == View.VISIBLE) {
                m_bEnabled = false;
                Util.HideLayout(mSpeedometer, false);
            }
        });
    }

}
