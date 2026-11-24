package com.wn.blackrussia.game.ui.blackpass

import android.view.Gravity
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.FrameLayout
import android.widget.ImageView
import android.widget.ProgressBar
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.wn.blackrussia.R
import com.wn.blackrussia.game.ui.donate.BrDonate
import com.wn.blackrussia.game.ui.donate.DonateCatalog
import com.wn.blackrussia.game.ui.donate.DonateItem
import com.wn.blackrussia.game.ui.util.EntitySnaps
import com.wn.blackrussia.game.ui.util.SnapShot

// Картинка награды выбирается по её типу: часть лежит в ресурсах, машины и
// предметы движок рисует сам.
internal fun bindAwardImage(image: ImageView, reward: BlackPassReward?) {
    EntitySnaps.clear(image)

    if (reward == null) {
        image.setImageDrawable(null)
        return
    }

    when (reward.typeId) {
        BlackPassReward.TYPE_EXP -> image.setImageResource(R.drawable.bp_exp)
        BlackPassReward.TYPE_MONEY -> image.setImageResource(R.drawable.bp_money)
        BlackPassReward.TYPE_COINS -> image.setImageResource(R.drawable.bp_bc)

        BlackPassReward.TYPE_VIP, BlackPassReward.TYPE_VIP_ALT -> image.setImageResource(
            when (reward.awardId) {
                0 -> R.drawable.vip_silver
                1 -> R.drawable.vip_gold
                else -> R.drawable.vip_platinum
            }
        )

        BlackPassReward.TYPE_CASE -> image.setImageResource(
            when (reward.awardId) {
                0 -> R.drawable.bp_brullet
                1 -> R.drawable.bp_srullet
                else -> R.drawable.bp_rullet
            }
        )

        // Награда-предмет хранит номер товара витрины, а не модель игры.
        // Каким он будет на снимке - машиной, педом или объектом - решает тип
        // самого товара, поэтому разворачиваем его через каталог доната.
        BlackPassReward.TYPE_CAR, BlackPassReward.TYPE_CAR_ALT,
        BlackPassReward.TYPE_ITEM, BlackPassReward.TYPE_ITEM_ALT -> {
            image.setImageResource(R.drawable.bp_gift)

            DonateCatalog.get(reward.awardId)?.let { item -> loadItemSnap(image, item) }
        }

        else -> image.setImageResource(R.drawable.bp_gift)
    }
}

private fun loadItemSnap(image: ImageView, item: DonateItem) {
    val snap = when (item.type) {
        DonateItem.TYPE_CAR -> SnapShot(
            type = EntitySnaps.TYPE_VEHICLE, modelId = item.gameId,
            rotX = 20.0f, rotY = 180.0f, rotZ = 45.0f, zoom = 0.78f,
            color1 = BrDonate.vehicleColor(item.colors.getOrElse(0) { 0 }),
            color2 = BrDonate.vehicleColor(item.colors.getOrElse(1) { 0 })
        )

        DonateItem.TYPE_SKIN -> SnapShot(
            type = EntitySnaps.TYPE_PED, modelId = item.gameId,
            rotX = 20.0f, rotY = 180.0f, rotZ = 45.0f, zoom = 0.78f
        )

        DonateItem.TYPE_ACCESSORY -> SnapShot(
            type = EntitySnaps.TYPE_OBJECT, modelId = item.modelId,
            rotX = item.rotation.getOrElse(0) { 0.0f },
            rotY = item.rotation.getOrElse(1) { 0.0f },
            rotZ = item.rotation.getOrElse(2) { 0.0f },
            zoom = item.modelZoom
        )

        else -> return
    }

    EntitySnaps.load(snap, image)
}

