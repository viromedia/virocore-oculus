//
//  EventDelegate_JNI.cpp
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

#include "EventDelegate_JNI.h"
#include "Node_JNI.h"
#include "ViroUtils_JNI.h"
#include "VROARPointCloud.h"

#if VRO_PLATFORM_ANDROID
#include "arcore/ARUtils_JNI.h"

#define VRO_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_com_viro_core_EventDelegate_##method_name
#else
#define VRO_METHOD(return_type, method_name) \
    return_type EventDelegate_##method_name
#endif

extern "C" {

VRO_METHOD(VRO_REF(EventDelegate_JNI), nativeCreateDelegate)(VRO_NO_ARGS) {
   VRO_METHOD_PREAMBLE;
   std::shared_ptr<EventDelegate_JNI> delegate = std::make_shared<EventDelegate_JNI>(obj, env);
   return VRO_REF_NEW(EventDelegate_JNI, delegate);
}

VRO_METHOD(void, nativeDestroyDelegate)(VRO_ARGS
                                        VRO_REF(EventDelegate_JNI) native_node_ref) {
    // TODO: figure out why this is needed
    VROPlatformDispatchAsyncRenderer([native_node_ref]{
        VRO_REF_DELETE(VRONode, native_node_ref);
    });
}

VRO_METHOD(void, nativeEnableEvent)(VRO_ARGS
                                    VRO_REF(EventDelegate_JNI) native_node_ref,
                                    VRO_INT eventTypeId,
                                    VRO_BOOL enabled) {

    std::weak_ptr<EventDelegate_JNI> delegate_w = VRO_REF_GET(EventDelegate_JNI, native_node_ref);
    VROPlatformDispatchAsyncRenderer([delegate_w, eventTypeId, enabled] {
        std::shared_ptr<EventDelegate_JNI> delegate = delegate_w.lock();
        if (!delegate) {
            return;
        }
        VROEventDelegate::EventAction eventType = static_cast<VROEventDelegate::EventAction>(eventTypeId);
        delegate->setEnabledEvent(eventType, enabled);
    });
}

}  // extern "C"

void EventDelegate_JNI::onButtonEvent(std::vector<ButtonEvent> &events) {
    VROEventDelegate::onButtonEvent(events);
    if (events.size() == 0) {
        return;
    }

    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);
    VROPlatformDispatchAsyncApplication([weakObj, events] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        VRO_OBJECT_ARRAY eventArray = VRO_NEW_OBJECT_ARRAY(events.size(), "com/viro/core/EventDelegate$ButtonEvent");
        jobject outerClass = VROPlatformConstructHostObject("com/viro/core/EventDelegate", "()V");

        for (int i = 0; i < events.size(); i ++) {
            ButtonEvent event = events[i];
            jobject jEventObj = VROPlatformConstructHostObject("com/viro/core/EventDelegate$ButtonEvent", "(Lcom/viro/core/EventDelegate;)V", outerClass);
            VROPlatformSetInt(env, jEventObj, "deviceId", event.deviceId);
            VROPlatformSetInt(env, jEventObj, "source", event.source);
            VROPlatformSetInt(env, jEventObj, "hitNodeId", event.hitResultNode->getUniqueID());
            VROPlatformSetInt(env, jEventObj, "clickIntState", (int)event.state);

            VRO_OBJECT jPos = VROPlatformConstructHostObject("com/viro/core/Vector", "(FFF)V",
                    event.intersecPos.x, event.intersecPos.y, event.intersecPos.z);
            VROPlatformSetObject(env, jEventObj, "intersecPos", "Lcom/viro/core/Vector;", jPos);
            VRO_ARRAY_SET(eventArray, i, jEventObj);

            // Clean up our local reference to jObject after.
            VRO_DELETE_LOCAL_REF(jEventObj);
            VRO_DELETE_LOCAL_REF(jPos);
        }

        // Notify the Java delegate
        VROPlatformCallHostFunction(localObj,
                                    "onClick", "([Lcom/viro/core/EventDelegate$ButtonEvent;)V", eventArray);
        VRO_DELETE_LOCAL_REF(outerClass);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}

