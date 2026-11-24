package com.wn.blackrussia.game.ui.tuning

class TuningBoxItem(var id: Int, var name: String)

class TuningDetailDiagnosticItem(var id: Int, var nameDetail: String, var resName: String) {
    var valueState = 0
    var cost = 0
    var checked = false
}

class TuningMenuType(var titleType: String) {
    var clickable = false
}
