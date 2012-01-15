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

import java.lang.Runtime;
import java.io.IOException;
import android.app.Activity;
import android.util.Log;
import android.os.Bundle;
import android.view.WindowManager;
import android.widget.TextView;
import android.content.Intent;


public class RunDemo extends Activity
{
    private Process process = null;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
	super.onCreate(savedInstanceState);

	setContentView(R.layout.run);

	getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
			     WindowManager.LayoutParams.FLAG_FULLSCREEN);

	Intent intent = getIntent();
	String program = intent.getStringExtra("org.remali.premalidemo.program");

	Log.i("PreMaliDemo", program);

	((TextView) findViewById(R.id.run_message)).setText(program);

	try
	{
	    process = Runtime.getRuntime().exec(program);
	}
	catch (IOException e)
	{
	    Log.e("PreMaliDemo", "exec failed");
	    process = null;
	}
    }

    @Override
    public void onStop()
    {
	super.onStop();

	if (process != null)
	{
	    process.destroy();
	    process = null;
	}
    }
}
