package com.wn.blackrussia.game.ui.blackpass

import android.graphics.Color
import android.text.Html
import android.view.LayoutInflater
import android.view.View
import android.widget.Button
import android.widget.FrameLayout
import android.widget.ProgressBar
import android.widget.TextView
import androidx.recyclerview.widget.GridLayoutManager
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import android.view.ViewGroup
import com.wn.blackrussia.R
import com.wn.blackrussia.game.Multiplayer
import com.wn.blackrussia.game.ui.donate.BrDonate
import com.wn.blackrussia.game.ui.util.Notify
import com.wn.blackrussia.game.ui.util.UILayout
import org.json.JSONObject

// Black Pass живёт внутри доната отдельным экраном контейнера, а не своим окном:
// шапка с балансом и ряд вкладок остаются донатовскими.
class BrBlackPass(private val screen: BrDonate) : UILayout() {

    private var root: View? = null

    private val state = BlackPassState()

    private lateinit var mainFrame: FrameLayout
    private lateinit var tabButtons: List<Button>
    private lateinit var tabViews: List<View>

    private var currentTab = TAB_PASS

    private lateinit var levelsAdapter: BpLevelsAdapter
    private lateinit var levelPriceAdapter: BpLevelPriceAdapter
    private lateinit var passTasksAdapter: BpTasksAdapter
    private lateinit var marathonTasksAdapter: BpTasksAdapter
    private lateinit var marathonAdapter: BpMarathonAdapter
    private lateinit var ratingAdapter: BpRatingAdapter

    override fun getView(): View? = root

    override fun onLayoutClose() {}

    override fun onLayoutShown() {
        request(ACTION_STATE)
        refreshCurrentTab()
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup): View {
        BlackPassCatalog.load()

        val view = inflater.inflate(R.layout.br_bp, null, false)
        root = view

        mainFrame = view.findViewById(R.id.bp_main_frame)

        tabViews = listOf(
            inflater.inflate(R.layout.bp_pass, null, false),
            inflater.inflate(R.layout.bp_tasks, null, false),
            inflater.inflate(R.layout.bp_marathon, null, false),
            inflater.inflate(R.layout.bp_rating, null, false),
            inflater.inflate(R.layout.bp_activate, null, false)
        )

        tabButtons = listOf(
            view.findViewById(R.id.br_bp_butt_pass),
            view.findViewById(R.id.br_bp_butt_tasks),
            view.findViewById(R.id.br_bp_butt_marathon),
            view.findViewById(R.id.br_bp_butt_rating)
        )

        for ((index, button) in tabButtons.withIndex()) {
            button.setOnClickListener { showTab(index) }
        }

        preparePass()
        prepareTasks()
        prepareMarathon()
        prepareRating()
        prepareActivate()

        for (tab in tabViews) {
            mainFrame.addView(tab)
            tab.visibility = View.INVISIBLE
        }

        showTab(TAB_PASS)

        request(ACTION_STATE)

        return view
    }

    private fun preparePass() {
        val view = tabViews[TAB_PASS]

        levelsAdapter = BpLevelsAdapter(state) { level, premium -> claimLevel(level, premium) }
        levelPriceAdapter = BpLevelPriceAdapter(state) { buyLevel(state.level + 1) }

        val levels = view.findViewById<RecyclerView>(R.id.bp_pass_RV).apply {
            layoutManager = LinearLayoutManager(Multiplayer.getInstance(), RecyclerView.HORIZONTAL, false)
            adapter = levelsAdapter
        }

        val prices = view.findViewById<RecyclerView>(R.id.bp_pass_levelprice_RV).apply {
            layoutManager = LinearLayoutManager(Multiplayer.getInstance(), RecyclerView.HORIZONTAL, false)
            adapter = levelPriceAdapter
        }

        linkScroll(levels, prices)

        view.findViewById<Button>(R.id.bp_pass_premium_butt).setOnClickListener { showTab(TAB_ACTIVATE) }
    }

    // Шкала уровней и полоса докупки едут как одно целое: чужой слушатель на
    // время сдвига снимается, иначе прокрутка уходит в бесконечный отклик.
    private fun linkScroll(first: RecyclerView, second: RecyclerView) {
        val listeners = arrayOfNulls<RecyclerView.OnScrollListener>(2)

        listeners[0] = object : RecyclerView.OnScrollListener() {
            override fun onScrolled(view: RecyclerView, dx: Int, dy: Int) {
                second.removeOnScrollListener(listeners[1]!!)
                second.scrollBy(dx, dy)
                second.addOnScrollListener(listeners[1]!!)
            }
        }

        listeners[1] = object : RecyclerView.OnScrollListener() {
            override fun onScrolled(view: RecyclerView, dx: Int, dy: Int) {
                first.removeOnScrollListener(listeners[0]!!)
                first.scrollBy(dx, dy)
                first.addOnScrollListener(listeners[0]!!)
            }
        }

        first.addOnScrollListener(listeners[0]!!)
        second.addOnScrollListener(listeners[1]!!)
    }

