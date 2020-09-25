//
//  VROInputPresenterCardboard.h
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

#ifndef VROInputPresenterOVR_H
#define VROInputPresenterOVR_H

#include <memory>
#include <string>
#include <vector>
#include <VROReticle.h>
#include <VROPlatformUtil.h>
#include <VROBox.h>
#include <VROMaterial.h>
#include <VROFBXLoader.h>
#include <VROLight.h>
#include <VROOBJLoader.h>
#include "VRORenderContext.h"
#include "VROInputControllerBase.h"
#include "VROEventDelegate.h"
#include "VROInputType.h"

const std::string kTouchLeftModel = "file:///android_asset/touch_controller_left.vrx";
const std::string kTouchRightModel = "file:///android_asset/touch_controller_right.vrx";
const float kTiltedDayDreamOffset = 0.24002f;

class VROInputPresenterOVR : public VROInputPresenter {
public:
    VROInputPresenterOVR(std::shared_ptr<VRODriver> driver) {
        std::shared_ptr<VROTexture> reticleTexture
                = std::make_shared<VROTexture>(true,
                                               VROMipmapMode::Runtime,
                                               VROPlatformLoadImageFromAsset("dd_reticle_large.png", VROTextureInternalFormat::RGBA8));

        std::shared_ptr<VROReticle> leftReticle = std::make_shared<VROReticle>(reticleTexture);
        leftReticle->setPointerFixed(false);
        leftReticle->setIsHudBased(false);
        addReticle(ViroOculusInputEvent::ControllerLeftId, leftReticle);

        std::shared_ptr<VROReticle> rightReticle = std::make_shared<VROReticle>(reticleTexture);
        rightReticle->setPointerFixed(false);
        rightReticle->setIsHudBased(false);
        addReticle(ViroOculusInputEvent::ControllerRightId, rightReticle);

        _laserTexture = std::make_shared<VROTexture>(true,
                VROMipmapMode::Runtime, VROPlatformLoadImageFromAsset("ddLaserTexture.jpg", VROTextureInternalFormat::RGBA8));

        _LeftControllerBaseNode = std::make_shared<VRONode>();
        _rightControllerBaseNode = std::make_shared<VRONode>();
        _rightControllerBaseNodeAttachment = std::make_shared<VRONode>();
        _LeftControllerBaseNodeAttachment = std::make_shared<VRONode>();
        _LeftControllerBaseNode->addChildNode(_LeftControllerBaseNodeAttachment);
        _rightControllerBaseNode->addChildNode(_rightControllerBaseNodeAttachment);
        getRootNode()->addChildNode(_LeftControllerBaseNode);
        getRootNode()->addChildNode(_rightControllerBaseNode);
        initControllerModel(_LeftControllerBaseNode, driver, kTouchRightModel);
        initControllerModel(_rightControllerBaseNode, driver, kTouchLeftModel);
        getRootNode()->setIgnoreEventHandling(true);
        getRootNode()->setSelectable(false);
    }
    ~VROInputPresenterOVR() {}

     void onMove(std::vector<VROEventDelegate::MoveEvent> &events) {
        for (VROEventDelegate::MoveEvent event : events) {
            if (event.deviceId == ViroOculusInputEvent::ControllerLeftId) {
                _LeftControllerBaseNode->setWorldTransform(event.pos, event.rot, false);
            } else if (event.deviceId == ViroOculusInputEvent::ControllerRightId){
                _rightControllerBaseNode->setWorldTransform(event.pos, event.rot, false);
            }
        }

        VROInputPresenter::onMove(events);
    }

    virtual void onHover(std::vector<HoverEvent> &events) {
        for (VROEventDelegate::HoverEvent event : events) {
            if (event.deviceId == ViroOculusInputEvent::ControllerLeftId) {
                VROInputPresenter::onHoveredReticle(event.deviceId, event, _LeftControllerBaseNode->getWorldPosition());
            } else if (event.deviceId == ViroOculusInputEvent::ControllerRightId){
                VROInputPresenter::onHoveredReticle(event.deviceId, event, _rightControllerBaseNode->getWorldPosition());
            }
        }

        VROInputPresenter::onHover(events);
    }

