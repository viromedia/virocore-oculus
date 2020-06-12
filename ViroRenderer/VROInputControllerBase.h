//
//  VROInputControllerBase.h
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

#ifndef VROInputControllerBase_h
#define VROInputControllerBase_h

#include <stdio.h>
#include <vector>
#include <string>
#include <memory>
#include <set>
#include <float.h>
#include "VROInputPresenter.h"
#include "VROScene.h"
#include "VRORenderContext.h"
#include "VROEventDelegate.h"
#include "VROHitTestResult.h"
#include "VRONode.h"
#include "VROGeometry.h"
#include "VROInputType.h"

static float kSceneBackgroundDistance = 8;

/*
 Responsible for mapping generalized input data from a controller, to a unified
 set of VROEventDelegate.EventTypes. It then notifies corresponding VROEventDelegates
 in the current scene about the event type that has been triggered.
 
 For example, VROInputControllerDaydream maps the onTouchPadClick event onto
 a Viro onPrimaryClick event type within VROInputControllerBase, of which then notifies all
 VROEventDelegates about such an event.
 */
class VROInputControllerBase {
public:
    VROInputControllerBase(std::shared_ptr<VRODriver> driver);
    virtual ~VROInputControllerBase(){}

    /*
     Called when the renderer is about to be backgrounded within Android's lifecycle.
     */
    virtual void onPause() {
        // No-op
    }

    /*
     Called when the renderer is about to be foregrounded within Android's lifecycle.
     */
    virtual void onResume() {
        // No-op
    }

    void attachScene(std::shared_ptr<VROScene> scene) {
        _scene = scene;
    }
    void detachScene() {
        _scene = nullptr;
    }

    /*
     Get the presenter, creating it if it does not yet exist. Must be invoked on the
     rendering thread.
     */
    std::shared_ptr<VROInputPresenter> getPresenter() {
        std::shared_ptr<VRODriver> driver = _driver.lock();
        if (!_controllerPresenter && driver) {
            _controllerPresenter = createPresenter(driver);
            registerEventDelegate(_controllerPresenter);
        }
        return _controllerPresenter;
    }

    virtual std::string getHeadset() = 0;
    virtual std::string getController() = 0;

   /*
    For notifying components outside the scene tree, we specifically register
    them here to be tracked by the VROEventManager. Calling registerEventDelegate
    twice with the same delegate will only have callbacks be triggered once.
    */
    void registerEventDelegate(std::shared_ptr<VROEventDelegate> delegate){
        _delegates.insert(delegate);
    }
    void removeEventDelegate(std::shared_ptr<VROEventDelegate> delegate){
        _delegates.erase(delegate);
    }

protected:

    /*
     onProcess is to be implemented by derived classes to drive the processing
     of platform-specific input events and map them to viro-specific input events.
     */
    virtual void onProcess(const VROCamera &camera) {
        //No-op
    }

    void onButtonEvent(std::vector<VROEventDelegate::ButtonEvent> &events);
    void onMove(std::vector<VROEventDelegate::MoveEvent> &events, bool shouldUpdateHitTests);
    void onHover(std::vector<VROEventDelegate::MoveEvent> &events);

    /*
     Below are Viro-specific input events to be trigged by derived Input Controller
     classes; these are the Viro-sepcific events that platform-specific events
     are mapped to.
     */


    /*
     Function that attempts to notify a delegate on the Scene of the current camera transform.
     */
    void notifyCameraTransform(const VROCamera &camera);

protected:
    
    virtual std::shared_ptr<VROInputPresenter> createPresenter(std::shared_ptr<VRODriver> driver) {
        perror("Error: Derived class should create a presenter for BaseInputController to consume!");
        return nullptr;
    }

    /*
     Update the hit node, performing an intersection from the camera's position
     toward the given direction.
     */
    void updateHitNode(const VROCamera &camera, VROVector3f origin, VROVector3f ray);

    /*
     Event state for tracking business logic that is determined across multiple
     input events fed into the VROInputControllerBase (like Click and Hover).
     */
    struct EventState {

        /*
         Last result that was returned from the hit test.
         */
        std::shared_ptr<VROHitTestResult> hitResult;


        /*
         Tracked Events for state
         */
        std::shared_ptr<VROEventDelegate::ButtonEvent> lastTrackedButtonEvent;
        std::shared_ptr<VROEventDelegate::HoverEvent> lastTrackedHoverEvent;
        std::shared_ptr<VRONode> lastTrackedOnHoveredNode;

    } TouchControllerLeft, TouchControllerRight;

    /*
     Event states are mapped to a deviceId input.
     */
    std::map<int, EventState> _deviceControllerState;

    /*
     UI presenter for this input controller.
     */
    std::shared_ptr<VROInputPresenter> _controllerPresenter;

    /*
     Delegates registered within the manager to be notified of events
     to an element that is outside the scene tree.
     */
    std::set<std::shared_ptr<VROEventDelegate>> _delegates;

    /*
     Returns the hit test result for the closest node that was hit.
     */
    VROHitTestResult hitTest(VROVector3f origin, VROVector3f ray, bool boundsOnly);

    std::shared_ptr<VROScene> _scene;
private:

    /*
     Returns the first node that is able to handle the event action by bubbling it up.
     If nothing is able to handle the event, nullptr is returned.
     */
    std::shared_ptr<VRONode> getNodeToHandleEvent(VROEventDelegate::EventAction action,
                                                  std::shared_ptr<VRONode> startingNode);

    /*
     Weak pointer to the driver.
     */
    std::weak_ptr<VRODriver> _driver;
};

#endif
