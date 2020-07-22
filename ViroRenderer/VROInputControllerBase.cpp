//
//  VROInputControllerBase.cpp
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

#include "VROInputControllerBase.h"
#include "VROTime.h"
#include "VROPortal.h"

static bool sSceneBackgroundAdd = true;
static bool sHasHoverEvents = true;

VROInputControllerBase::VROInputControllerBase(std::shared_ptr<VRODriver> driver) :
    _driver(driver) {
    _scene = nullptr;
}

/**
 * OnButtonEvents are triggered on both controller-level and node-level event delegates whenever
 * button-specific events has been triggered on the underlying hardware (corresponding
 * VROInputController).
 *
 * For the case of ClickDown and ClickUp events, delegates are triggered with a unique deviceId
 * and sourceId. For the case of Clicked Events, delegates are only triggered iff the last ClickDown
 * input event matches the last ClickUp input event in terms of deviceId and sourceId. Note that
 * these are always triggered after a clickdown-clickup as a separate event.
 *
 * NOTE: It is important to know the slight difference into how delegates are triggered.
 *
 * Controller-Level Event Delegate: When triggered, contains ButtonEvents of ALL deviceIDs at
 * that point in time. This is to provide developers a single point from which to base business
 * logic that may be dependent on multiple inputs at multiple times.
 *
 * Node-Level Event Delegates: When triggered, contains the ButtonEvents coming from
 * the controller (with a unique device Id) that has focused on the node. Thus, ClickDown,
 * ClickUp, Clicked events were done intentionally for that node, from a given controller.
 */
void VROInputControllerBase::onButtonEvent(std::vector<VROEventDelegate::ButtonEvent> &events) {
    std::map<int, std::vector<VROEventDelegate::ButtonEvent>> eventsByControllerId;
    std::map<int, VROEventDelegate::ButtonEvent> clickedEvents;
    std::vector<VROEventDelegate::ButtonEvent> clickedEventsAll;

    // For each button event:
    // - populate it with the cached hit point done for this controller id
    // - if it's a clickDown event, update lastTrackedButtonEvent (used for determining clicked)
    // - if it's a clickUp event, compare with lastTrackedButtonEvent
    for (VROEventDelegate::ButtonEvent &event : events) {
        // If no required hit result is found, we can't trigger button events.
        if (_deviceControllerState.find(event.deviceId) == _deviceControllerState.end()) {
            return;
        }

        std::shared_ptr<VROHitTestResult> hitResult = _deviceControllerState[event.deviceId].hitResult;
        VROVector3f hitLoc = hitResult->getLocation();
        VROVector3f pos = {hitLoc.x, hitLoc.y, hitLoc.z};
        event.intersecPos = pos;
        event.hitResultNode = hitResult->getNode();
        eventsByControllerId[event.deviceId].push_back(event);

        // Update lastTracked Button event if needed.
        if (event.state == VROEventDelegate::ClickDown) {
            _deviceControllerState[event.deviceId].lastTrackedButtonEvent = std::make_shared<VROEventDelegate::ButtonEvent>(event);
        }

        // Finally, if it's a clickup event, determine if we have "clicked".
        // Note that deviceId, source and targeted node will have to match.
        std::shared_ptr<VROEventDelegate::ButtonEvent> lastTrackedButtonEvent
                = _deviceControllerState[event.deviceId].lastTrackedButtonEvent;
        if (lastTrackedButtonEvent != nullptr &&
            event.state == VROEventDelegate::ClickUp &&
            lastTrackedButtonEvent->deviceId == event.deviceId &&
            lastTrackedButtonEvent->source == event.source &&
            lastTrackedButtonEvent->hitResultNode == event.hitResultNode) {
            _deviceControllerState[event.deviceId].lastTrackedButtonEvent = nullptr;

            VROEventDelegate::ButtonEvent clickedEvent = {event.deviceId, event.source, VROEventDelegate::Clicked,
                                        event.intersecPos, event.hitResultNode};
            clickedEvents[event.deviceId] = clickedEvent;
            clickedEventsAll.push_back(clickedEvent);
        }
    }

    // Notify all internal delegates of ClickDown-ClickUp events.
    for (std::shared_ptr<VROEventDelegate> delegate : _delegates) {
        delegate->onButtonEvent(events);
    }

    // Now notify node-delegates of ClickDown-ClickUp events (specific to targeted deviceId)
    std::map<int, std::vector<VROEventDelegate::ButtonEvent>>::iterator it;
    for (it = eventsByControllerId.begin(); it != eventsByControllerId.end(); ++it) {
        int deviceId = it->first;
        std::vector<VROEventDelegate::ButtonEvent> events = it->second;
        std::shared_ptr<VRONode> hitNode = _deviceControllerState[deviceId].hitResult->getNode();
        std::shared_ptr<VRONode> eventNode = getNodeToHandleEvent(
                VROEventDelegate::EventAction::OnClick, hitNode);

        if (eventNode != nullptr && eventNode->getEventDelegate() != nullptr) {
            eventNode->getEventDelegate()->onButtonEvent(events);
        }
    }

    // Now process clicked events, if any. This is done apart and separately after all
    // ClickDown-ClickUp events have been triggered.
    if (clickedEvents.size() == 0) {
        return;
    }

    // Notify all internal delegates of Clicked events.
    for (std::shared_ptr<VROEventDelegate> delegate : _delegates) {
        delegate->onButtonEvent(clickedEventsAll);
    }

    // Now notify node-delegates of the Clicked Event.
    std::map<int, VROEventDelegate::ButtonEvent>::iterator it2;
    for (it2 = clickedEvents.begin(); it2 != clickedEvents.end(); ++it2) {
        int deviceId = it2->first;
        VROEventDelegate::ButtonEvent event = it2->second;

        std::shared_ptr<VRONode> hitNode = _deviceControllerState[deviceId].hitResult->getNode();
        std::shared_ptr<VRONode> eventNode = getNodeToHandleEvent(
                VROEventDelegate::EventAction::OnClick, hitNode);
        std::vector<VROEventDelegate::ButtonEvent> events = {event};

        if (eventNode != nullptr && eventNode->getEventDelegate() != nullptr) {
            eventNode->getEventDelegate()->onButtonEvent(events);
        }
    }
}

