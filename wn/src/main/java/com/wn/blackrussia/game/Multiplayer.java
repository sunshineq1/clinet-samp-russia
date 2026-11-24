package com.wn.blackrussia.game;

import android.annotation.SuppressLint;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.graphics.Bitmap;
import android.hardware.Sensor;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.FrameLayout;

import com.google.firebase.crashlytics.FirebaseCrashlytics;
import com.nvidia.devtech.NvEventQueueActivity;
import com.wn.blackrussia.HeightProvider;
import com.wn.blackrussia.game.ui.AttachEdit;
import com.wn.blackrussia.game.ui.BrDialogWindow;
import com.wn.blackrussia.game.ui.BrNotification;
import com.wn.blackrussia.game.ui.BrChooseServer;
import com.wn.blackrussia.game.ui.BrSpeed;
import com.wn.blackrussia.game.ui.Buttons;
import com.wn.blackrussia.game.ui.keyboard.KeyboardStandardManager;
import com.wn.blackrussia.game.ui.registration.GUIRegistration;
import com.wn.blackrussia.game.ui.util.FrameRate;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.UnsupportedEncodingException;
import java.util.Date;

public class Multiplayer extends Game implements SensorEventListener, HeightProvider.HeightListener {
    private static final String TAG = "Multiplayer";
    private static Multiplayer instance;

    protected boolean wantsAccelerometer = false;
    protected SensorManager mSensorManager = null;
    protected ClipboardManager mClipboardManager = null;
    protected int mSensorDelay = SensorManager.SENSOR_DELAY_GAME; //other options: SensorManager.SENSOR_DELAY_FASTEST, SensorManager.SENSOR_DELAY_NORMAL and SensorManager.SENSOR_DELAY_UI
    protected Display display = null;

    public KeyboardStandardManager mKeyboardStandardManager = null;

    private HeightProvider mHeightProvider;

    private AttachEdit mAttachEdit = null;

    private BrDialogWindow mDialog = null;
    private BrNotification mBrNotification = null;
    private GUIRegistration mBrReg = null;
    private BrSpeed mBrSpeed = null;

    private Buttons mButtons;

    public native void sendDialogResponse(int i, int i2, int i3, byte[] str);
    public native void sendCommand(byte[] str);

    public native void sendServer(int id);
    public native void jniDestroy();

    public native void sendG();

    public static Multiplayer getInstance() {
        return instance;
    }
    public FrameLayout getmRootFrame() {
        return NvEventQueueActivity.getInstance().getmAndroidUI();
    }
    public FrameLayout getParentLayout() { // костыль
        return getmRootFrame();
    }

    public Boolean isSupportFingerPrint = false;
    public boolean isFingerprintSupported() {
        return isSupportFingerPrint;
    }

    private Boolean isRegShowing = false;
    public void closeGUIRegistration() {
        GUIRegistration.close();
    }

    public native void toggleDrawing2dStuff(Boolean isHide);
    public native boolean isPlayerDriving();
    public native void togglePlayer(int toggle);

    private void showTab() {
    }

    private void hideTab() {
    }

    private void setTab(int id, String name, int score, int ping) {
    }

    private void clearTab() {

    }

    private void showLoadingScreen(String text) {
        // экран загрузки показывает процент, копирайт ему не нужен
    }

    private void hideLoadingScreen() {
        BrChooseServer screen = GUIManager.INSTANCE.get(GUIManager.UI_ID_SELECTSERVER);
        if (screen != null) {
            screen.hideLoading();
        }
    }