void EventDelegate_JNI::onHover(std::vector<HoverEvent> &events) {
    VROEventDelegate::onHover(events);
    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);
    VROPlatformDispatchAsyncApplication([weakObj, events] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        VRO_OBJECT_ARRAY eventArray = VRO_NEW_OBJECT_ARRAY(events.size(), "com/viro/core/EventDelegate$HoverEvent");
        jobject outerClass = VROPlatformConstructHostObject("com/viro/core/EventDelegate", "()V");

        for (int i = 0; i < events.size(); i ++) {
            HoverEvent event = events[i];
            jobject jEventObj = VROPlatformConstructHostObject("com/viro/core/EventDelegate$HoverEvent", "(Lcom/viro/core/EventDelegate;)V", outerClass);
            VROPlatformSetInt(env, jEventObj, "deviceId", event.deviceId);
            VROPlatformSetInt(env, jEventObj, "source", event.source);
            VROPlatformSetBool(env, jEventObj, "isHovering", event.isHovering);
            VRO_OBJECT jPos = VROPlatformConstructHostObject("com/viro/core/Vector", "(FFF)V",
                                                             event.intersecPos.x, event.intersecPos.y, event.intersecPos.z);
            VROPlatformSetObject(env, jEventObj, "intersecPos", "Lcom/viro/core/Vector;", jPos);
            VRO_ARRAY_SET(eventArray, i, jEventObj);

            // Clean up our local reference to jObject after.
            VRO_DELETE_LOCAL_REF(jEventObj);
            VRO_DELETE_LOCAL_REF(jPos);
        }

        // Notify the Java delegate
        VROPlatformCallHostFunction(localObj,
                                    "onHover", "([Lcom/viro/core/EventDelegate$HoverEvent;)V", eventArray);
        VRO_DELETE_LOCAL_REF(outerClass);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}

void EventDelegate_JNI::onThumbStickEvent(std::vector<ThumbStickEvent> &events){
    pwarn("Daniel JNI onThumbStickEvent");

    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);
    VROPlatformDispatchAsyncApplication([weakObj, events] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        VRO_OBJECT_ARRAY eventArray = VRO_NEW_OBJECT_ARRAY(events.size(), "com/viro/core/EventDelegate$ThumbStickEvent");
        jobject outerClass = VROPlatformConstructHostObject("com/viro/core/EventDelegate", "()V");

        for (int i = 0; i < events.size(); i ++) {
            ThumbStickEvent event = events[i];
            jobject jEventObj = VROPlatformConstructHostObject("com/viro/core/EventDelegate$ThumbStickEvent", "(Lcom/viro/core/EventDelegate;)V", outerClass);
            VROPlatformSetInt(env, jEventObj, "deviceId", event.deviceId);
            VROPlatformSetInt(env, jEventObj, "source", event.source);
            VROPlatformSetBool(env, jEventObj, "isPressed", event.isPressed);
            VRO_OBJECT jPos = VROPlatformConstructHostObject("com/viro/core/Vector", "(FFF)V",
                                                             event.axisLocation.x, event.axisLocation.y, event.axisLocation.z);
            VROPlatformSetObject(env, jEventObj, "axisLocation", "Lcom/viro/core/Vector;", jPos);
            VRO_ARRAY_SET(eventArray, i, jEventObj);
pwarn("Daniel thumb stick pressed %d", event.isPressed);
            // Clean up our local reference to jObject after.
            VRO_DELETE_LOCAL_REF(jEventObj);
            VRO_DELETE_LOCAL_REF(jPos);
        }

        // Notify the Java delegate
        VROPlatformCallHostFunction(localObj,
                                    "onThumbStickEvent", "([Lcom/viro/core/EventDelegate$ThumbStickEvent;)V", eventArray);
        VRO_DELETE_LOCAL_REF(outerClass);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}

