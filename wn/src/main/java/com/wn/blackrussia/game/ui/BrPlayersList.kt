package com.wn.blackrussia.game.ui

import android.text.Editable
import android.text.TextWatcher
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.inputmethod.InputMethodManager
import android.widget.TextView
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.wn.blackrussia.R
import com.wn.blackrussia.databinding.PlayersListDialogBinding
import com.wn.blackrussia.game.Multiplayer
import org.json.JSONObject

class BrPlayersList : NativeGui<PlayersListDialogBinding>(
    PlayersListDialogBinding::class,
    hud = false,
    chat = false
) {
    private data class Player(val id: Int, val nick: String, val level: Int, val ping: Int)

    private val players = mutableListOf<Player>()
    private val shown = mutableListOf<Player>()

    private val adapter = PlayerAdapter()

    private var searchText = ""
    private var selectedId = -1

    private val refresh = object : Runnable {
        override fun run() {
            requestData()
            binding.root.postDelayed(this, REFRESH_INTERVAL_MS)
        }
    }

    init {
        Multiplayer.getInstance().runOnUiThread { initUI() }
    }

    private fun initUI() {
        binding.playersList.layoutManager =
            LinearLayoutManager(Multiplayer.getInstance(), LinearLayoutManager.VERTICAL, false)
        binding.playersList.adapter = adapter

        binding.searchView.addTextChangedListener(object : TextWatcher {
            override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}
            override fun afterTextChanged(s: Editable?) {}

            override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {
                searchText = s?.toString().orEmpty()
                updateSearchIcons()
                applyFilter()
            }
        })

        binding.iconClearSearchText.setOnClickListener {
            binding.searchView.setText("")

            val imm = Multiplayer.getInstance()
                .getSystemService(android.content.Context.INPUT_METHOD_SERVICE) as InputMethodManager
            imm.hideSoftInputFromWindow(binding.root.windowToken, 0)

            binding.searchView.clearFocus()
        }

        binding.playersListClose.setOnClickListener { closeWindow() }

        updateSearchIcons()
        updateCounter()

        binding.root.bringToFront()
        binding.root.elevation = OVERLAY_ELEVATION

        requestData()
        binding.root.postDelayed(refresh, REFRESH_INTERVAL_MS)
    }

    private fun updateSearchIcons() {
        if (searchText.isEmpty()) {
            binding.iconSearchView.visibility = View.VISIBLE
            binding.iconClearSearchText.visibility = View.INVISIBLE
            binding.searchView.setHint(R.string.players_list_search_hint)
        } else {
            binding.iconSearchView.visibility = View.INVISIBLE
            binding.iconClearSearchText.visibility = View.VISIBLE
        }
    }

    private fun updateCounter() {
        binding.playersListValuePlayers.text = "$TEXT_PLAYERS${players.size}"
    }

    private fun applyFilter() {
        shown.clear()

        if (searchText.isEmpty()) {
            shown.addAll(players)
        } else {
            val needle = searchText.lowercase()
            shown.addAll(players.filter {
                it.nick.lowercase().contains(needle) || it.id.toString().contains(needle)
            })
        }

        adapter.notifyDataSetChanged()
    }

    private fun requestData() {
        sendPacket(0, JSONObject().put(KEY_ACTION, ACTION_REQUEST).toString())
    }

    private fun closeWindow() {
        sendPacket(0, JSONObject().put(KEY_ACTION, ACTION_CLOSED).toString())
        destroy()
    }

    override fun destroy() {
        binding.root.removeCallbacks(refresh)
        super.destroy()
    }

    override fun receivePacket(actionId: Int, json: String) {
        if (json.isEmpty()) {
            return
        }

        val packet = runCatching { JSONObject(json) }.getOrNull() ?: return
        if (packet.optInt(KEY_ACTION) != ACTION_REQUEST) {
            return
        }

        val data = packet.optJSONArray(KEY_DATA) ?: return

        players.clear()
        for (i in 0 until data.length()) {
            val item = data.optJSONObject(i) ?: continue
            players += Player(
                id = item.optInt("id"),
                nick = item.optString("nick"),
                level = item.optInt("level"),
                ping = item.optInt("ping")
            )
        }

        applyFilter()
        updateCounter()
    }

    private inner class PlayerAdapter : RecyclerView.Adapter<PlayerAdapter.Holder>() {

        inner class Holder(view: View) : RecyclerView.ViewHolder(view) {
            val background: View = view.findViewById(R.id.background_item)
            val id: TextView = view.findViewById(R.id.player_id)
            val name: TextView = view.findViewById(R.id.player_name)
            val level: TextView = view.findViewById(R.id.player_level)
            val ping: TextView = view.findViewById(R.id.player_ping)
        }

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Holder {
            val view = LayoutInflater.from(parent.context)
                .inflate(R.layout.players_list_item, parent, false)
            return Holder(view)
        }

        override fun getItemCount() = shown.size

        override fun onBindViewHolder(holder: Holder, position: Int) {
            val player = shown[position]

            holder.id.text = player.id.toString()
            holder.name.text = player.nick
            holder.level.text = player.level.toString()
            holder.ping.text = player.ping.toString()

            holder.background.isSelected = player.id == selectedId

            holder.itemView.setOnClickListener {
                val previous = shown.indexOfFirst { it.id == selectedId }
                selectedId = player.id

                if (previous >= 0) {
                    notifyItemChanged(previous)
                }
                notifyItemChanged(position)
            }
        }
    }

    private companion object {
        const val KEY_ACTION = "t"
        const val KEY_DATA = "data"

        const val ACTION_REQUEST = 1
        const val ACTION_CLOSED = 2

        const val REFRESH_INTERVAL_MS = 5000L
        const val OVERLAY_ELEVATION = 100.0f
        const val TEXT_PLAYERS = "Онлайн: "
    }
}
