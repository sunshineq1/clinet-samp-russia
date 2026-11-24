package blackrussia.online.activities;

import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.view.View;
import android.view.animation.AnimationUtils;
import android.widget.ImageView;

import com.akexorcist.roundcornerprogressbar.RoundCornerProgressBar;
import com.joom.paranoid.Obfuscate;
import com.wn.blackrussia.R;
import blackrussia.online.adapters.SliderStoriesAdapter;
import blackrussia.online.other.Lists;
import com.smarteist.autoimageslider.SliderAnimations;
import com.smarteist.autoimageslider.SliderView;

@Obfuscate
public class StoryActivity extends AppCompatActivity {
    private RoundCornerProgressBar progressStory;
    private SliderStoriesAdapter sliderStoriesAdapter;
    private SliderView sliderView;
    private CountDownTimer countDownTimer;
    private long progress;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_story);
        this.sliderView = (SliderView) findViewById(R.id.sliderView);
        this.progressStory = (RoundCornerProgressBar) findViewById(R.id.progressStory);
        ((ImageView) findViewById(R.id.closeStory)).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                view.startAnimation(AnimationUtils.loadAnimation(getApplicationContext(), R.anim.button_click));
                closeStory();
            }
        });
        SliderStoriesAdapter sliderStoriesAdapter = new SliderStoriesAdapter(this);
        this.sliderStoriesAdapter = sliderStoriesAdapter;
        this.sliderView.setSliderAdapter(sliderStoriesAdapter);
        this.sliderView.setSliderTransformAnimation(SliderAnimations.SIMPLETRANSFORMATION);
        this.sliderStoriesAdapter.addItems(Lists.nlist);
        this.sliderView.setCurrentPageListener(i -> startTimer());
        int intExtra = getIntent().getIntExtra("position", 0);
        this.sliderView.getSliderPager().setCurrentItem(intExtra, false);
        this.sliderView.getPagerIndicator().setSelection(intExtra);
        this.progressStory.setProgress(0.0f);
        startTimer();
    }


    public void closeStory() {
        countDownTimer.cancel();
        countDownTimer = null;
        progress = 0;
        startActivity(new Intent(getApplicationContext(), MainActivity.class));
    }

    private void startTimer() {
        if (countDownTimer != null) {
            countDownTimer.cancel();
            progress = 0;
            countDownTimer = null;
        }
        countDownTimer = new CountDownTimer(5000, 1) {
            @Override
            public void onTick(long j) {
                progress = 5000-j;
                progressStory.setProgress(progress);
            }
            @SuppressLint("SuspiciousIndentation")
            @Override
            public void onFinish() {
                if(sliderView.getCurrentPagePosition() + 1 == sliderStoriesAdapter.getCount())
                    closeStory();
                else
                sliderView.setCurrentPagePosition(sliderView.getCurrentPagePosition() + 1);
            }
        }.start();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        countDownTimer.cancel();
        progress = 0;
        countDownTimer = null;
    }
}