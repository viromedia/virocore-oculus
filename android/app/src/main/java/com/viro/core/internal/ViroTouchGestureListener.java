//
//  Copyright (c) 2017-present, ViroMedia, Inc.
//  All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining
//  a copy of this software and associated documentation files (the
//  "Software"), to deal in the Software without restriction, including
//  without limitation the rights to use, copy, modify, merge, publish,
//  distribute, sublicense, and/or sell copies of the Software, and to
//  permit persons to whom the Software is furnished to do so, subject to
//  the following conditions:
//
//  The above copyright notice and this permission notice shall be included
//  in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

package com.viro.core.internal;

import android.content.Context;
import android.graphics.PointF;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.View;
import android.util.Log;

import com.viro.core.Renderer;

/*
 Class that handles and notifies RendererJni with touch and gesture events. This extends
 SimpleOnGestureListener, because we only want to listen to a subset of the gestures.
 */
public class ViroTouchGestureListener extends GestureDetector.SimpleOnGestureListener
        implements View.OnTouchListener, ScaleGestureDetector.OnScaleGestureListener,
        RotationGestureDetector.OnRotateGestureListener {

    private static final long MIN_FINGER_TOUCH_DURATION = 50; // milliseconds
    private static final long INVALID_FINGER_DOWN_TIME = -1;

    // we tie these with the MotionEvent values because we use them in the renderer.
    private static final int EVENT_BEGIN = MotionEvent.ACTION_DOWN;
    private static final int EVENT_END = MotionEvent.ACTION_UP;
    private static final int EVENT_MOVE = MotionEvent.ACTION_MOVE;

    private Renderer mNativeRenderer;
    // whether or not the renderer has been destroyed.
    private boolean mDestroyed = false;

    private ScaleGestureDetector mScaleDetector;
    private RotationGestureDetector mRotateDetector;

    private View.OnTouchListener mUserTouchListener;

    private long mFingerDownTime = 0;
    private boolean mIsTouching = false;
    private boolean mIsScaling = false;
    private boolean mIsRotating = false;
    private float mLatestScale = 1.0f;
    private float mLastTouchX = 0;
    private float mLastTouchY = 0;

    public ViroTouchGestureListener(Context context, Renderer rendererJni) {
        mNativeRenderer = rendererJni;
        mScaleDetector = new ScaleGestureDetector(context, this);
        mScaleDetector.setQuickScaleEnabled(false);
        mRotateDetector = new RotationGestureDetector(this);
    }

    public void destroy() {
        mDestroyed = true;
    }

    public void setOnTouchListener(View.OnTouchListener listener) {
        mUserTouchListener = listener;
    }

    /*
     * -- OnTouchListener Implementation --
     */

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        // No-op
        return false;
    }

    /*
     * -- OnScaleGestureListener Implementation --
     */
    @Override
    public boolean onScaleBegin(ScaleGestureDetector detector) {
        // No-op
        return false;
    }

    @Override
    public boolean onScale(ScaleGestureDetector detector) {
        // No-op
        return false;
    }

    @Override
    public void onScaleEnd(ScaleGestureDetector detector) {
        // No-op
    }

    /*
     * -- OnRotateGestureListener Implementation --
     */

    @Override
    public void onRotateBegin(RotationGestureDetector detector) {
        // No-op
    }

    @Override
    public void onRotate(RotationGestureDetector detector) {
        // No-op
    }

    @Override
    public void onRotateEnd(RotationGestureDetector detector) {
        // No-op
    }
}
