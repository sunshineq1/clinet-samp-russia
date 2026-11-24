package blackrussia.online.other;

import com.joom.paranoid.Obfuscate;

import java.net.HttpURLConnection;
import java.net.URL;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.io.File;

@Obfuscate
public class HTMLFileDownloader {
    public static int startDownload(String urlFrom, String pathTo) {
        try {
            URL url = new URL(urlFrom);
            HttpURLConnection httpConn = (HttpURLConnection) url.openConnection();
            httpConn.connect();
            int responseCode = httpConn.getResponseCode();

            // проверяем, что сервер вернул успешный код
            if (responseCode == HttpURLConnection.HTTP_OK) {
                File file = new File(pathTo);
                if (file.exists())
                    file.delete();
                file.getParentFile().mkdirs();
                file.createNewFile();

                InputStream inputStream = httpConn.getInputStream();
                FileOutputStream outputStream = new FileOutputStream(pathTo);

                byte[] buffer = new byte[4096];
                int bytesRead = -1;

                while ((bytesRead = inputStream.read(buffer)) != -1) {
                    outputStream.write(buffer, 0, bytesRead);
                }
                outputStream.close();
                inputStream.close();
            } else {
                System.out.println("HTMLFileDownloader Ошибка при загрузке файла. Код ответа: " + responseCode);
            }
            httpConn.disconnect();
            return responseCode;
        } catch (IOException ex) {
            System.out.println("/ HTMLFileDownloader Исключение при загрузке файла: " + ex.getMessage());
            ex.printStackTrace();
        }
        return -1;
    }
}
