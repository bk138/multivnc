/**
 * Copyright (C) 2009 Michael A. MacDonald
 */
package com.coboltforge.dontmind.multivnc;

import com.antlersoft.android.db.FieldAccessor;
import com.antlersoft.android.db.TableInterface;

/**
 * @author Michael A. MacDonald
 */
@TableInterface(TableName = "MOST_RECENT", ImplementingIsAbstract = false, ImplementingClassName = "MostRecentBean")
public interface IMostRecentBean {

    @FieldAccessor
    long get_Id();

    @FieldAccessor(Name = "CONNECTION_ID")
    long getConnectionId();

    @FieldAccessor(Name = "SHOW_SPLASH_VERSION")
    long getShowSplashVersion();

    @FieldAccessor(Name = "TEXT_INDEX")
    long getTextIndex();
}
