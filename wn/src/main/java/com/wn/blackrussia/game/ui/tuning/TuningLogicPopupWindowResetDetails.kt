package com.wn.blackrussia.game.ui.tuning

import android.os.Handler
import android.os.Looper
import android.view.View
import android.widget.Button
import android.widget.ImageView
import com.wn.blackrussia.R

class TuningLogicPopupWindowResetDetails(
    private val root: BrTuning,
    private val view: View,
    private val selectorId: Int,
    private val typeLayout: Int,
    private val established: Int
) {

    private val handler = Handler(Looper.getMainLooper())

    fun start() {
        val buttonApplyReset = view.findViewById<Button>(R.id.button_apply_reset_details)
        val buttonCloseApplyReset = view.findViewById<ImageView>(R.id.button_close_submenu_apply)

        buttonCloseApplyReset.setOnClickListener { button ->
            button.startAnimation(root.animation())
            handler.postDelayed({ root.dismissPopupWindowResetDetails() }, CLOSE_DELAY)
        }

        buttonApplyReset.setOnClickListener { button ->
            button.startAnimation(root.animation())

            // Корень должен знать тип экрана до ухода запроса: по нему он решает,
            // что обновлять, когда сервер подтвердит сброс
            root.typeLayout = typeLayout

            handler.postDelayed({
                root.sendResetDetails(TuningConstants.OPERATION_RESET_DETAILS, selectorId, established)
                root.dismissPopupWindowResetDetails()
            }, CLOSE_DELAY)
        }
    }

    companion object {
        private const val CLOSE_DELAY = 250L
    }
}
