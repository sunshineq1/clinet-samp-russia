package com.wn.blackrussia.game.ui.tuning

import android.os.Handler
import android.os.Looper
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.animation.Animation
import android.view.animation.AnimationUtils
import com.wn.blackrussia.R
import com.wn.blackrussia.databinding.TuningFragmentInfoBinding
import com.wn.blackrussia.game.common.UILayout

class UILayoutTuningViewInfoParameters(private val screen: BrTuning) : UILayout() {

    private var binding: TuningFragmentInfoBinding? = null

    private lateinit var anim: Animation

    private val handler = Handler(Looper.getMainLooper())

    override fun getView(): View? = binding?.root

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup): View {
        val view = TuningFragmentInfoBinding.inflate(inflater)
        binding = view

        anim = AnimationUtils.loadAnimation(context, R.anim.button_click)

        view.backToSubmenuInfo.setOnClickListener { button ->
            button.startAnimation(anim)
            handler.postDelayed({ screen.listenerLayout(BrTuning.LAYOUT_DETAILS_LIST) }, CLICK_DELAY)
        }

        view.tuningViewBox.setOnClickListener { button ->
            button.startAnimation(anim)

            handler.postDelayed({
                screen.listenerLayout(BrTuning.LAYOUT_OPENING_BOX)
                setVisibleThisWindow(false)
            }, CLICK_DELAY)
        }

        view.buttonBackDetailsInfoText.setOnClickListener { button ->
            button.startAnimation(anim)
            handler.postDelayed({ screen.listenerLayout(BrTuning.LAYOUT_DETAILS_LIST) }, CLICK_DELAY)
        }

        return view.root
    }

    override fun onLayoutShown() {}

    override fun onLayoutClose() {}

    // Окно прячется, а не снимается: коробка открывается поверх него и потом возвращает его обратно
    fun setVisibleThisWindow(isVisible: Boolean) {
        binding?.root?.visibility = if (isVisible) View.VISIBLE else View.INVISIBLE
    }

    private companion object {
        const val CLICK_DELAY = 250L
    }
}
