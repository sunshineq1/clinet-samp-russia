package com.wn.blackrussia.game.ui.brguifix;

import android.app.Activity;

import org.json.JSONObject;

public interface ISAMPGUI {
    void close(JSONObject json);
    int getGuiId();
    boolean isShowingGui();
    void onPacketIncoming(JSONObject json);
    void show(JSONObject json, GUIManager manager, Activity activity);
}
