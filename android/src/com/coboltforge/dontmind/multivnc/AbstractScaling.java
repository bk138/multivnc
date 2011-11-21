/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package com.coboltforge.dontmind.multivnc;

import android.widget.ImageView;

/**
 * @author Michael A. MacDonald
 * 
 * A scaling mode for the VncCanvas; based on ImageView.ScaleType
 */
abstract class AbstractScaling {
	public static final int zoomableId = -1;
	private static final int scaleModeIds[] = { zoomableId };
	
	private static AbstractScaling[] scalings;

	static AbstractScaling getById(int id)
	{
		if ( scalings==null)
		{
			scalings=new AbstractScaling[scaleModeIds.length];
		}
		for ( int i=0; i<scaleModeIds.length; ++i)
		{
			if ( scaleModeIds[i]==id)
			{
				if ( scalings[i]==null)
				{
					switch ( id )
					{
				
					case zoomableId:
						scalings[i]=new ZoomScaling();
						break;
					}
				}
				return scalings[i];
			}
		}
		throw new IllegalArgumentException("Unknown scaling id " + id);
	}
	
	float getScale() { return 1; }
	
	void zoomIn(VncCanvasActivity activity) {}
	void zoomOut(VncCanvasActivity activity) {}
	
	static AbstractScaling getByScaleType(ImageView.ScaleType scaleType)
	{
		for (int i : scaleModeIds)
		{
			AbstractScaling s = getById(i);
			if (s.scaleType==scaleType)
				return s;
		}
		throw new IllegalArgumentException("Unsupported scale type: "+ scaleType.toString());
	}
	
	private int id;
	protected ImageView.ScaleType scaleType;
	
	protected AbstractScaling(int id, ImageView.ScaleType scaleType)
	{
		this.id = id;
		this.scaleType = scaleType;
	}
	
	/**
	 * 
	 * @return Id corresponding to menu item that sets this scale type
	 */
	int getId()
	{
		return id;
	}

	/**
	 * Sets the activity's scale type to the scaling
	 * @param activity
	 */
	void setScaleTypeForActivity(VncCanvasActivity activity)
	{
		activity.zoomer.hide();
		activity.vncCanvas.scaling = this;
		activity.vncCanvas.setScaleType(scaleType);
		activity.getConnection().setScaleMode(scaleType);
		
		activity.getConnection().Gen_update(activity.database.getWritableDatabase());
	}
	

	/**
	 * True if this scale type allows panning of the image
	 * @return
	 */
	abstract boolean isAbleToPan();
	
	/**
	 * True if the listed input mode is valid for this scaling mode
	 * @param mode Id of the input mode
	 * @return True if the input mode is compatible with the scaling mode
	 */
	abstract boolean isValidInputMode(int mode);
	
	/**
	 * Change the scaling and focus dynamically, as from a detected scale gesture
	 * @param activity Activity containing to canvas to scale
	 * @param scaleFactor Factor by which to adjust scaling
	 * @param fx Focus X of center of scale change
	 * @param fy Focus Y of center of scale change
	 */
	void adjust(VncCanvasActivity activity, float scaleFactor, float fx, float fy)
	{
	
	}
}
