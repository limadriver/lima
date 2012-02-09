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

package org.limadriver.limaredemo;

import java.lang.Runtime;
import java.io.IOException;
import android.app.Activity;
import android.util.Log;
import android.os.Bundle;
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

	Intent intent = getIntent();
	String program = intent.getStringExtra("org.limadriver.limaredemo.program");

	Log.i("LimareDemo", program);

	((TextView) findViewById(R.id.run_message)).setText(program);

	try
	{
	    process = Runtime.getRuntime().exec(program);
	}
	catch (IOException e)
	{
	    Log.e("LimareDemo", "exec failed");
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
