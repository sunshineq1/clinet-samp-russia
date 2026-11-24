package blackrussia.online.network;

import com.joom.paranoid.Obfuscate;

import blackrussia.online.model.News;
import blackrussia.online.model.Servers;

import java.util.ArrayList;
import java.util.List;

import retrofit2.Call;
import retrofit2.http.GET;

@Obfuscate
public interface Interface {
    @GET("launcher/servers.json")
    Call<List<Servers>> getServers();

    @GET("launcher/stories.json")
    Call<List<News>> getNews();

    @GET("launcher/text1.json")
    Call<ArrayList<TextLoader>> getTextLoader();
}
