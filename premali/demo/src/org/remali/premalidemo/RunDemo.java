
package org.remali.premalidemo;

import java.lang.Runtime;
import java.lang.String;
import java.io.IOException;
import android.app.Activity;
import android.util.Log;
import android.os.Bundle;
import android.view.WindowManager;
import android.widget.TextView;
import android.content.Intent;


public class RunDemo extends Activity
{
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
	    Runtime.getRuntime().exec(program);
	}
	catch (IOException e)
	{
	}
    }
}
