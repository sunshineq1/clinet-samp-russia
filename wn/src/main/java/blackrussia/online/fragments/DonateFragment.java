package blackrussia.online.fragments;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;

import android.app.Dialog;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.fragment.app.Fragment;

import android.view.View.OnClickListener;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AnimationUtils;
import android.view.animation.Animation;
import android.view.LayoutInflater;

import com.joom.paranoid.Obfuscate;
import com.wn.blackrussia.R;

@Obfuscate
public class DonateFragment extends Fragment {
	
	Animation animation;
	public Dialog dialog;
	public EditText nik;
    public EditText sum;
	
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		View inflate = inflater.inflate(R.layout.fragment_donate, container, false);
		
		TextView textView = (TextView) inflate.findViewById(R.id.deposit);
		
		Animation animation = AnimationUtils.loadAnimation(getContext(), R.anim.button_click);
		
		nik = inflate.findViewById(R.id.nik);
        sum = inflate.findViewById(R.id.sum);
		
		textView.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                v.startAnimation(animation);
				onClickDeposit();
            }
        });
		
        return inflate;
    }
	
	public void onClickDeposit() {
        if (nik.getText().toString().isEmpty()) {
            Toast.makeText(getActivity(), "Введите ник!", Toast.LENGTH_SHORT).show();
        } else if (!nik.getText().toString().contains("_")) {
            Toast.makeText(getActivity(),"Ник должен иметь \"_\"!", Toast.LENGTH_SHORT).show();
        } else if (nik.getText().toString().length() < 4) {
            Toast.makeText(getActivity(), "Минимальная длина ника 4 символа!", Toast.LENGTH_SHORT).show();
        } else if (this.sum.getText().toString().isEmpty()) {
            Toast.makeText(getActivity(), "Введите сумму!", Toast.LENGTH_SHORT).show();
        } else {
            startActivity(new Intent("android.intent.action.VIEW", Uri.parse("https://t.me/weikton_official")));
        }
    }
}
