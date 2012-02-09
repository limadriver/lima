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
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;


public class Adapter extends ArrayAdapter<String>
{
    private Context context;
    private Vector<String> programs;

    public Adapter(Context context, Vector<String> programs)
    {
	super(context, R.layout.listitem, programs);
	this.context = context;
	this.programs = programs;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent)
    {
	LayoutInflater inflater = (LayoutInflater)
	    context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

	File file = new File(programs.get(position));

	View view = inflater.inflate(R.layout.listitem, parent, false);

	TextView textView = (TextView) view.findViewById(R.id.label);
	textView.setText(file.getName());

	return view;
    }
}
