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
 * Java JNI wrapper responsible for registering and implementing event
 * delegate callbacks across the JNI bridge. It ultimately links the
 * following classes below.
 *
 * Java JNI Wrapper     : com.viro.renderer.EventDelegateJni.java
 * Cpp JNI wrapper      : EventDelegate_JNI.cpp
 * Cpp Object           : VROEventDelegate.cpp
 */
package com.viro.core;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

/**
 * @hide
 */
public class EventDelegate {

    /*
     * Delegate interface to be implemented by a java view component so as
     * to receive event callbacks triggered from native. Implemented delegates
     * must be set within EventDelegateJni through setEventDelegateCallback()
     * for it to be triggered.
     *
     * These callbacks correspond to the set or subset of EventTypes above
     * (Note they may not correspond 1 to 1 - certain EventTypes may not yet
     * be needed or useful for Java views or components).
     *
     * @hide
     */
    public interface EventDelegateCallback {
        void onClick(ArrayList<ButtonEvent> events);
        void onHover(ArrayList<HoverEvent> events);
        void onThumbStickEvent(ArrayList<ThumbStickEvent> events);
        void onWeightedTriggerEvent(ArrayList<TriggerEvent> events);
        void onMove(ArrayList<MoveEvent> events);
        void onControllerStatus(ArrayList<ControllerStatus> events);
        void onHover(int source, Node node, boolean isHovering, float hitLoc[]);
        void onClick(int source, Node node, ClickState clickState, float hitLoc[]);
    }

    long mNativeRef;
    private WeakReference<EventDelegateCallback> mDelegate = null;

    /**
     * Construct a new EventDelegate.
     */
    public EventDelegate() {
        mNativeRef = nativeCreateDelegate();
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
     * Release native resources associated with this EventDelegate.
     */
    public void dispose() {
        if (mNativeRef != 0) {
            nativeDestroyDelegate(mNativeRef);
            mNativeRef = 0;
        }
    }

    /**
     * Enables or disables the given event type. If enabled, the callback corresponding to
     * the even type is triggered, else, disabled otherwise. Note: All EventTypes are
     * disabled by default.
     */
    public void setEventEnabled(EventAction type, boolean enabled) {
        nativeEnableEvent(mNativeRef, type.mTypeId, enabled);
    }

    public void setEventDelegateCallback(EventDelegateCallback delegate) {
        mDelegate = new WeakReference<EventDelegateCallback>(delegate);
    }

    /*
     Native Functions called into JNI
     */
    private native long nativeCreateDelegate();
    private native void nativeDestroyDelegate(long mNativeNodeRef);
    private native void nativeEnableEvent(long mNativeNodeRef, int eventType, boolean enabled);

    /*
     * EventAction types corresponding to VROEventDelegate.h, used for
     * describing EventSource types. For example, if a click event
     * was ClickUp or ClickDown.
     *
     * IMPORTANT: Do Not change the Enum Values!!! Simply add additional
     * event types as need be. This should always map directly to
     * VROEventDelegate.h
     */
    public enum EventAction {
        ON_HOVER(1),                            // Node + Controller based
        ON_CLICK(2),                            // Node + Controller based
        ON_MOVE(3),                             // Controller based only
        ON_THUMBSTICK(4),                       // Controller based only
        ON_TRIGGER(5),                          // Controller based only
        ON_CONTROLLER_STATUS(6);                // Controller based only

        public final int mTypeId;

        EventAction(int id){
            mTypeId = id;
        }

        private static Map<Integer, EventAction> map = new HashMap<Integer, EventAction>();
        static {
            for (EventAction action : EventAction.values()) {
                map.put(action.mTypeId, action);
            }
        }
        public static EventAction valueOf(int id) {
            return map.get(id);
        }
    }

    public class ButtonEvent {
        public int deviceId;
        public int source;
        public int hitNodeId;
        public ClickState state;
        public Vector intersecPos;

        // Internal helper convertion param
        private int clickIntState;
    }

    public class HoverEvent {
        public int deviceId;
        public int source;
        public boolean isHovering;
        public Vector intersecPos;
    }

    public class ThumbStickEvent {
        public int deviceId;
        public int source;
        public boolean isPressed;
        public Vector axisLocation;
    }

    public class TriggerEvent {
        public int deviceId;
        public int source;
        public float weight;
    }

    public class MoveEvent {
        public int deviceId;
        public int source;
        public Vector pos;
        public Quaternion rot;
    }
    public class ControllerStatus {
        public int deviceId;
        public boolean isConnected;
        public boolean is6Dof;
        public int batteryPercentage;
    }

    /*
     Callback functions called from JNI (triggered from native)
     that then trigger the corresponding EventDelegateCallback (mDelegate)
     that has been set through setEventDelegateCallback().
     */
    void onClick(ButtonEvent[] events) {
        for (ButtonEvent event : events) {
            event.state = ClickState.valueOf(event.clickIntState);
        }

        if (mDelegate != null && mDelegate.get() != null) {
            mDelegate.get().onClick(new ArrayList<ButtonEvent>(Arrays.asList(events)));
        }
    }

    void onHover(HoverEvent[] events) {
        if (mDelegate != null && mDelegate.get() != null) {
            mDelegate.get().onHover(new ArrayList<HoverEvent>(Arrays.asList(events)));
        }
    }

    void onThumbStickEvent(ThumbStickEvent[] events) {
        if (mDelegate != null && mDelegate.get() != null) {
            mDelegate.get().onThumbStickEvent(new ArrayList<ThumbStickEvent>(Arrays.asList(events)));
        }
    }

    void onWeightedTriggerEvent(TriggerEvent[] events) {
        if (mDelegate != null && mDelegate.get() != null) {
            mDelegate.get().onWeightedTriggerEvent(new ArrayList<TriggerEvent>(Arrays.asList(events)));
        }
    }

    void onMove(MoveEvent[] events) {
        if (mDelegate != null && mDelegate.get() != null) {
            mDelegate.get().onMove(new ArrayList<MoveEvent>(Arrays.asList(events)));
        }
    }

    void onControllerStatus(ControllerStatus[] events) {
        if (mDelegate != null && mDelegate.get() != null) {
            mDelegate.get().onControllerStatus(new ArrayList<ControllerStatus>(Arrays.asList(events)));
        }
    }

    //// LEGACY ////

    /**
     * @hide
     */
    void onHover(int source, int nodeId, boolean isHovering, float[] position) {
        Node node = Node.getNodeWithID(nodeId);
        if (mDelegate != null && mDelegate.get() != null) {
            mDelegate.get().onHover(source, node, isHovering, position);
        }
    }
    /**
     * @hide
     */
    void onClick(int source, int nodeId, int clickState, float[] position) {
        Node node = Node.getNodeWithID(nodeId);
        if (mDelegate != null && mDelegate.get() != null) {
            mDelegate.get().onClick(source, node, ClickState.valueOf(clickState), position);
        }
    }
}
