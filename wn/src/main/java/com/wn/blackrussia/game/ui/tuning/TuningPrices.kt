package com.wn.blackrussia.game.ui.tuning

import kotlin.math.roundToLong

object TuningPrices {
    private val BASE = intArrayOf(
        1000, 1000, 500, 600, 1000, 5000, 400, 1000, 1000, 10000,
        450, 450, 1000, 1000, 700, 15000, 15000, 15000, 15000, 15000,
        50000, 150000, 300, 150000, 10000, 15000, 10000, 0, 50000
    )

    val size get() = BASE.size

    private fun of(base: Double, gos: Int, percent: Double) = base + gos * percent / 100.0

    fun detailPrice(type: Int, gosCost: Int, tuneId: Int): Int {
        val price: Double = when {
            type == -1 -> of(15000.0, gosCost, 8.0)

            type < 0 || type >= BASE.size -> 0.0

            type in 15..19 -> when (tuneId) {
                0 -> of(BASE[type].toDouble(), gosCost, 4.0)
                1 -> of(BASE[type].toDouble(), gosCost, 7.0)
                2 -> of(BASE[type].toDouble(), gosCost, 5.0)
                3 -> of(20.0, gosCost, 0.005)
                else -> 0.0
            }

            type == 20 -> when (tuneId) {
                in 0..3 -> of(BASE[type].toDouble(), gosCost, 1.0)
                4 -> of(150.0, gosCost, 0.003)
                else -> 0.0
            }

            type == 21 -> of(BASE[type].toDouble(), gosCost, 40.0)

            type == 22 -> of(BASE[type].toDouble(), gosCost, 0.05).coerceAtMost(10000.0)

            type == 23 -> of(BASE[type].toDouble(), gosCost, 30.0)

            type == 24 -> of(BASE[type].toDouble(), gosCost, 1.0)

            type == 25 -> when (tuneId) {
                1009 -> of(BASE[type].toDouble(), gosCost, 2.0)
                1008 -> of(BASE[type].toDouble(), gosCost, 5.0)
                1010 -> of(BASE[type].toDouble(), gosCost, 7.0)
                else -> 0.0
            }

            type == 26 -> of(BASE[type].toDouble(), gosCost, 0.1)

            type == 27 -> when (tuneId) {
                in 14..23, in 26..29 -> of(100000.0, gosCost, 7.0)
                36, 37 -> of(150000.0, gosCost, 15.0)
                in 24..25, in 30..35, in 38..44 -> of(350.0, gosCost, 0.003)
                else -> 0.0
            }

            type == 28 -> of(BASE[type].toDouble(), gosCost, 3.0)

            else -> of(BASE[type].toDouble(), gosCost, 1.0)
        }

        return price.roundToLong().toInt()
    }

    // Цена услуги по селектору, восстановлено из GUITuning.setCost
    fun selectorPrice(selector: Int, gosCost: Int): Int {
        val price: Double = when (selector) {
            0, 1, 32 -> of(10000.0, gosCost, 1.0)
            2 -> of(50000.0, gosCost, 1.0)
            3, 4 -> of(20000.0, gosCost, 1.0)
            10 -> of(20000.0, gosCost, 2.0)
            11, 12, 13 -> of(50000.0, gosCost, 1.0)
            22 -> of(40000.0, gosCost, 10.0)
            23 -> of(30000.0, gosCost, 7.0)
            24, 25 -> of(40000.0, gosCost, 3.0)
            26, 27 -> of(40000.0, gosCost, 5.0)
            28, 29 -> of(50000.0, gosCost, 5.0)
            30 -> of(100000.0, gosCost, 7.0)
            else -> 0.0
        }

        return price.roundToLong().toInt()
    }
}
