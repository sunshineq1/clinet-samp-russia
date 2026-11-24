package blackrussia.online;

import static blackrussia.online.Settings.GAME_PATH;
import static blackrussia.online.Settings.GAME_URL;
import static blackrussia.online.Settings.GAME_URL_2;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.PowerManager;

import androidx.core.app.NotificationCompat;

import com.joom.paranoid.Obfuscate;
import com.wn.blackrussia.R;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.RandomAccessFile;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicBoolean;

import blackrussia.online.other.FileCheckSumSHA;

@Obfuscate
public class DownloadService extends Service {

    public static final String ACTION_START  = "blackrussia.online.action.DOWNLOAD_START";
    public static final String ACTION_CANCEL = "blackrussia.online.action.DOWNLOAD_CANCEL";

    private static final String CHANNEL_ID = "br_download";
    private static final int NOTIFICATION_ID = 2;

    private static final int CONNECT_TIMEOUT_MS = 15000;
    private static final int READ_TIMEOUT_MS    = 30000;
    private static final int MAX_ATTEMPTS       = 3;
    private static final int BUFFER_SIZE        = 65536;
    private static final long UI_UPDATE_PERIOD_MS  = 100;
    private static final long NOTIFY_UPDATE_PERIOD_MS = 1000;

    public static final int PHASE_PREPARING = 0;
    public static final int PHASE_CHECKING  = 1;
    public static final int PHASE_DOWNLOADING = 2;

    public static final int STATE_IDLE    = 0;
    public static final int STATE_RUNNING = 1;
    public static final int STATE_DONE    = 2;
    public static final int STATE_FAILED  = 3;

    public interface Listener {
        void onProgress(int phase, int percent, String fileName, long current, long total);
        void onFinished(boolean success, String error);
    }

    private static final Handler sMain = new Handler(Looper.getMainLooper());
    private static volatile Listener sListener;

    private static volatile int    sState   = STATE_IDLE;
    private static volatile int    sPhase   = PHASE_PREPARING;
    private static volatile int    sPercent = 0;
    private static volatile String sFileName = "";
    private static volatile long   sCurrent = 0;
    private static volatile long   sTotal   = 0;
    private static volatile String sError   = null;

    private final AtomicBoolean mCancelled = new AtomicBoolean(false);
    private ExecutorService mExecutor;
    private NotificationManager mNotificationManager;
    private PowerManager.WakeLock mWakeLock;
    private long mLastNotifyMs = 0;

