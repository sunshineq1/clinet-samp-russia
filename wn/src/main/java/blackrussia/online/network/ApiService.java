package blackrussia.online.network;

import com.joom.paranoid.Obfuscate;

import blackrussia.online.Settings;
import okhttp3.OkHttpClient;
import retrofit2.Retrofit;
import retrofit2.converter.gson.GsonConverterFactory;

@Obfuscate
public class ApiService {
    private static ApiService instance;
    private Retrofit retrofit = new Retrofit.Builder().client(new OkHttpClient.Builder().addInterceptor(new UserAgentInterceptor("")).build()).baseUrl(Settings.API).addConverterFactory(GsonConverterFactory.create()).build();

    private ApiService() {
    }

    public static ApiService getInstance() {
        if (instance == null) {
            instance = new ApiService();
        }
        return instance;
    }
}
