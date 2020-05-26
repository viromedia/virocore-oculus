//
//  VROEventDelegate.h
//  ViroRenderer
//
//  Copyright © 2016 Viro Media. All rights reserved.
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

#ifndef VROEventDelegate_h
#define VROEventDelegate_h

#include <stdio.h>
#include <vector>
#include <string>
#include <memory>
#include <set>
#include <map>
#include "VROVector3f.h"
#include "VROHitTestResult.h"
#include "VROInputType.h"

#include <limits>
class VROARHitTestResult;
class VROARPointCloud;

static const float kOnFuseReset = std::numeric_limits<float>::max();

/*
 Class for both registering for and implementing event delegate callbacks.
 */
class VROEventDelegate {
public:
    /*
     Enum EventAction types that are supported by this delegate, used for
     describing InputSources from InputTypes.h. For example, an OnClick
     action may originate from a ViroDayDream AppButton inputSource.
     
     IMPORTANT: Enum values should match EventAction within EventDelegateJni.java
     as the standard format to be passed through the JNI layer.
     Do Not change the Enum Values!!! Simply add additional event types as need be.
     */
    enum EventAction {
        OnClick = 1,
        OnHover = 2,
        OnMove = 3,
        OnThumbStick = 4,
        OnControllerStatus = 5,
        OnCameraTransformUpdate = 6,

        OnPinch = 99,
        OnDrag = 100,
        OnRotate = 200,
        OnFuse = 300,
    };

    /*
     ClickState enum describing the OnClick Event action.
     */
    enum ClickState {
        ClickDown = 1,
        ClickUp = 2,
        Clicked = 3
    };

    //////////


    struct ButtonEvent {
        int deviceId;
        int source;
        ClickState state;
        VROVector3f intersecPos;

        // Node the click event is performed on?
        std::shared_ptr<VRONode> hitResultNode;
    };

    struct HoverEvent {
        int deviceId;
        int source;
        bool isHovering;
        VROVector3f intersecPos;

        // Node the click event is performed on?
        std::shared_ptr<VRONode> onHoveredNode;
        std::shared_ptr<VRONode> offHoveredNode;
    };

    struct MoveEvent {
        int deviceId;
        int source;
        VROVector3f pos;
        VROQuaternion rot;
    };

    struct ThumbStickEvent {
        int deviceId;
        int source;
        bool isPressed;
        VROVector3f axisLocation;
    };



    /////////
    /*
     TouchState enum describing the OnTouch Event action.
     */
    enum TouchState {
        TouchDown = 1,
        TouchDownMove = 2,
        TouchUp = 3,
    };
    
    enum PinchState {
        PinchStart = 1,
        PinchMove = 2,
        PinchEnd = 3,
    };
    
    enum RotateState {
        RotateStart = 1,
        RotateMove = 2,
        RotateEnd = 3,
    };

    enum SwipeState {
        SwipeUp = 1,
        SwipeDown = 2,
        SwipeLeft = 3,
        SwipeRight = 4
    };

    enum DragState {
        Start = 1,
        Dragging = 2,
        End = 3
    };

    /*
     Enum ControllerStatus types describing the availability status of the
     current input controller.
     
     IMPORTANT: Enum values should match EventSource within EventDelegateJni.java
     as the standard format to be passed through the JNI layer.
     Do Not change the Enum Values!!! Simply add additional event types as need be.
     */
    enum ControllerStatus {
        Unknown = 1,
        Connecting = 2,
        Connected = 3,
        Disconnected = 4,
        Error = 5
    };


