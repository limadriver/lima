/*
 * Copyright (c) 2012 Arvin Schnell <arvin.schnell@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

package org.remali.premalidemo;

import java.lang.String;
import java.util.Vector;
import java.io.File;
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
import android.content.Intent;


public class PreMaliDemo extends ListActivity
{
    private Vector<String> programs = new Vector<String>();

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

	Log.i("PreMaliDemo", "create");

	getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
			     WindowManager.LayoutParams.FLAG_FULLSCREEN);

	File[] files = new File("/system/bin/premali/premali").listFiles();
	if (files != null)
	{
	    for (File file : files)
	    {
		if (file.canExecute())
		{
		    Log.i("PreMaliDemo", file.getAbsolutePath());
		    programs.add(file.getAbsolutePath());
		}
	    }
	}
	else
	{
	    Log.e("PreMaliDemo", "no programs found");
	}

	setListAdapter(new ArrayAdapter<String>(this, R.layout.listitem, programs));

	ListView listview = getListView();

	listview.setOnItemClickListener(new OnItemClickListener() {
	    public void onItemClick(AdapterView<?> parent, View view,
				    int position, long id)
	    {
		String program = programs.get((int) id);
		runProgram(program);
	    }
	});
    }

    public void runProgram(String program)
    {
	Intent intent = new Intent(this, RunDemo.class);
	intent.putExtra("org.remali.premalidemo.program", program);
	startActivity(intent);
    }
}