void VROInputControllerBase::onMove(std::vector<VROEventDelegate::MoveEvent> &events, bool shouldUpdatehitTests) {
    shouldUpdatehitTests = shouldUpdatehitTests || sHasHoverEvents;

    // Update all device input's transforms.
    if (shouldUpdatehitTests && _scene != nullptr) {
        for (VROEventDelegate::MoveEvent event : events) {
            VROVector3f origin = event.pos;
            VROVector3f controllerForward = event.rot.getMatrix().multiply(kBaseForward);
            std::shared_ptr<VROHitTestResult> hitResult = std::make_shared<VROHitTestResult>(hitTest(origin, controllerForward, true));
            _deviceControllerState[event.deviceId].hitResult = hitResult;
        }
    }

    // Notify delegates on positional movement.
    for (std::shared_ptr<VROEventDelegate> delegate : _delegates) {
        delegate->onMove(events);
    }

    // Process hovering based on movement
    onHover(events);
}

/**
 * Hovered Events are only tracked per device Id.
 */
void VROInputControllerBase::onHover(std::vector<VROEventDelegate::MoveEvent> &events) {
    if (!sHasHoverEvents) {
        return;
    }

    // Notify the main delegates of hover events, where the focus is "hovering" on.
    std::vector<VROEventDelegate::HoverEvent> hoverEvents;

    // Iterate each move event (there should be a single move event to a device id).
    for (VROEventDelegate::MoveEvent e : events) {
        if (_deviceControllerState[e.deviceId].hitResult == nullptr) {
            return;
        }

        std::shared_ptr<VRONode> hoveredNode = _deviceControllerState[e.deviceId].hitResult->getNode();
        VROVector3f hitPos = _deviceControllerState[e.deviceId].hitResult->getLocation();
        bool isBackgroundHit = _deviceControllerState[e.deviceId].hitResult->isBackgroundHit();

        // Based on what was last hovered, update on / off hovered nodes.
        std::shared_ptr<VROEventDelegate::HoverEvent> lastHoveredEvent = _deviceControllerState[e.deviceId].lastTrackedHoverEvent;
        std::shared_ptr<VRONode> lastHoveredNode = lastHoveredEvent == nullptr ? nullptr : lastHoveredEvent->onHoveredNode;
        VROEventDelegate::HoverEvent tracked = { e.deviceId, e.source, true, hitPos, hoveredNode, lastHoveredNode, isBackgroundHit};

        // Update our last hovered event and vecs for notification.
        _deviceControllerState[e.deviceId].lastTrackedHoverEvent = std::make_shared<VROEventDelegate::HoverEvent>(tracked);
        hoverEvents.push_back(tracked);
    }

    // Notify all internal delegates. Hover will always be through here since it's not node specific.
    for (std::shared_ptr<VROEventDelegate> delegate : _delegates) {
        delegate->onHover(hoverEvents);
    }

    // Notify node-specific delegates. Based on the corresponding node, trigger hover on / off flag.
    for (VROEventDelegate::HoverEvent event : hoverEvents) {
        std::shared_ptr<VRONode> onHoveredNode = _deviceControllerState[event.deviceId].hitResult->getNode();
        std::shared_ptr<VRONode> onHoverEventNode = getNodeToHandleEvent(VROEventDelegate::EventAction::OnHover, onHoveredNode);
        event.isHovering = true;
        std::vector<VROEventDelegate::HoverEvent> events = {event};
        if (onHoverEventNode != nullptr && onHoverEventNode->getEventDelegate() != nullptr) {
            onHoverEventNode->getEventDelegate()->onHover(events);
        }

        // Also notify the last hovered node that we have hovered 'off'.
        std::shared_ptr<VRONode> lastTrackedOnHoveredNode = _deviceControllerState[event.deviceId].lastTrackedOnHoveredNode;
        if (lastTrackedOnHoveredNode != onHoveredNode && lastTrackedOnHoveredNode != nullptr) {
            event.isHovering = false;
            std::vector<VROEventDelegate::HoverEvent> events2 = {event};
            if (lastTrackedOnHoveredNode->getEventDelegate() != nullptr) {
                lastTrackedOnHoveredNode->getEventDelegate()->onHover(events2);
            }
        }
        _deviceControllerState[event.deviceId].lastTrackedOnHoveredNode = onHoverEventNode;
    }
}