void EventDelegate_JNI::onWeightedTriggerEvent(std::vector<TriggerEvent> &events){
    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);
    VROPlatformDispatchAsyncApplication([weakObj, events] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        VRO_OBJECT_ARRAY eventArray = VRO_NEW_OBJECT_ARRAY(events.size(), "com/viro/core/EventDelegate$TriggerEvent");
        jobject outerClass = VROPlatformConstructHostObject("com/viro/core/EventDelegate", "()V");

        for (int i = 0; i < events.size(); i ++) {
            TriggerEvent event = events[i];
            jobject jEventObj = VROPlatformConstructHostObject("com/viro/core/EventDelegate$TriggerEvent", "(Lcom/viro/core/EventDelegate;)V", outerClass);
            VROPlatformSetInt(env, jEventObj, "deviceId", event.deviceId);
            VROPlatformSetInt(env, jEventObj, "source", event.source);
            VROPlatformSetFloat(env, jEventObj, "weight", event.weight);
            VRO_ARRAY_SET(eventArray, i, jEventObj);

            pwarn("Daniel trigger Trigger JNI");
            // Clean up our local reference to jObject after.
            VRO_DELETE_LOCAL_REF(jEventObj);
        }

        // Notify the Java delegate
        VROPlatformCallHostFunction(localObj,
                                    "onWeightedTriggerEvent", "([Lcom/viro/core/EventDelegate$TriggerEvent;)V", eventArray);
        VRO_DELETE_LOCAL_REF(outerClass);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}
void EventDelegate_JNI::onMove(std::vector<MoveEvent> &events) {
    VROEventDelegate::onMove(events);
    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);
    VROPlatformDispatchAsyncApplication([weakObj, events] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        VRO_OBJECT_ARRAY eventArray = VRO_NEW_OBJECT_ARRAY(events.size(), "com/viro/core/EventDelegate$MoveEvent");
        jobject outerClass = VROPlatformConstructHostObject("com/viro/core/EventDelegate", "()V");

        for (int i = 0; i < events.size(); i ++) {
            MoveEvent event = events[i];
            jobject jEventObj = VROPlatformConstructHostObject("com/viro/core/EventDelegate$MoveEvent", "(Lcom/viro/core/EventDelegate;)V", outerClass);
            VROPlatformSetInt(env, jEventObj, "deviceId", event.deviceId);
            VROPlatformSetInt(env, jEventObj, "source", event.source);
            VRO_OBJECT jPos = VROPlatformConstructHostObject("com/viro/core/Vector", "(FFF)V",
                                                             event.pos.x, event.pos.y, event.pos.z);
            VROPlatformSetObject(env, jEventObj, "pos", "Lcom/viro/core/Vector;", jPos);

            VRO_OBJECT jRot = VROPlatformConstructHostObject("com/viro/core/Quaternion", "(FFFF)V",
                                                             event.rot.X, event.rot.Y, event.rot.Z, event.rot.W);
            VROPlatformSetObject(env, jEventObj, "rot", "Lcom/viro/core/Quaternion;", jRot);


            VRO_ARRAY_SET(eventArray, i, jEventObj);

            // Clean up our local reference to jObject after.
            VRO_DELETE_LOCAL_REF(jEventObj);
            VRO_DELETE_LOCAL_REF(jPos);
            VRO_DELETE_LOCAL_REF(jRot);
        }

        // Notify the Java delegate
        VROPlatformCallHostFunction(localObj,
                                    "onMove", "([Lcom/viro/core/EventDelegate$MoveEvent;)V", eventArray);
        VRO_DELETE_LOCAL_REF(outerClass);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}

void EventDelegate_JNI::onControllerStatus(std::vector<ControllerStat> events) {
    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);
    VROPlatformDispatchAsyncApplication([weakObj, events] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        VRO_OBJECT_ARRAY eventArray = VRO_NEW_OBJECT_ARRAY(events.size(), "com/viro/core/EventDelegate$ControllerStatus");
        jobject outerClass = VROPlatformConstructHostObject("com/viro/core/EventDelegate", "()V");

        for (int i = 0; i < events.size(); i ++) {
            ControllerStat event = events[i];
            jobject jEventObj = VROPlatformConstructHostObject("com/viro/core/EventDelegate$ControllerStatus", "(Lcom/viro/core/EventDelegate;)V", outerClass);
            VROPlatformSetInt(env, jEventObj, "deviceId", event.deviceId);
            VROPlatformSetBool(env, jEventObj, "isConnected", event.isConnected);
            VROPlatformSetBool(env, jEventObj, "is6Dof", event.is6Dof);
            VROPlatformSetInt(env, jEventObj, "batteryPercentage", event.batteryPercentage);
            VRO_ARRAY_SET(eventArray, i, jEventObj);
pwarn("Daniel JNI onControllerStatus");
            // Clean up our local reference to jObject after.
            VRO_DELETE_LOCAL_REF(jEventObj);
        }

        // Notify the Java delegate
        VROPlatformCallHostFunction(localObj,
                                    "onControllerStatus", "([Lcom/viro/core/EventDelegate$ControllerStatus;)V", eventArray);
        VRO_DELETE_LOCAL_REF(outerClass);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}