    private fun prepareTasks() {
        val view = tabViews[TAB_TASKS]

        passTasksAdapter = BpTasksAdapter(state, marathon = false,
            onOpen = { entry, exp -> showTaskDetails(entry, exp) }, onClaim = { claimTask(it) })

        marathonTasksAdapter = BpTasksAdapter(state, marathon = true,
            onOpen = { entry, exp -> showTaskDetails(entry, exp) }, onClaim = { claimTask(it) })

        view.findViewById<RecyclerView>(R.id.bp_tasks_RV).apply {
            layoutManager = GridLayoutManager(Multiplayer.getInstance(), 2)
            adapter = passTasksAdapter
        }

        view.findViewById<RecyclerView>(R.id.bp_tasks_mRV).apply {
            layoutManager = LinearLayoutManager(Multiplayer.getInstance())
            adapter = marathonTasksAdapter
        }
    }

    private fun prepareMarathon() {
        val view = tabViews[TAB_MARATHON]

        marathonAdapter = BpMarathonAdapter(state) { day -> claimMarathon(day) }

        view.findViewById<RecyclerView>(R.id.bp_marathon_RV).apply {
            layoutManager = LinearLayoutManager(Multiplayer.getInstance(), RecyclerView.HORIZONTAL, false)
            adapter = marathonAdapter
        }
    }

    private fun prepareRating() {
        val view = tabViews[TAB_RATING]

        ratingAdapter = BpRatingAdapter(state)

        view.findViewById<RecyclerView>(R.id.bp_rating_RV).apply {
            layoutManager = LinearLayoutManager(Multiplayer.getInstance())
            adapter = ratingAdapter
        }

        view.findViewById<Button>(R.id.bp_rating_butt).setOnClickListener { request(ACTION_RATING) }

        // Оригинал приравнивает высоту к параметру ширины, а не к измеренной:
        // ширина там match_parent, то есть высота становится такой же тянущейся.
        val avatar = view.findViewById<View>(R.id.bp_rating_img)

        view.findViewById<View>(R.id.bp_rating_frame).viewTreeObserver.addOnGlobalLayoutListener {
            val params = avatar.layoutParams

            if (params.height != params.width) {
                params.height = params.width
                avatar.layoutParams = params
            }
        }
    }

    private fun prepareActivate() {
        val view = tabViews[TAB_ACTIVATE]

        view.findViewById<TextView>(R.id.bp_act_text1).text =
            Html.fromHtml("Пакет <font color='#FFBA08'>Premium</font>")

        view.findViewById<TextView>(R.id.bp_act_text4).text =
            Html.fromHtml("Все преимущества из <font color='#FFBA08'>BLACK PASS PREMIUM</font>")

        view.findViewById<Button>(R.id.bp_act_back).setOnClickListener { showTab(TAB_PASS) }
        view.findViewById<Button>(R.id.bp_act_buy).setOnClickListener { buyPremium(TIER_PREMIUM) }
        view.findViewById<Button>(R.id.bp_act_premium_buy).setOnClickListener { buyPremium(TIER_DELUXE) }
    }

    private fun updateActivate() {
        val view = tabViews[TAB_ACTIVATE]

        view.findViewById<Button>(R.id.bp_act_buy).text = "${state.premiumPrice} BC"
        view.findViewById<Button>(R.id.bp_act_premium_buy).text = "${state.deluxePrice} BC"

        // блок выгоды показывается только когда на пропуск идёт акция
        view.findViewById<View>(R.id.bp_act_text5).visibility = View.GONE
        view.findViewById<View>(R.id.bp_act_text3).visibility = View.VISIBLE
        view.findViewById<View>(R.id.bp_act_text8).visibility = View.INVISIBLE
        view.findViewById<View>(R.id.bp_act_text6).visibility = View.GONE
        view.findViewById<View>(R.id.bp_act_text7).visibility = View.INVISIBLE

        view.findViewById<View>(R.id.bp_act_premium_buy).visibility = View.VISIBLE
        view.findViewById<View>(R.id.bp_act_buy).visibility = View.VISIBLE
    }

