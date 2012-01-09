
package org.remali.premalidemo;

import java.lang.Runtime;
import java.lang.String;
import java.util.Vector;
import java.io.File;
import java.io.IOException;
import android.app.Activity;
import android.util.Log;
import android.os.Bundle;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;

public class PreMaliDemo extends Activity
{
    private Vector<String> programs = new Vector<String>();


    @Override
    public void onCreate(Bundle savedInstanceState)
    {
	Log.i("PreMaliDemo", "create");

        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

	getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
			     WindowManager.LayoutParams.FLAG_FULLSCREEN);

	File[] files = new File("/system/bin/premali/premali").listFiles();
	for (File file : files)
	{
	    if (file.canExecute())
	    {
		Log.i("PreMaliDemo", file.getAbsolutePath());
		programs.add(file.getAbsolutePath());
	    }
	}
    }


    public void triangle1(View view) throws IOException
    {
	Log.i("PreMaliDemo", "triangle1");

	Runtime.getRuntime().exec("/system/bin/premali/premali/triangle_smoothed");
    }


    public void triangle2(View view) throws IOException
    {
	Log.i("PreMaliDemo", "triangle2");

	Runtime.getRuntime().exec("/system/bin/premali/premali/triangle_smoothed_inverted");
    }
}