#pragma viro_api_legacy

/*
 This integer represents a value which no Node should return when getUniqueID()
 is invoked. This value is derived from the behavior of sUniqueIDGenerator in
 VRONode.h.
 */
static int sNullNodeID = -1;

void EventDelegate_JNI::onHover(int source, std::shared_ptr<VRONode> node, bool isHovering, std::vector<float> position) {
    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);

    VROPlatformDispatchAsyncApplication([weakObj, source, node, isHovering, position] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        VRO_FLOAT_ARRAY positionArray;
        if (position.size() == 3) {
            int returnLength = 3;
            positionArray = VRO_NEW_FLOAT_ARRAY(returnLength);

            VRO_FLOAT tempArr[returnLength];
            tempArr[0] = position.at(0);
            tempArr[1] = position.at(1);
            tempArr[2] = position.at(2);
            VRO_FLOAT_ARRAY_SET(positionArray, 0, 3, tempArr);
        } else {
            positionArray = nullptr;
        }

        int nodeId = node != nullptr ? node->getUniqueID() : sNullNodeID;
        VROPlatformCallHostFunction(localObj,
                                    "onHover", "(IIZ[F)V", source, nodeId, isHovering, positionArray);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}

void EventDelegate_JNI::onClick(int source, std::shared_ptr<VRONode> node, ClickState clickState, std::vector<float> position) {
    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);

    VROPlatformDispatchAsyncApplication([weakObj, source, node, clickState, position] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        VRO_FLOAT_ARRAY positionArray;
        if (position.size() == 3) {
            int returnLength = 3;
            positionArray = VRO_NEW_FLOAT_ARRAY(returnLength);
            VRO_FLOAT tempArr[returnLength];
            tempArr[0] = position.at(0);
            tempArr[1] = position.at(1);
            tempArr[2] = position.at(2);
            VRO_FLOAT_ARRAY_SET(positionArray, 0, 3, tempArr);
        } else {
            positionArray = nullptr;
        }

        int nodeId = node != nullptr ? node->getUniqueID() : sNullNodeID;
        VROPlatformCallHostFunction(localObj,
                                    "onClick", "(III[F)V", source, nodeId, clickState, positionArray);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}

void EventDelegate_JNI::onMove(int source, std::shared_ptr<VRONode> node, VROVector3f rotation, VROVector3f position, VROVector3f forwardVec) {
    //No-op
}

void EventDelegate_JNI::onCameraTransformUpdate(VROVector3f position, VROVector3f rotation, VROVector3f forward, VROVector3f up) {
    VRO_ENV env = VROPlatformGetJNIEnv();
    VRO_WEAK weakObj = VRO_NEW_WEAK_GLOBAL_REF(_javaObject);

    VROPlatformDispatchAsyncApplication([weakObj, position, rotation, forward, up] {
        VRO_ENV env = VROPlatformGetJNIEnv();
        VRO_OBJECT localObj = VRO_NEW_LOCAL_REF(weakObj);
        if (VRO_IS_OBJECT_NULL(localObj)) {
            return;
        }

        VROPlatformCallHostFunction(localObj, "onCameraTransformUpdate", "(FFFFFFFFFFFF)V",
                                    position.x, position.y, position.z, rotation.x, rotation.y, rotation.z,
                                    forward.x, forward.y, forward.z, up.x, up.y, up.z);
        VRO_DELETE_LOCAL_REF(localObj);
        VRO_DELETE_WEAK_GLOBAL_REF(weakObj);
    });
}