package com.wn.blackrussia.game.ui.util
import com.wn.blackrussia.game.ui.util.KUtils

object ConvertViewCoordsToGta {
    class Data (
        var x: Float = 0.0f,
        var y: Float = 0.0f,
        var width: Float = 0.0f,
        var height: Float = width
    )

    fun convertCoordsToGta(data: Data) : Data {
        val returnData = Data()

        val screenWidth = KUtils.screenSize.first.toFloat()
        val screenHeight = KUtils.screenSize.second.toFloat()

        val viewPercentX = (data.x + (data.width / 2f) ) / screenWidth
        val viewPercentY = (data.y + (data.height / 2.25f) ) / screenHeight

        returnData.x = 640f * viewPercentX
        returnData.y = 480f * viewPercentY

        val viewPercentWidth    = data.width / 2 / screenWidth
        val viewPercentHeight   = data.height / 3.333f / screenHeight

        returnData.width = 640f * viewPercentWidth
        returnData.height = 480f * viewPercentHeight

        return returnData
    }
}