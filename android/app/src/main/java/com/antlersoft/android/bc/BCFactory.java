/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package com.antlersoft.android.bc;

import android.content.Context;

/**
 * Create interface implementations appropriate to the current version of the SDK;
 * implementations can allow use of higher-level SDK calls in .apk's that will still run
 * on lower-level SDK's
 *
 * @author Michael A. MacDonald
 */
public class BCFactory {

    private static BCFactory _theInstance = new BCFactory();

    private IBCStorageContext bcStorageContext;

    /**
     * This is here so checking the static doesn't get optimized away;
     * note we can't use SDK_INT because that is too new
     *
     * @return sdk version
     */
    int getSdkVersion() {
        try {
            return Integer.parseInt(android.os.Build.VERSION.SDK);
        } catch (NumberFormatException nfe) {
            return 1;
        }
    }

    @SuppressWarnings("unchecked")
    static private Class[] scaleDetectorConstructorArgs = new Class[]{Context.class, OnScaleGestureListener.class};

    /**
     * Return an instance of an implementation of {@link IBCScaleGestureDetector} appropriate to the SDK of this device.
     * This will work very much like android.view.ScaleGestureDetector on SDK >= 5.  For previous
     * SDK versions, it is a dummy implementation that does nothing and will never call the listener.
     * <p>
     * Note that unlike the other methods in this class, the returned interface instance is not
     * stateless.
     *
     * @param context  The context to which the detector is applied
     * @param listener The listener to which the implementation will send scale events
     * @return The gesture detector
     */
    public IBCScaleGestureDetector getScaleGestureDetector(Context context, OnScaleGestureListener listener) {
        IBCScaleGestureDetector result;

        if (getSdkVersion() >= 5) {
            try {
                result = (IBCScaleGestureDetector) getClass().getClassLoader().
                        loadClass("com.antlersoft.android.bc.ScaleGestureDetector").
                        getConstructor(scaleDetectorConstructorArgs).newInstance(new Object[]{context, listener});
            } catch (Exception e) {
                throw new RuntimeException("Error instantiating ScaleGestureDetector", e);
            }
        } else {
            result = new DummyScaleGestureDetector();
        }
        return result;
    }

    /**
     * @return An implementation of IBCStorageContext appropriate for the running Android release
     */
    public IBCStorageContext getStorageContext() {
        if (bcStorageContext == null) {
            synchronized (this) {
                if (bcStorageContext == null) {
                    if (getSdkVersion() >= 8) {
                        try {
                            bcStorageContext = (IBCStorageContext) getClass().getClassLoader().loadClass("com.antlersoft.android.bc.BCStorageContext8").newInstance();
                        } catch (Exception ie) {
                            throw new RuntimeException("Error instantiating", ie);
                        }
                    } else {
                        try {
                            bcStorageContext = (IBCStorageContext) getClass().getClassLoader().loadClass("com.antlersoft.android.bc.BCStorageContext7").newInstance();
                        } catch (Exception ie) {
                            throw new RuntimeException("Error instantiating", ie);
                        }
                    }
                }
            }
        }
        return bcStorageContext;
    }

    /**
     * Returns the only instance of this class, which manages the SDK specific interface
     * implementations
     *
     * @return Factory instance
     */
    public static BCFactory getInstance() {
        return _theInstance;
    }
}
