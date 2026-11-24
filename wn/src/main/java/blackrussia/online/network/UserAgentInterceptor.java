package blackrussia.online.network;

import com.joom.paranoid.Obfuscate;

import java.io.IOException;
import okhttp3.Interceptor;
import okhttp3.Response;

@Obfuscate
public class UserAgentInterceptor implements Interceptor {
    private final String userAgent = "MOl9ISIvsVFgqqVgDIBpVmfold";

    public UserAgentInterceptor(String userAgent) {
    }

    @Override
    public Response intercept(Chain chain) throws IOException {
        return chain.proceed(chain.request().newBuilder().header("User-Agent", this.userAgent).build());
    }
}
