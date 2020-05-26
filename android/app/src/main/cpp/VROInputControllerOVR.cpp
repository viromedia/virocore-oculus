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

enum ClickState : int;

void VROInputControllerOVR::processInput(std::vector<ControllerSnapShot> snapShots){
    std::vector<VROEventDelegate::ButtonEvent> allButtonEvents;
    std::vector<VROEventDelegate::ThumbStickEvent> allThumbStickEvents;
    std::vector<VROEventDelegate::MoveEvent> allMoveEvents;

    for (ControllerSnapShot snapShot : snapShots) {
        int deviceID = snapShot.deviceID;
        if (_controllerSnapShots.find(deviceID) == _controllerSnapShots.end()){
            pwarn("Controller is not supported");
            continue;
        }

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
            pwarn("Daniel distance is : %f", d);
            // Process through all Controller Status data.
            allThumbStickEvents.push_back({deviceID, ViroOculusInputEvent::Thumbstick,
                                           snapShot.buttonJoyStickPressed, snapShot.joyStickAxis});
        }

        // Process through all controller positional data.
        if (!currentSnap.position.isEqual(snapShot.position) ||
            !currentSnap.rotation.equals(snapShot.rotation)) {
            allMoveEvents.push_back({deviceID, deviceID, snapShot.position, snapShot.rotation});
        }

        // Finally, save a version of the processed snapshot to cache.
        _controllerSnapShots[deviceID] = snapShot;
    }

    // First update positional transforms (used for hit testing).
    // Move Events SHOULD only be the size of the number of supported devices.
    bool shouldHitTest = allButtonEvents.size() > 0;
    if (allMoveEvents.size() > 0) {
        VROInputControllerBase::onMove(allMoveEvents, shouldHitTest);
    }

    // Finally process all button, hover and thumbstick events.
    if (allButtonEvents.size()> 0) {
       // pwarn("Daniel process onbutton event");
        VROInputControllerBase::onButtonEvent(allButtonEvents);
    }
}

void VROInputControllerOVR::handleOVRKeyEvent(int keyCode, int action){
    VROEventDelegate::ClickState state = action == AKEY_EVENT_ACTION_DOWN ?
                                         VROEventDelegate::ClickState::ClickDown :  VROEventDelegate::ClickState::ClickUp;
    if (keyCode == AKEYCODE_BACK ) {
        //    VROInputControllerBase::onButtonEvent(ViroOculus::BackButton, state);
    }
}


// TODO: Hook in Camera updates properly.
void VROInputControllerOVR::onProcess(const VROCamera &camera) {
    // Grab controller orientation
    VROQuaternion rotation = camera.getRotation();
    VROVector3f controllerForward = rotation.getMatrix().multiply(kBaseForward);

    // Perform hit test
    //VROInputControllerBase::updateHitNode(camera, camera.getPosition(), controllerForward);

    // Process orientation and update delegates
    // VROInputControllerBase::onMove(ViroOculus::InputSource::Controller, camera.getPosition(), rotation, controllerForward);
    // VROInputControllerBase::processGazeEvent(ViroOculus::InputSource::Controller);
    notifyCameraTransform(camera);
}
