package com.wn.blackrussia.game.ui.tuning

class AddDataForBox(private val root: BrTuning) {

    val allNameInBox = ArrayList<TuningBoxItem>()

    fun build() {
        val differential = detailsForSelector(TuningConstants.SELECTOR_DIFFERENTIAL)
        val brakers = detailsForSelector(TuningConstants.SELECTOR_BRAKERS)
        val airFilters = detailsForSelector(TuningConstants.SELECTOR_AIR_FILTER)
        val engines = detailsForSelector(TuningConstants.SELECTOR_ENGINE)
        val superchargers = detailsForSelector(TuningConstants.SELECTOR_SUPERCHARGER)

        clearFirstElement(differential)
        clearFirstElement(brakers)
        clearFirstElement(airFilters)
        clearFirstElement(engines)
        clearFirstElement(superchargers)

        allNameInBox.clear()
        addNameSelectorInList(TuningConstants.SELECTOR_DIFFERENTIAL, differential)
        addNameSelectorInList(TuningConstants.SELECTOR_BRAKERS, brakers)
        addNameSelectorInList(TuningConstants.SELECTOR_AIR_FILTER, airFilters)
        addNameSelectorInList(TuningConstants.SELECTOR_ENGINE, engines)
        addNameSelectorInList(TuningConstants.SELECTOR_SUPERCHARGER, superchargers)
    }

    private fun detailsForSelector(selector: Int): MutableList<Tune> =
        root.correctDetailsListFormatTune(root.tuneItemsForSelector(selector)).toMutableList()

    // Сама коробка лежит в том же селекторе, что и её содержимое, в список её не берём
    private fun clearFirstElement(details: MutableList<Tune>) {
        if (details.isEmpty() || details[0].name != BOX_ITEM_NAME) return

        details.removeAt(0)
    }

    private fun addNameSelectorInList(selector: Int, details: List<Tune>) {
        val prefix = prefixForSelector(selector)

        for (detail in details) {
            allNameInBox += TuningBoxItem(detail.id, prefix + detail.name)
        }
    }

    // У электромобилей та же силовая установка называется иначе
    private fun prefixForSelector(selector: Int): String {
        if (root.carId in ELECTRIC_CARS) {
            return when (selector) {
                TuningConstants.SELECTOR_DIFFERENTIAL -> "Батарея "
                TuningConstants.SELECTOR_BRAKERS -> BRAKERS_PREFIX
                TuningConstants.SELECTOR_AIR_FILTER -> "Инвертор "
                TuningConstants.SELECTOR_ENGINE -> "Электродвигатель "
                TuningConstants.SELECTOR_SUPERCHARGER -> "Контроллер "
                else -> ""
            }
        }

        return when (selector) {
            TuningConstants.SELECTOR_DIFFERENTIAL -> "Дифференциал "
            TuningConstants.SELECTOR_BRAKERS -> BRAKERS_PREFIX
            TuningConstants.SELECTOR_AIR_FILTER -> "Турбонаддув "
            TuningConstants.SELECTOR_ENGINE -> "Двигатель "
            TuningConstants.SELECTOR_SUPERCHARGER -> "Нагнетатель "
            else -> ""
        }
    }

    private companion object {
        const val BOX_ITEM_NAME = "Коробка"
        const val BRAKERS_PREFIX = "Тормоза "

        val ELECTRIC_CARS = intArrayOf(
            TuningConstants.ID_TESLA_MODEL_X,
            TuningConstants.ID_TESLA_MODEL_S,
            TuningConstants.ID_PORSCHE_TAYCAN,
            TuningConstants.ID_MERCEDES_EQS
        )
    }
}
