package blackrussia.online.other;

import com.joom.paranoid.Obfuscate;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.BufferedInputStream;
import java.security.MessageDigest;
import java.io.File;

@Obfuscate
public class FileCheckSumSHA {

    public static String getCheckSum(String filepath, MessageDigest md) throws IOException {

        if(!new File(filepath).exists())
            return null;

        // хеширование файла с помощью BufferedInputStream
        try (BufferedInputStream bis = new BufferedInputStream(new FileInputStream(filepath))) {
            byte[] buffer = new byte[8192]; // размер буфера 8 кб
            int count;
            while ((count = bis.read(buffer)) > 0) {
                md.update(buffer, 0, count);
            }
        }

        // bytes to hex
        StringBuilder result = new StringBuilder();
        for (byte b : md.digest()) {
            result.append(String.format("%02x", b));
        }
        return result.toString();
    }
}