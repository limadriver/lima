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
