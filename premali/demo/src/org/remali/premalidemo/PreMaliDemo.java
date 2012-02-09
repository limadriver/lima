/*
 * Copyright (c) 2012 Arvin Schnell <arvin.schnell@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

package org.remali.premalidemo;

import java.util.Vector;
import java.io.File;
import android.app.Activity;
import android.util.Log;
import android.os.Bundle;
import android.view.View;
import android.widget.ListView;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.content.Intent;


public class PreMaliDemo extends Activity
{
    private Vector<String> programs = new Vector<String>();

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

	Log.i("PreMaliDemo", "create");

	setContentView(R.layout.main);

	loadPrograms();

	ListView listview = (ListView) findViewById(R.id.list);

	Adapter adapter = new Adapter(this, programs);
	listview.setAdapter(adapter);

	listview.setOnItemClickListener(new OnItemClickListener() {
	    public void onItemClick(AdapterView<?> parent, View view,
				    int position, long id)
	    {
		runProgram(position);
	    }
	});
    }

    private void loadPrograms()
    {
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
    }

    private void runProgram(int position)
    {
	String program = programs.get(position);

	Intent intent = new Intent(this, RunDemo.class);
	intent.putExtra("org.remali.premalidemo.program", program);
	startActivity(intent);
    }
}
