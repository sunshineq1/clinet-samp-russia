package blackrussia.online.model;

import com.google.gson.annotations.SerializedName;
import com.joom.paranoid.Obfuscate;

@Obfuscate
public class Links {
    @SerializedName("URL_GAME_FILES")
    private String URL_GAME_FILES;
    @SerializedName("URL_PRIVACY")
    private String URL_PRIVACY;

    public Links() { }
    public final String getUrlFiles() {
        return URL_GAME_FILES;
    }
    public final String getPrivacy() {
        return URL_PRIVACY;
    }
}
