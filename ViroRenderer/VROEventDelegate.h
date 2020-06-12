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
        OnHover = 1,
        OnClick = 2,
        OnMove = 3,
        OnThumbStick = 4,
        OnControllerStatus = 5,
        OnCameraTransformUpdate = 6,
    };

    /*
     ClickState enum describing the OnClick Event action.
     */
    enum ClickState {
        ClickDown = 1,
        ClickUp = 2,
        Clicked = 3
    };

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

    struct TriggerEvent {
        int deviceId;
        int source;
        float weight;
    };

    struct ControllerStat {
        int deviceId;
        bool isConnected;
        bool is6Dof;
        int batteryPercentage;
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
        VROEventDelegate::ButtonEvent e = events.front();
        std::vector<float> intPos = {e.intersecPos.x, e.intersecPos.y, e.intersecPos.z};
        onClick(e.source, e.hitResultNode, e.state, intPos);
    }

    // Can be triggered on Nodes
    virtual void onHover(std::vector<HoverEvent> &events) {
        VROEventDelegate::HoverEvent e = events.front();
        std::vector<float> intPos = {e.intersecPos.x, e.intersecPos.y, e.intersecPos.z};
        onHover(e.source, e.onHoveredNode, e.isHovering, intPos);
    }

    // Can represent Events from different DeviceIDs, not sources
    virtual void onMove(std::vector<MoveEvent> &events) {
        VROEventDelegate::MoveEvent e = events.front();
        onMove(e.source, nullptr, e.rot.toEuler(), e.pos, VROVector3f());
    }

    virtual void onThumbStickEvent(std::vector<ThumbStickEvent> &events) {
        //No-op
    }

    virtual void onWeightedTriggerEvent(std::vector<TriggerEvent> &events) {
        //No-op
    }

    virtual void onControllerStatus(std::vector<ControllerStat> status) {
        //No-op
    }

    /*
     Delegate events for the listening of single events, from the old API bridge.
     Meant for simple API usage and backwards Viro Compatibility.
     */
    virtual void onHover(int source, std::shared_ptr<VRONode> node, bool isHovering, std::vector<float> position) {
        //No-op
    }

    virtual void onClick(int source, std::shared_ptr<VRONode> node, ClickState clickState, std::vector<float> position) {
        //No-op
    }

    virtual void onCameraTransformUpdate(VROVector3f position, VROVector3f rotation, VROVector3f forward, VROVector3f up) {
        //No-op
    }

    virtual void onMove(int source, std::shared_ptr<VRONode> node, VROVector3f rotation, VROVector3f position, VROVector3f forwardVec) {
        //No-op
    }
private:
    std::map<VROEventDelegate::EventAction , bool> _enabledEventMap;
};
#endif