VROHitTestResult VROInputControllerBase::hitTest(VROVector3f origin, VROVector3f ray, bool boundsOnly) {
    std::vector<VROHitTestResult> results;
    std::shared_ptr<VROPortal> sceneRootNode = _scene->getRootNode();

    // Grab all the nodes that were hit
    std::vector<VROHitTestResult> nodeResults = sceneRootNode->hitTest(origin, ray, boundsOnly);
    results.insert(results.end(), nodeResults.begin(), nodeResults.end());

    // Sort and get the closest node
    std::sort(results.begin(), results.end(), [](VROHitTestResult a, VROHitTestResult b) {
        return a.getDistance() < b.getDistance();
    });

    // Return the closest hit element, if any.
    for (int i = 0; i < results.size(); i++) {
        if (!results[i].getNode()->getIgnoreEventHandling()) {
            return results[i];
        }
    }
    
    VROVector3f backgroundPosition = origin + (ray * kSceneBackgroundDistance);
    VROHitTestResult sceneBackgroundHitResult = { sceneRootNode, backgroundPosition,
                                                  kSceneBackgroundDistance, true };
    return sceneBackgroundHitResult;
}

std::shared_ptr<VRONode> VROInputControllerBase::getNodeToHandleEvent(VROEventDelegate::EventAction action,
                                                                      std::shared_ptr<VRONode> node){
    // Base condition, we are asking for the scene's root node's parent, return.
    if (node == nullptr) {
        return nullptr;
    }

    std::shared_ptr<VROEventDelegate> delegate = node->getEventDelegate();
    if (delegate != nullptr && delegate->isEventEnabled(action)) {
        return node;
    } else {
        return getNodeToHandleEvent(action, node->getParentNode());
    }
}