    public void setPauseState(boolean pause) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (pause) {
                    if(mDialog.getVis())
                        mDialog.tempToggle(false);
                    mAttachEdit.hideWithoutReset();
                } else {
                    if(mDialog.m_bIsTempToggled)
                        mDialog.tempToggle(true);
                    if(mAttachEdit.isShow)
                        mAttachEdit.showWithoutReset();
                }
            }
        });
    }

    public void exitGame(){
        FirebaseCrashlytics.getInstance().setCrashlyticsCollectionEnabled(false);

        finishAndRemoveTask();
        System.exit(0);
    }

    private native void onInputEnd(byte[] str);
    public void OnInputEnd(String str) {
        byte[] toReturn = null;
        try {
            toReturn = str.getBytes("windows-1251");
        }
        catch(UnsupportedEncodingException e) {
        }

        try {
            onInputEnd(toReturn);
        } catch (UnsatisfiedLinkError e5) {
            Log.e(TAG, e5.getMessage());
        }
    }

    public void showInputLayout()
    {

    }

    public void hideInputLayout()
    {

    }

    private void showEditObject()
    {

    }

    private void hideEditObject()
    {

    }

    @SuppressLint("WrongConstant")
    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "**** onCreate");

        // ставим до super.onCreate: он вызывает systemInit, а тот уже поднимает
        // экран загрузки через getInstance()
        instance = this;

        super.onCreate(savedInstanceState);

        if(wantsAccelerometer && (mSensorManager == null)) {
            mSensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
        }
        mClipboardManager = (ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
        display = ((WindowManager) this.getSystemService(WINDOW_SERVICE)).getDefaultDisplay();
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);

        mAttachEdit = new AttachEdit(this);
        mButtons = new Buttons(this);

        mBrSpeed = new BrSpeed(this);
        mBrNotification = new BrNotification();
        mDialog = new BrDialogWindow(this);
        mBrReg = new GUIRegistration();

        mKeyboardStandardManager = new KeyboardStandardManager(this);
        mKeyboardStandardManager.selectLang(KeyboardStandardManager.KEYBOARD_LANG_RU);

        registerBackHandler();

        FrameRate helper = new FrameRate(this);
        helper.requestMaxRefreshRate(this);

        int maxFrameRate = helper.getMaxRefreshRate();
        Log.i(TAG, "Device FPS: " + maxFrameRate);

        try {
            initializeClient(maxFrameRate);
        } catch (UnsatisfiedLinkError e5) {
            Log.e(TAG, e5.getMessage());
        }
    }

    private native void initializeClient(int maxFrameRate);
    public native void specialCall(int id);

    @Override
    public void onStart() {
        Log.i(TAG, "**** onStart");
        super.onStart();
    }

    @Override
    public void onRestart() {
        Log.i(TAG, "**** onRestart");
        super.onRestart();
    }

    @Override
    public void onResume() {
        Log.i(TAG, "**** onResume");
        super.onResume();

        if(mSensorManager != null)
            mSensorManager.registerListener(
                    this,
                    mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),
                    mSensorDelay);
    }

    public native void onEventBackPressed();

    // На Android 13+ система перешла на OnBackInvokedCallback, а с Android 15
    // (targetSdk 35+) predictive back включён принудительно и onBackPressed()
    // больше не вызывается: активность просто уничтожалась, пока нативный поток
    // продолжал работать с уже мёртвым окном — отсюда вылет по «назад».
    private void registerBackHandler() {
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.TIRAMISU) {
            getOnBackInvokedDispatcher().registerOnBackInvokedCallback(
                    android.window.OnBackInvokedDispatcher.PRIORITY_DEFAULT,
                    this::handleBackPressed);
        }
    }

    // Ниже Android 13 колбэка нет — там работает обычный onBackPressed.
    @Override
    public void onBackPressed() {
        if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.TIRAMISU) {
            handleBackPressed();
        }
        // Активность не закрываем: выходом из игры управляет клиент.
    }

    private void handleBackPressed() {
        if (GUIManager.handleBackPressed()) {
            return;
        }

        try {
            onEventBackPressed();
        } catch (UnsatisfiedLinkError ignored) {
        }
    }

    /*@Override
    public void onBackPressed() {
        onEventBackPressed();
        super.onBackPressed();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if(keyCode == KeyEvent.KEYCODE_BACK) {
            onEventBackPressed();
        }

        return super.onKeyDown(keyCode, event);
    }*/

    @Override
    public void onPause() {
        Log.i(TAG, "**** onPause");
        super.onPause();
    }

    @Override
    public void onStop() {
        Log.i(TAG, "**** onStop");
        if(mSensorManager != null)
            mSensorManager.unregisterListener(this);
        super.onStop();
    }

    @Override
    public void onDestroy() {
        Log.i(TAG, "**** onDestroy");
        jniDestroy();
        super.onDestroy();
    }

    @Override
    public void onHeightChanged(int orientation, int height) {
        if (mDialog != null) {
            mDialog.onHeightChanged(height);
        }
    }

    private void openScreenshot(File imageFile) {
        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_VIEW);
        Uri uri = Uri.fromFile(imageFile);
        intent.setDataAndType(uri, "image/*");
        startActivity(intent);
    }

    private void takeScreenshot() {
        Date now = new Date();
        android.text.format.DateFormat.format("yyyy-MM-dd_hh:mm:ss", now);

        try {
            // image naming and path  to include sd card  appending name you choose for file
            String mPath = getExternalFilesDir(null) + "/" + now + ".jpg";

            // create bitmap screen capture
            View v1 = getWindow().getDecorView().getRootView();
            v1.setDrawingCacheEnabled(true);
            Bitmap bitmap = Bitmap.createBitmap(v1.getDrawingCache());
            v1.setDrawingCacheEnabled(false);

            File imageFile = new File(mPath);

            FileOutputStream outputStream = new FileOutputStream(imageFile);
            int quality = 100;
            bitmap.compress(Bitmap.CompressFormat.JPEG, quality, outputStream);
            outputStream.flush();
            outputStream.close();

            //openScreenshot(imageFile);
        } catch (Throwable e) {
            // Several error may come out with file handling or DOM
            e.printStackTrace();
        }
    }

    public void toggleCustomButtons(boolean status)
    {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                //nLog.d(TAG, "toggleCustomButtons()");
                mButtons.toggleVisibility(status);
            }
        });
    }

    public void showDialog(int dialogId, int dialogTypeId, String caption, String content, String leftBtnText, String rightBtnText) {
        runOnUiThread(() -> {
            if (dialogId == 2) {
                JSONObject json = new JSONObject();
                try {
                    json.put("mIsRegistration", true);
                    json.put("mHasFingerprint", 0);
                    json.put("mNeedPasswordNow", true);
                    json.put("mHasMail", 0);

                    if(!isRegShowing) {
                        mBrReg.show(json, this);
                        isRegShowing = true;
                    }
                } catch (JSONException ignored) {

                }
            } else if (dialogId == 1) {
                JSONObject json2 = new JSONObject();
                try {
                    json2.put("mIsRegistration", false);
                    json2.put("mHasFingerprint", 0);
                    json2.put("mNeedPasswordNow", true);
                    json2.put("mHasMail", 0);

                    mBrReg.show(json2, this);
                } catch (JSONException ignored) {
                }
            } else {
                this.mDialog.show(dialogId, dialogTypeId, caption, content, leftBtnText, rightBtnText);
            }});
    }

    public void showNotification(int type, String text, int duration, String actionforBtn, String textBtn) {
        JSONObject jSONObject = new JSONObject();
        try {
            jSONObject.put("t", type);
            jSONObject.put("d", duration);
            jSONObject.put("k", textBtn);
            jSONObject.put("a", actionforBtn);
            jSONObject.put("i",text);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        localShowNotification(jSONObject);
    }

    public void localShowNotification(JSONObject jSONObject) {
        runOnUiThread(() -> mBrNotification.newInstance().show(jSONObject));
    }

    public void toggleSpeed(boolean state) {
        runOnUiThread(() -> {
            mBrSpeed.toggleVisibility(state);
        });
    }
    public void setSpeedToggle(boolean state) {
        runOnUiThread(() -> {
            mBrSpeed.m_bUI = state;
        });
    }
    public void updateSpeed(
            int speed, int fuel, int hp, int engine, int light, int lock, int turn) {
        runOnUiThread(() -> {
            mBrSpeed.UpdateSpeedInfo(
                    speed, fuel, hp, engine, light, lock, turn);
        });
    }
    public boolean getSpeedVisibility() {
        return mBrSpeed.m_bEnabled;
    }


    public native int[] getGraphicsSettings();

    public native void sendJsonData(int guiid, byte[] data);

    public native void applyGraphicsSettings(int water, int reflections, int drawDistance,
                                             boolean lowVehicleDist, boolean optimizedRender,
                                             boolean antialiasing, boolean enhancedSky);

    private android.widget.PopupWindow mGraphicSettingsWindow;

    public void showGraphicSettings() {
        runOnUiThread(() -> {
            if (mGraphicSettingsWindow != null && mGraphicSettingsWindow.isShowing()) {
                return;
            }

            int[] v = getGraphicsSettings();
            if (v == null || v.length < 7) {
                v = new int[] { 2, 2, 50, 0, 0, 0, 1 };
            }

            android.view.ContextThemeWrapper themed = new android.view.ContextThemeWrapper(
                    this, com.wn.blackrussia.R.style.AppTheme);
            android.view.View root = android.view.LayoutInflater.from(themed)
                    .inflate(com.wn.blackrussia.R.layout.br_graphic_settings, null, false);

            com.wn.blackrussia.game.common.GUIPopupWindow window =
                    new com.wn.blackrussia.game.common.GUIPopupWindow(root, -2, -2, true);
            window.setSoftInputMode(16);

            android.widget.SeekBar water =
                    root.findViewById(com.wn.blackrussia.R.id.water_progress);
            android.widget.SeekBar reflections =
                    root.findViewById(com.wn.blackrussia.R.id.carrefl_progress);
            android.widget.SeekBar distance =
                    root.findViewById(com.wn.blackrussia.R.id.distance_progress);
            android.widget.TextView waterLabel =
                    root.findViewById(com.wn.blackrussia.R.id.water_text);
            android.widget.TextView reflectionsLabel =
                    root.findViewById(com.wn.blackrussia.R.id.carrefl_text);
            android.widget.TextView distanceLabel =
                    root.findViewById(com.wn.blackrussia.R.id.distance_text);
            android.widget.Switch lowVehicle =
                    root.findViewById(com.wn.blackrussia.R.id.switch_lower_cars);
            android.widget.Switch optimized =
                    root.findViewById(com.wn.blackrussia.R.id.switch_lower_dd);
            android.widget.Switch antialiasing =
                    root.findViewById(com.wn.blackrussia.R.id.switch_aa);
            android.widget.Switch enhancedSky =
                    root.findViewById(com.wn.blackrussia.R.id.switch_sky);

            water.setProgress(v[0]);
            reflections.setProgress(v[1]);
            distance.setProgress(v[2]);
            lowVehicle.setChecked(v[3] != 0);
            optimized.setChecked(v[4] != 0);
            antialiasing.setChecked(v[5] != 0);
            enhancedSky.setChecked(v[6] != 0);

            waterLabel.setText(labelFor(LABEL_QUALITY, v[0]));
            reflectionsLabel.setText(labelFor(LABEL_DISABLED, v[1]));
            distanceLabel.setText(labelFor(LABEL_DISTANCE, v[2]));

            water.setOnSeekBarChangeListener(new SimpleSeekListener(waterLabel, LABEL_QUALITY));
            reflections.setOnSeekBarChangeListener(new SimpleSeekListener(reflectionsLabel, LABEL_DISABLED));
            distance.setOnSeekBarChangeListener(new SimpleSeekListener(distanceLabel, LABEL_DISTANCE));

            root.findViewById(com.wn.blackrussia.R.id.graphic_apply).setOnClickListener(view -> {
                applyGraphicsSettings(
                        water.getProgress(),
                        reflections.getProgress(),
                        distance.getProgress(),
                        lowVehicle.isChecked(),
                        optimized.isChecked(),
                        antialiasing.isChecked(),
                        enhancedSky.isChecked());
                window.dismiss();
            });

            root.findViewById(com.wn.blackrussia.R.id.graphic_close)
                    .setOnClickListener(view -> window.dismiss());

            window.setOnDismissListener(() -> mGraphicSettingsWindow = null);

            mGraphicSettingsWindow = window;
            window.showAtLocation(getParentLayout(), android.view.Gravity.CENTER, 0, 0);
        });
    }

    private static final int LABEL_QUALITY  = 0;
    private static final int LABEL_DISABLED = 1;
    private static final int LABEL_DISTANCE = 2;

    private static String settingName(int value) {
        switch (value) {
            case 0:  return "Стандартная";
            case 1:  return "Низкая";
            case 2:  return "Средняя";
            case 3:  return "Высокая";
            default: return "";
        }
    }

    private static String settingDisabledName(int value) {
        switch (value) {
            case 0:  return "Отключено";
            case 1:  return "Низкие";
            case 2:  return "Средние";
            case 3:  return "Высокие";
            default: return "";
        }
    }

    private static String drawSettingName(int value) {
        if (value <= 25) return "Низкая";
        if (value <= 50) return "Средняя";
        if (value <= 75) return "Высокая";
        return "Оч. Высокая";
    }

    private static String labelFor(int kind, int value) {
        if (kind == LABEL_DISABLED) return settingDisabledName(value);
        if (kind == LABEL_DISTANCE) return drawSettingName(value);
        return settingName(value);
    }

    private static class SimpleSeekListener implements android.widget.SeekBar.OnSeekBarChangeListener {
        private final android.widget.TextView label;
        private final int kind;

        SimpleSeekListener(android.widget.TextView label, int kind) {
            this.label = label;
            this.kind = kind;
        }

        @Override
        public void onProgressChanged(android.widget.SeekBar seekBar, int progress, boolean fromUser) {
            if (label == null) return;
            label.setText(labelFor(kind, progress));
        }

        @Override
        public void onStartTrackingTouch(android.widget.SeekBar seekBar) {}

        @Override
        public void onStopTrackingTouch(android.widget.SeekBar seekBar) {}
    }

    public native void displayAccessState(boolean state);
    public void SetVisibleKeyboardStandard(int active, int type)
    {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mKeyboardStandardManager.setVisible(active, type, false);
                mKeyboardStandardManager.setCallableEnter(new Runnable() {
                    @Override
                    public void run() {
                        String originalText = mKeyboardStandardManager.keyboardTextInput.getText().toString();
                        OnInputEnd(originalText);

                        if (originalText.length() > 0) {
                            mKeyboardStandardManager.addHistoryText(originalText);
                        }
                        mKeyboardStandardManager.keyboardTextInput.setText("");
                    }
                });

                mKeyboardStandardManager.setCallableClose(new Runnable() {
                    @Override
                    public void run() {
                        mKeyboardStandardManager.setVisible(0, 0, false);
                        OnInputEnd("");
                    }
                });
            }
        });
    }
}
