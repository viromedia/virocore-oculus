//
//  VROInputControllerOVR.h
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

#ifndef VROInputControllerOVR_H
#define VROInputControllerOVR_H
#include <memory>
#include "VRORenderContext.h"
#include "VROInputControllerBase.h"
#include "VROInputPresenterOVR.h"
#include <android/input.h>

class VROInputControllerOVR : public VROInputControllerBase {

public:
    // Snapshot containing the last queried contoller input data.
    struct ControllerSnapShot {
        int deviceID;
        bool buttonAPressed;
        bool buttonBPressed;
        bool buttonTriggerIndexPressed;
        bool buttonTrggerHandPressed;
        float triggerIndexWeight;
        float triggerHandWeight;
        bool buttonJoyStickPressed;
        VROVector3f joyStickAxis;
        VROVector3f position;
        VROQuaternion rotation;
    };

    VROInputControllerOVR(std::shared_ptr<VRODriver> driver) : VROInputControllerBase(driver) {}
    virtual ~VROInputControllerOVR(){}

    void processInput(std::vector<ControllerSnapShot> snapshots);

    void handleOVRKeyEvent(int keyCode, int action); // TODO: Volume buttons?

    virtual std::string getHeadset() {
        return "gearvr";
    }

    virtual std::string getController() {
        return "gearvr";
    }

protected:
    std::shared_ptr<VROInputPresenter> createPresenter(std::shared_ptr<VRODriver> driver) {
        return std::make_shared<VROInputPresenterOVR>(driver);
    }

private:
    // Set default configurations
    std::map<int, ControllerSnapShot> _controllerSnapShots = {
            {536870915, {1, false, false, false, false, false, VROVector3f(), VROVector3f(), VROQuaternion(), 0.0f, 0.0f}},
            {536870914, {1, false, false, false, false, false, VROVector3f(), VROVector3f(), VROQuaternion(), 0.0f, 0.0f}},
    };
};
#endif
