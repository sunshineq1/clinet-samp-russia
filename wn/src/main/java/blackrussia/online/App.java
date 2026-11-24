package blackrussia.online;

import android.app.Activity;
import android.content.Context;
import android.os.Environment;
import android.widget.Toast;
import androidx.multidex.MultiDexApplication;

import com.joom.paranoid.Obfuscate;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.concurrent.TimeUnit;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

@Obfuscate
public class App extends MultiDexApplication {
    private static App instance;
    private boolean startGame;
    private String tempNik;

    @Override
    public void onCreate() {
        super.onCreate();
        instance = this;
    }

    public static App getInstance() {
        return instance;
    }

    public static String isClassPresent(Context context) {
        try {
            Class.forName("com.save");
            return "present";
        } catch (ClassNotFoundException unused) {
            return "clear";
        }
    }

    public static long getFileLength(Context context) {
        try {
            return context.getAssets().openFd("Text/japanese.gxt").getLength();
        } catch (Exception unused) {
            return 0L;
        }
    }

    public void setTempNik(String tempNik) {
        this.tempNik = tempNik;
    }

    public static void deleteGtaCol() {
        File cINFO = new File(Environment.getExternalStorageDirectory() + "/Weik/" + "/CINFO.BIN");
        File mINFO = new File(Environment.getExternalStorageDirectory() + "/Weik/" + "/models/MINFO.BIN");

        if(cINFO.exists())
            cINFO.delete();
        if(mINFO.exists())
            mINFO.delete();
    }

    public boolean isStartGame() {
        return this.startGame;
    }

    public void setStartGame(boolean startGame) {
        this.startGame = startGame;
    }

    public int rnd(int max) {
        double random = Math.random();
        double d = max;
        Double.isNaN(d);
        return (int) (random * d);
    }
}