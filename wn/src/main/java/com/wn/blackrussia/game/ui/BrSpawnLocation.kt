package com.wn.blackrussia.game.ui

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.Toast
import android.widget.TextView
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.wn.blackrussia.R
import com.wn.blackrussia.databinding.SpawnLocationLayoutBinding
import com.wn.blackrussia.game.Multiplayer
import org.json.JSONObject

class BrSpawnLocation : NativeGui<SpawnLocationLayoutBinding>(
    SpawnLocationLayoutBinding::class,
    hud = false,
    chat = false
) {
    data class Place(val id: Int, val place: Int, val name: String) {
        var available = true
    }

    private val places = mutableListOf<Place>()
    private val adapter = PlaceAdapter()

    private var selectedId = -1

    init {
        Multiplayer.getInstance().runOnUiThread {
            loadPlaces()

            binding.recyclerViewSpawnLocation.layoutManager =
                LinearLayoutManager(Multiplayer.getInstance(), LinearLayoutManager.HORIZONTAL, false)
            binding.recyclerViewSpawnLocation.adapter = adapter

            binding.buttonEnter.setOnClickListener { confirm() }
        }
    }

    private fun loadPlaces() {
        val raw = Multiplayer.getInstance().resources
            .openRawResource(R.raw.spawn_location)
            .bufferedReader()
            .use { it.readText() }

        val array = JSONObject(raw).optJSONArray(FIELD_LIST) ?: return

        places.clear()
        for (i in 0 until array.length()) {
            val item = array.optJSONObject(i) ?: continue
            places += Place(
                id = item.optInt("id"),
                place = item.optInt("place"),
                name = item.optString("place_name")
            )
        }

        places.sortBy { it.place }
    }

    override fun receivePacket(actionId: Int, json: String) {
        if (json.isEmpty()) {
            return
        }

        val packet = runCatching { JSONObject(json) }.getOrNull() ?: return

        packet.optJSONArray(FIELD_AVAILABILITY)?.let { availability ->
            val allowed = HashSet<Int>(availability.length())
            for (i in 0 until availability.length()) {
                allowed += availability.optInt(i)
            }

            places.forEach { it.available = allowed.contains(it.id) }

            if (places.none { it.id == selectedId && it.available }) {
                selectedId = places.firstOrNull { it.available }?.id ?: -1
            }

            adapter.notifyDataSetChanged()
        }

        when (packet.optInt(FIELD_STATUS)) {
            STATUS_UNAVAILABLE -> Toast.makeText(
                Multiplayer.getInstance(), R.string.spawn_location_error, Toast.LENGTH_SHORT
            ).show()

            STATUS_CLOSE -> destroy()
        }
    }

    private fun confirm() {
        val place = places.firstOrNull { it.id == selectedId } ?: return
        if (!place.available) {
            return
        }

        sendPacket(0, JSONObject().put(FIELD_CHOICE, place.id).toString())
    }

    private inner class PlaceAdapter : RecyclerView.Adapter<PlaceAdapter.Holder>() {

        inner class Holder(view: View) : RecyclerView.ViewHolder(view) {
            val image: ImageView = view.findViewById(R.id.image_view_place)
            val name: TextView = view.findViewById(R.id.text_view_place_name)
            val selection: View = view.findViewById(R.id.view_selection)
            val unavailable: View = view.findViewById(R.id.view_selection_unavailable)
            val lock: ImageView = view.findViewById(R.id.iv_lock)
        }

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Holder {
            val view = LayoutInflater.from(parent.context)
                .inflate(R.layout.spawn_location_item, parent, false)
            return Holder(view)
        }

        override fun getItemCount() = places.size

        override fun onBindViewHolder(holder: Holder, position: Int) {
            val place = places[position]

            holder.image.setImageResource(PLACE_IMAGES.getOrElse(place.id) { PLACE_IMAGES[1] })
            holder.name.text = place.name
            holder.selection.visibility =
                if (place.id == selectedId) View.VISIBLE else View.INVISIBLE
            holder.unavailable.visibility = if (place.available) View.GONE else View.VISIBLE
            holder.lock.visibility = if (place.available) View.GONE else View.VISIBLE

            holder.itemView.setOnClickListener {
                if (!place.available || place.id == selectedId) {
                    return@setOnClickListener
                }

                val previous = places.indexOfFirst { it.id == selectedId }
                selectedId = place.id

                if (previous >= 0) {
                    notifyItemChanged(previous)
                }
                notifyItemChanged(position)
            }
        }
    }

    private companion object {
        val PLACE_IMAGES = intArrayOf(
            R.drawable.spawn_location_res_0,
            R.drawable.spawn_location_res_1,
            R.drawable.spawn_location_res_2,
            R.drawable.spawn_location_res_3,
            R.drawable.spawn_location_res_4,
            R.drawable.spawn_location_res_5,
            R.drawable.spawn_location_res_6,
            R.drawable.spawn_location_res_7
        )

        const val FIELD_LIST = "spawnLocation"
        const val FIELD_AVAILABILITY = "m"
        const val FIELD_STATUS = "s"
        const val FIELD_CHOICE = "t"

        const val STATUS_UNAVAILABLE = 1
        const val STATUS_CLOSE = 2
    }
}
