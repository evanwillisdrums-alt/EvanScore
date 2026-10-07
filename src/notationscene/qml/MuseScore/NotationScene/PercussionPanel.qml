/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2024 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents
import MuseScore.NotationScene

Item {
    id: root

    property NavigationSection navigationSection: null
    property int contentNavigationPanelOrderStart: 1

    signal resizeRequested(var newWidth, var newHeight)

    anchors.fill: parent

    property Component toolbarComponent: PercussionPanelToolBar {
        navigationSection: root.navigationSection
        navigationOrderStart: root.contentNavigationPanelOrderStart

        model: percModel

        panelWidth: root.width
    }

    function resizePanelToContentHeight() {
        var newHeight = padGrid.cellHeight + (soundTitleLabel.height * 2);
        root.resizeRequested(root.width, newHeight);
    }

    Component.onCompleted: {
        padGrid.model.init();
        root.resizePanelToContentHeight();
    }

    PercussionPanelModel {
        id: percModel

        Component.onCompleted: {
            percModel.init();
        }

        onCurrentPanelModeChanged: {
            // Cancel any active keyboard swaps when the panel mode changes
            if (padGrid.isKeyboardSwapActive) {
                padGrid.swapOriginPad = null;
                padGrid.isKeyboardSwapActive = false;
                padGrid.model.endPadSwap(-1);
            }
        }
    }

    StyledIconLabel {
        id: soundTitleIcon

        anchors.verticalCenter: soundTitleLabel.verticalCenter
        anchors.right: soundTitleLabel.left

        anchors.rightMargin: 6

        visible: percModel.enabled && !percModel.soundTitle.isEmpty

        color: ui.theme.fontPrimaryColor

        iconCode: IconCode.AUDIO
    }

    StyledTextLabel {
        id: soundTitleLabel

        anchors {
            top: root.top
            right: root.right

            bottomMargin: 8
            rightMargin: 16
        }

        visible: percModel.enabled && !percModel.soundTitle.isEmpty

        text: percModel.soundTitle
    }

    StyledFlickable {
        id: flickable

        anchors.top: soundTitleLabel.bottom
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter

        width: parent.width

        contentWidth: rowLayout.width
        contentHeight: rowLayout.height + rowLayout.anchors.topMargin

        clip: true
        flickableDirection: Flickable.HorizontalFlick
        boundsBehavior: Flickable.StopAtBounds
        StyledScrollBar.horizontal: horizontalScrollBar

        WheelHandler {
            target: null
            onWheel: function (event) {
                let delta = event.pixelDelta.x || event.pixelDelta.y || (event.angleDelta.x || event.angleDelta.y) / 3;
                flickable.contentX = Math.max(0, Math.min(flickable.contentWidth - flickable.width, flickable.contentX - delta));
                event.accepted = true;
            }
        }

        RowLayout {
            id: rowLayout

            // Keep layout-edit controls at the end of the horizontal strip.
            readonly property int sideColumnsWidth: addRowButton.width

            QtObject {
                id: navigationPrv

                // Keep navigation on the same pad when moving between its main and footer controls.
                property var currentPadNavigationIndex: [0, 0]

                function onPadNavigationEvent(event) {
                    var navigationRow = navigationPrv.currentPadNavigationIndex[0];
                    var navigationColumn = navigationPrv.currentPadNavigationIndex[1];

                    if (navigationRow >= padGrid.numRows || navigationColumn >= padGrid.numColumns) {
                        navigationPrv.currentPadNavigationIndex = [0, 0];
                    }

                    if (event.type === NavigationEvent.AboutActive) {
                        event.setData("controlIndex", navigationPrv.currentPadNavigationIndex);
                    }
                }
            }

            height: padGrid.cellHeight * padGrid.numRows
            anchors.top: parent.top
            anchors.topMargin: Math.max((flickable.height - height) / 2, 0)
            spacing: padGrid.spacing / 2

            Item {
                id: padGrid

                readonly property int numRows: 1
                readonly property int numColumns: model.numPads
                readonly property int spacing: 12

                property PercussionPanelPad swapOriginPad: null
                property bool isKeyboardSwapActive: false

                width: padRow.width
                Layout.fillHeight: true
                Layout.preferredWidth: width

                property int cellWidth: 104
                property int cellHeight: 88

                property alias model: padRepeater.model

                NavigationPanel {
                    id: padsNavPanel

                    name: "PercussionPanelPads"
                    section: root.navigationSection
                    order: root.contentNavigationPanelOrderStart + 2 // +2 for toolbar

                    onNavigationEvent: function (event) {
                        navigationPrv.onPadNavigationEvent(event);
                    }
                }

                NavigationPanel {
                    id: padFootersNavPanel

                    name: "PercussionPanelFooters"
                    section: root.navigationSection
                    order: padsNavPanel.order + 1

                    enabled: percModel.currentPanelMode !== PanelMode.EDIT_LAYOUT

                    onNavigationEvent: function (event) {
                        navigationPrv.onPadNavigationEvent(event);
                    }
                }

                Row {
                    id: padRow
                    spacing: padGrid.spacing
                    Repeater {
                        id: padRepeater
                        model: percModel.padListModel
                        delegate: Item {
                            id: padArea

                            required property PercussionPanelPadModel padModel
                            required property int index

                            visible: Boolean(padModel) || percModel.currentPanelMode === PanelMode.EDIT_LAYOUT
                            width: padGrid.cellWidth
                            height: padGrid.cellHeight

                            PercussionPanelPad {
                                id: pad

                                anchors.centerIn: parent

                                width: parent.width + pad.totalBorderWidth - padGrid.spacing
                                height: parent.height + pad.totalBorderWidth - padGrid.spacing

                                padModel: padArea.padModel
                                panelEnabled: percModel.enabled
                                panelMode: percModel.currentPanelMode
                                useNotationPreview: percModel.useNotationPreview
                                notationPreviewNumStaffLines: percModel.notationPreviewNumStaffLines
                                notationPreviewBackgroundColor: percModel.notationPreviewBackgroundColor

                                // When swapping, only show the outline for the swap origin  and the swap target...
                                showEditOutline: percModel.currentPanelMode === PanelMode.EDIT_LAYOUT && (!Boolean(padGrid.swapOriginPad) || padGrid.swapOriginPad === pad)
                                showOriginBackground: pad.containsDrag || (pad === padGrid.swapOriginPad && !padGrid.isKeyboardSwapActive)

                                panelHasActiveKeyboardSwap: padGrid.isKeyboardSwapActive
                                dragParent: root

                                navigationRow: 0
                                navigationColumn: padArea.index
                                padNavigation.panel: padsNavPanel
                                footerNavigation.panel: padFootersNavPanel

                                onStartPadSwapRequested: function (isKeyboardSwap) {
                                    padGrid.swapOriginPad = pad;
                                    padGrid.isKeyboardSwapActive = isKeyboardSwap;
                                    padGrid.model.startPadSwap(padArea.index);
                                    if (isKeyboardSwap) {
                                        pad.padNavigation.requestActive();
                                    }
                                }

                                onEndPadSwapRequested: {
                                    padGrid.swapOriginPad = null;
                                    padGrid.isKeyboardSwapActive = false;
                                    padGrid.model.endPadSwap(padArea.index);
                                }

                                onCancelPadSwapRequested: {
                                    padGrid.swapOriginPad = null;
                                    padGrid.isKeyboardSwapActive = false;
                                    padGrid.model.endPadSwap(-1);
                                }

                                onHasActiveControlChanged: {
                                    if (!pad.hasActiveControl) {
                                        return;
                                    }
                                    navigationPrv.currentPadNavigationIndex = [pad.navigationRow, pad.navigationColumn];
                                    let left = padArea.mapToItem(flickable.contentItem, 0, 0).x;
                                    if (left < flickable.contentX)
                                        flickable.contentX = left;
                                    else if (left + padArea.width > flickable.contentX + flickable.width)
                                        flickable.contentX = left + padArea.width - flickable.width;
                                }

                                Connections {
                                    target: padGrid.model

                                    function onPadFocusRequested(padIndex) {
                                        if (padArea.index !== padIndex) {
                                            return;
                                        }

                                        // Focus pad only if keyboard navigation has started
                                        if (root.navigationSection.active) {
                                            pad.padNavigation.requestActive();
                                        }
                                    }

                                    function onNumPadsChanged() {
                                        root.resizePanelToContentHeight();
                                    }
                                }
                            }

                            states: [
                                // If this is the swap target - move the swappable area to the swap origin (preview the swap)
                                State {
                                    name: "SWAP_TARGET"
                                    when: Boolean(padGrid.swapOriginPad) && (pad.containsDrag || pad.padNavigation.active) && padGrid.swapOriginPad !== pad

                                    ParentChange {
                                        target: pad.swappableArea
                                        parent: padGrid.swapOriginPad
                                    }
                                    AnchorChanges {
                                        target: pad.swappableArea
                                        anchors.verticalCenter: padGrid.swapOriginPad.verticalCenter
                                        anchors.horizontalCenter: padGrid.swapOriginPad.horizontalCenter
                                    }
                                    PropertyChanges {
                                        target: pad
                                        showEditOutline: true
                                    }

                                    // Origin background not needed for the dragged pad when a preview is taking place...
                                    PropertyChanges {
                                        target: padGrid.swapOriginPad
                                        showOriginBackground: false
                                    }

                                    // In the case of a keyboard swap, we also need to move the origin pad
                                    ParentChange {
                                        target: padGrid.isKeyboardSwapActive && Boolean(padGrid.swapOriginPad) ? padGrid.swapOriginPad.swappableArea : null
                                        parent: pad
                                    }
                                    AnchorChanges {
                                        target: padGrid.isKeyboardSwapActive && Boolean(padGrid.swapOriginPad) ? padGrid.swapOriginPad.swappableArea : null
                                        anchors.verticalCenter: pad.verticalCenter
                                        anchors.horizontalCenter: pad.horizontalCenter
                                    }
                                }
                            ]
                        }
                    }
                }
            }
            NavigationPanel {
                id: addRowButtonPanel

                name: "PercussionPanelAddRowButton"
                section: root.navigationSection
                order: padFootersNavPanel.order + 1

                enabled: addRowButton.visible
            }

            FlatButton {
                id: addRowButton

                // Display to the right of the last pad
                Layout.alignment: Qt.AlignBottom
                Layout.bottomMargin: (padGrid.cellHeight / 2) - (height / 2)

                visible: percModel.currentPanelMode === PanelMode.EDIT_LAYOUT
                enabled: !padGrid.isKeyboardSwapActive

                icon: IconCode.PLUS
                text: qsTrc("notation/percussion", "Add slots")
                orientation: Qt.Horizontal

                navigation.panel: addRowButtonPanel
                drawFocusBorderInsideRect: true

                onClicked: {
                    padGrid.model.addEmptyRow/*focusFirstInNewRow*/ (true);
                    flickable.contentX = Math.max(0, flickable.contentWidth - flickable.width);
                }
            }
        }

        StyledTextLabel {
            id: panelDisabledLabel

            visible: !percModel.enabled

            anchors.centerIn: parent
            anchors.verticalCenterOffset: rowLayout.anchors.topMargin / 2

            font: ui.theme.bodyFont
            text: qsTrc("notation/percussion", "Select an unpitched percussion staff to see available sounds")
        }
    }

    StyledScrollBar {
        id: horizontalScrollBar
        orientation: Qt.Horizontal
        width: flickable.width
        anchors.horizontalCenter: flickable.horizontalCenter
        anchors.bottom: root.bottom
    }
}
