package blackrussia.online;

import android.util.Log;

import com.joom.paranoid.Obfuscate;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

@Obfuscate
public class Logger {
    private static final String FILENAME = "log.txt";
    public static final int LOG_LEVEL_DEBUG = 3;
    public static final int LOG_LEVEL_ERROR = 2;
    public static final int LOG_LEVEL_INFO = 0;
    public static final int LOG_LEVEL_WARNING = 1;
    static boolean mInited = false;
    private static String mPath;

    private static void logToFile(String str) {
        File file = new File(mPath);
        try {
            if (!file.exists()) file.createNewFile();
        } catch (IOException e) {
            e.printStackTrace();
        }
        FileWriter fileWriter = null;
        try {
            fileWriter = new FileWriter(new File(mPath), false);
            fileWriter.write(str);
            fileWriter.flush();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                if (fileWriter != null) fileWriter.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public static void init (String path) {
        if (mInited) {
            return;
        }
        mPath = path;
        try {
            new File(path + FILENAME).delete();
        } catch (Exception unused) {
        }
        mInited = true;
    }

    public static void NativeLog(int level, byte[] str) {
        try {
            Log(level, new String(str, "windows-1251"));
        } catch (Exception unused) {
        }
    }

    public static void Log(int level, String str) {
        if (level == LOG_LEVEL_INFO) {
            info(str);
        } else if (level == LOG_LEVEL_WARNING) {
            warn(str);
        } else if (level == LOG_LEVEL_ERROR) {
            error(str);
        } else if (level == LOG_LEVEL_DEBUG) {
            debug(str);
        } else {
            debug(str);
        }
    }

    public static void debug(String string) {
        Log.d("SAMP-DEBUG", string);
        String str = "[Debug] " + string;
        logToFile(str);
    }

    public static void info(String string) {
        Log.i("SAMP-INFO", string);
        String str = "[Info] " + string;
        logToFile(str);
    }

    public static void warn(String string) {
        Log.w("SAMP-WARN", string);
        String str = "[Warning] " + string;
        logToFile(str);
    }

    public static void error(String string) {
        Log.e("SAMP-ERROR", string);
        String str = "[Error] " + string;
        logToFile(str);
    }
}