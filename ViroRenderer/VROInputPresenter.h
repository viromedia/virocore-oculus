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
class VROInputControllerBase;
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

    virtual void onButtonEvent(std::vector<ButtonEvent>  &events) {
        passert_thread(__func__);
        
        std::shared_ptr<VROEventDelegate> delegate = getDelegate();
        if (delegate != nullptr && delegate->isEventEnabled(VROEventDelegate::EventAction::OnClick)){
            delegate->onButtonEvent(events);
        }
    }

    virtual void onMove(std::vector<MoveEvent> &events) {
        passert_thread(__func__);

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

    virtual void onThumbStickEvent(std::vector<ThumbStickEvent> &events) {
        passert_thread(__func__);

        std::shared_ptr<VROEventDelegate> delegate = getDelegate();
        if (delegate != nullptr && delegate->isEventEnabled(VROEventDelegate::EventAction::OnThumbStick)){
            delegate->onThumbStickEvent(events);
        }
    }

    virtual void onWeightedTriggerEvent(std::vector<TriggerEvent> &events) {
        passert_thread(__func__);

        std::shared_ptr<VROEventDelegate> delegate = getDelegate();
        if (delegate != nullptr && delegate->isEventEnabled(VROEventDelegate::EventAction::OnTrigger)){
            delegate->onWeightedTriggerEvent(events);
        }
    }

    virtual void onControllerStatus(std::vector<ControllerStat> events) {
        passert_thread(__func__);

        std::shared_ptr<VROEventDelegate> delegate = getDelegate();
        if (delegate != nullptr && delegate->isEventEnabled(VROEventDelegate::EventAction::OnControllerStatus)){
            delegate->onControllerStatus(events);
        }
    }

    std::map<int, std::shared_ptr<VROReticle>> getReticles() {
        return _reticles;
    }

    void addReticle(int id, std::shared_ptr<VROReticle> reticle){
        _reticles[id] = reticle;
        _reticleInitialPositionSet = false;

        if (!reticle->isHudBased()) {
            _rootNode->addChildNode(reticle->getBaseNode());
        }
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

    void setStickyReticleDepth(bool sticky) {
        _reticleStickyDepth = sticky;
    }

    void setForcedRender(bool forced) {
        _forcedRender = forced;
    }

    bool getForcedRender() {
        return _forcedRender;
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
        VROVector3f recticlePosition = event.intersecPos;

        // **Experimental**
        // For reticle behavior that can be occluded and is in the world.
        if (!_reticles[reticleId]->isHudBased() && !_reticleStickyDepth) {
            if (event.isBackgroundHit) {
                VROVector3f proj = (event.intersecPos - controllerPos).normalize();
                recticlePosition = controllerPos + (proj.scale(16));
                depth = 16;
            } else {
                float posDepth = event.intersecPos.distance(controllerPos);
                posDepth = VROMathClamp(posDepth, 0.5, INFINITY);
                depth = -(posDepth);
            }

            float worldPerScreen = _camera->getWorldPerScreen(depth);
            float radius = fabs(worldPerScreen) * kReticleSizeMultiple;
            _reticles[reticleId]->setRadius(radius);

            if (!_reticles[reticleId]->isHeadlocked() && _camera != nullptr) {
                _reticles[reticleId]->setPosition(recticlePosition);
            }

        } else if (!_reticles[reticleId]->isHudBased()) {
            float posDepth = event.intersecPos.distance(controllerPos);
            posDepth = VROMathClamp(posDepth, 0.5, INFINITY);
            depth = -(posDepth);

            if (!event.isBackgroundHit || kDebugSceneBackgroundDistance) {
                _reticles[reticleId]->setLastKnownDistance(-depth);
            }

            float lastKnownDistance = _reticles[reticleId]->getLastKnownDistance();
            if (!_reticles[reticleId]->isHeadlocked() && _camera != nullptr) {
                VROVector3f proj = (event.intersecPos - controllerPos).normalize();
                VROVector3f finalPos = controllerPos + proj.scale(lastKnownDistance);
                _reticles[reticleId]->setPosition(finalPos);
            }

            float worldPerScreen = _camera->getWorldPerScreen(lastKnownDistance);
            float radius = fabs(worldPerScreen) * kReticleSizeMultiple;
            _reticles[reticleId]->setRadius(radius);
        } else if (!_reticles[reticleId]->isHeadlocked()) {
            depth = VROMathClamp(depth, 1.0, INFINITY);
            float worldPerScreen = _camera->getWorldPerScreen(depth);
            float radius = fabs(worldPerScreen) * kReticleSizeMultiple;
            _reticles[reticleId]->setRadius(radius);
            _reticles[reticleId]->setPosition(recticlePosition);
        } else {
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
            if (!_reticleInitialPositionSet || !event.isBackgroundHit || kDebugSceneBackgroundDistance) {
                _reticles[reticleId]->setPosition(VROVector3f(0, 0, depth));
                _reticleInitialPositionSet = true;

                float worldPerScreen = _camera->getWorldPerScreen(depth);
                float radius = fabs(worldPerScreen) * kReticleSizeMultiple;
                _reticles[reticleId]->setRadius(radius);
            }
        }
    }

private:
    std::weak_ptr<VROEventDelegate> _eventDelegateWeak;
    std::map<int, std::shared_ptr<VROReticle>> _reticles;
    bool _reticleInitialPositionSet;
    VROAtomic<VROVector3f> _lastKnownForward;
    bool _reticleStickyDepth = true;

    /*
     If true, the controller is re-rendered last after a complete render pass. This is
     a mitigation for ensuring that controllers are rendered irregardless of which portal
     they are in (for cases where the controllers are trasversing across portals).
     */
    bool _forcedRender = false;

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
