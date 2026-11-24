package com.kingfisher.easyviewindicator;

import android.animation.Animator;
import android.animation.AnimatorInflater;
import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.view.View;
import android.view.animation.Interpolator;
import android.widget.LinearLayout;

import com.wn.blackrussia.R;

public class AnyViewIndicator extends LinearLayout {
    private static final int DEFAULT_INDICATOR_SIZE_DP = 5;

    private int currentPosition;
    private int itemCount;

    private boolean isAnimationEnable = true;

    private Animator mAnimatorIn;
    private Animator mAnimatorOut;
    private Animator mImmediateAnimatorIn;
    private Animator mImmediateAnimatorOut;

    private int mAnimatorResId = R.animator.scale_with_alpha;
    private int mAnimatorReverseResId = 0;

    private int mIndicatorBackgroundResId = R.drawable.white_radius;
    private int mIndicatorUnselectedBackgroundResId = R.drawable.white_radius;

    private int mIndicatorWidth = -1;
    private int mIndicatorHeight = -1;
    private int mIndicatorMargin = -1;

    protected int mLastPosition = -1;

    public AnyViewIndicator(Context context) {
        super(context);
        init(context, null);
    }

    public AnyViewIndicator(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context, attrs);
    }

    public AnyViewIndicator(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context, attrs);
    }

    public AnyViewIndicator(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        init(context, attrs);
    }

    private void init(Context context, AttributeSet attrs) {
        handleTypedArray(context, attrs);
        checkIndicatorConfig(context);
    }

    private void handleTypedArray(Context context, AttributeSet attrs) {
        if (attrs == null) {
            return;
        }

        TypedArray array = context.obtainStyledAttributes(attrs, R.styleable.AnyViewIndicator);

        mIndicatorWidth = array.getDimensionPixelSize(R.styleable.AnyViewIndicator_avi_width, -1);
        mIndicatorHeight = array.getDimensionPixelSize(R.styleable.AnyViewIndicator_avi_height, -1);
        mIndicatorMargin = array.getDimensionPixelSize(R.styleable.AnyViewIndicator_avi_margin, -1);

        mAnimatorResId = array.getResourceId(R.styleable.AnyViewIndicator_avi_animator, R.animator.scale_with_alpha);
        mAnimatorReverseResId = array.getResourceId(R.styleable.AnyViewIndicator_avi_animator_reverse, 0);

        mIndicatorBackgroundResId = array.getResourceId(R.styleable.AnyViewIndicator_avi_drawable, R.drawable.white_radius);
        mIndicatorUnselectedBackgroundResId = array.getResourceId(
                R.styleable.AnyViewIndicator_avi_drawable_unselected, mIndicatorBackgroundResId);

        isAnimationEnable = array.getBoolean(R.styleable.AnyViewIndicator_avi_animation_enable, true);

        setOrientation(array.getInt(R.styleable.AnyViewIndicator_avi_orientation, -1) == 1 ? VERTICAL : HORIZONTAL);

        int gravity = array.getInt(R.styleable.AnyViewIndicator_avi_gravity, -1);
        setGravity(gravity < 0 ? android.view.Gravity.CENTER : gravity);

        array.recycle();
    }

    public void configureIndicator(int width, int height, int margin) {
        configureIndicator(width, height, margin, R.animator.scale_with_alpha, 0,
                R.drawable.white_radius, R.drawable.white_radius);
    }

    public void configureIndicator(int width, int height, int margin, int animatorResId,
                                   int animatorReverseResId, int backgroundResId, int unselectedBackgroundResId) {
        mIndicatorWidth = width;
        mIndicatorHeight = height;
        mIndicatorMargin = margin;
        mAnimatorResId = animatorResId;
        mAnimatorReverseResId = animatorReverseResId;
        mIndicatorBackgroundResId = backgroundResId;
        mIndicatorUnselectedBackgroundResId = unselectedBackgroundResId;

        checkIndicatorConfig(getContext());
    }

    private void checkIndicatorConfig(Context context) {
        if (mIndicatorWidth < 0) mIndicatorWidth = dip2px(DEFAULT_INDICATOR_SIZE_DP);
        if (mIndicatorHeight < 0) mIndicatorHeight = dip2px(DEFAULT_INDICATOR_SIZE_DP);
        if (mIndicatorMargin < 0) mIndicatorMargin = dip2px(DEFAULT_INDICATOR_SIZE_DP);

        if (mAnimatorResId == 0) mAnimatorResId = R.animator.scale_with_alpha;

        mAnimatorOut = createAnimatorOut(context);

        mImmediateAnimatorOut = createAnimatorOut(context);
        mImmediateAnimatorOut.setDuration(0L);

        mAnimatorIn = createAnimatorIn(context);

        mImmediateAnimatorIn = createAnimatorIn(context);
        mImmediateAnimatorIn.setDuration(0L);

        if (mIndicatorBackgroundResId == 0) mIndicatorBackgroundResId = R.drawable.white_radius;
        if (mIndicatorUnselectedBackgroundResId == 0) mIndicatorUnselectedBackgroundResId = mIndicatorBackgroundResId;
    }

    private Animator createAnimatorOut(Context context) {
        return AnimatorInflater.loadAnimator(context, mAnimatorResId);
    }

    private Animator createAnimatorIn(Context context) {
        if (mAnimatorReverseResId != 0) {
            return AnimatorInflater.loadAnimator(context, mAnimatorReverseResId);
        }

        Animator animator = AnimatorInflater.loadAnimator(context, mAnimatorResId);
        animator.setInterpolator(new ReverseInterpolator());
        return animator;
    }

    protected void updateCircleIndicator() {
        int count = getItemCount();
        if (count == getChildCount()) {
            return;
        }

        mLastPosition = mLastPosition < count ? getCurrentPosition() : -1;

        if (mLastPosition == -1 && count > 0) {
            mLastPosition = 0;
        }

        createIndicators();
    }

    protected void onCurrentLocationChange() {
        if (isAnimationEnable) {
            if (mAnimatorIn.isRunning()) {
                mAnimatorIn.end();
                mAnimatorIn.cancel();
            }
            if (mAnimatorOut.isRunning()) {
                mAnimatorOut.end();
                mAnimatorOut.cancel();
            }
        }

        if (mLastPosition >= 0) {
            View previous = getChildAt(mLastPosition);
            if (previous != null) {
                previous.setBackgroundResource(mIndicatorUnselectedBackgroundResId);

                if (isAnimationEnable) {
                    mAnimatorIn.setTarget(previous);
                    mAnimatorIn.start();
                }
            }
        }

        int position = getCurrentPosition();
        View current = getChildAt(position);
        if (current != null) {
            current.setBackgroundResource(mIndicatorBackgroundResId);

            if (isAnimationEnable) {
                mAnimatorOut.setTarget(current);
                mAnimatorOut.start();
            }
        }

        mLastPosition = position;
    }

    protected int getItemCount() {
        return itemCount;
    }

    public void setItemCount(int count) {
        itemCount = count;
        updateCircleIndicator();
    }

    protected int getCurrentPosition() {
        return currentPosition;
    }

    public void setCurrentPosition(int position) {
        currentPosition = position;
        onCurrentLocationChange();
    }

    private void createIndicators() {
        removeAllViews();

        int count = getItemCount();
        if (count <= 0) {
            return;
        }

        int position = Math.max(getCurrentPosition(), 0);
        int orientation = getOrientation();

        for (int i = 0; i < count; i++) {
            if (position == i) {
                addIndicator(orientation, mIndicatorBackgroundResId, mImmediateAnimatorOut);
            } else {
                addIndicator(orientation, mIndicatorUnselectedBackgroundResId, mImmediateAnimatorIn);
            }
        }
    }

    private void addIndicator(int orientation, int backgroundResId, Animator animator) {
        if (isAnimationEnable && animator.isRunning()) {
            animator.end();
            animator.cancel();
        }

        View view = new View(getContext());
        view.setBackgroundResource(backgroundResId);
        addView(view, mIndicatorWidth, mIndicatorHeight);

        LayoutParams params = (LayoutParams) view.getLayoutParams();
        if (orientation == HORIZONTAL) {
            params.leftMargin = mIndicatorMargin;
            params.rightMargin = mIndicatorMargin;
        } else {
            params.topMargin = mIndicatorMargin;
            params.bottomMargin = mIndicatorMargin;
        }
        view.setLayoutParams(params);

        if (isAnimationEnable) {
            animator.setTarget(view);
            animator.start();
        }
    }

    public int dip2px(float dp) {
        return (int) ((dp * getResources().getDisplayMetrics().density) + 0.5f);
    }

    private static class ReverseInterpolator implements Interpolator {
        @Override
        public float getInterpolation(float input) {
            return Math.abs(1.0f - input);
        }
    }
}