    private fun showTab(tab: Int) {
        currentTab = tab

        for ((index, view) in tabViews.withIndex()) {
            view.visibility = if (index == tab) View.VISIBLE else View.INVISIBLE
        }

        for ((index, button) in tabButtons.withIndex()) {
            button.setBackgroundResource(
                if (index == tab) R.drawable.button_yellow_gradient else R.drawable.donate_tile_button_gray
            )
            button.setTextColor(if (index == tab) COLOR_ACTIVE else COLOR_INACTIVE)
        }

        when (tab) {
            TAB_TASKS -> request(ACTION_TASKS)
            TAB_RATING -> request(ACTION_RATING)
        }

        refreshCurrentTab()
    }

    private fun refreshCurrentTab() {
        when (currentTab) {
            TAB_PASS -> {
                levelsAdapter.notifyDataSetChanged()
                levelPriceAdapter.notifyDataSetChanged()

                // «Активен» стоит при купленном пропуске, кнопка покупки — наоборот
                tabViews[TAB_PASS].findViewById<View>(R.id.bp_pass_act).visibility =
                    if (state.premium) View.VISIBLE else View.GONE

                tabViews[TAB_PASS].findViewById<View>(R.id.bp_pass_premium_butt).visibility =
                    if (state.premium) View.GONE else View.VISIBLE
            }

            TAB_TASKS -> {
                passTasksAdapter.refresh()
                marathonTasksAdapter.refresh()

                val left = state.tasksSecondsLeft.coerceAtLeast(0)

                tabViews[TAB_TASKS].findViewById<TextView>(R.id.bp_tasks_time).text = Html.fromHtml(
                    "До обновления <font color='#FFBA08'>" +
                        "${left / 3600}ч. ${left % 3600 / 60}м.</font>"
                )
            }

            TAB_MARATHON -> {
                marathonAdapter.notifyDataSetChanged()

                tabViews[TAB_MARATHON].findViewById<TextView>(R.id.bp_marathon_time).text =
                    formatRemaining(state.marathonSecondsLeft)
            }

            TAB_RATING -> {
                ratingAdapter.notifyDataSetChanged()

                tabViews[TAB_RATING].findViewById<TextView>(R.id.bp_rating_order).text =
                    "${state.ownPlace} место"
            }
        }
    }

    private fun updateHeader() {
        requireRoot().findViewById<TextView>(R.id.br_bp_name).text = state.seasonName
        requireRoot().findViewById<TextView>(R.id.br_bp_level).text = state.level.toString()

        requireRoot().findViewById<TextView>(R.id.br_bp_leader).text = Html.fromHtml(
            "Лидер сезона <font color='#FFBA08'>${state.leaderName.uppercase()}</font>"
        )

        requireRoot().findViewById<TextView>(R.id.br_bp_time).text = Html.fromHtml(
            "До конца сезона осталось: <font color='#FFBA08'>" +
                "${state.secondsLeft / 86400} дн. ${state.secondsLeft % 86400 / 3600} ч.</font>"
        )

        requireRoot().findViewById<TextView>(R.id.br_bp_exp).text = "${state.exp} из ${state.expPerLevel} очков"

        val progress: ProgressBar = requireRoot().findViewById(R.id.br_bp_progress)
        progress.max = state.expPerLevel
        progress.progress = state.exp.coerceIn(0, state.expPerLevel)
    }

    // Окно с описанием задания: отсюда же его и меняют, замена стоит монеты
    private fun showTaskDetails(entry: TaskProgress, exp: Int) {
        val task = BlackPassCatalog.task(entry.id) ?: return

        val view = LayoutInflater.from(Multiplayer.getInstance()).inflate(R.layout.bp_task, null, false)

        view.findViewById<TextView>(R.id.bp_task_header).text = when {
            entry.slot >= BlackPassState.SLOT_MARATHON -> "Задание марафона не выполнено"
            entry.slot >= BlackPassState.SLOT_PREMIUM -> "Задание премиума не выполнено"
            else -> "Задание стандарта не выполнено"
        }

        view.findViewById<TextView>(R.id.bp_task_name).text = task.text
        view.findViewById<TextView>(R.id.bp_task_description).text = Html.fromHtml(task.description)
        view.findViewById<TextView>(R.id.bp_task_exp).text = "+ $exp очков сезонного опыта"
        view.findViewById<TextView>(R.id.bp_task_coins).text = "${task.award} P"

        view.findViewById<Button>(R.id.bp_task_close).setOnClickListener { requireRoot().removeView(view) }
        view.findViewById<Button>(R.id.bp_task_exch).setOnClickListener {
            requireRoot().removeView(view)
            rerollTask(entry)
        }

        requireRoot().addView(view)
        view.bringToFront()
    }

