package com.example.femforandroid;

import android.view.Surface;
import android.content.res.AssetManager;

final class JNIHandler {
    static public native void init(AssetManager assetManager, String externalFilesDir);
    static public native void setOutputSurface(Surface surface);
    static public native void start();
    static public native void stop();
    static public native void destroy();
}
