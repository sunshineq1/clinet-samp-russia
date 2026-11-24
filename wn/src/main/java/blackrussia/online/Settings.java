package blackrussia.online;

import android.os.Environment;

import com.joom.paranoid.Obfuscate;

@Obfuscate
public class Settings {
    public static Boolean isCorrupted = false;

    public static final long LAUNCHER_DELAY = 2500;
    public static final long ANIMATION_DELAY = 200; // non
    public static String API = "https://wh3606.web3.maze-host.ru/br-fuss/";

    public static final String POLICY_URL = "http://api.blackrussia.online/launchpolitic.pdf";
    public static String CLIENT_PACKAGE = "com.rockstargames.gtacr";
    public static String LAUNCHER_PACKAGE = "blackrussia.online";
    public static String URL_DISCORD = "https://discord.gg/blackrussia";
    public static String URL_DONATE = "https://blackrussia.online/donate_v2/pay_choice.php?is_mobile=true";
    public static String URL_TELEGRAM = "https://t.me/br_dev";
    public static String URL_VK = "https://vk.com/blackrussia.online";

    public static final String GAME_PATH = Environment.getExternalStorageDirectory() + "/Weik";
    public static final String DOWNLOAD_APK_NAME = "launcher.apk";
    public static final String GAME_URL = "http://wh3606.web3.maze-host.ru/br-fuss/gamefiles";
    public static final String GAME_URL_2 = "http://wh3606.web3.maze-host.ru/br-fuss/api.json";

    public static final String LAUNCHER_APK_URL = "http://weikton.ru/fuss/launcher.apk";
    public static final String LAUNCHER_JSON_URL = "http://weikton.ru/fuss/output-metadata.json";

    public static class UserInfo {
        public String hash;
        public String name;
    }
}