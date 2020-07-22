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

/*
 * Java JNI wrapper for linking the following classes below across the bridge.
 *
 * Android Java Object  : com.viromedia.bridge.view.Controller.java
 * Java JNI Wrapper     : com.viro.renderer.ControllerJni.java
 * Cpp JNI wrapper      : Controller_JNI.cpp
 * Cpp Object           : VROInputPresenter
 */
package com.viro.core;

import android.util.Log;

import java.util.ArrayList;

/**
 * Controller represents the UI through which the user interacts with the {@link Scene}. The exact
 * form of Controller depends on the underlying platform. For example, for Daydream this represents
 * the Daydream controller (the laser pointer). For Cardboard and GearVR, Controller is effectively
 * the head-mounted display itself (and its tap button).
 * <p>
 * Controllers are also able to listen to all events. The Controller is is notified of all events
 * that occur within the Scene, regardless of what {@link Node} those events are associated with
 * (with the exception of Hover events). This is useful if you wish to be alerted of all events
 * occurring in the Scene in a centralized place.
 * <p>
 * Controller should not be constructed by your app. It is retrieved via {@link
 * ViroView#getController()}.
 * <p>
 * For an extended discussion on Controller types and input events, refer to the <a
 * href="https://virocore.viromedia.com/docs/events">Input Event Guide</a>.
 */
public class Controller implements EventDelegate.EventDelegateCallback {

    private ViroContext mViroContext;
    private boolean mReticleVisible = true;
    private boolean mControllerVisible = true;

    private EventDelegate mEventDelegate;
    private ClickListener mClickListener;
    private HoverListener mHoverListener;

