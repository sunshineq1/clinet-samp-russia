package blackrussia.online.adapters;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.recyclerview.widget.RecyclerView;

import com.joom.paranoid.Obfuscate;
import com.wn.blackrussia.R;
import blackrussia.online.network.TextLoader;
import java.util.ArrayList;
import java.util.List;

@Obfuscate
public class SliderTextAdapter extends RecyclerView.Adapter<SliderTextAdapter.Holder> {
    private Context context;
    private List<TextLoader> sliderItems = new ArrayList();

    public SliderTextAdapter(Context context) {
        this.context = context;
    }

    public void addItems(List<TextLoader> sliderItems) {
        this.sliderItems = sliderItems;
        notifyDataSetChanged();
    }

    public void deleteItem(int position) {
        this.sliderItems.remove(position);
        notifyDataSetChanged();
    }

    public void addItem(TextLoader sliderItem) {
        this.sliderItems.add(sliderItem);
        notifyDataSetChanged();
    }

    @Override
    public Holder onCreateViewHolder(ViewGroup parent, int viewType) {
        View inflate = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_slider_loader, (ViewGroup) null);
        inflate.setLayoutParams(new LinearLayout.LayoutParams(-1, -1));
        return new Holder(inflate);
    }

    @Override
    public void onBindViewHolder(Holder holder, int position) {
        if (this.sliderItems.size() > 0) {
            List<TextLoader> list = this.sliderItems;
            holder.text.setText(list.get(position % list.size()).getText());
        }
    }

    @Override
    public int getItemCount() {
        return this.sliderItems.size() == 0 ? 0 : 1000;
    }

    public class Holder extends RecyclerView.ViewHolder {
        View itemView;
        TextView text;

        public Holder(View itemView) {
            super(itemView);
            this.text = (TextView) itemView.findViewById(R.id.text);
            this.itemView = itemView;
        }
    }
}
