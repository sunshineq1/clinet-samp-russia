package com.wn.blackrussia.game;

import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.view.KeyEvent;

import com.bytedance.shadowhook.ShadowHook;
import com.joom.paranoid.Obfuscate;
import com.wardrumstudios.utils.WarMedia;

public class Game extends WarMedia {
    static String vmVersion;
    private boolean once = false;

    static {
        ShadowHook.init(new ShadowHook.ConfigBuilder()
                .setMode(ShadowHook.Mode.UNIQUE)
                .build());

        vmVersion = null;
        System.out.println("**** Loading SO's");
        try {
            vmVersion = System.getProperty("java.vm.version");
            System.out.println("vmVersion " + vmVersion);
            System.loadLibrary("ImmEmulatorJ");
        }
        catch (ExceptionInInitializerError | UnsatisfiedLinkError ignored) {
        }
        System.loadLibrary("Game");
        System.loadLibrary("bass");
        System.loadLibrary("multiplayer");
    }

    public static void staticEnterSocialClub()
    {
        //  gtasaSelf.EnterSocialClub();
    }

    public static void staticExitSocialClub() {
        //gtasaSelf.ExitSocialClub();
    }

    public void AfterDownloadFunction() {

    }

    public void EnterSocialClub() {

    }

    public void ExitSocialClub() {

    }

    public boolean ServiceAppCommand(String str, String str2)
    {
        return false;
    }

    public int ServiceAppCommandValue(String str, String str2)
    {
        return 0;
    }

    public native void main();

    public void onActivityResult(int i, int i2, Intent intent)
    {
        super.onActivityResult(i, i2, intent);
    }

    public void onConfigurationChanged(Configuration configuration)
    {
        super.onConfigurationChanged(configuration);
    }

    public void onCreate(Bundle bundle) {
        if (!once) {
            once = true;
        }

        System.out.println("Game onCreate");
        super.onCreate(bundle);
    }

    public void onDestroy()
    {
        System.out.println("Game onDestroy");
        super.onDestroy();
    }

    public boolean onKeyDown(int i, KeyEvent keyEvent)
    {
        return super.onKeyDown(i, keyEvent);
    }

    public void onPause()
    {
        System.out.println("Game onPause");
        super.onPause();
    }

    public void onRestart()
    {
        System.out.println("Game onRestart");
        super.onRestart();
    }

    public void onResume()
    {
        System.out.println("Game onResume");
        super.onResume();
    }

    public void onStart()
    {
        System.out.println("Game onStart");
        super.onStart();
    }

    public void onStop()
    {
        System.out.println("Game onStop");
        super.onStop();
    }

    public native void setCurrentScreenSize(int i, int i2);
}