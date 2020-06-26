//
//  VROSceneRendererSceneView.cpp
//  ViroRenderer
//
//  Created by Raj Advani on 11/7/2017.
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

#include "VROSceneRendererSceneView.h"

#include <android/log.h>
#include <assert.h>
#include <stdlib.h>
#include <cmath>
#include <random>
#include <VROTime.h>
#include <VROProjector.h>
#include <VROARHitTestResult.h>

#include "VRODriverOpenGLAndroid.h"
#include "VROGVRUtil.h"
#include "VRONodeCamera.h"
#include "VROMatrix4f.h"
#include "VROViewport.h"
#include "VRORenderer.h"
#include "VROSurface.h"
#include "VRONode.h"
#include "VROAllocationTracker.h"

static VROVector3f const kZeroVector = VROVector3f();

#pragma mark - Setup

VROSceneRendererSceneView::VROSceneRendererSceneView(VRORendererConfiguration config,
                                                     std::shared_ptr<gvr::AudioApi> gvrAudio,
                                                     jobject viroViewJNI) {
    perror("VROSceneRendererSceneView is Not Supported");
    _driver = std::make_shared<VRODriverOpenGLAndroid>(gvrAudio);
    // instantiate the input controller w/ viewport size (0,0) and update it later.
    _renderer = std::make_shared<VRORenderer>(config, nullptr);
}

VROSceneRendererSceneView::~VROSceneRendererSceneView() {

}

#pragma mark - Rendering

void VROSceneRendererSceneView::initGL() {

}

void VROSceneRendererSceneView::onDrawFrame() {
    renderFrame();

    ++_frame;
    ALLOCATION_TRACKER_PRINT();
}

void VROSceneRendererSceneView::renderFrame() {
    glEnable(GL_DEPTH_TEST);
    _driver->setCullMode(VROCullMode::Back);

    VROViewport viewport(0, 0, _surfaceSize.width, _surfaceSize.height);
    VROFieldOfView fov = _renderer->computeUserFieldOfView(viewport.getWidth(), viewport.getHeight());
    VROMatrix4f projection = fov.toPerspectiveProjection(kZNear, _renderer->getFarClippingPlane());

    _renderer->prepareFrame(_frame, viewport, fov, VROMatrix4f::identity(), VROVector3f(),
                            projection, _driver);
    _renderer->renderEye(VROEyeType::Monocular, _renderer->getLookAtMatrix(), projection, viewport, _driver);
    _renderer->renderHUD(VROEyeType::Monocular, VROMatrix4f::identity(), projection, _driver);
    _renderer->endFrame(_driver);
}

/*
 Update render sizes as the surface changes.
 */
void VROSceneRendererSceneView::onSurfaceChanged(jobject surface, VRO_INT width, VRO_INT height) {
    VROThreadRestricted::setThread(VROThreadName::Renderer);

    _surfaceSize.width = width;
    _surfaceSize.height = height;
}

void VROSceneRendererSceneView::onPause() {
    std::shared_ptr<VROSceneRendererSceneView> shared = shared_from_this();
    VROPlatformDispatchAsyncRenderer([shared] {
        shared->_renderer->getInputController()->onPause();
        shared->_driver->pause();
    });
}

void VROSceneRendererSceneView::onResume() {
    std::shared_ptr<VROSceneRendererSceneView> shared = shared_from_this();

    VROPlatformDispatchAsyncRenderer([shared] {
        shared->_renderer->getInputController()->onResume();
        shared->_driver->resume();
    });
}

void VROSceneRendererSceneView::setVRModeEnabled(bool enabled) {

}