// Шкала уровней: верхняя дорожка — премиум, нижняя — обычная.
class BpLevelsAdapter(
    private val state: BlackPassState,
    private val onClaim: (level: Int, premium: Boolean) -> Unit
) : RecyclerView.Adapter<BpLevelsAdapter.Holder>() {

    class Holder(view: View) : RecyclerView.ViewHolder(view) {
        val premiumWindow: FrameLayout = view.findViewById(R.id.bp_ipass_pwindow)
        val premiumName: TextView = view.findViewById(R.id.bp_ipass_pname)
        val premiumGet: TextView = view.findViewById(R.id.bp_ipass_pget)
        val premiumLock: ImageView = view.findViewById(R.id.bp_ipass_plock)
        val premiumCheck: ImageView = view.findViewById(R.id.bp_ipass_pcheck)

        val window: FrameLayout = view.findViewById(R.id.bp_ipass_window)
        val name: TextView = view.findViewById(R.id.bp_ipass_name)
        val get: TextView = view.findViewById(R.id.bp_ipass_get)
        val lock: ImageView = view.findViewById(R.id.bp_ipass_lock)
        val check: ImageView = view.findViewById(R.id.bp_ipass_check)

        val premiumFrame: ImageView = view.findViewById(R.id.bp_ipass_pframe)
        val frame: ImageView = view.findViewById(R.id.bp_ipass_frame)

        val previousLine: View = view.findViewById(R.id.bp_ipass_prev_line)
        val nextLine: View = view.findViewById(R.id.bp_ipass_next_line)
        val circle: View = view.findViewById(R.id.bp_ipass_circle)
        val position: TextView = view.findViewById(R.id.bp_ipass_pos)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Holder {
        val view = LayoutInflater.from(parent.context).inflate(R.layout.bp_pass_item, parent, false)
        return Holder(view)
    }

    override fun getItemCount() = BlackPassCatalog.levelCount

    override fun onBindViewHolder(holder: Holder, position: Int) {
        val level = position + 1

        holder.position.text = level.toString()

        // пройденная часть шкалы подсвечена, дальше уровня — гасим
        holder.circle.setBackgroundResource(
            if (level <= state.level) R.drawable.oval_red else R.drawable.oval_black
        )
        holder.previousLine.setBackgroundResource(R.drawable.yellow_gradient)
        holder.nextLine.setBackgroundResource(R.drawable.yellow_gradient)

        if (position >= state.level - 1) {
            if (position >= state.level) {
                holder.previousLine.setBackgroundColor(0)
            }

            holder.nextLine.setBackgroundColor(0)
        }

        bindAwardImage(holder.frame, BlackPassCatalog.standardAt(level))
        bindAwardImage(holder.premiumFrame, BlackPassCatalog.premiumAt(level))

        bindTrack(
            reward = BlackPassCatalog.standardAt(level),
            level = level,
            premiumTrack = false,
            name = holder.name, get = holder.get, lock = holder.lock, check = holder.check,
            window = holder.window
        )

        bindTrack(
            reward = BlackPassCatalog.premiumAt(level),
            level = level,
            premiumTrack = true,
            name = holder.premiumName, get = holder.premiumGet, lock = holder.premiumLock,
            check = holder.premiumCheck, window = holder.premiumWindow
        )
    }

    private fun bindTrack(
        reward: BlackPassReward?,
        level: Int,
        premiumTrack: Boolean,
        name: TextView,
        get: TextView,
        lock: ImageView,
        check: ImageView,
        window: FrameLayout
    ) {
        name.text = reward?.name.orEmpty()

        val locked = !state.isUnlocked(level) || (premiumTrack && !state.premium)

        name.setBackgroundResource(if (locked) R.drawable.bp_header_gray else R.drawable.bp_yellow_header)
        name.setTextColor(if (locked) 0xFFFFFFFF.toInt() else 0xFF000000.toInt())

        val available = state.canClaim(level, premiumTrack)

        // Замок стоит на закрытом уровне, галочка — на открытом, «забрать» —
        // только пока награда не взята. Подписи берутся из разметки.
        if (locked) {
            get.visibility = View.INVISIBLE
            check.visibility = View.INVISIBLE
            lock.visibility = View.VISIBLE

            window.setOnClickListener(null)
            return
        }

        check.visibility = View.VISIBLE
        lock.visibility = View.INVISIBLE

        if (available) {
            get.visibility = View.VISIBLE
            window.setOnClickListener { onClaim(level, premiumTrack) }
        } else {
            get.visibility = View.INVISIBLE
            window.setOnClickListener(null)
        }
    }
}

// Ценник докупки уровня — это ровно две распорки поверх шкалы: первая шириной
// в пройденные уровни оканчивается кнопкой, вторая добирает остаток строки.
// Поэтому кнопка едет вместе со шкалой, а не висит над ней.
class BpLevelPriceAdapter(
    private val state: BlackPassState,
    private val onBuy: () -> Unit
) : RecyclerView.Adapter<BpLevelPriceAdapter.Holder>() {

    class Holder(view: View) : RecyclerView.ViewHolder(view) {
        val root: FrameLayout = view.findViewById(R.id.bp_levelprice_root)
        val frame: FrameLayout = view.findViewById(R.id.bp_levelprice_frame)
        val text: TextView = view.findViewById(R.id.bp_levelprice_text)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Holder {
        val view = LayoutInflater.from(parent.context).inflate(R.layout.bp_level_price, parent, false)
        return Holder(view)
    }

    override fun getItemCount() = 2

    override fun onBindViewHolder(holder: Holder, position: Int) {
        val levels = BlackPassCatalog.levelCount
        val density = holder.itemView.resources.displayMetrics.density

        val widthDp = if (position == 0) {
            state.level * LEVEL_WIDTH_DP + LEVEL_WIDTH_DP / 2
        } else {
            (levels - state.level) * LEVEL_WIDTH_DP - LEVEL_WIDTH_DP / 2
        }

        holder.root.layoutParams = holder.root.layoutParams.apply {
            width = (widthDp * density).toInt()
        }

        if (position == 0 && state.level != 0 && state.level < levels) {
            holder.root.visibility = View.VISIBLE
            holder.text.text = "${state.levelPrice} BC"
        } else {
            holder.root.visibility = View.INVISIBLE
        }

        holder.frame.setOnClickListener { onBuy() }
    }

    private companion object {
        const val LEVEL_WIDTH_DP = 90
    }
}

// Задания пропуска идут сеткой в две колонки: чётная ячейка — стандартное
// задание, нечётная — премиальное. Марафонные тем же классом, но списком.
class BpTasksAdapter(
    private val state: BlackPassState,
    private val marathon: Boolean,
    private val onOpen: (TaskProgress, Int) -> Unit,
    private val onClaim: (TaskProgress) -> Unit
) : RecyclerView.Adapter<BpTasksAdapter.Holder>() {

    fun refresh() = notifyDataSetChanged()

    class Holder(view: View) : RecyclerView.ViewHolder(view) {
        val locked: View = view.findViewById(R.id.bp_itasks_locked)
        val available: View = view.findViewById(R.id.bp_itasks_avaible)

        val header: View = view.findViewById(R.id.bp_itasks_header)
        val headerText: TextView = view.findViewById(R.id.bp_itasks_htext)
        val text: TextView = view.findViewById(R.id.bp_itasks_text)

        val block: View = view.findViewById(R.id.bp_itasks_ll)
        val progress: ProgressBar = view.findViewById(R.id.bp_itask_progress)
        val progressText: TextView = view.findViewById(R.id.bp_itasks_ptext)
        val expText: TextView = view.findViewById(R.id.bp_itasks_etext)

        val button: Button = view.findViewById(R.id.bp_itasks_butt)
        val refresh: ImageView = view.findViewById(R.id.bp_iatsks_refresh)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Holder {
        val view = LayoutInflater.from(parent.context).inflate(R.layout.bp_item_tasks, parent, false)
        return Holder(view)
    }

    // Строк ровно столько, сколько заданий пришло на сегодня; премиальная
    // колонка тянется до той же высоты, чтобы сетка не разъезжалась.
    override fun getItemCount(): Int {
        if (marathon) {
            return state.marathonRows
        }

        return maxOf(state.standardRows, state.premiumRows) * 2
    }

    private fun slotOf(position: Int): Int {
        if (marathon) {
            return BlackPassState.SLOT_MARATHON + position
        }

        val row = position / 2

        return if (position % 2 == 0) row else BlackPassState.SLOT_PREMIUM + row
    }

    // Карточка тянется на всю ширину ячейки и отбивается полями; у готового
    // задания снизу остаётся место под кнопку.
    private fun stretch(holder: Holder, bottomDp: Int) {
        val density = holder.itemView.resources.displayMetrics.density
        val margin = (MARGIN_DP * density).toInt()

        holder.available.layoutParams = FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT,
            FrameLayout.LayoutParams.WRAP_CONTENT,
            Gravity.CENTER_HORIZONTAL
        ).apply { setMargins(margin, margin, margin, (bottomDp * density).toInt()) }
    }

    override fun onBindViewHolder(holder: Holder, position: Int) {
        val slot = slotOf(position)
        val premium = !marathon && position % 2 != 0

        stretch(holder, 0)

        val entry = state.taskAt(slot)
        val task = entry?.let { BlackPassCatalog.task(it.id) }

        // Премиальная колонка закрыта без пропуска; пустой слот и задание, для
        // которого нет описания, выглядят так же — пустая карточка хуже замка.
        if (entry == null || task == null || (premium && !state.premium)) {
            holder.available.visibility = View.INVISIBLE
            holder.locked.visibility = View.VISIBLE
            holder.button.visibility = View.INVISIBLE
            return
        }

        holder.button.visibility = View.INVISIBLE
        holder.available.visibility = View.VISIBLE
        holder.locked.visibility = View.INVISIBLE
        holder.available.setOnClickListener(null)

        holder.headerText.text = task.type
        holder.text.text = task.text

        val exp = BlackPassState.expForSlot(slot)

        if (entry.claimed) {
            holder.block.visibility = View.GONE
            holder.refresh.visibility = View.INVISIBLE
            holder.header.setBackgroundResource(R.drawable.bp_yellow_header)
            holder.headerText.setTextColor(COLOR_DONE)
            holder.headerText.text = "Выполнено"
            return
        }

        if (entry.done) {
            holder.block.visibility = View.GONE
            holder.refresh.visibility = View.INVISIBLE
            holder.header.setBackgroundResource(R.drawable.bp_yellow_header)
            holder.headerText.setTextColor(COLOR_DONE)

            stretch(holder, CLAIM_BOTTOM_DP)

            holder.button.visibility = View.VISIBLE
            holder.button.text = "Получить +$exp оч."
            holder.button.setOnClickListener { onClaim(entry) }
            holder.available.setOnClickListener { onClaim(entry) }
            return
        }

        holder.block.visibility = View.VISIBLE
        holder.refresh.visibility = View.VISIBLE
        holder.header.setBackgroundResource(R.drawable.bp_header_gray)
        holder.headerText.setTextColor(COLOR_ACTIVE)

        val goal = task.count.coerceAtLeast(1)

        holder.progress.max = goal
        holder.progress.progress = entry.progress.coerceIn(0, goal)
        holder.progressText.text = "${entry.progress.coerceAtMost(goal)} из $goal"
        holder.expText.text = "+$exp оч."

        holder.available.setOnClickListener { onOpen(entry, exp) }
    }

    private companion object {
        const val COLOR_DONE = 0xFF000000.toInt()
        const val COLOR_ACTIVE = 0xFFFFFFFF.toInt()

        const val MARGIN_DP = 5
        const val CLAIM_BOTTOM_DP = 10
    }
}

class BpMarathonAdapter(
    private val state: BlackPassState,
    private val onClaim: (day: Int) -> Unit
) : RecyclerView.Adapter<BpMarathonAdapter.Holder>() {

    class Holder(view: View) : RecyclerView.ViewHolder(view) {
        val image: ImageView = view.findViewById(R.id.bp_imarathon_img)
        val check: ImageView = view.findViewById(R.id.bp_imarathon_check)
        val button: Button = view.findViewById(R.id.bp_imarathon_butt)
        val text: TextView = view.findViewById(R.id.bp_imarathon_text)
        val number: TextView = view.findViewById(R.id.bp_imarathon_num)

        val previousLine: View = view.findViewById(R.id.bp_imarathon_prev_line)
        val nextLine: View = view.findViewById(R.id.bp_imarathon_next_line)
        val circle: View = view.findViewById(R.id.bp_imarathon_circle)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Holder {
        val view = LayoutInflater.from(parent.context).inflate(R.layout.bp_item_marathon, parent, false)
        return Holder(view)
    }

    override fun getItemCount() = BlackPassCatalog.marathon.size

    override fun onBindViewHolder(holder: Holder, position: Int) {
        val day = position + 1
        val reward = BlackPassCatalog.marathon.getOrNull(position)

        holder.number.text = day.toString()
        holder.text.text = reward?.name.orEmpty()

        bindAwardImage(holder.image, reward)

        holder.circle.setBackgroundResource(R.drawable.oval_red)
        holder.previousLine.setBackgroundResource(R.drawable.yellow_gradient)
        holder.nextLine.setBackgroundResource(R.drawable.yellow_gradient)

        // непройденная часть дорожки гасится цветом фона, а не прозрачностью
        if (position >= state.marathonDay - 1) {
            if (position >= state.marathonDay) {
                holder.circle.setBackgroundResource(R.drawable.oval_black)
                holder.previousLine.setBackgroundColor(TRACK_OFF)
            }

            holder.nextLine.setBackgroundColor(TRACK_OFF)
        }

        val claimed = state.claimedMarathon.contains(day)
        val available = !claimed && day <= state.marathonDay

        holder.check.visibility = View.VISIBLE

        if (available) {
            holder.button.visibility = View.VISIBLE
            holder.button.text = "Получить ${reward?.name.orEmpty()}"
            holder.text.visibility = View.INVISIBLE
            holder.check.setImageResource(R.drawable.bp_check)

            holder.button.setOnClickListener { onClaim(day) }
            return
        }

        holder.text.visibility = View.VISIBLE
        holder.button.visibility = View.INVISIBLE
        holder.button.setOnClickListener(null)

        holder.check.setImageResource(if (day > state.marathonDay) R.drawable.bp_lock else R.drawable.bp_check)
    }

    private companion object {
        const val TRACK_OFF = 0xFF1F262A.toInt()
    }
}

class BpRatingAdapter(private val state: BlackPassState) :
    RecyclerView.Adapter<BpRatingAdapter.Holder>() {

    class Holder(view: View) : RecyclerView.ViewHolder(view) {
        val medal: ImageView = view.findViewById(R.id.bp_irating_medal)
        val name: TextView = view.findViewById(R.id.bp_irating_name)
        val exp: TextView = view.findViewById(R.id.bp_irating_exp)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Holder {
        val view = LayoutInflater.from(parent.context).inflate(R.layout.bp_item_rating, parent, false)
        return Holder(view)
    }

    override fun getItemCount() = RATING_ROWS

    override fun onBindViewHolder(holder: Holder, position: Int) {
        holder.medal.setImageResource(
            when (position) {
                0 -> R.drawable.medal_gold
                1 -> R.drawable.medal_silver
                2 -> R.drawable.medal_bronze
                else -> R.drawable.medal_other
            }
        )

        val entry = state.rating.getOrNull(position)

        holder.name.text = entry?.name.orEmpty()
        holder.exp.text = if (entry == null) "" else "${BrDonate.formatPrice(entry.exp)} очков"
    }

    private companion object {
        // Место в таблице всегда десять, даже если игроков меньше
        const val RATING_ROWS = 10
    }
}
