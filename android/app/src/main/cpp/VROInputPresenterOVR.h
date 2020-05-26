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

class VROInputPresenterOVR : public VROInputPresenter {
public:
    VROInputPresenterOVR(std::shared_ptr<VRODriver> driver) {
        std::shared_ptr<VROTexture> reticleTexture
                = std::make_shared<VROTexture>(true,
                                               VROMipmapMode::Runtime,
                                               VROPlatformLoadImageFromAsset("dd_reticle_large.png", VROTextureInternalFormat::RGBA8));
        std::shared_ptr<VROReticle> LeftReticle = std::make_shared<VROReticle>(reticleTexture);
        LeftReticle->setPointerFixed(false);
        addReticle(ViroOculusInputEvent::ControllerLeftId, LeftReticle);

        std::shared_ptr<VROReticle> rightReticle = std::make_shared<VROReticle>(reticleTexture);
        rightReticle->setPointerFixed(false);
        addReticle(ViroOculusInputEvent::ControllerRightId, rightReticle);

        _laserTexture = std::make_shared<VROTexture>(true,
                VROMipmapMode::Runtime, VROPlatformLoadImageFromAsset("ddLaserTexture.jpg", VROTextureInternalFormat::RGBA8));

        _LeftControllerBaseNode = std::make_shared<VRONode>();
        _rightControllerBaseNode = std::make_shared<VRONode>();
        getRootNode()->addChildNode(_LeftControllerBaseNode);
        getRootNode()->addChildNode(_rightControllerBaseNode);
        initControllerModel(_LeftControllerBaseNode, driver, kTouchLeftModel);
        initControllerModel(_rightControllerBaseNode, driver, kTouchRightModel);
        getRootNode()->setIgnoreEventHandling(true);
        getRootNode()->setSelectable(false);
    }
    std::shared_ptr<VRONode> cubeNode;
    ~VROInputPresenterOVR() {}


    /*
    void attachLaserToController(std::shared_ptr<VRODriver> driver) {
        _laserTexture = std::make_shared<VROTexture>(true,
                                                     VROMipmapMode::Runtime,
                                                     VROPlatformLoadImageFromAsset("ddLaserTexture.jpg", VROTextureInternalFormat::RGBA8));
        // Create our laser obj
        std::string controllerObjAsset = VROPlatformCopyAssetToFile("ddlaser.obj");
        _pointerNode = std::make_shared<VRONode>();
        VROOBJLoader::loadOBJFromResource(controllerObjAsset, VROResourceType::LocalFile, _pointerNode, driver, [this](std::shared_ptr<VRONode> node, bool success) {
            if (!success) {
                perr("ERROR when loading controller obj!");
                return;
            }

            // Set the lighting on this material to be constant
            const std::shared_ptr<VROMaterial> &material = node->getGeometry()->getMaterials().front();
            material->setLightingModel(VROLightingModel::Constant);
            material->getDiffuse().setTexture(_laserTexture);
            material->setWritesToDepthBuffer(false);
            material->setReadsFromDepthBuffer(false);
            material->setReceivesShadows(false);
        });

        _pointerNode->setPosition(_controllerNode->getPosition());
        _pointerNode->setOpacity(0.6);
        _pointerNode->setSelectable(false);
        _elbowNode->addChildNode(_pointerNode);
    }*/

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

    // Can represent Events from different DeviceIDs, not sources
    // Can be triggered on Nodes.
    virtual void onHover(std::vector<HoverEvent> &events) {
        for (VROEventDelegate::HoverEvent event : events) {
            if (event.deviceId == ViroOculusInputEvent::ControllerLeftId) {
                VROInputPresenter::onHoveredReticle(event.deviceId, event, _LeftControllerBaseNode->getWorldPosition());
            } else if (event.deviceId == ViroOculusInputEvent::ControllerRightId){
                VROInputPresenter::onHoveredReticle(event.deviceId, event, _rightControllerBaseNode->getWorldPosition());


                cubeNode->setWorldTransform(event.intersecPos, VROQuaternion());

            }
        }

        VROInputPresenter::onHover(events);
    }

private:
    std::shared_ptr<VROTexture> _laserTexture;
    std::shared_ptr<VRONode> _pointerNode;
    std::shared_ptr<VRONode> _rightControllerBaseNode;
    std::shared_ptr<VRONode> _LeftControllerBaseNode;

    void initControllerModel(std::shared_ptr<VRONode> node, std::shared_ptr<VRODriver> driver, std::string modelSource) {
        // Create our controller model
        std::shared_ptr<VRONode> controllerNod2 = std::make_shared<VRONode>();
        controllerNod2->setScale({0.015, 0.015, 0.015});
        controllerNod2->setRotationEulerY(toRadians(-180));
        VROFBXLoader::loadFBXFromResource(modelSource, VROResourceType::URL, controllerNod2, driver, [this](std::shared_ptr<VRONode> node, bool success) {
            if (!success) {
                perr("ERROR when loading controller models!");
                return;
            }
        });

        // std::shared_ptr<VRONode> controllerNode = std::make_shared<VRONode>();
        node->addChildNode(controllerNod2);
        std::shared_ptr<VROLight> light = std::make_shared<VROLight>(VROLightType::Directional);
        light->setIntensity(2100);
        light->setDirection( { 0, -1.0, 0.0 });
        node->addLight(light);

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
            material->setLightingModel(VROLightingModel::Constant);
            material->getDiffuse().setTexture(_laserTexture);
            material->setReceivesShadows(false);
        });

        _pointerNode->setPosition(controllerNod2->getPosition());
        _pointerNode->setOpacity(1.0); // 0.6
        _pointerNode->setSelectable(false);
        node->addChildNode(_pointerNode);
    }
};
#endif
