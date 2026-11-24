package com.wn.blackrussia.game.ui

import android.animation.Animator
import android.content.Context
import android.animation.AnimatorListenerAdapter
import android.graphics.Color
import android.graphics.PorterDuff
import android.text.Html
import android.view.LayoutInflater
import android.view.View
import android.view.animation.AnimationUtils
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.TextView
import androidx.core.content.res.ResourcesCompat
import androidx.core.view.isVisible
import blackrussia.online.model.Servers
import blackrussia.online.other.Lists
import com.akexorcist.roundcornerprogressbar.RoundCornerProgressBar
import com.wn.blackrussia.R
import com.wn.blackrussia.databinding.BrServerselectBinding
import com.wn.blackrussia.game.Multiplayer
import org.json.JSONObject

class BrChooseServer : NativeGui<BrServerselectBinding>(
    BrServerselectBinding::class,
    hud = false,
    chat = false
) {
    @Suppress("UNCHECKED_CAST")
    private val servers: List<Servers>
        get() = Lists.slist as List<Servers>

    private var selected = 0
    private var uiReady = false
    private var switchAfterInit = false

    init {
        Multiplayer.getInstance().runOnUiThread { initUI() }
    }

    private fun initUI() {
        bringToFront()

        binding.chooseServerRootLoading.visibility = View.VISIBLE
        binding.chooseServerRootChoose.visibility = View.GONE

        if (servers.isEmpty()) {
            // список ещё не пришёл из лаунчера — ждём и пробуем снова
            binding.root.postDelayed({ initUI() }, RETRY_DELAY_MS)
            return
        }

        selected = loadLastServer().coerceIn(0, servers.size - 1)

        binding.brServersPlay.setOnClickListener { view ->
            playClickAnimation(view)
            connect(selected)
        }

        binding.serverMainFrame.setOnClickListener { view -> playClickAnimation(view) }

        binding.listServersChoose.alpha = 0.0f
        binding.listServersChoose.visibility = View.GONE
        binding.mainServersChoose.visibility = View.VISIBLE

        binding.serversBtnMyServer.setOnClickListener { view ->
            playClickAnimation(view)
            view.background = drawable(R.drawable.button_red_rectangle)
            binding.allServersButton.background = drawable(R.drawable.button_br_red_unfilled_ss)

            if (!binding.mainServersChoose.isVisible) {
                crossFade(binding.listServersChoose, binding.mainServersChoose)
            }
        }

        binding.allServersButton.setOnClickListener { view ->
            playClickAnimation(view)
            view.background = drawable(R.drawable.button_red_rectangle)
            binding.serversBtnMyServer.background = drawable(R.drawable.button_br_red_unfilled_ss)

            if (!binding.listServersChoose.isVisible) {
                crossFade(binding.mainServersChoose, binding.listServersChoose)
            }
        }

        bindServerCard(binding.mainServersChoose, selected)
        buildServerList()

        uiReady = true

        if (switchAfterInit) {
            switchAfterInit = false
            showChooseScreen(animate = true)
        }
    }

    private fun bringToFront() {
        binding.root.bringToFront()
        binding.root.elevation = OVERLAY_ELEVATION
        (binding.root.parent as? View)?.requestLayout()
    }

    private fun playClickAnimation(view: View) {
        view.startAnimation(AnimationUtils.loadAnimation(Multiplayer.getInstance(), R.anim.button_click))
    }

    private fun drawable(id: Int) =
        ResourcesCompat.getDrawable(Multiplayer.getInstance().resources, id, null)

    private fun crossFade(from: View, to: View) {
        from.animate().alpha(0.0f).setDuration(100L)
            .setListener(object : AnimatorListenerAdapter() {
                override fun onAnimationEnd(animation: Animator) {
                    from.alpha = 0.0f
                    from.visibility = View.GONE

                    to.visibility = View.VISIBLE
                    to.alpha = 0.0f
                    to.animate().alpha(1.0f).setDuration(100L).setListener(null).start()
                }
            }).start()
    }

    private fun bindServerCard(card: View, index: Int) {
        val server = servers.getOrNull(index) ?: return

        card.findViewById<TextView>(R.id.br_server_name).text = server.getname()

        if (server.getname().contains("test")) {
            return
        }

        val color = Color.parseColor("#" + server.color)

        card.findViewById<View>(R.id.server_list_back_color)
            .background.setColorFilter(color, PorterDuff.Mode.SRC_ATOP)

        card.findViewById<RoundCornerProgressBar>(R.id.br_server_progress).apply {
            progressColor = color
            progress = onlinePercent(server)
        }

        card.findViewById<ImageView>(R.id.br_server_image)
            .setColorFilter(color, PorterDuff.Mode.SRC_ATOP)

        card.findViewById<TextView>(R.id.br_server_online).text =
            Html.fromHtml("${server.online}<font color='#808080'>/${server.getmaxOnline()}")
    }

    private fun onlinePercent(server: Servers): Float {
        val max = server.getmaxOnline()
        if (max <= 0) {
            return 0.0f
        }
        return (server.online.toDouble() / max.toDouble() * 100.0).toInt().toFloat()
    }

    private fun buildServerList() {
        val activity = Multiplayer.getInstance()
        val inflater = LayoutInflater.from(activity)
        val container = binding.scrollLayoutServers

        container.removeAllViews()
        container.isScrollbarFadingEnabled = false

        val rows = servers.size / COLUMNS + 1

        for (row in 0 until rows) {
            val line = LinearLayout(activity)
            line.layoutParams = LinearLayout.LayoutParams(-1, -1).apply {
                weight = 1.0f / rows
                if (row < rows - 1) {
                    setMargins(0, 0, 0, dp(12.0f))
                }
            }
            line.orientation = LinearLayout.HORIZONTAL

            for (column in 0 until COLUMNS) {
                val index = row * COLUMNS + column
                if (index >= servers.size) {
                    break
                }

                val card = inflater.inflate(R.layout.br_serverselect_server, null, false)
                card.layoutParams = LinearLayout.LayoutParams(dp(160.0f), dp(80.0f)).apply {
                    weight = 0.25f
                    if (row < 5) {
                        setMargins(0, 0, dp(20.0f), 0)
                    }
                }

                card.setOnClickListener { view ->
                    playClickAnimation(view)
                    connect(index)
                }

                bindServerCard(card, index)
                line.addView(card)
            }

            container.addView(line)
        }
    }

    private fun dp(value: Float) =
        (value * Multiplayer.getInstance().resources.displayMetrics.density).toInt()

    private fun loadLastServer(): Int =
        Multiplayer.getInstance()
            .getSharedPreferences(PREFS, Context.MODE_PRIVATE)
            .getInt(KEY_LAST_SERVER, 0)

    private fun saveLastServer(index: Int) {
        Multiplayer.getInstance()
            .getSharedPreferences(PREFS, Context.MODE_PRIVATE)
            .edit()
            .putInt(KEY_LAST_SERVER, index)
            .apply()
    }

    private fun connect(index: Int) {
        selected = index
        saveLastServer(index)

        sendPacket(0, JSONObject().put(FIELD_CHOICE, index).toString())
        Multiplayer.getInstance().sendServer(index)
    }

    fun hideLoading() {
        Multiplayer.getInstance().runOnUiThread { destroy() }
    }

    private fun setProgress(percent: Int) {
        binding.brLsProgress.text = "${percent.coerceAtMost(99)}%"
    }

    private fun showChooseScreen(animate: Boolean) {
        binding.chooseServerRootChoose.visibility = View.VISIBLE
        binding.chooseServerRootChoose.alpha = 0.0f

        if (!animate) {
            binding.chooseServerRootChoose.alpha = 1.0f
            binding.chooseServerRootLoading.visibility = View.GONE
            return
        }

        binding.chooseServerRootChoose.animate().setDuration(1500L).alpha(1.0f)
            .setListener(object : AnimatorListenerAdapter() {
                override fun onAnimationEnd(animation: Animator) {
                    binding.chooseServerRootLoading.visibility = View.GONE
                }
            }).start()
    }

    override fun receivePacket(actionId: Int, json: String) {
        if (json.isEmpty()) {
            return
        }

        val packet = runCatching { JSONObject(json) }.getOrNull() ?: return

        bringToFront()

        when (packet.optInt(FIELD_TYPE, -1)) {
            TYPE_PROGRESS -> setProgress(packet.optInt(FIELD_PROGRESS))

            TYPE_READY -> {
                if (!uiReady) {
                    switchAfterInit = true
                    return
                }
                showChooseScreen(animate = true)
            }
        }
    }

    private companion object {
        const val COLUMNS = 4
        const val RETRY_DELAY_MS = 300L
        const val OVERLAY_ELEVATION = 100.0f

        const val PREFS = "br_servers"
        const val KEY_LAST_SERVER = "last_server"

        const val FIELD_TYPE = "t"
        const val FIELD_PROGRESS = "s"
        const val FIELD_CHOICE = "b"

        const val TYPE_PROGRESS = 0
        const val TYPE_READY = 1
    }
}