    /**
     * @hide
     * @param viroContext
     */
    Controller(ViroContext viroContext) {
        mViroContext = viroContext;
        mEventDelegate = new EventDelegate();
        mEventDelegate.setEventDelegateCallback(this);
        nativeSetEventDelegate(mViroContext.mNativeRef, mEventDelegate.mNativeRef);
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            dispose();
        } finally {
            super.finalize();
        }
    }

    /**
     * Release native resources associated with this Controller.
     */
    public void dispose() {
        mEventDelegate.dispose();
    }

    /**
     * @hide
     * @param delegate
     */
    public void setEventDelegate(EventDelegate delegate) {
        if (mEventDelegate != null) {
            mEventDelegate.dispose();
        }
        mEventDelegate = delegate;
        nativeSetEventDelegate(mViroContext.mNativeRef, delegate.mNativeRef);
    }

    /**
     * Set the reticle visibility on or off. The reticle is the small pointer that appears at the
     * center of the screen, which is useful for tapping and fusing on objects.
     * <p>
     * Defaults to true.
     *
     * @param visible True to make the reticle visible, false to make it invisible.
     */
    public void setReticleVisible(boolean visible) {
        mReticleVisible = visible;
        nativeEnableReticle(mViroContext.mNativeRef, visible);
    }

    /**
     * Returns true if the reticle is currently visible.
     *
     * @return True if the reticle is visible.
     */
    public boolean isReticleVisible() {
        return mReticleVisible;
    }

    /**
     * Set the controller to visible or invisible. This currently only applies to the Daydream
     * platform, and determines whether the Daydream controller is displayed on the screen.
     * <p>
     * Defaults to true.
     *
     * @param visible True to display the controller.
     */
    public void setControllerVisible(boolean visible) {
        mControllerVisible = visible;
        nativeEnableController(mViroContext.mNativeRef, visible);
    }

    /**
     * Returns true if the Controller is currently visible.
     *
     * @return True if the Controller is visible.
     */
    public boolean isControllerVisible() {
        return mControllerVisible;
    }

    /**
     * Get the direction in which the Controller is pointing. For Daydream devices this returns
     * the direction the pointer if pointing, and for other devices it returns the direction the
     * user is facing (e.g. in the direction of the reticle).
     *
     * @return The direction the Controller is pointing.
     */
    public Vector getForwardVector() {
        return new Vector(nativeGetControllerForwardVector(mViroContext.mNativeRef));
    }

    /**
     * @hide
     * @param callback
     */
    public void getControllerForwardVectorAsync(ControllerJniCallback callback){
        nativeGetControllerForwardVectorAsync(mViroContext.mNativeRef, callback);
    }

    /**
     * Set a {@link ClickListener} to respond when users click with the Controller.
     *
     * @param listener The listener to attach, or null to remove any installed listener.
     */
    public void setClickListener(ClickListener listener) {
        mClickListener = listener;
        if (listener != null) {
            mEventDelegate.setEventEnabled(EventDelegate.EventAction.ON_CLICK, true);
        }
        else {
            mEventDelegate.setEventEnabled(EventDelegate.EventAction.ON_CLICK, false);
        }
    }

    /**
     * Get the {@link ClickListener} that is currently installed for this Controller.
     *
     * @return The installed listener, or null if none is installed.
     */
    public ClickListener getClickListener() {
        return mClickListener;
    }


    /**
     * Set the {@link HoverListener} to respond when users hover with the Controller.
     *
     * @param listener The listener to attach, or null to remove any installed listener.
     */
    public void setHoverListener(HoverListener listener) {
        mHoverListener = listener;
        if (listener != null) {
            mEventDelegate.setEventEnabled(EventDelegate.EventAction.ON_HOVER, true);
        }
        else {
            mEventDelegate.setEventEnabled(EventDelegate.EventAction.ON_HOVER, false);
        }
    }

    /**
     * Get the {@link HoverListener} that is currently installed for this Controller.
     *
     * @return The installed listener, or null if none is installed.
     */
    public HoverListener getHoverListener() {
        return mHoverListener;
    }

    /**
     * Set the bit mask that determines what lights will illuminate this Node. This bit mask is
     * bitwise and-ed (&) with each Light's influenceBitMask. If the result is > 0, then the Light
     * will illuminate this controller. The default value is 0x1.
     *
     * @param bitMask The bit mask to set.
     */
    public void setLightReceivingBitMask(int bitMask) {
        nativeSetLightReceivingBitMask(mViroContext.mNativeRef, bitMask);
    }

    /**
     * @hide
     */
    @Override
    public void onClick(int source, Node node, ClickState clickState, float[] hitLoc) {
        if (mClickListener != null) {
            Vector hitLocVec = hitLoc != null ? new Vector(hitLoc) : null;
            mClickListener.onClickState(source, node, clickState, hitLocVec);
            if (clickState == ClickState.CLICKED) {
                mClickListener.onClick(source, node, hitLocVec);
            }
        }
    }

    /**
     * @hide
     */
    @Override
    public void onHover(int source, Node node, boolean isHovering, float[] hitLoc) {
        if (mHoverListener != null) {
            Vector hitLocVec = hitLoc != null ? new Vector(hitLoc) : null;
            mHoverListener.onHover(source, node, isHovering, hitLocVec);
        }
    }

    @Override
    public void onClick(ArrayList<EventDelegate.ButtonEvent> events) {
        // TODO: No-op, Not implemented for Viro-core
    }

    @Override
    public void onHover(ArrayList<EventDelegate.HoverEvent> events) {
        // TODO: No-op, Not implemented for Viro-core
    }

    @Override
    public void onThumbStickEvent(ArrayList<EventDelegate.ThumbStickEvent> events) {
        // TODO: No-op, Not implemented for Viro-core
    }

    @Override
    public void onWeightedTriggerEvent(ArrayList<EventDelegate.TriggerEvent> events) {
        // TODO: No-op, Not implemented for Viro-core
    }

    @Override
    public void onMove(ArrayList<EventDelegate.MoveEvent> events) {
        // TODO: No-op, Not implemented for Viro-core
    }

    @Override
    public void onControllerStatus(ArrayList<EventDelegate.ControllerStatus> events) {
        // TODO: No-op, Not implemented for Viro-core
    }

    private native void nativeSetEventDelegate(long contextRef, long delegateRef);
    private native void nativeEnableReticle(long contextRef, boolean enabled);
    private native void nativeEnableController(long contextRef, boolean enabled);
    private native float[] nativeGetControllerForwardVector(long contextRef);
    private native void nativeGetControllerForwardVectorAsync(long renderContextRef, ControllerJniCallback callback);
    protected native void nativeSetLightReceivingBitMask(long contextRef, int bitMask);

    /**
     * @hide
     */
    public interface ControllerJniCallback{
        void onGetForwardVector(float x, float y, float z);
    }
}
