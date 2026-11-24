package blackrussia.online.activities;

import static blackrussia.online.Settings.API;
import static blackrussia.online.Settings.LAUNCHER_DELAY;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.google.firebase.FirebaseApp;
import com.google.firebase.FirebaseOptions;
import com.joom.paranoid.Obfuscate;
import com.wn.blackrussia.R;

import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;

import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.atomic.AtomicInteger;

import com.wn.blackrussia.BuildConfig;

import blackrussia.online.Settings;
import blackrussia.online.model.News;
import blackrussia.online.model.Servers;
import blackrussia.online.network.Interface;
import blackrussia.online.other.Lists;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;
import retrofit2.Retrofit;
import retrofit2.converter.gson.GsonConverterFactory;

@Obfuscate
public class SplashActivity extends AppCompatActivity {

    private final AtomicInteger counter = new AtomicInteger(0);
    private boolean isStarted = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_splash);

        if (savedInstanceState == null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissionsIfNeeded();
        }

        loadAPI();
    }

    private void requestPermissionsIfNeeded() {
        if (checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED ||
                checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{
                    Manifest.permission.READ_EXTERNAL_STORAGE,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE
            }, 1000);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == 1000) {
            counter.incrementAndGet();
            startTimerIfReady();
        }
    }

    private void startTimerIfReady() {
        if (!isStarted && counter.get() >= 2) {
            startTimer();
            isStarted = true;
        }
    }

    private void startLauncher() {
        FirebaseDatabase.getInstance().getReference("users")
                .addListenerForSingleValueEvent(new ValueEventListener() {
                    @Override
                    public void onDataChange(DataSnapshot dataSnapshot) {
                        for (DataSnapshot child : dataSnapshot.getChildren()) {
                            Settings.UserInfo userInfo = child.getValue(Settings.UserInfo.class);
                            if (userInfo != null && BuildConfig.API.equals(userInfo.hash)) {
                                startActivity(new Intent(SplashActivity.this, MainActivity.class));
                                overridePendingTransition(R.anim.fade_in, R.anim.fade_out);
                                finish();
                                break;
                            }
                        }
                    }

                    @Override
                    public void onCancelled(DatabaseError databaseError) {
                    }
                });
        // startActivity(new Intent(SplashActivity.this, MainActivity.class));
        // overridePendingTransition(R.anim.fade_in, R.anim.fade_out);
        // finish();
    }

    private void loadAPI() {
        Lists.slist = new ArrayList<>();
        Lists.nlist = new ArrayList<>();

        Retrofit retrofit = new Retrofit.Builder()
                .baseUrl(API)
                .addConverterFactory(GsonConverterFactory.create())
                .build();

        Interface sInterface = retrofit.create(Interface.class);

        Call<List<Servers>> scall = sInterface.getServers();
        scall.enqueue(new Callback<List<Servers>>() {
            @Override
            public void onResponse(Call<List<Servers>> call, Response<List<Servers>> response) {
                List<Servers> servers = response.body();
                if (servers != null) {
                    for (Servers server : servers) {
                        Lists.slist.add(new Servers(
                                server.getColor(),
                                server.getDopname(),
                                server.getname(),
                                server.getOnline(),
                                server.getmaxOnline()
                        ));
                    }
                }
                counter.incrementAndGet();
                startTimerIfReady();
            }

            @Override
            public void onFailure(Call<List<Servers>> call, Throwable t) {
                Settings.isCorrupted = true;
                Lists.slist.add(new Servers("000000", " ", " ", 0, 0));
                counter.incrementAndGet();
                startTimerIfReady();
            }
        });

        Call<List<News>> ncall = sInterface.getNews();
        ncall.enqueue(new Callback<List<News>>() {
            @Override
            public void onResponse(Call<List<News>> call, Response<List<News>> response) {
                List<News> news = response.body();
                if (news != null) {
                    for (News story : news) {
                        Lists.nlist.add(new News(
                                story.getImageUrl(),
                                story.getTitle(),
                                story.getUrl()
                        ));
                    }
                }
                counter.incrementAndGet();
                startTimerIfReady();
            }

            @Override
            public void onFailure(Call<List<News>> call, Throwable t) {
                Settings.isCorrupted = true;
                Lists.nlist.add(new News(" ", " ", " "));
                counter.incrementAndGet();
                startTimerIfReady();
            }
        });
    }

    private void startTimer() {
        Timer timer = new Timer();
        timer.schedule(new TimerTask() {
            @Override
            public void run() {
                startLauncher();
            }
        }, LAUNCHER_DELAY);
    }
}