//
//  EventDelegate_JNI.h
//  ViroRenderer
//
//  Copyright © 2018 Viro Media. All rights reserved.
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

#ifndef EventDelegate_JNI_h
#define EventDelegate_JNI_h

#include <memory>
#include <stdarg.h>
#include <iostream>
#include "VRONode.h"
#include "VROBillboardConstraint.h"
#include "VROPlatformUtil.h"
#include "VROGeometry.h"
#include "VRONode.h"
#include "VROARHitTestResult.h"
#include "Node_JNI.h"

#include "VRODefines.h"
#include VRO_C_INCLUDE

/**
 * EventDelegate_JNI implements a JNI abstraction of the VROEventDelegate to
 * both allow java objects to register for events, and to notify them of
 * delegate events across the JNI bridge.
 */
class EventDelegate_JNI : public VROEventDelegate {
public:
    EventDelegate_JNI(VRO_OBJECT obj, VRO_ENV env) :
        _javaObject(VRO_NEW_WEAK_GLOBAL_REF(obj)) {

    }

    ~EventDelegate_JNI() {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_DELETE_WEAK_GLOBAL_REF(_javaObject);
    }

    /*
     virtual void onButtonEvent(std::vector<ButtonEvent> &events)
     virtual void onHover(std::vector<HoverEvent> &events)
     virtual void onThumbStickEvent(std::vector<ThumbStickEvent> &events)
     virtual void onWeightedTriggerEvent(std::vector<TriggerEvent> &events)
     virtual void onControllerStatus(std::vector<ControllerStat> status)
     virtual void onMove(std::vector<MoveEvent> &events)
     */

    /*
     Java event delegates to be triggered across the JNI bridge.
     */
    void onHover(int source, std::shared_ptr<VRONode> node, bool isHovering, std::vector<float> position);
    void onClick(int source, std::shared_ptr<VRONode> node, ClickState clickState, std::vector<float> position);
    void onMove(int source, std::shared_ptr<VRONode> node, VROVector3f rotation, VROVector3f position, VROVector3f forwardVec);
    void onControllerStatus(std::vector<ControllerStat> status);
    void onCameraTransformUpdate(VROVector3f position, VROVector3f rotation, VROVector3f forward, VROVector3f up);

private:
    VRO_OBJECT _javaObject;
};

#endif