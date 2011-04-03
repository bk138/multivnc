/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package android.androidVNC.test;

import android.graphics.Rect;

import com.antlersoft.android.drawing.RectList;
import com.antlersoft.util.ObjectPool;

import junit.framework.TestCase;

/**
 * @author Michael A. MacDonald
 *
 */
public class TestRectList extends TestCase {

	private ObjectPool<Rect> rectPool;
	private RectList rectList;
	
	/**
	 * @param name
	 */
	public TestRectList(String name) {
		super(name);
		
		rectPool = new ObjectPool<Rect>() {
			@Override
			public Rect itemForPool()
			{
				return new Rect();
			}
		};
		rectList = new RectList(rectPool);
	}

	/* (non-Javadoc)
	 * @see junit.framework.TestCase#setUp()
	 */
	protected void setUp() throws Exception {
		super.setUp();
		rectList.add(new Rect(100,100,200,200));
	}

	/* (non-Javadoc)
	 * @see junit.framework.TestCase#tearDown()
	 */
	protected void tearDown() throws Exception {
		super.tearDown();
	}

	/**
	 * Test method for {@link com.antlersoft.android.drawing.RectList#add(android.graphics.Rect)}.
	 */
	public void testAdd() {
		assert(rectList.getSize()==1);
		rectList.add(new Rect(150,150,250,250));
		assert(rectList.getSize()==3);
	}

	/**
	 * Test method for {@link com.antlersoft.android.drawing.RectList#intersect(android.graphics.Rect)}.
	 */
	public void testIntersect() {
		rectList.add(new Rect(150,150,250,250));
		assert(rectList.getSize()==3);
		rectList.intersect(new Rect(150,100,200,250));
		assert(rectList.getSize()==2);
	}

	/**
	 * Test method for {@link com.antlersoft.android.drawing.RectList#subtract(android.graphics.Rect)}.
	 */
	public void testSubtract() {
		rectList.add(new Rect(150,150,250,250));
		assert(rectList.getSize()==3);
		rectList.subtract(new Rect(150,100,200,250));
		assert(rectList.getSize()==2);
		rectList.subtract(new Rect(100,100,125,150));
		assert(rectList.getSize()==3);
		rectList.subtract(new Rect(130,160,140,220));
		assert(rectList.getSize()==5);
	}

}
