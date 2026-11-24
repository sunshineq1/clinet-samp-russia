package blackrussia.online;

import static blackrussia.online.Settings.API;
import static blackrussia.online.Settings.GAME_PATH;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.view.WindowManager;
import android.view.animation.AnimationUtils;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;

import androidx.viewpager2.widget.ViewPager2;

import com.akexorcist.roundcornerprogressbar.RoundCornerProgressBar;
import com.joom.paranoid.Obfuscate;
import com.wn.blackrussia.R;
import com.wn.blackrussia.game.Multiplayer;
import com.wn.blackrussia.game.ui.util.Util;

import blackrussia.online.activities.MainActivity;
import blackrussia.online.adapters.SliderTextAdapter;

import org.ini4j.Wini;

import blackrussia.online.network.Interface;
import blackrussia.online.network.TextLoader;
import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;
import retrofit2.Retrofit;
import retrofit2.converter.gson.GsonConverterFactory;

@Obfuscate
public class Downloader extends AppCompatActivity implements DownloadService.Listener {
    private TextView loadingInfo, fileText;
    private RoundCornerProgressBar progressLoading;
    private TextView percentsCommonInfo;
    private Handler handler;
    private TextView repeatButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_loader);
        initialize(savedInstanceState);

        handler = new Handler();

        Retrofit retrofit = new Retrofit.Builder()
                .baseUrl(API)
                .addConverterFactory(GsonConverterFactory.create())
                .build();
        Interface sInterface = retrofit.create(Interface.class);

        repeatButton = findViewById(R.id.repeatButton);
        repeatButton.setOnClickListener(view -> {
            long freeMemory = Utils.getFreeMemory();
            if (freeMemory < 2000) {
                Toast.makeText(getApplicationContext(), String.format(getString(R.string.warning_free_memory), freeMemory), Toast.LENGTH_LONG).show();
                startActivity(new Intent(getApplicationContext(), MainActivity.class));
                finish();
                return;
            }
            InstallGame();
            repeatButton.setVisibility(View.INVISIBLE);
        });

        ViewPager2 viewPager2 = findViewById(R.id.viewPager);
        SliderTextAdapter sliderTextAdapter = new SliderTextAdapter(this);
        viewPager2.setAdapter(sliderTextAdapter);
        viewPager2.registerOnPageChangeCallback(new AnonymousClass1(handler, viewPager2));

        sInterface.getTextLoader().enqueue(new Callback<ArrayList<TextLoader>>() {
            @Override
            public void onFailure(Call<ArrayList<TextLoader>> call, Throwable t) {}

            @Override
            public void onResponse(Call<ArrayList<TextLoader>> call, Response<ArrayList<TextLoader>> response) {
                if (response.body() != null) {
                    ArrayList<TextLoader> body = response.body();
                    sliderTextAdapter.addItems(body);
                    viewPager2.setCurrentItem(body.size() * 10, false);
                }
            }
        });

        ((ImageView) findViewById(R.id.leftButton)).setOnClickListener(view -> {
            view.startAnimation(AnimationUtils.loadAnimation(getApplicationContext(), R.anim.button_click));
            viewPager2.setCurrentItem(viewPager2.getCurrentItem() - 1);
        });
        ((ImageView) findViewById(R.id.rightButton)).setOnClickListener(view -> {
            view.startAnimation(AnimationUtils.loadAnimation(getApplicationContext(), R.anim.button_click));
            viewPager2.setCurrentItem(viewPager2.getCurrentItem() + 1);
        });

        requestRuntimePermissions();
    }

    private void requestRuntimePermissions() {
        if (Build.VERSION.SDK_INT >= 33 &&
                checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) == PackageManager.PERMISSION_DENIED) {
            requestPermissions(new String[] { Manifest.permission.POST_NOTIFICATIONS }, 1001);
            return;
        }

        if (Build.VERSION.SDK_INT >= 23 &&
                (checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_DENIED ||
                        checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_DENIED)) {
            requestPermissions(new String[] {
                    Manifest.permission.READ_EXTERNAL_STORAGE,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1000);
        } else {
            checkAndInstall();
        }
    }

    private void checkAndInstall() {
        if (DownloadService.getState() == DownloadService.STATE_RUNNING) {
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            return;
        }

        long freeMemory = Utils.getFreeMemory();
        if (freeMemory < 2000) {
            Toast.makeText(getApplicationContext(), String.format(getString(R.string.warning_free_memory), freeMemory), Toast.LENGTH_LONG).show();
            startActivity(new Intent(getApplicationContext(), MainActivity.class));
            finish();
        } else {
            InstallGame();
        }
    }

    class AnonymousClass1 extends ViewPager2.OnPageChangeCallback {
        Handler _handler;
        ViewPager2 _viewPager;

        AnonymousClass1(Handler handler, ViewPager2 viewPager) {
            _handler = handler;
            _viewPager = viewPager;
        }

        @Override
        public void onPageSelected(int position) {
            super.onPageSelected(position);
            _handler.removeCallbacksAndMessages(null);
            _handler.postDelayed(() -> _viewPager.setCurrentItem(_viewPager.getCurrentItem() + 1), 5000L);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == 1001) {
            requestRuntimePermissions();
        } else if (requestCode == 1000) {
            checkAndInstall();
        }
    }

    @Override
    protected void onStart() {
        super.onStart();
        DownloadService.setListener(this);
    }

    @Override
    protected void onStop() {
        DownloadService.setListener(null);
        super.onStop();
    }

    @Override
    public void onProgress(int phase, int percent, String fileName, long current, long total) {
        switch (phase) {
            case DownloadService.PHASE_PREPARING:
                loadingInfo.setText("Подготовка...");
                fileText.setText("");
                break;
            case DownloadService.PHASE_CHECKING:
                loadingInfo.setText("Проверка файлов...");
                fileText.setText("");
                break;
            default:
                loadingInfo.setText("Загрузка: " + fileName);
                fileText.setText(formatSize(current) + " из " + formatSize(total));
                break;
        }
        percentsCommonInfo.setText(percent + "%");
        progressLoading.setProgress(percent);
    }

    @Override
    public void onFinished(boolean success, String error) {
        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        DownloadService.resetState();

        if (success) {
            checkNickAndStart();
        } else {
            if (error != null) {
                Toast.makeText(this, error, Toast.LENGTH_LONG).show();
            }
            repeatButton.setVisibility(View.VISIBLE);
        }
    }

    private String formatSize(long bytes) {
        if (bytes < 1024) return bytes + " B";
        int exp = (int) (Math.log(bytes) / Math.log(1024));
        String pre = "KMGTPE".charAt(exp - 1) + "";
        return String.format("%.1f %sB", bytes / Math.pow(1024, exp), pre);
    }

    private void checkNickAndStart() {
        String nick = "";
        File f = new File(GAME_PATH + "/SAMP/settings.ini");
        if (f.exists()) {
            try {
                Wini w = new Wini(f);
                nick = w.get("client", "name");
            } catch (IOException e) { e.printStackTrace(); }
        }

        if (nick == null || nick.isEmpty() || !nick.contains("_") || nick.length() < 4) {
            Toast.makeText(this, "Проверьте никнейм (формат Name_Surname)", Toast.LENGTH_SHORT).show();
            Intent i = new Intent(this, MainActivity.class);
            i.putExtra(MainActivity.EXTRA_OPEN_SETTINGS, true);
            startActivity(i);
        } else {
            Util.HideLayout(findViewById(R.id.root), true);
            startActivity(new Intent(this, Multiplayer.class));
        }
        finish();
    }

    private void InstallGame() {
        loadingInfo.setVisibility(View.VISIBLE);
        progressLoading.setVisibility(View.VISIBLE);
        progressLoading.setMax(100);
        progressLoading.setProgress(0);
        percentsCommonInfo.setText("0%");
        repeatButton.setVisibility(View.GONE);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        DownloadService.resetState();
        DownloadService.start(this);
    }

    private void initialize(Bundle savedInstanceState) {
        loadingInfo = findViewById(R.id.loadingText);
        progressLoading = findViewById(R.id.progressBar);
        fileText = findViewById(R.id.fileName);
        percentsCommonInfo = findViewById(R.id.loadingPercent);
    }

    @Override
    public void onBackPressed() {

    }

    @Override
    protected void onDestroy() {
        DownloadService.setListener(null);
        super.onDestroy();
    }
}
