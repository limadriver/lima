
package org.remali.premalidemo;

import java.lang.Runtime;
import java.lang.String;
import java.util.Vector;
import java.io.File;
import java.io.IOException;
import android.app.ListActivity;
import android.util.Log;
import android.os.Bundle;
import android.view.View;
import android.view.WindowManager;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;


public class PreMaliDemo extends ListActivity
{
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
	Log.i("PreMaliDemo", "create");

        super.onCreate(savedInstanceState);

	Vector<String> programs = new Vector<String>();

	File[] files = new File("/system/bin/premali/premali").listFiles();
	for (File file : files)
	{
	    if (file.canExecute())
	    {
		Log.i("PreMaliDemo", file.getAbsolutePath());
		programs.add(file.getAbsolutePath());
	    }
	}

	setListAdapter(new ArrayAdapter<String>(this, R.layout.listitem, programs));

	ListView listview = getListView();

	listview.setOnItemClickListener(new OnItemClickListener() {
	    public void onItemClick(AdapterView<?> parent, View view,
				    int position, long id)
	    {
		String program = ((TextView) view).getText().toString();
		runProgram(program);
	    }
	});

	getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
			     WindowManager.LayoutParams.FLAG_FULLSCREEN);
    }


    public void runProgram(String program)
    {
	Log.i("PreMaliDemo", program);

	try
	{
	    Runtime.getRuntime().exec(program);
	}
	catch (IOException e)
	{
	}
    }
}
