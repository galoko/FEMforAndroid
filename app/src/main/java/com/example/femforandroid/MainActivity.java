package com.example.femforandroid;

import android.app.Activity;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.os.PowerManager;
import 	android.os.PowerManager.WakeLock;
import android.content.Context;

public class MainActivity extends Activity implements SurfaceHolder.Callback {

    private AssetManager assetManager;

    static {
        System.loadLibrary("main");
    }

    SurfaceHolder holder;

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        assetManager = getAssets();

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        SurfaceView surfaceView = (SurfaceView)findViewById(R.id.surfaceview);
        surfaceView.getHolder().addCallback(this);
    }

    @Override
    protected void onStart() {
        super.onStart();
        JNIHandler.init(assetManager, getExternalFilesDir(null).getAbsolutePath());
    }

    @Override
    protected void onResume() {
        super.onResume();
        JNIHandler.start();
        setOutputSurfaceFromHolder();
    }

    @Override
    protected void onPause() {
        super.onPause();
        JNIHandler.stop();
        JNIHandler.setOutputSurface(null);
    }

    @Override
    protected void onStop() {
        super.onStop();
        JNIHandler.destroy();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    public void surfaceCreated(SurfaceHolder holder) {
        this.holder = holder;
        setOutputSurfaceFromHolder();
    }

    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        this.holder = holder;
        setOutputSurfaceFromHolder();
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
        this.holder = null;
        setOutputSurfaceFromHolder();
    }

    private void setOutputSurfaceFromHolder() {
        JNIHandler.setOutputSurface(holder != null ? holder.getSurface() : null);
    }
}
