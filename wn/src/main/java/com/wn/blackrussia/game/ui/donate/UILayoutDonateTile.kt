package com.wn.blackrussia.game.ui.donate

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.ViewTreeObserver
import android.widget.Button
import android.widget.TextView
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.kingfisher.easyviewindicator.AnyViewIndicator
import com.wn.blackrussia.R
import com.wn.blackrussia.game.Multiplayer
import com.wn.blackrussia.game.ui.util.CenterItemLayoutManager
import com.wn.blackrussia.game.ui.util.UILayout
import kotlin.math.ceil

class UILayoutDonateTile(private val screen: BrDonate) : UILayout() {

    private var root: View? = null

    private lateinit var pageHeader: TextView
    private lateinit var backButton: Button
    private lateinit var list: RecyclerView
    private lateinit var indicator: AnyViewIndicator

    private val adapter = DonateTileAdapter(screen, { screen.onTileClicked(it) }, { screen.openGift() })

    override fun getView(): View? = root

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup): View {
        val view = inflater.inflate(R.layout.donate_tiles, null, false)
        root = view

        pageHeader = view.findViewById(R.id.donateTextHeader)
        backButton = view.findViewById(R.id.donateButtonBack)
        list = view.findViewById(R.id.mainRV)
        indicator = view.findViewById(R.id.rvIndicator)

        backButton.setOnClickListener { screen.goBackFromDetail() }

        list.adapter = adapter

        // до первой раскладки ширина полки неизвестна, но менеджер нужен сразу,
        // иначе RecyclerView пропускает layout и полка остаётся пустой
        list.layoutManager = LinearLayoutManager(Multiplayer.getInstance(), RecyclerView.HORIZONTAL, false)

        // ширина плитки известна только после первой раскладки списка
        list.viewTreeObserver.addOnGlobalLayoutListener(object : ViewTreeObserver.OnGlobalLayoutListener {
            override fun onGlobalLayout() {
                list.viewTreeObserver.removeOnGlobalLayoutListener(this)

                list.layoutManager = CenterItemLayoutManager(
                    context, RecyclerView.HORIZONTAL,
                    screen.itemWidth, list.width, adapter.itemCount
                )
            }
        })

        list.addOnScrollListener(object : RecyclerView.OnScrollListener() {
            private var lastPage = -1

            override fun onScrollStateChanged(recyclerView: RecyclerView, newState: Int) {
                super.onScrollStateChanged(recyclerView, newState)

                if (newState != RecyclerView.SCROLL_STATE_IDLE) {
                    return
                }

                val manager = recyclerView.layoutManager as? LinearLayoutManager ?: return
                val page = ceil(manager.findLastVisibleItemPosition() / TILES_PER_PAGE).toInt() - 1

                if (lastPage != page) {
                    indicator.setCurrentPosition(page.coerceAtLeast(0))
                    lastPage = page
                }
            }
        })

        return view
    }

    override fun onLayoutShown() {
        applyPage(screen.currentPage)
    }

    override fun onLayoutClose() {}

    fun applyPage(page: Int) {
        if (root == null) {
            return
        }

        // подарок живёт только на «Акциях»
        adapter.giftSeconds = if (page == DonateCatalog.PAGE_SALES) screen.giftSeconds else -1
        adapter.submit(screen.itemsForPage(page))

        // Новый раздел начинается сначала, а не там, где остановились в прошлом
        list.scrollToPosition(0)

        backButton.visibility = if (screen.isDetailPage(page)) View.VISIBLE else View.GONE

        val pages = ceil((adapter.itemCount - 1) / TILES_PER_PAGE).toInt()

        indicator.setItemCount(pages)
        indicator.visibility = if (pages <= 1) View.INVISIBLE else View.VISIBLE
        indicator.setCurrentPosition(0)

        pageHeader.text = titleFor(page)
    }

    fun refreshItem(item: DonateItem) {
        adapter.refreshItem(item)
    }

    private fun titleFor(page: Int) = when (page) {
        DonateCatalog.PAGE_SALES -> "Акции и спецпредложения"
        DonateCatalog.PAGE_CARS -> "Транспорт"
        DonateCatalog.PAGE_SKINS -> "Скины"
        DonateCatalog.PAGE_PACKS -> "Наборы"
        DonateCatalog.PAGE_VIP -> "VIP пакеты"
        DonateCatalog.PAGE_MONEY -> "Денежные пакеты"
        DonateCatalog.PAGE_ACCESSORIES -> "Аксессуары"
        DonateCatalog.PAGE_CAR_DETAIL, DonateCatalog.PAGE_VIP_DETAIL -> screen.selectedItem?.header.orEmpty()
        else -> ""
    }

    private companion object {
        const val TILES_PER_PAGE = 4.0
    }
}
