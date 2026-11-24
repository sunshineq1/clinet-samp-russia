package blackrussia.online.activities;

import static blackrussia.online.Settings.GAME_PATH;
import static blackrussia.online.Settings.LAUNCHER_JSON_URL;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Build;

import android.os.Environment;
import android.text.Html;
import android.widget.*;
import android.graphics.PorterDuff;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import android.content.Intent;

import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentTransaction;

import android.view.View.OnClickListener;
import android.view.View;
import android.view.animation.AnimationUtils;
import android.view.animation.Animation;

import com.google.firebase.analytics.FirebaseAnalytics;
import com.google.firebase.messaging.FirebaseMessaging;

import blackrussia.online.App;
import blackrussia.online.Downloader;
import blackrussia.online.Logger;

import com.joom.paranoid.Obfuscate;
import com.wn.blackrussia.R;
import com.wn.blackrussia.game.Multiplayer;

import blackrussia.online.Settings;
import blackrussia.online.Utils;
import blackrussia.online.fragments.MonitoringFragment;
import blackrussia.online.fragments.DonateFragment;
import blackrussia.online.fragments.SettingsFragment;

import org.ini4j.Wini;
import org.json.JSONArray;
import org.json.JSONObject;

import java.io.File;

import java.io.IOException;
import java.util.Timer;
import java.util.TimerTask;

import blackrussia.online.Preferences;
import blackrussia.online.other.GetMicrophonePermission;
import blackrussia.online.other.GetStoragePermission;
import blackrussia.online.other.VersionComparator;

import org.json.JSONException;

import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;

@Obfuscate
public class MainActivity extends AppCompatActivity {
    public static final String EXTRA_OPEN_SETTINGS = "open_settings";

    private int PERMISSION_STORAGE = 101;
    private int PERMISSION_MICRO = 102;
	public LinearLayout donateButton;
    public DonateFragment donateFragment;
    public TextView donateTV;
    public LinearLayout monitoringButton;
    public MonitoringFragment monitoringFragment;
    private ProgressDialog progressDialog;
    private String renderer;
    public ImageView monitoringImage, donateImage, playImage, settingsImage;
    public TextView monitoringTV;
    public LinearLayout playButton, settingsButton;
    public SettingsFragment settingsFragment;
    public TextView settingsTV;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        View decorView = getWindow().getDecorView();
        int uiOptions = View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
        decorView.setSystemUiVisibility(uiOptions);

        monitoringFragment = new MonitoringFragment();
        settingsFragment = new SettingsFragment();
        donateFragment = new DonateFragment();
        replaceFragment(monitoringFragment);

		Animation animation = AnimationUtils.loadAnimation(this, R.anim.button_click);
		
		monitoringTV = (TextView) findViewById(R.id.monitoringTV);
        settingsTV = (TextView) findViewById(R.id.settingsTV);
        donateTV = (TextView) findViewById(R.id.donateTV);

        ProgressDialog progressDialog = new ProgressDialog(this, R.style.MyTheme);
        this.progressDialog = progressDialog;
        progressDialog.setIndeterminateDrawable(ContextCompat.getDrawable(this, R.drawable.progressbar_custom));
        this.progressDialog.setCancelable(false);
        this.progressDialog.setProgressStyle(16973854);

        monitoringImage = (ImageView) findViewById(R.id.monitoringImage);
        settingsImage = (ImageView) findViewById(R.id.settingsImage);
        donateImage = (ImageView) findViewById(R.id.donateImage);
        playImage = (ImageView) findViewById(R.id.playImage);
		
        monitoringButton = (LinearLayout) findViewById(R.id.monitoringButton);
        settingsButton = (LinearLayout) findViewById(R.id.settingsButton);
        donateButton = (LinearLayout) findViewById(R.id.donateButton);
        playButton = (LinearLayout) findViewById(R.id.playButton);

        initLogger();
		
		monitoringButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                v.startAnimation(animation);
                onClickMonitoring();
            }
        });
		
        settingsButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                v.startAnimation(animation);
                onClickSettings();
            }
        });

        donateButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
               v.startAnimation(animation);
			   onClickDonate();
            }
        });
		
        playButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
               v.startAnimation(animation);
               progressDialog.show();

               Timer t = new Timer();
               t.schedule(new TimerTask() {
                   @Override
                   public void run() {
                       if (progressDialog != null) {
                           progressDialog.dismiss();
                       }

                       onClickPlay();
                   }
               }, 800L);
            }
        });

        String stringExtra = Preferences.getString(getApplicationContext(), Preferences.NIK);
        if (stringExtra != null) {
            FirebaseAnalytics firebaseAnalytics = FirebaseAnalytics.getInstance(getApplicationContext());
            Bundle bundle = new Bundle();
            bundle.putString("open", stringExtra);
            firebaseAnalytics.logEvent("push", bundle);
        }

        FirebaseMessaging.getInstance().subscribeToTopic("all");

        if (!Preferences.getBoolean(this, Preferences.PRIVACY)) {
            Preferences.clear(this, Preferences.NIK);
            privacyPolicy();
        }

        long freeMemory = Utils.getFreeMemory();
        if (freeMemory < 2000)
            Toast.makeText(getApplicationContext(), String.format(getString(R.string.warning_free_memory), Long.valueOf(freeMemory)), Toast.LENGTH_LONG).show();

        if (getIntent().getBooleanExtra(EXTRA_OPEN_SETTINGS, false)) {
            onClickSettings();
        }
    }
    public void onPause() {
        ProgressDialog progressDialog = this.progressDialog;
        if (progressDialog != null && progressDialog.isShowing()) {
            this.progressDialog.dismiss();
            this.progressDialog = null;
        }
        if (this.progressDialog != null) {
            this.progressDialog = null;
        }
        super.onPause();
    }
    @Override
    public void onResume() {
        super.onResume();
        if (this.progressDialog == null) {
            ProgressDialog progressDialog = new ProgressDialog(this, R.style.MyTheme);
            this.progressDialog = progressDialog;
            progressDialog.setIndeterminateDrawable(ContextCompat.getDrawable(this, R.drawable.progressbar_custom));
            this.progressDialog.setCancelable(false);
            this.progressDialog.setProgressStyle(16973854);
        }
    }

    public String getDataPath() {
        if (Environment.getExternalStorageState().equals("mounted")) {
            File externalFilesDir = getExternalFilesDir(null);
            return externalFilesDir.getAbsolutePath() + "/";
        }
        return null;
    }

    private void initLogger() {
        try {
            Logger.init(GAME_PATH + "/SAMP/");
        } catch (Exception unused) {
        }
    }

	public void onClickPlay() {
        if(Settings.isCorrupted)
            return;

        if (!GetMicrophonePermission.ISPermission(MainActivity.this)) {
            GetMicrophonePermission.requestPermission(MainActivity.this, PERMISSION_MICRO);
            return;
        }
        if (!GetStoragePermission.ISPermission(MainActivity.this)) {
            GetStoragePermission.requestPermission(MainActivity.this, PERMISSION_STORAGE);
            return;
        }

        try {
            File f = new File(GAME_PATH + "/SAMP/settings.ini");
            if(f.exists()) {
                Wini w = new Wini(new File(GAME_PATH + "/SAMP/settings.ini"));
                if(!w.get("client", "name").isEmpty()) {
                    com.wn.storage.Preferences.setNick(w.get("client", "name"));
                    // Preferences.putString(MainActivity.this, Preferences.NIK, w.get("client", "name"));
                }
                w.store();
            } else {
                Preferences.clear(this, Preferences.NIK);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        startActivity(new Intent(getApplicationContext(), /*Downloader.class*/Multiplayer.class));
        overridePendingTransition(R.anim.fade_in, R.anim.fade_out);
        finish();
    }

    public void onClickSettings() {
            setTextColor(settingsButton, settingsTV, settingsImage);
            replaceFragment(this.settingsFragment);
    }

    public void onClickDonate() {
            setTextColor(donateButton, donateTV, donateImage);
            replaceFragment(donateFragment);
    }
    
	public void onClickMonitoring() {
            setTextColor(monitoringButton, monitoringTV, monitoringImage);
            replaceFragment(monitoringFragment);
    }

    @SuppressLint("NewApi")
    public void setTextColor(LinearLayout linearLayout, TextView textView, ImageView imageView) {
        monitoringButton.setAlpha(0.45f);
        settingsButton.setAlpha(0.45f);
        donateButton.setAlpha(0.45f);
        monitoringTV.setTextColor(getResources().getColor(R.color.menuTextDisable));
        settingsTV.setTextColor(getResources().getColor(R.color.menuTextDisable));
        donateTV.setTextColor(getResources().getColor(R.color.menuTextDisable));
        monitoringImage.setColorFilter(getResources().getColor(R.color.menuTextDisable), PorterDuff.Mode.SRC_IN);
        settingsImage.setColorFilter(getResources().getColor(R.color.menuTextDisable), PorterDuff.Mode.SRC_IN);
        donateImage.setColorFilter(getResources().getColor(R.color.menuTextDisable), PorterDuff.Mode.SRC_IN);
        linearLayout.setAlpha(1.0f);
        textView.setTextColor(getResources().getColor(R.color.menuTextEnable));
        imageView.setColorFilter(getResources().getColor(R.color.menuTextEnable), PorterDuff.Mode.SRC_IN);
    }

    public void replaceFragment(Fragment fragment) {
        FragmentTransaction beginTransaction = getSupportFragmentManager().beginTransaction();
        beginTransaction.replace(R.id.container, fragment);
        beginTransaction.commit();
    }
	
	@SuppressLint("WrongConstant")
    public boolean isRecordAudioPermissionGranted() {
        if (Build.VERSION.SDK_INT < 23 || checkSelfPermission("android.permission.RECORD_AUDIO") == 0) {
            return true;
        }
        ActivityCompat.requestPermissions(this, new String[]{"android.permission.RECORD_AUDIO"}, 2);
        return false;
    }

    @SuppressLint("WrongConstant")
    public boolean isStoragePermissionGranted() {
        if (Build.VERSION.SDK_INT < 23 || checkSelfPermission("android.permission.WRITE_EXTERNAL_STORAGE") == 0) {
            return true;
        }
        ActivityCompat.requestPermissions(this, new String[]{"android.permission.WRITE_EXTERNAL_STORAGE"}, 1);
        return false;
    }
	
	public void onDestroy() {
        super.onDestroy();
        
    }

    public void onRestart() {
        super.onRestart();
    }

    @SuppressLint("ResourceType")
    private void privacyPolicy() {
        Dialog dialog = new Dialog(this);
        dialog.setContentView(R.layout.item_dialog_privacy);
        dialog.setCancelable(false);
        dialog.getWindow().setBackgroundDrawableResource(17170445);
        dialog.getWindow().setLayout(-1, -2);
        TextView textView = (TextView) dialog.findViewById(R.id.message);
        textView.setText(Html.fromHtml("<u>" + getResources().getString(R.string.privacyPolicy2) + "</u>"));
        textView.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                startActivity(new Intent("android.intent.action.VIEW", Uri.parse("http://api.blackrussia.online/launchpolitic.pdf")));
            }
        });
        ((TextView) dialog.findViewById(R.id.cancel)).setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                view.startAnimation(AnimationUtils.loadAnimation(getApplicationContext(), R.anim.button_click));

                dialog.dismiss();
                finish();
            }
        });
        TextView textView2 = (TextView) dialog.findViewById(R.id.ok);
        textView2.setText(getResources().getString(R.string.ok));
        textView2.setOnClickListener(new OnClickListener() {
            @Override
            public final void onClick(View view) {
                view.startAnimation(AnimationUtils.loadAnimation(getApplicationContext(), R.anim.button_click));

                dialog.dismiss();
                Preferences.putBoolean(getApplicationContext(), Preferences.PRIVACY, true);
            }
        });
        dialog.show();
    }
} 