    // Disable all event callbacks by default
    VROEventDelegate() {
        _enabledEventMap[VROEventDelegate::EventAction::OnMove] = false;                 // OVR
        _enabledEventMap[VROEventDelegate::EventAction::OnClick] = false;                // OVR
        _enabledEventMap[VROEventDelegate::EventAction::OnThumbStick] = false;                // OVR
        _enabledEventMap[VROEventDelegate::EventAction::OnControllerStatus] = false;     // OVR
        _enabledEventMap[VROEventDelegate::EventAction::OnCameraTransformUpdate] = false; // TODO
        _enabledEventMap[VROEventDelegate::EventAction::OnHover] = false;
    }
    /*
     Informs the renderer to enable / disable the triggering of
     specific EventSource delegate callbacks.
     */
    void setEnabledEvent(VROEventDelegate::EventAction type, bool enabled) {
        _enabledEventMap[type] = enabled;
    }

    bool isEventEnabled(VROEventDelegate::EventAction type) {
        return _enabledEventMap[type];
    }

    // Can represent Events from different DeviceIDs, AND sources
    // Can be triggered on Nodes
    virtual void onButtonEvent(std::vector<ButtonEvent> &events) {
        //No-op
    }

    // Can represent Events from different DeviceIDs, not sources
    virtual void onMove(std::vector<MoveEvent> &events) {
        //No-op
    }

    // Can represent Events from different DeviceIDs, not sources
    // Can be triggered on Nodes.
    virtual void onHover(std::vector<HoverEvent> &events) {
        //No-op
    }

    virtual void onThumbStickEvent(std::vector<ThumbStickEvent> &events) {
        //No-op
    }


    /*
     Delegate events triggered by the VROInputControllerBase.
     */
    virtual void onHover(int source, std::shared_ptr<VRONode> node, bool isHovering, std::vector<float> position) {
        //No-op
    }

    virtual void onClick(int source, std::shared_ptr<VRONode> node, ClickState clickState, std::vector<float> position) {
        //No-op
    }
    
    virtual void onTouch(int source, std::shared_ptr<VRONode> node, TouchState touchState, float x, float y) {
        //No-op
    }

    virtual void onMove(int source, std::shared_ptr<VRONode> node, VROVector3f rotation, VROVector3f position, VROVector3f forwardVec) {
        //No-op
    }

    virtual void onControllerStatus(int source, ControllerStatus status) {
        //No-op
    }

    virtual void onGazeHit(int source, std::shared_ptr<VRONode> node, const VROHitTestResult &hit) {
        //No-op
    }

    virtual void onSwipe(int source, std::shared_ptr<VRONode> node, SwipeState swipeState) {
        //No-op
    }

    virtual void onScroll(int source, std::shared_ptr<VRONode> node, float x, float y) {
        //No-op
    }

    virtual void onDrag(int source, std::shared_ptr<VRONode> node, VROVector3f newPosition) {
        //No-op
    }

    virtual void onFuse(int source, std::shared_ptr<VRONode> node, float timeToFuseRatio) {
        //No-op
    }
    
    virtual void onPinch(int source, std::shared_ptr<VRONode> node, float scaleFactor, PinchState pinchState) {
        //No-op
    }
    
    virtual void onRotate(int source, std::shared_ptr<VRONode> node, float rotationRadians, RotateState rotateState) {
        //No-op
    }

    virtual void onCameraARHitTest(std::vector<std::shared_ptr<VROARHitTestResult>> results) {
        //No-op
    }

    virtual void onARPointCloudUpdate(std::shared_ptr<VROARPointCloud> pointCloud) {
        //No-op
    }

    virtual void onCameraTransformUpdate(VROVector3f position, VROVector3f rotation, VROVector3f forward, VROVector3f up) {
        //No-op
    }

    void setTimeToFuse(float durationInMillis){
        _timeToFuseDuration = durationInMillis;
    }

    float getTimeToFuse(){
        return _timeToFuseDuration;
    }

private:
    
    std::map<VROEventDelegate::EventAction , bool> _enabledEventMap;

    /*
     Duration used to count down from for triggering onFuse events, in milliseconds.
     Defaults to 2000 milliseconds.
     */
    float _timeToFuseDuration = 2000;
};
#endif