    public static void start(Context context) {
        Intent intent = new Intent(context, DownloadService.class);
        intent.setAction(ACTION_START);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            context.startForegroundService(intent);
        } else {
            context.startService(intent);
        }
    }

    public static void cancel(Context context) {
        Intent intent = new Intent(context, DownloadService.class);
        intent.setAction(ACTION_CANCEL);
        context.startService(intent);
    }

    public static void setListener(Listener listener) {
        sListener = listener;
        if (listener == null) return;

        final int phase = sPhase, percent = sPercent;
        final String file = sFileName;
        final long cur = sCurrent, total = sTotal;
        final int state = sState;
        final String error = sError;

        sMain.post(() -> {
            Listener l = sListener;
            if (l == null) return;
            l.onProgress(phase, percent, file, cur, total);
            if (state == STATE_DONE)   l.onFinished(true, null);
            if (state == STATE_FAILED) l.onFinished(false, error);
        });
    }

    public static int getState() {
        return sState;
    }

    public static void resetState() {
        if (sState == STATE_DONE || sState == STATE_FAILED) {
            sState = STATE_IDLE;
            sError = null;
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mNotificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        createChannel();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        final String action = intent != null ? intent.getAction() : null;

        if (ACTION_CANCEL.equals(action)) {
            mCancelled.set(true);
            stopSelf();
            return START_NOT_STICKY;
        }

        if (sState == STATE_RUNNING) {
            return START_NOT_STICKY;
        }

        sState = STATE_RUNNING;
        sError = null;
        sPercent = 0;
        sPhase = PHASE_PREPARING;
        sFileName = "";
        sCurrent = sTotal = 0;
        mCancelled.set(false);

        startForegroundCompat(buildNotification("Подготовка...", 0, true));
        acquireWakeLock();

        if (mExecutor == null || mExecutor.isShutdown()) {
            mExecutor = Executors.newSingleThreadExecutor();
        }
        mExecutor.execute(this::run);

        return START_NOT_STICKY;
    }

    @Override
    public void onDestroy() {
        mCancelled.set(true);
        if (mExecutor != null) mExecutor.shutdownNow();
        releaseWakeLock();
        super.onDestroy();
    }

    @Override
    public void onTaskRemoved(Intent rootIntent) {
        super.onTaskRemoved(rootIntent);
    }

    private void run() {
        boolean success = false;
        String error = null;
        try {
            Map<String, String> hashes = new LinkedHashMap<>();
            Map<String, Long> sizes = new LinkedHashMap<>();

            if (!fetchManifest(hashes, sizes)) {
                error = "Не удалось получить список файлов";
            } else {
                List<String> outdated = collectOutdated(hashes, sizes);
                if (outdated == null) {
                    error = "Проверка файлов прервана";
                } else if (downloadAll(outdated, sizes)) {
                    success = true;
                } else {
                    error = mCancelled.get() ? "Отменено" : "Ошибка загрузки файлов";
                }
            }
        } catch (Throwable t) {
            error = t.getMessage() != null ? t.getMessage() : t.getClass().getSimpleName();
        }

        if (mCancelled.get()) {
            success = false;
            if (error == null) error = "Отменено";
        }

        sState = success ? STATE_DONE : STATE_FAILED;
        sError = error;

        final boolean fSuccess = success;
        final String fError = error;
        sMain.post(() -> {
            Listener l = sListener;
            if (l != null) l.onFinished(fSuccess, fError);
        });

        releaseWakeLock();
        stopForegroundCompat();

        if (!success && !mCancelled.get()) {
            mNotificationManager.notify(NOTIFICATION_ID,
                    buildNotification("Загрузка не завершена: " + fError, 0, false));
        }

        stopSelf();
    }

    private boolean fetchManifest(Map<String, String> hashes, Map<String, Long> sizes) {
        File dir = new File(GAME_PATH);
        if (!dir.exists() && !dir.mkdirs()) return false;

        for (int attempt = 1; attempt <= MAX_ATTEMPTS && !mCancelled.get(); attempt++) {
            HttpURLConnection connection = null;
            try {
                connection = openConnection(new URL(GAME_URL_2), -1);
                if (connection.getResponseCode() != HttpURLConnection.HTTP_OK) continue;

                StringBuilder builder = new StringBuilder();
                try (BufferedReader reader = new BufferedReader(
                        new InputStreamReader(connection.getInputStream()))) {
                    String line;
                    while ((line = reader.readLine()) != null) builder.append(line);
                }

                JSONObject json = new JSONObject(builder.toString());
                int allCount = json.length();
                int counter = 0;

                for (Iterator<String> it = json.keys(); it.hasNext(); ) {
                    if (mCancelled.get()) return false;
                    String key = it.next();
                    JSONArray array = json.optJSONArray(key);
                    if (array == null || array.length() < 2) continue;

                    hashes.put(key, array.getString(0));
                    sizes.put(key, array.getLong(1));

                    counter++;
                    publish(PHASE_PREPARING, (int) (counter * 100L / Math.max(1, allCount)), "", 0, 0);
                }
                return !hashes.isEmpty();
            } catch (Exception ignored) {
            } finally {
                if (connection != null) connection.disconnect();
            }
        }
        return false;
    }

    private List<String> collectOutdated(Map<String, String> hashes, Map<String, Long> sizes) {
        List<String> outdated = new ArrayList<>();
        MessageDigest md;
        try {
            md = MessageDigest.getInstance("SHA-256");
        } catch (Exception e) {
            return null;
        }

        int total = hashes.size();
        int checked = 0;

        for (Map.Entry<String, String> entry : hashes.entrySet()) {
            if (mCancelled.get()) return null;

            String key = entry.getKey();
            File file = new File(GAME_PATH + "/" + key);

            boolean needs;
            if (!file.exists()) {
                needs = true;
            } else if (isStaticFile(key)) {
                needs = false;
            } else {
                Long expectedSize = sizes.get(key);
                if (expectedSize != null && expectedSize > 0 && file.length() != expectedSize) {
                    needs = true;
                } else {
                    String hash = null;
                    try {
                        hash = FileCheckSumSHA.getCheckSum(file.getAbsolutePath(), md);
                    } catch (IOException ignored) {
                    }
                    needs = hash == null || !hash.equals(entry.getValue());
                }
            }

            if (needs) outdated.add(key);

            checked++;
            publish(PHASE_CHECKING, (int) (checked * 100L / Math.max(1, total)), file.getName(), 0, 0);
        }
        return outdated;
    }

    private boolean downloadAll(List<String> keys, Map<String, Long> sizes) {
        if (keys.isEmpty()) return true;

        long totalBytes = 0;
        for (String key : keys) {
            Long size = sizes.get(key);
            if (size != null && size > 0) totalBytes += size;
        }

        long doneBytes = 0;
        for (String key : keys) {
            if (mCancelled.get()) return false;

            Long expected = sizes.get(key);
            long fileSize = expected != null ? expected : 0;

            if (!downloadFile(key, fileSize, doneBytes, totalBytes)) return false;

            doneBytes += fileSize > 0 ? fileSize : new File(GAME_PATH + "/" + key).length();
        }
        return true;
    }

    private boolean downloadFile(String key, long expectedSize, long doneBytes, long totalBytes) {
        final File target = new File(GAME_PATH + "/" + key);
        final File part = new File(target.getAbsolutePath() + ".part");

        File parent = target.getParentFile();
        if (parent != null && !parent.exists() && !parent.mkdirs()) return false;

        for (int attempt = 1; attempt <= MAX_ATTEMPTS && !mCancelled.get(); attempt++) {
            HttpURLConnection connection = null;
            try {
                long offset = part.exists() ? part.length() : 0;
                if (expectedSize > 0 && offset > expectedSize) {
                    if (!part.delete()) return false;
                    offset = 0;
                }

                connection = openConnection(new URL(GAME_URL + "/" + encodePath(key)), offset);
                int code = connection.getResponseCode();

                if (code == HttpURLConnection.HTTP_PARTIAL) {
                    // сервер продолжил с offset
                } else if (code == HttpURLConnection.HTTP_OK) {
                    offset = 0;
                } else {
                    continue;
                }

                long contentLength = connection.getContentLengthLong();
                long fileSize = expectedSize > 0 ? expectedSize
                        : (contentLength > 0 ? contentLength + offset : 0);

                try (InputStream is = connection.getInputStream();
                     RandomAccessFile out = new RandomAccessFile(part, "rw")) {

                    out.setLength(offset);
                    out.seek(offset);

                    byte[] buffer = new byte[BUFFER_SIZE];
                    long written = offset;
                    int read;

                    while ((read = is.read(buffer)) != -1) {
                        if (mCancelled.get()) return false;
                        out.write(buffer, 0, read);
                        written += read;

                        int percent = totalBytes > 0
                                ? (int) ((doneBytes + written) * 100L / totalBytes)
                                : (fileSize > 0 ? (int) (written * 100L / fileSize) : 0);

                        publish(PHASE_DOWNLOADING, Math.min(100, percent),
                                target.getName(), written, fileSize > 0 ? fileSize : written);
                    }
                }

                if (expectedSize > 0 && part.length() != expectedSize) {
                    continue;
                }

                if (target.exists() && !target.delete()) return false;
                if (!part.renameTo(target)) return false;
                return true;

            } catch (Exception ignored) {
            } finally {
                if (connection != null) connection.disconnect();
            }
        }
        return false;
    }

    private static String encodePath(String key) {
        StringBuilder sb = new StringBuilder();
        for (String segment : key.split("/")) {
            if (sb.length() > 0) sb.append('/');
            try {
                sb.append(URLEncoder.encode(segment, "UTF-8").replace("+", "%20"));
            } catch (IOException e) {
                sb.append(segment);
            }
        }
        return sb.toString();
    }

    private HttpURLConnection openConnection(URL url, long offset) throws IOException {
        HttpURLConnection connection = (HttpURLConnection) url.openConnection();
        connection.setConnectTimeout(CONNECT_TIMEOUT_MS);
        connection.setReadTimeout(READ_TIMEOUT_MS);
        connection.setInstanceFollowRedirects(true);
        connection.setRequestProperty("Accept-Encoding", "identity");
        if (offset > 0) connection.setRequestProperty("Range", "bytes=" + offset + "-");
        connection.connect();
        return connection;
    }

    private static boolean isStaticFile(String key) {
        return key.equals("gta_sa.set") || key.equals("SAMP/settings.ini") ||
                key.equals("wn_graphics.cfg") || key.equals("gtasatelem.set") ||
                key.equals("reBRClientLog.txt") || key.equals("crash_log.txt") ||
                key.equals("Adjustable.cfg");
    }

    private long mLastPublishMs = 0;

    private void publish(int phase, int percent, String fileName, long current, long total) {
        sPhase = phase;
        sPercent = percent;
        sFileName = fileName;
        sCurrent = current;
        sTotal = total;

        long now = android.os.SystemClock.uptimeMillis();
        if (now - mLastPublishMs < UI_UPDATE_PERIOD_MS && percent < 100) return;
        mLastPublishMs = now;

        sMain.post(() -> {
            Listener l = sListener;
            if (l != null) l.onProgress(phase, percent, fileName, current, total);
        });

        if (now - mLastNotifyMs >= NOTIFY_UPDATE_PERIOD_MS || percent >= 100) {
            mLastNotifyMs = now;
            String text;
            switch (phase) {
                case PHASE_PREPARING: text = "Подготовка..."; break;
                case PHASE_CHECKING:  text = "Проверка файлов..."; break;
                default:              text = "Загрузка: " + fileName; break;
            }
            mNotificationManager.notify(NOTIFICATION_ID, buildNotification(text, percent, false));
        }
    }

    private void createChannel() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return;
        NotificationChannel channel = new NotificationChannel(CHANNEL_ID,
                "Загрузка файлов игры", NotificationManager.IMPORTANCE_LOW);
        channel.setShowBadge(false);
        channel.setSound(null, null);
        mNotificationManager.createNotificationChannel(channel);
    }

    private Notification buildNotification(String text, int percent, boolean indeterminate) {
        Intent openIntent = new Intent(this, Downloader.class);
        openIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);

        int flags = PendingIntent.FLAG_UPDATE_CURRENT;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) flags |= PendingIntent.FLAG_IMMUTABLE;
        PendingIntent contentIntent = PendingIntent.getActivity(this, 0, openIntent, flags);

        Intent cancelIntent = new Intent(this, DownloadService.class);
        cancelIntent.setAction(ACTION_CANCEL);
        PendingIntent cancelPending = PendingIntent.getService(this, 1, cancelIntent, flags);

        return new NotificationCompat.Builder(this, CHANNEL_ID)
                .setSmallIcon(R.drawable.logo_splash)
                .setContentTitle("Black Russia")
                .setContentText(text)
                .setContentIntent(contentIntent)
                .setOngoing(sState == STATE_RUNNING)
                .setOnlyAlertOnce(true)
                .setPriority(NotificationCompat.PRIORITY_LOW)
                .setProgress(100, percent, indeterminate)
                .addAction(0, "Отмена", cancelPending)
                .build();
    }

    private void startForegroundCompat(Notification notification) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            startForeground(NOTIFICATION_ID, notification,
                    ServiceInfo.FOREGROUND_SERVICE_TYPE_DATA_SYNC);
        } else {
            startForeground(NOTIFICATION_ID, notification);
        }
    }

    private void stopForegroundCompat() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            stopForeground(Service.STOP_FOREGROUND_REMOVE);
        } else {
            stopForeground(true);
        }
    }

    private void acquireWakeLock() {
        try {
            PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
            if (pm == null) return;
            mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "reBR:download");
            mWakeLock.acquire(60 * 60 * 1000L);
        } catch (Exception ignored) {
        }
    }

    private void releaseWakeLock() {
        try {
            if (mWakeLock != null && mWakeLock.isHeld()) mWakeLock.release();
        } catch (Exception ignored) {
        }
        mWakeLock = null;
    }
}
