package com.wn.blackrussia.game.ui.tuning

import android.annotation.SuppressLint
import android.os.Handler
import android.os.Looper
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.animation.Animation
import android.view.animation.AnimationUtils
import com.wn.blackrussia.R
import com.wn.blackrussia.databinding.TuningFragmentViewCarBinding
import com.wn.blackrussia.game.Multiplayer
import com.wn.blackrussia.game.common.UILayout

class UILayoutTuningViewCar(private val screen: BrTuning) : UILayout() {

    private var binding: TuningFragmentViewCarBinding? = null

    private lateinit var anim: Animation

    private val handler = Handler(Looper.getMainLooper())

    private var doorOpened = false
    private var lightOn = false

    override fun getView(): View? = binding?.root

    @SuppressLint("ClickableViewAccessibility")
    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup): View {
        val view = TuningFragmentViewCarBinding.inflate(inflater)
        binding = view

        anim = AnimationUtils.loadAnimation(context, R.anim.button_click)

        view.buttonDoorView.setOnClickListener { button ->
            button.startAnimation(anim)

            if (doorOpened) {
                setDoorState(false, R.drawable.tuning_door_off)
                screen.sendAction(TuningConstants.OPERATION_CLOSE_DOOR)
            } else {
                setDoorState(true, R.drawable.tuning_door_on)
                screen.sendAction(TuningConstants.OPERATION_OPEN_DOOR)
            }
        }

        view.buttonLightView.setOnClickListener { button ->
            button.startAnimation(anim)

            if (lightOn) {
                setLightState(false, R.drawable.tuning_light_off)
                screen.sendAction(TuningConstants.OPERATION_TURN_OFF_LIGHT)
            } else {
                setLightState(true, R.drawable.tuning_light_on)
                screen.sendAction(TuningConstants.OPERATION_TURN_ON_LIGHT)
            }
        }

        view.backMenuView.setOnClickListener { button ->
            button.startAnimation(anim)

            handler.postDelayed({
                screen.listenerButtonBackFromViewCar()
                screen.sendAction(TuningConstants.OPERATION_CLOSE_VIEW_CAR)
            }, CLICK_DELAY)
        }

        // Экран осмотра поверх игры: касания уходят в движок, ими игрок крутит камеру вокруг машины
        view.root.setOnTouchListener { _, event ->
            val x = IntArray(POINTERS)
            val y = IntArray(POINTERS)

            for (index in 0 until event.pointerCount) {
                val pointerId = event.getPointerId(index)
                if (pointerId >= POINTERS) continue

                x[pointerId] = event.getX(index).toInt()
                y[pointerId] = event.getY(index).toInt()
            }

            Multiplayer.getInstance().customMultiTouchEvent(
                event.actionMasked, event.getPointerId(event.actionIndex),
                x[0], y[0], x[1], y[1], x[2], y[2]
            )

            true
        }

        return view.root
    }

    override fun onLayoutShown() {}

    override fun onLayoutClose() {}

    private fun setDoorState(opened: Boolean, image: Int) {
        doorOpened = opened
        binding?.buttonDoorView?.setImageResource(image)
    }

    private fun setLightState(on: Boolean, image: Int) {
        lightOn = on
        binding?.buttonLightView?.setImageResource(image)
    }

    private companion object {
        const val CLICK_DELAY = 250L
        const val POINTERS = 3
    }
}
