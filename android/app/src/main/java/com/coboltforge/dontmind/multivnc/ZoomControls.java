/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * Copyright (C) 2011 Michael A. MacDonald
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

package com.coboltforge.dontmind.multivnc;

import android.content.Context;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ZoomButton;

/**
 * The {@code ZoomControls} class displays a simple set of controls used for zooming and
 * provides callbacks to register for events. */
public class ZoomControls extends LinearLayout {

    private final ZoomButton mZoomIn;
    private final ZoomButton mZoomOut;
    private final ImageButton mZoomKeyboard;
        
    public ZoomControls(Context context) {
        this(context, null);
    }

    public ZoomControls(Context context, AttributeSet attrs) {
        super(context, attrs);
        setFocusable(false);
        
        LayoutInflater inflater = (LayoutInflater) context
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        inflater.inflate(R.layout.zoom_controls, this, // we are the parent
                true);
        
        mZoomIn = (ZoomButton) findViewById(R.id.zoomIn);
        mZoomOut = (ZoomButton) findViewById(R.id.zoomOut);
        mZoomKeyboard = (ImageButton) findViewById(R.id.zoomKeys);
    }

    public void setOnZoomInClickListener(OnClickListener listener) {
        mZoomIn.setOnClickListener(listener);
    }
    
    public void setOnZoomOutClickListener(OnClickListener listener) {
        mZoomOut.setOnClickListener(listener);
    }
    
    public void setOnZoomKeyboardClickListener(OnClickListener listener) {
    	mZoomKeyboard.setOnClickListener(listener);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        
        /* Consume all touch events so they don't get dispatched to the view
         * beneath this view.
         */
        return true;
    }
    
    public void show() {
        setVisibility(VISIBLE);

        //Workaround for buggy GLSurfaceView.
        //See https://stackoverflow.com/questions/11236336/setvisibilityview-visible-doesnt-always-work-ideas
        requestLayout();
    }
    
    public void hide() {
        setVisibility(INVISIBLE);
    }

    
    public void setIsZoomInEnabled(boolean isEnabled) {
        mZoomIn.setEnabled(isEnabled);
        if(!isEnabled)
            mZoomIn.getBackground().setColorFilter(Color.GRAY, PorterDuff.Mode.SRC_ATOP);
        else
            mZoomIn.getBackground().setColorFilter(null);
    }
    
    public void setIsZoomOutEnabled(boolean isEnabled) {
        mZoomOut.setEnabled(isEnabled);
        if(!isEnabled)
            mZoomOut.getBackground().setColorFilter(Color.GRAY, PorterDuff.Mode.SRC_ATOP);
        else
            mZoomOut.getBackground().setColorFilter(null);
    }
    
    @Override
    public boolean hasFocus() {
        return mZoomIn.hasFocus() || mZoomOut.hasFocus();
    }
}
