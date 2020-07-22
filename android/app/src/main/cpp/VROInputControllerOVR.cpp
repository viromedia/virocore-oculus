//
//  VROInputControllerOVR.cpp
//  ViroRenderer
//
//  Copyright © 2017 Viro Media. All rights reserved.
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

#include "VROInputControllerOVR.h"
#include <inttypes.h>

enum ClickState : int;

VROInputControllerOVR::VROInputControllerOVR(std::shared_ptr<VRODriver> driver) :
        VROInputControllerBase(driver) {
    // Set default snapshots.
    _controllerSnapShots = {
            {ViroOculusInputEvent::ControllerRightId, {ViroOculusInputEvent::ControllerRightId,
                                                       false, false, false, false, false,
                                                       0.0f, 0.0f,
                                                       VROVector3f(), VROVector3f(), VROQuaternion(),
                                                       false, 0}},
            {ViroOculusInputEvent::ControllerLeftId, {ViroOculusInputEvent::ControllerRightId,
                                                       false, false, false, false, false,
                                                       0.0f, 0.0f,
                                                       VROVector3f(), VROVector3f(), VROQuaternion(),
                                                       false, 0}}};
}

void VROInputControllerOVR::processControllerState(std::vector<ControllerSnapShot> &snapShots) {
    std::vector<VROEventDelegate::ControllerStat> allControllerStats;
    // Iterate through each deviceID and notify delegates if it had changed.
    std::map<int, ControllerSnapShot>::iterator it;
    for ( it = _controllerSnapShots.begin(); it != _controllerSnapShots.end(); it++ ) {
        VROEventDelegate::ControllerStat stat;

        // Grab the cached snapshot to compare against
        int cachedDeviceId = it->first;
        ControllerSnapShot cachedSnapShot = it->second;

        // Compare against latest data to determine if something has changed.
        int currentDeviceId = -1;
        bool snapShotIsDifferent = false;
        ControllerSnapShot currentSnap;

        // First grab the snapshot with the same ID as the currently selected cached one.
        for (ControllerSnapShot &inputSnap : snapShots) {
            if (inputSnap.deviceID == cachedDeviceId) {
                currentDeviceId = inputSnap.deviceID;
                currentSnap = inputSnap;
                break;
            }
        }

        // if the device is not a part of the latest data, it is considered disconnected.
        if (currentDeviceId == -1 && cachedSnapShot.isConnected) {
            snapShotIsDifferent = true;
            stat.deviceId = cachedDeviceId;
            stat.isConnected = false;
            stat.is6Dof = false;
            stat.batteryPercentage = 0;
        } else if (currentDeviceId != -1) {
            snapShotIsDifferent = cachedSnapShot.isConnected != true ||
                                  cachedSnapShot.trackingStatus6Dof != currentSnap.trackingStatus6Dof ||
                                  cachedSnapShot.batteryPercentage != currentSnap.batteryPercentage;

            stat.deviceId = currentSnap.deviceID;
            stat.isConnected = true;
            stat.is6Dof = currentSnap.trackingStatus6Dof;
            stat.batteryPercentage = currentSnap.batteryPercentage;
        }

        // Add to the vector to notify delegates with and update the cache.
        if (snapShotIsDifferent) {
            allControllerStats.push_back({stat});
            _controllerSnapShots[cachedDeviceId].isConnected =  stat.isConnected;
            _controllerSnapShots[cachedDeviceId].trackingStatus6Dof = stat.is6Dof;
            _controllerSnapShots[cachedDeviceId].batteryPercentage = stat.batteryPercentage;
        }
    }

    if (allControllerStats.size() <= 0) {
        return;
    }

    // Notify all delegates
    for (std::shared_ptr<VROEventDelegate> delegate : _delegates) {
        delegate->onControllerStatus(allControllerStats);
    }
}

