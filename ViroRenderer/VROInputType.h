//
//  VROControllerInputType.h
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

#ifndef VROInputType_h
#define VROInputType_h

#include <stdio.h>
#include <vector>
#include <string>
#include <memory>
#include <set>
#include <float.h>

/**
 * Header file containing sets of DeviceIds representing the device used per platform
 * and the InputSource representing the inputs triggered on those devices.
 */
namespace ViroOculusInputEvent {
    const int ControllerRightId = 536870914;
    const int ControllerLeftId = 536870915;
    enum InputSource {
        None                = 0,
        Button_A            = 1,
        Button_B            = 2,
        Trigger_Index       = 3,
        Trigger_Hand        = 4,
        Thumbstick          = 5,
    };
}
#endif
