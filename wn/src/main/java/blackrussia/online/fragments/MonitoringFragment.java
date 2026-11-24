package blackrussia.online.fragments;

import android.os.Bundle;

import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.LinearLayoutManager;

import androidx.fragment.app.Fragment;

import android.view.View;
import android.view.LayoutInflater;
import android.view.ViewGroup;

import java.util.ArrayList;

import com.joom.paranoid.Obfuscate;
import com.wn.blackrussia.R;
import blackrussia.online.Settings;
import blackrussia.online.adapters.NewsAdapter;
import blackrussia.online.model.News;
import blackrussia.online.other.Lists;

import blackrussia.online.adapters.ServersAdapter;
import blackrussia.online.model.Servers;

@Obfuscate
public class MonitoringFragment extends Fragment {
	
	RecyclerView recyclerNews;
	NewsAdapter newsAdapter;
	ArrayList<News> nlist;
	RecyclerView recyclerServers;
	ServersAdapter serversAdapter;
	ArrayList<Servers> slist;
	
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		View inflate = inflater.inflate(R.layout.fragment_monitoring, container, false);

		// LAGS
		recyclerNews = inflate.findViewById(R.id.newsRV);
		recyclerNews.setHasFixedSize(true);

		recyclerServers = inflate.findViewById(R.id.ourServersRV);
		recyclerServers.setHasFixedSize(true);
		// -------------------------------------------------

		attachNews();
		attachServers();
		
        return inflate;
    }

	public void attachNews() {
		if(Settings.isCorrupted)
			return;

		recyclerNews.setLayoutManager(new LinearLayoutManager(getActivity(), LinearLayoutManager.HORIZONTAL, false));

		this.nlist = Lists.nlist;
		newsAdapter = new NewsAdapter(getContext(), this.nlist);
		recyclerNews.setAdapter(newsAdapter);
	}

	public void attachServers() {
		if(Settings.isCorrupted)
			return;

		recyclerServers.setLayoutManager(new LinearLayoutManager(getActivity()));

		this.slist = Lists.slist;
		serversAdapter = new ServersAdapter(getContext(), this.slist);
		recyclerServers.setAdapter(serversAdapter);
	}
}