void VROInputControllerOVR::processInput(std::vector<ControllerSnapShot> snapShots, const VROCamera &camera){
    std::vector<VROEventDelegate::ButtonEvent> allButtonEvents;
    std::vector<VROEventDelegate::ThumbStickEvent> allThumbStickEvents;
    std::vector<VROEventDelegate::MoveEvent> allMoveEvents;
    std::vector<VROEventDelegate::TriggerEvent> allWeightedTriggerEvents;
    processControllerState(snapShots);

    // Do nothing if we have no data
    if (snapShots.size() == 0) {
        return;
    }

    // Reprocess event positional data with the updated camera.
    for (ControllerSnapShot &snapShot : snapShots) {
        snapShot.position = snapShot.position + camera.getBasePosition();
    }

    for (ControllerSnapShot snapShot : snapShots) {
        int deviceID = snapShot.deviceID;
        _controllerSnapShots[deviceID].isConnected = true;
        ControllerSnapShot &currentSnap = _controllerSnapShots[deviceID];

        // Process through all the buttons, notify if changed.
        if (currentSnap.buttonAPressed != snapShot.buttonAPressed) {
            VROEventDelegate::ClickState state = snapShot.buttonAPressed ?
                                                 VROEventDelegate::ClickState::ClickDown :
                                                 VROEventDelegate::ClickState::ClickUp;
            allButtonEvents.push_back({deviceID, ViroOculusInputEvent::Button_A, state, VROVector3f(), nullptr});
        }
        if (currentSnap.buttonBPressed != snapShot.buttonBPressed) {
            VROEventDelegate::ClickState state = snapShot.buttonBPressed ?
                                                 VROEventDelegate::ClickState::ClickDown :
                                                 VROEventDelegate::ClickState::ClickUp;
            allButtonEvents.push_back({deviceID, ViroOculusInputEvent::Button_B, state, VROVector3f(), nullptr});
        }
        if (currentSnap.buttonTrggerHandPressed != snapShot.buttonTrggerHandPressed) {
            VROEventDelegate::ClickState state = snapShot.buttonTrggerHandPressed ?
                                                 VROEventDelegate::ClickState::ClickDown :
                                                 VROEventDelegate::ClickState::ClickUp;
            allButtonEvents.push_back({deviceID, ViroOculusInputEvent::Trigger_Hand, state, VROVector3f(), nullptr});
        }
        if (currentSnap.buttonTriggerIndexPressed != snapShot.buttonTriggerIndexPressed) {
            VROEventDelegate::ClickState state = snapShot.buttonTriggerIndexPressed ?
                                                 VROEventDelegate::ClickState::ClickDown :
                                                 VROEventDelegate::ClickState::ClickUp;
            allButtonEvents.push_back({deviceID, ViroOculusInputEvent::Trigger_Index, state, VROVector3f(), nullptr});
        }
        if (currentSnap.buttonJoyStickPressed != snapShot.buttonJoyStickPressed) {
            VROEventDelegate::ClickState state = snapShot.buttonJoyStickPressed ?
                                                 VROEventDelegate::ClickState::ClickDown :
                                                 VROEventDelegate::ClickState::ClickUp;
            allButtonEvents.push_back({deviceID, ViroOculusInputEvent::Thumbstick, state, VROVector3f(), nullptr});
        }

        // Process through all Joystick data.
        float d = currentSnap.joyStickAxis.distance(snapShot.joyStickAxis);
        if (currentSnap.buttonJoyStickPressed != snapShot.buttonJoyStickPressed || d > 0) {
            // Process through all Controller Status data.
            allThumbStickEvents.push_back({deviceID, ViroOculusInputEvent::Thumbstick,
                                           snapShot.buttonJoyStickPressed, snapShot.joyStickAxis});
        }

        // Group together all trigger weighted events
        if (currentSnap.triggerIndexWeight != snapShot.triggerIndexWeight) {
            allWeightedTriggerEvents.push_back({deviceID, ViroOculusInputEvent::Trigger_Index, snapShot.triggerIndexWeight});
        }

        if (currentSnap.triggerHandWeight != snapShot.triggerHandWeight) {
            allWeightedTriggerEvents.push_back({deviceID, ViroOculusInputEvent::Trigger_Index, snapShot.triggerHandWeight});
        }

        // Process through all controller positional data.
        if (!currentSnap.position.isEqual(snapShot.position) ||
            !currentSnap.rotation.equals(snapShot.rotation, 0.0001)) {
            allMoveEvents.push_back({deviceID, deviceID, snapShot.position, snapShot.rotation});
        }
        _controllerSnapShots[deviceID] = snapShot;
    }

    // First update positional transforms (used for hit testing).
    // Move Events SHOULD only be the size of the number of supported devices.
    bool shouldHitTest = allButtonEvents.size() > 0;
    if (allMoveEvents.size() > 0) {
        VROInputControllerBase::onMove(allMoveEvents, shouldHitTest);
    }

    // Finally process all button events.
    if (allButtonEvents.size()> 0) {
        VROInputControllerBase::onButtonEvent(allButtonEvents);
    }

    // Trigger Oculus specific trigger delegates here.
    if (allWeightedTriggerEvents.size() > 0) {
        for (std::shared_ptr<VROEventDelegate> delegate : _delegates) {
            delegate->onWeightedTriggerEvent(allWeightedTriggerEvents);
        }
    }

    if (allThumbStickEvents.size() > 0) {
        for (std::shared_ptr<VROEventDelegate> delegate : _delegates) {
            delegate->onThumbStickEvent(allThumbStickEvents);
        }
    }
}

void VROInputControllerOVR::handleOVRKeyEvent(int keyCode, int action){
    VROEventDelegate::ClickState state = action == AKEY_EVENT_ACTION_DOWN ?
                                         VROEventDelegate::ClickState::ClickDown :  VROEventDelegate::ClickState::ClickUp;
    if (keyCode == AKEYCODE_BACK ) {
        //    VROInputControllerBase::onButtonEvent(ViroOculus::BackButton, state);
    }
}
