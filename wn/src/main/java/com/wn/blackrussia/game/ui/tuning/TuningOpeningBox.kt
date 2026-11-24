package com.wn.blackrussia.game.ui.tuning

import android.animation.AnimatorInflater
import android.animation.AnimatorSet
import android.os.Handler
import android.os.Looper
import android.view.View
import android.view.animation.Animation
import android.view.animation.AnimationUtils
import android.widget.Button
import android.widget.LinearLayout
import android.widget.RelativeLayout
import android.widget.TextView
import androidx.constraintlayout.widget.ConstraintLayout
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.wn.blackrussia.R

class TuningOpeningBox(private val root: BrTuning, private val view: View) {

    private lateinit var listBoxItems: RecyclerView
    private lateinit var buttonExit: Button
    private lateinit var buttonOpenBox: ConstraintLayout
    private lateinit var valuePrice: TextView
    private lateinit var valueCurrency: TextView
    private lateinit var nameThisBox: TextView
    private lateinit var nameBoxInCard: TextView
    private lateinit var nameDetailBox: TextView
    private lateinit var typeDetailBox: TextView
    private lateinit var blockBuying: LinearLayout
    private lateinit var blockOpening: RelativeLayout
    private lateinit var cardInvisible: RelativeLayout
    private lateinit var cardVisible: RelativeLayout
    private lateinit var cardBackground: View

    private val context = view.context
    private val anim: Animation = AnimationUtils.loadAnimation(context, R.anim.button_click)
    private val handler = Handler(Looper.getMainLooper())

    var statusOpeningBox = 0

    fun startOpeningBox(cost: Int, currency: Int) {
        findViews()
        setBoxListInView()
        setListeners()
        setDataInView(cost, currency)
    }

    private fun findViews() {
        listBoxItems = view.findViewById(R.id.recv_box_items)
        buttonExit = view.findViewById(R.id.button_exit_card_view)
        buttonOpenBox = view.findViewById(R.id.tuning_price_box)
        valuePrice = view.findViewById(R.id.tuning_value_price_box)
        valueCurrency = view.findViewById(R.id.tuning_currency_box)
        nameThisBox = view.findViewById(R.id.name_this_box)
        nameBoxInCard = view.findViewById(R.id.title_check_box_invisible)
        nameDetailBox = view.findViewById(R.id.name_detail_in_box)
        typeDetailBox = view.findViewById(R.id.type_detail_in_box)
        blockBuying = view.findViewById(R.id.view_box_buying)
        blockOpening = view.findViewById(R.id.block_open_cards)
        cardInvisible = view.findViewById(R.id.box_card_invisible)
        cardVisible = view.findViewById(R.id.box_card_visible)
        cardBackground = view.findViewById(R.id.bg_box_open_card)
    }

    private fun setBoxListInView() {
        listBoxItems.adapter = TuningBoxAdapter(root.namesForBox)
        listBoxItems.layoutManager = LinearLayoutManager(context, LinearLayoutManager.VERTICAL, false)
    }

    private fun setDataInView(boxCost: Int, currency: Int) {
        val currencyName = when (currency) {
            TuningConstants.CURRENCY_RUB_MONEY -> {
                buttonOpenBox.visibility = View.VISIBLE
                RUBLE
            }

            TuningConstants.CURRENCY_BC_MONEY -> {
                buttonOpenBox.visibility = View.VISIBLE
                BLACK_COIN
            }

            else -> {
                buttonOpenBox.visibility = View.INVISIBLE
                ""
            }
        }

        nameThisBox.text = BOX_NAME
        valuePrice.text = BrTuning.priceWithSpaces(boxCost)
        valueCurrency.text = currencyName
        nameBoxInCard.text = BOX_NAME
    }

    // На карточке сверху идёт группа детали, снизу — её собственное название
    fun setDataOpeningBox(typeDetail: String, nameDetail: String) {
        nameDetailBox.text = typeDetail
        typeDetailBox.text = nameDetail
    }

    private fun setListeners() {
        buttonExit.setOnClickListener { button ->
            button.startAnimation(anim)
            handler.postDelayed({ root.closeBoxCardView() }, CLICK_DELAY)
        }

        buttonOpenBox.setOnClickListener { button ->
            button.startAnimation(anim)
            handler.postDelayed({
                root.layoutDetailsList.buyThisDetail?.let { detail ->
                    root.sendIntegerData(TuningConstants.OPERATION_OPEN_BOX, detail.id)
                }
            }, CLICK_DELAY)
        }

        cardInvisible.setOnClickListener { flipCard() }

        cardVisible.setOnClickListener { root.closeBoxCardView() }
    }

    private fun flipCard() {
        statusOpeningBox = 0

        val flipOut = AnimatorInflater.loadAnimator(context, R.animator.flip_out) as AnimatorSet
        flipOut.setTarget(cardInvisible)

        val flipIn = AnimatorInflater.loadAnimator(context, R.animator.flip_in) as AnimatorSet
        flipIn.setTarget(cardVisible)

        cardBackground.visibility = View.INVISIBLE
        flipOut.start()
        flipIn.start()
        cardVisible.visibility = View.VISIBLE

        handler.postDelayed({ cardInvisible.visibility = View.GONE }, HIDE_CARD_DELAY)

        handler.postDelayed({
            cardBackground.visibility = View.VISIBLE
            cardBackground.background =
                ContextCompat.getDrawable(context, R.drawable.tuning_bg_box_card_visible)
        }, SHOW_LIGHT_DELAY)
    }

    fun playOpening() {
        blockBuying.visibility = View.GONE
        blockOpening.visibility = View.VISIBLE
    }

    private companion object {
        const val BOX_NAME = "С деталями"
        const val RUBLE = "₽"
        const val BLACK_COIN = "BC"

        const val CLICK_DELAY = 250L
        const val HIDE_CARD_DELAY = 800L
        const val SHOW_LIGHT_DELAY = 1500L
    }
}
