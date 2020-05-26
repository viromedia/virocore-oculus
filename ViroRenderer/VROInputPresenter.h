//
//  VROInputPresenter.h
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

#ifndef VROInputPresenter_H
#define VROInputPresenter_H

#include <memory>
#include "VROAtomic.h"
#include "VROEventDelegate.h"
#include "VRORenderContext.h"
#include "VROReticle.h"
#include "VRONode.h"
#include "VROMath.h"
#include "VROInputType.h"
#include "VROThreadRestricted.h"

static const float kReticleSizeMultiple = 4;
static const bool kDebugSceneBackgroundDistance = false;

/*
 VROInputPresenter contains all UI view implementations to be displayed for a given
 VROInputController.
 */
class VROInputPresenter : public VROEventDelegate, public VROThreadRestricted {
public:
    
    VROInputPresenter() : VROThreadRestricted(VROThreadName::Renderer) {
        _rootNode = std::make_shared<VRONode>();
        _camera = nullptr;
    }

    ~VROInputPresenter() {}

    std::shared_ptr<VRONode> getRootNode(){
        return _rootNode;
    }

    void setEventDelegate(std::shared_ptr<VROEventDelegate> delegate){
        _eventDelegateWeak = delegate;
    }

    virtual void onButtonEvent(std::vector<ButtonEvent> events) {
        passert_thread(__func__);
        
        std::shared_ptr<VROEventDelegate> delegate = getDelegate();
        if (delegate != nullptr && delegate->isEventEnabled(VROEventDelegate::EventAction::OnClick)){
            delegate->onButtonEvent(events);
        }
    }

    virtual void onMove(std::vector<MoveEvent> events) {
        passert_thread(__func__);
       // _lastKnownForward = forwardVec;

        std::shared_ptr<VROEventDelegate> delegate = getDelegate();
        if (delegate != nullptr && delegate->isEventEnabled(VROEventDelegate::EventAction::OnMove)){
            delegate->onMove(events);
        }
    }
    
    virtual void onHover(std::vector<HoverEvent> &events) {
        passert_thread(__func__);

        std::shared_ptr<VROEventDelegate> delegate = getDelegate();
        if (delegate != nullptr && delegate->isEventEnabled(VROEventDelegate::EventAction::OnHover)){
            delegate->onHover(events);
        }
    }

    std::map<int, std::shared_ptr<VROReticle>> getReticles() {
        return _reticles;
    }

    void addReticle(int id, std::shared_ptr<VROReticle> reticle){
        _reticles[id] = reticle;
        _reticleInitialPositionSet = false;
    }

    VROVector3f getLastKnownForward() {
        return _lastKnownForward;
    }

    void updateLastKnownForward(VROVector3f forward) {
        _lastKnownForward = forward;
    }

    void updateCamera(const VROCamera &camera) {
        _camera = std::make_shared<VROCamera>(camera);
    }

protected:
    std::shared_ptr<VROCamera> _camera;
    std::shared_ptr<VRONode> _rootNode;

    void onHoveredReticle(int reticleId, HoverEvent event, VROVector3f controllerPos) {
        passert_thread(__func__);
        if (_reticles[reticleId] == nullptr) {
            return;
        }

        float depth = -(event.intersecPos.distance(controllerPos));
        if (!_reticles[reticleId]->isHeadlocked() && _camera != nullptr) {
            _reticles[reticleId]->setPosition(event.intersecPos);

            float worldPerScreen = _camera->getWorldPerScreen(depth);
            float radius = fabs(worldPerScreen) * kReticleSizeMultiple;
            radius = VROMathClamp(radius, 0.005, 0.05);
            _reticles[reticleId]->setRadius(radius);
        }
      //  else {
            // Lock the Reticle's position to the center of the screen
            // for fixed pointer mode (usually Cardboard). The reticle is
            // rendered as HUD object, with view matrix identity (e.g. it
            // always follows the headset)

            // Set the 'depth' of the reticle to the object it is hovering
            // over, then set the radius to compensate for that distance so
            // that the reticle stays the same size. The depth effectively
            // determines the difference in reticle position between the two
            // eyes.

            // Only use the background depth if this is our first time
            // positioning the reticle. Otherwise we maintain the current
            // reticle depth, to avoid reticle 'popping' that occurs when
            // the user moves from an actual focused object to the background.
            // The background has no 'actual' depth so this is ok.
         //   if (!_reticleInitialPositionSet || !hit.isBackgroundHit() || kDebugSceneBackgroundDistance) {
         //       _reticle->setPosition(VROVector3f(0, 0, depth));
               // _reticleInitialPositionSet = true;

             //   float worldPerScreen = hit.getCamera().getWorldPerScreen(depth);
                //float radius = fabs(worldPerScreen) * kReticleSizeMultiple;
                //_reticle->setRadius(radius);
         //   }
        //}
    }

private:

    std::weak_ptr<VROEventDelegate> _eventDelegateWeak;

    std::map<int, std::shared_ptr<VROReticle>> _reticles;
    bool _reticleInitialPositionSet;
    VROAtomic<VROVector3f> _lastKnownForward;

    /*
     Event delegate for triggering calls back to Controller_JNI.
     */
    std::shared_ptr<VROEventDelegate> getDelegate(){
        if (_eventDelegateWeak.expired()){
            return nullptr;
        }
        return _eventDelegateWeak.lock();
    }
    
};
#endif
