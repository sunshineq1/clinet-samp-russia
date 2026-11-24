package blackrussia.online.fragments;

import static blackrussia.online.Settings.GAME_PATH;
import static blackrussia.online.Settings.POLICY_URL;
import static blackrussia.online.Settings.URL_DISCORD;
import static blackrussia.online.Settings.URL_TELEGRAM;
import static blackrussia.online.Settings.URL_VK;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.view.inputmethod.InputMethodManager;
import android.widget.*;
import android.widget.Toast;

import androidx.fragment.app.Fragment;

import com.joom.paranoid.Obfuscate;
import com.wn.blackrussia.R;
import blackrussia.online.Preferences;
import blackrussia.online.Downloader;

import org.ini4j.Wini;

import java.io.File;
import java.io.IOException;

@Obfuscate
public class SettingsFragment extends Fragment {

    Animation animation;
    public TextView nickname;
    String nickName;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View inflate = inflater.inflate(R.layout.fragment_settings, container, false);
        Animation animation = AnimationUtils.loadAnimation(getContext(), R.anim.button_click);

        nickname = (TextView) inflate.findViewById(R.id.editText);
        InitLogic();

        ToggleButton toggleButton = (ToggleButton) inflate.findViewById(R.id.notification);
        toggleButton.setChecked(Preferences.getBoolean(getContext(), Preferences.NOTIFICATION, true));
        toggleButton.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean z) {
                Preferences.putBoolean(getContext(), Preferences.NOTIFICATION, z);
            }
        });

        ((TextView) inflate.findViewById(R.id.privacyPolicy)).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                startActivity(new Intent("android.intent.action.VIEW", Uri.parse(POLICY_URL)));
            }
        });

        ((TextView) inflate.findViewById(R.id.reinstallGame))
                .setOnClickListener(
                        new View.OnClickListener() {
                            public void onClick(View v) {
                                v.startAnimation(animation);
								onClickReinstallGame();
                            }
        });
        ((TextView) inflate.findViewById(R.id.resetSettings))
                .setOnClickListener(
                        new View.OnClickListener() {
                            public void onClick(View v) {
                                v.startAnimation(animation);
                                onClickResetSettings();
                            }
        });
        ((ImageView) inflate.findViewById(R.id.telegramButton))
                .setOnClickListener(
                        new View.OnClickListener() {
                            public void onClick(View v) {
                                v.startAnimation(animation);
                                startActivity(new Intent("android.intent.action.VIEW", Uri.parse(URL_TELEGRAM)));
                            }
        });

        ((ImageView) inflate.findViewById(R.id.vkButton))
                .setOnClickListener(
                        new View.OnClickListener() {
                            public void onClick(View v) {
                                v.startAnimation(animation);
                                startActivity(
                                        new Intent("android.intent.action.VIEW", Uri.parse(URL_VK)));
                            }
                        });
        ((ImageView) inflate.findViewById(R.id.discordButton))
                .setOnClickListener(
                        new View.OnClickListener() {
                            public void onClick(View v) {
                                v.startAnimation(animation);
                                startActivity(new Intent("android.intent.action.VIEW", Uri.parse(URL_DISCORD)));
                            }
        });

        ((TextView) nickname)
                .setOnClickListener(
                        new View.OnClickListener() {
                            public void onClick(View v) {
                                File f = new File(GAME_PATH + "/SAMP/settings.ini");

                                if(f.exists())
                                    onClickChangeNick();
                            }
        });
        return inflate;
    }

    private void InitLogic() {
        try {
            File f = new File(GAME_PATH + "/SAMP/settings.ini");
            if(f.exists()) {
                Wini w = new Wini(new File(GAME_PATH + "/SAMP/settings.ini"));
                nickname.setText(w.get("client", "name"));
                com.wn.storage.Preferences.setNick(w.get("client", "name"));
                Preferences.putString(getContext(), Preferences.NIK, w.get("client", "name"));
                w.store();
            } else {
                nickname.setText("Введите свой никнейм");
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @SuppressLint("ResourceType")
    public void onClickChangeNick() {
        String objNick = "";
        try {
            File f = new File(GAME_PATH + "/SAMP/settings.ini");
            if(f.exists()) {
                Wini w = new Wini(new File(GAME_PATH + "/SAMP/settings.ini"));
                objNick = w.get("client", "name");
                w.store();
            } else {
                objNick = "Введите свой никнейм";
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        Dialog dialog = new Dialog(getActivity());
        dialog.setContentView(R.layout.item_change_nik);
        dialog.setCancelable(true);
        dialog.getWindow().setBackgroundDrawableResource(17170445);
        dialog.getWindow().setLayout(-1, -2);
        EditText editText = (EditText) dialog.findViewById(R.id.editText);
        editText.setText(objNick);

        ((TextView) dialog.findViewById(R.id.cancel)).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                view.startAnimation(AnimationUtils.loadAnimation(getContext(), R.anim.button_click));
                dialog.getClass();
                dialog.dismiss();
            }
        });
        ((TextView) dialog.findViewById(R.id.save)).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                uploadNick(editText, dialog, view);
            }
        });
        dialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialogInterface) {
                ((InputMethodManager) getContext().getSystemService("input_method")).toggleSoftInput(1, 0);
            }
        });
        dialog.show();
        editText.requestFocus();
        ((InputMethodManager) getContext().getSystemService("input_method")).toggleSoftInput(2, 0);
    }

    public void uploadNick(final EditText editText, final Dialog dialog, View view) {
        view.startAnimation(AnimationUtils.loadAnimation(getContext(), R.anim.button_click));
        final String obj = editText.getText().toString();
        if (obj.isEmpty()) {
            Toast.makeText(getActivity(), "Введите ник", Toast.LENGTH_SHORT).show();
        } else if (!obj.contains("_")) {
            Toast.makeText(getActivity(), "Ник должен содержать символ \"_\"", Toast.LENGTH_SHORT).show();
        } else if (obj.length() < 4) {
            Toast.makeText(getActivity(), "Длина ника должна быть не менее 4 символов", Toast.LENGTH_SHORT).show();
        } else {
            try {
                File f = new File(GAME_PATH + "/SAMP/settings.ini");
                if(f.exists()) {
                    Wini w = new Wini(new File(GAME_PATH + "/SAMP/settings.ini"));
                    w.put("client", "name", obj);
                    Toast.makeText(getActivity(), "Ваш новый никнейм успешно сохранен!", Toast.LENGTH_SHORT).show();
                    w.store();
                } else {
                    Toast.makeText(getActivity(), "Произошла ошибка", Toast.LENGTH_SHORT).show();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }

            InitLogic();
            dialog.getClass();
            dialog.dismiss();
        }
    }

    @SuppressLint("ResourceType")
    public void onClickReinstallGame() {
        Dialog dialog = new Dialog(getActivity());
        dialog.setContentView(R.layout.item_dialog_settings);
        dialog.setCancelable(true);
        dialog.getWindow().setBackgroundDrawableResource(17170445);
        dialog.getWindow().setLayout(-1, -2);
        ((TextView) dialog.findViewById(R.id.message)).setText(getContext().getResources().getString(R.string.reinstallGameQ));
        ((TextView) dialog.findViewById(R.id.cancel)).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                view.startAnimation(AnimationUtils.loadAnimation(getContext(), R.anim.button_click));
                dialog.getClass();
                dialog.dismiss();
            }
        });
        ((TextView) dialog.findViewById(R.id.ok)).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                dialog.dismiss();
                startActivity(new Intent(getActivity(), Downloader.class));
                Preferences.clear(getContext(), Preferences.NIK);
            }
        });
        dialog.show();
    }

    @SuppressLint("ResourceType")
    public void onClickResetSettings() {
        Dialog dialog = new Dialog(getActivity());
        dialog.setContentView(R.layout.item_dialog_settings);
        dialog.setCancelable(true);
        dialog.getWindow().setBackgroundDrawableResource(17170445);
        dialog.getWindow().setLayout(-1, -2);
        ((TextView) dialog.findViewById(R.id.message)).setText(getContext().getResources().getString(R.string.resetSettingsQ));
        ((TextView) dialog.findViewById(R.id.cancel)).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                view.startAnimation(AnimationUtils.loadAnimation(getContext(), R.anim.button_click));
                dialog.getClass();
                dialog.dismiss();
            }
        });
        ((TextView) dialog.findViewById(R.id.ok)).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                dialog.dismiss();
                startActivity(new Intent(getActivity(), Downloader.class));
            }
        });
        dialog.show();
    }
}