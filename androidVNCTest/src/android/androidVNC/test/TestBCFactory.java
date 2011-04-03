/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package android.androidVNC.test;

import junit.framework.TestCase;

import com.antlersoft.android.bc.*;

/**
 * @author Michael A. MacDonald
 */
public class TestBCFactory extends TestCase {

	/**
	 * Test method for {@link com.antlersoft.android.bc.BCFactory#getSdkVersion()}.
	 */
	public void testGetSdkVersion() {
		getSdkVersion();
	}
	
	int getSdkVersion()
	{
				try
				{
					return Integer.parseInt(android.os.Build.VERSION.SDK);
				}
				catch (NumberFormatException nfe)
				{
					return 1;
				}
	}

	/**
	 * Test method for {@link com.antlersoft.android.bc.BCFactory#getBCActivityManager()}.
	 */
	public void testGetBCActivityManager() {
		IBCActivityManager ibc=BCFactory.getInstance().getBCActivityManager();
		if ((getSdkVersion()<5 && ! ibc.getClass().getName().equals("com.antlersoft.android.bc.BCActivityManagerDefault") )||
				(getSdkVersion()>=5 && ! ibc.getClass().getName().equals("com.antlersoft.android.bc.BCActivityManagerV5")))
			fail("Unexpected class for "+getSdkVersion()+" "+ibc.getClass().getName());
	}

	/**
	 * Test method for {@link com.antlersoft.android.bc.BCFactory#getBCGestureDetector()}.
	 */
	public void testGetBCGestureDetector() {
		IBCGestureDetector ibc=BCFactory.getInstance().getBCGestureDetector();
		if ((getSdkVersion()<3 && ! ibc.getClass().getName().equals("com.antlersoft.android.bc.BCGestureDetectorOld") )||
				(getSdkVersion()>=3 && ! ibc.getClass().getName().equals("com.antlersoft.android.bc.BCGestureDetectorDefault")))
			fail("Unexpected class for "+getSdkVersion()+" "+ibc.getClass().getName());
	}

}