    // Награда сперва показывается игроку, и лишь при закрытии окна уходит запрос
    private fun showPrizeWindow(reward: BlackPassReward?, name: String, onClosed: () -> Unit) {
        val view = LayoutInflater.from(Multiplayer.getInstance()).inflate(R.layout.bp_prize, null, false)

        bindAwardImage(view.findViewById(R.id.bp_prize_img), reward)
        view.findViewById<TextView>(R.id.bp_prize_name).text = name

        val close = View.OnClickListener {
            requireRoot().removeView(view)
            onClosed()
        }

        view.findViewById<Button>(R.id.bp_prize_get).setOnClickListener(close)
        view.findViewById<Button>(R.id.bp_prize_close).setOnClickListener(close)

        requireRoot().addView(view)
        view.bringToFront()
    }

    private fun claimLevel(level: Int, premium: Boolean) {
        val reward = if (premium) BlackPassCatalog.premiumAt(level) else BlackPassCatalog.standardAt(level)

        showPrizeWindow(reward, reward?.name.orEmpty()) {
            send(
                JSONObject()
                    .put(KEY_TYPE, ACTION_CLAIM_LEVEL)
                    .put("level", level)
                    .put("prem", if (premium) 1 else 0)
            )
        }
    }

    private fun buyLevel(level: Int) {
        send(JSONObject().put(KEY_TYPE, ACTION_BUY_LEVEL).put("level", level))
    }

    private fun buyPremium(tier: Int) {
        send(JSONObject().put(KEY_TYPE, ACTION_BUY_PREMIUM).put("id", tier))
    }

    private fun rerollTask(entry: TaskProgress) {
        send(JSONObject().put(KEY_TYPE, ACTION_REROLL_TASK).put("task", entry.id))
    }

    private fun claimTask(entry: TaskProgress) {
        val task = BlackPassCatalog.task(entry.id)
        val exp = BlackPassState.expForSlot(entry.slot)

        val name = if ((task?.award ?: 0) != 0) "${task?.award}P" else "+$exp оч."

        showPrizeWindow(null, name) {
            send(JSONObject().put(KEY_TYPE, ACTION_CLAIM_TASK).put("task", entry.id))
        }
    }

    private fun claimMarathon(day: Int) {
        val reward = BlackPassCatalog.marathon.getOrNull(day - 1)

        showPrizeWindow(reward, reward?.name.orEmpty()) {
            send(JSONObject().put(KEY_TYPE, ACTION_CLAIM_MARATHON).put("day", day))
        }
    }

    private fun request(action: Int) {
        send(JSONObject().put(KEY_TYPE, action))
    }

    // Пакеты Black Pass идут через донат: снаружи t = -1, внутри свой признак
    private fun send(json: JSONObject) {
        screen.sendBlackPass(json.put("t", BLACKPASS_PACKET))
    }

    private fun requireRoot(): ViewGroup = root as ViewGroup

    fun onBlackPassPacket(packet: JSONObject) {
        when (packet.optInt(KEY_TYPE, -1)) {
            ACTION_STATE -> {
                state.applyMain(packet)
                updateHeader()
                updateActivate()
                refreshCurrentTab()
            }

            ACTION_TASKS -> {
                state.applyTasks(packet)
                if (currentTab == TAB_TASKS) refreshCurrentTab()
            }

            ACTION_RATING -> {
                state.applyRating(packet)
                if (currentTab == TAB_RATING) refreshCurrentTab()
            }

            ACTION_RESULT -> screen.showBlackPassResult(
                packet.optInt("ok") == 1,
                packet.optString("text"),
                packet.optInt("need")
            )

            ACTION_CLOSED -> screen.closeFromBlackPass()
        }
    }

    private fun formatRemaining(seconds: Int): String {
        val hours = seconds / 3600
        val minutes = seconds % 3600 / 60

        return "%02d:%02d".format(hours, minutes)
    }

    companion object {
        const val TAB_PASS = 0
        const val TAB_TASKS = 1
        const val TAB_MARATHON = 2
        const val TAB_RATING = 3
        const val TAB_ACTIVATE = 4

        const val TIER_DELUXE = 0
        const val TIER_PREMIUM = 1

        const val BLACKPASS_PACKET = -1

        const val KEY_TYPE = "bp"

        const val ACTION_STATE = 0
        const val ACTION_CLAIM_LEVEL = 1
        const val ACTION_BUY_PREMIUM = 2
        const val ACTION_BUY_LEVEL = 3
        const val ACTION_TASKS = 4
        const val ACTION_RATING = 5
        const val ACTION_CLAIM_MARATHON = 6
        const val ACTION_CLAIM_TASK = 7
        const val ACTION_RESULT = 8
        const val ACTION_REROLL_TASK = 10
        const val ACTION_CLOSED = 9

        private val COLOR_ACTIVE = Color.BLACK
        private val COLOR_INACTIVE = Color.WHITE
    }
}