    void setLightReceivingBitMask(int bitMask) {
        if (bitMask == -1) {
            // User wants to restore lights. Add them back if we haven't yet done so.
            if (_rightControllerBaseNode->getLights().size() == 0) {
                setLightForNode(_rightControllerBaseNode);
            }
            if (_LeftControllerBaseNode->getLights().size() == 0) {
                setLightForNode(_LeftControllerBaseNode);
            }
        } else {
            _rightControllerBaseNode->removeAllLights();
            _LeftControllerBaseNode->removeAllLights();
            _rightControllerBaseNode->setLightReceivingBitMask(bitMask, true);
            _LeftControllerBaseNode->setLightReceivingBitMask(bitMask, true);
        }
    }

    void setLightForNode(std::shared_ptr<VRONode> node) {
        std::shared_ptr<VROLight> ambientLight = std::make_shared<VROLight>(VROLightType::Ambient);
        ambientLight->setIntensity(3500);
        node->addLight(ambientLight);
        ambientLight->setInfluenceBitMask(1000);
    }

    void attachControllerChild(std::shared_ptr<VRONode> node, bool isRight) {
        if (isRight) {
            _rightControllerBaseNodeAttachment->addChildNode(node);
        } else {
            _LeftControllerBaseNodeAttachment->addChildNode(node);
        }
    }

    void detatchControllerChild(bool isRight) {
        if (isRight) {
            _rightControllerBaseNodeAttachment->removeAllChildren();
        } else {
            _LeftControllerBaseNodeAttachment->removeAllChildren();
        }
    }

private:
    std::shared_ptr<VROTexture> _laserTexture;
    std::shared_ptr<VRONode> _pointerNode;
    std::shared_ptr<VRONode> _rightControllerBaseNode;
    std::shared_ptr<VRONode> _LeftControllerBaseNode;
    std::shared_ptr<VRONode> _rightControllerBaseNodeAttachment;
    std::shared_ptr<VRONode> _LeftControllerBaseNodeAttachment;

    void initControllerModel(std::shared_ptr<VRONode> node, std::shared_ptr<VRODriver> driver, std::string modelSource) {
        setLightForNode(node);

        node->setScale({0.7, 0.7, 0.7});

        // Create our controller model
        std::shared_ptr<VRONode> controllerNod2 = std::make_shared<VRONode>();
        controllerNod2->setScale({0.015, 0.015, 0.015});
        controllerNod2->setRotationEulerY(toRadians(-180));
        VROFBXLoader::loadFBXFromResource(modelSource, VROResourceType::URL, controllerNod2, driver, [node,this](std::shared_ptr<VRONode> basenode, bool success) {
            if (!success) {
                perr("ERROR when loading controller models!");
                return;
            }
            basenode->setLightReceivingBitMask(1000, true);
            basenode->computeTransforms(node->getWorldTransform(), node->getRotation().getMatrix());
        });
        node->addChildNode(controllerNod2);

        // Create our laser obj
        std::string controllerObjAsset = VROPlatformCopyAssetToFile("ddlaser.obj");
        _pointerNode = std::make_shared<VRONode>();
        _pointerNode->setScale({0.1, 0.1, 0.1});
        VROOBJLoader::loadOBJFromResource(controllerObjAsset, VROResourceType::LocalFile, _pointerNode, driver, [this](std::shared_ptr<VRONode> node, bool success) {
            if (!success) {
                perr("ERROR when loading controller obj!");
                return;
            }

            // Set the lighting on this material to be constant
            const std::shared_ptr<VROMaterial> &material = node->getGeometry()->getMaterials().front();
            material->getDiffuse().setTexture(_laserTexture);
            material->setReceivesShadows(false);
        });

        // We'll need to rotate the line upwords (Since we are using a tilted daydream pointer cone)
        VROQuaternion offsetRot = _pointerNode->getRotation();
        offsetRot = offsetRot.fromAngleAxis(kTiltedDayDreamOffset, VROVector3f(1, 0,0));
        _pointerNode->setPosition(controllerNod2->getPosition());
        _pointerNode->setRotation(offsetRot);
        _pointerNode->setOpacity(1.0);
        _pointerNode->setSelectable(false);
        node->addChildNode(_pointerNode);
    }
};
#endif
