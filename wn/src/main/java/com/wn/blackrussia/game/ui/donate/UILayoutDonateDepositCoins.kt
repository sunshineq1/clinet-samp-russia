package com.wn.blackrussia.game.ui.donate

import android.text.Editable
import android.text.TextWatcher
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.wn.blackrussia.R
import com.wn.blackrussia.game.Multiplayer
import com.wn.blackrussia.game.ui.util.KUtils
import com.wn.blackrussia.game.ui.util.UILayout

class UILayoutDonateDepositCoins(private val screen: BrDonate) : UILayout() {

    private var root: View? = null

    private lateinit var input: EditText
    private lateinit var email: EditText
    private lateinit var payButton: Button
    private lateinit var salesText: TextView

    private var amount = 0

    // Кнопка оживает только когда заполнены и сумма, и почта
    private var amountEmpty = true
    private var emailEmpty = true

    override fun getView(): View? = root

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup): View {
        val view = inflater.inflate(R.layout.donate_coins_deposit, null, false)
        root = view

        input = view.findViewById(R.id.value)
        email = view.findViewById(R.id.email)
        payButton = view.findViewById(R.id.button)
        salesText = view.findViewById(R.id.sales)

        salesText.visibility = View.GONE

        view.findViewById<RecyclerView>(R.id.rv).apply {
            layoutManager = LinearLayoutManager(Multiplayer.getInstance())
            adapter = AdvantagesAdapter()
        }

        input.addTextChangedListener(watcher {
            amount = input.text.toString().toIntOrNull() ?: 0
            amountEmpty = amount <= 0

            payButton.text = BrDonate.formatPrice(amount) + " ₽"

            if (!amountEmpty) {
                screen.requestQuote(amount)
            }

            updateButtonState()
        })

        email.addTextChangedListener(watcher {
            emailEmpty = email.text.isEmpty()
            updateButtonState()
        })

        payButton.setOnClickListener {
            if (!amountEmpty && !emailEmpty) {
                screen.requestDeposit(amount)
            }
        }

        updateButtonState()

        return view
    }

    override fun onLayoutShown() {}

    override fun onLayoutClose() {}

    private fun watcher(onChanged: () -> Unit) = object : TextWatcher {
        override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}
        override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {}
        override fun afterTextChanged(s: Editable?) = onChanged()
    }

    private fun updateButtonState() {
        payButton.setBackgroundResource(
            if (amountEmpty || emailEmpty) R.drawable.donate_tile_button_greyp
            else R.drawable.donate_tile_button
        )
    }

    fun setInputCoins(coins: Int) {
        if (root == null) {
            return
        }

        input.setText(coins.toString())
    }

    // Ответ сервера: сколько Black Coins выйдет с учётом акции
    fun setCoins(coins: Int) {
        if (root == null) {
            return
        }

        salesText.visibility = View.VISIBLE
        salesText.text = "С учётом акции: " + BrDonate.formatPrice(coins) + " BC"
    }

    // Список под полями — это преимущества пополнения из витрины, а не суммы
    private inner class AdvantagesAdapter : RecyclerView.Adapter<AdvantagesAdapter.Holder>() {

        private val items = DonateCatalog.page(DonateCatalog.PAGE_ADVANTAGES)

        inner class Holder(view: View) : RecyclerView.ViewHolder(view) {
            val text: TextView = view.findViewById(R.id.text)
        }

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Holder {
            val view = LayoutInflater.from(parent.context)
                .inflate(R.layout.donate_coins_hot_item, parent, false)
            return Holder(view)
        }

        override fun getItemCount() = items.size

        override fun onBindViewHolder(holder: Holder, position: Int) {
            holder.text.text = KUtils.transfromColors(items[position].header)
        }
    }
}
