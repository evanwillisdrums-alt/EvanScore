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
import QtQuick

import Muse.Ui
import Muse.UiComponents
import MuseScore.NotationScene

Column {
    id: root

    property var padModel: null

    property int panelMode: -1
    property bool compactStrip: false
    property bool useNotationPreview: false
    property alias notationPreviewNumStaffLines: notationPreview.numStaffLines
    property color notationPreviewBackgroundColor: "transparent"

    property alias footerHeight: footerArea.height

    property bool padSwapActive: false

    property real cornerRadius: 0

    function openContextMenu(pos) {
        if (!root.padModel) {
            return;
        }

        if (!pos) {
            pos = menuLoader.parent.mapFromItem(root, 0, root.height);
        }

        menuLoader.show(pos, root.padModel.contextMenuItems);
    }

    QtObject {
        id: prv

        readonly property var currentColor: root.compactStrip ? ui.theme.accentColor : root.useNotationPreview ? root.notationPreviewBackgroundColor : ui.theme.accentColor

        readonly property var currentOpacityNormal: root.compactStrip ? 0 : root.useNotationPreview ? 0.9 : ui.theme.buttonOpacityNormal
        readonly property var currentOpacityHover: root.compactStrip ? 0.12 : root.useNotationPreview ? 0.7 : ui.theme.buttonOpacityHover
        readonly property var currentOpacityHit: root.compactStrip ? 0.25 : root.useNotationPreview ? 1.0 : ui.theme.buttonOpacityHit
    }

    Item {
        id: mainContentArea

        width: parent.width
        height: parent.height - separator.height - footerArea.height

        MouseArea {
            id: mouseArea
            anchors.fill: parent

            enabled: mainContentArea.enabled
            hoverEnabled: true

            acceptedButtons: Qt.LeftButton | Qt.RightButton

            onPressed: function (event) {
                ui.tooltip.hide(root);

                if (!Boolean(root.padModel)) {
                    return;
                }

                if (event.button === Qt.RightButton) {
                    let pos = menuLoader.parent.mapFromItem(mouseArea, event.x, event.y);
                    root.openContextMenu(pos);
                    return;
                }

                root.padModel.triggerPad(event.modifiers);
            }

            onContainsMouseChanged: {
                if (!Boolean(root.padModel)) {
                    ui.tooltip.hide(root);
                    return;
                }

                if (mouseArea.containsMouse && root.useNotationPreview) {
                    ui.tooltip.show(root, root.padModel.padName);
                } else {
                    ui.tooltip.hide(root);
                }
            }
        }

        RoundedRectangle {
            id: padBackground

            color: Utils.colorWithAlpha(prv.currentColor, prv.currentOpacityNormal)

            anchors.fill: parent
            topLeftRadius: root.cornerRadius
            topRightRadius: root.cornerRadius
        }

        StyledTextLabel {
            id: padNameLabel

            visible: !root.useNotationPreview

            anchors.centerIn: parent
            width: parent.width - 12

            wrapMode: Text.WordWrap
            maximumLineCount: 4
            font: ui.theme.bodyBoldFont

            text: Boolean(root.padModel) ? root.padModel.padName : ""
        }

        PaintedEngravingItem {
            id: notationPreview

            visible: root.useNotationPreview

            anchors.fill: parent

            engravingItem: Boolean(root.padModel) ? root.padModel.notationPreviewItem : null
            spatium: root.compactStrip ? Math.min(4.5, mainContentArea.width / 6) : 6.25

            opacity: 0.9
        }

        states: [
            State {
                name: "MOUSE_HOVERED"
                when: mouseArea.containsMouse && !mouseArea.pressed && !root.padSwapActive
                PropertyChanges {
                    target: padBackground
                    color: Utils.colorWithAlpha(prv.currentColor, prv.currentOpacityHover)
                }
            },
            State {
                name: "MOUSE_HIT"
                when: mouseArea.pressed || root.padSwapActive
                PropertyChanges {
                    target: padBackground
                    color: Utils.colorWithAlpha(prv.currentColor, prv.currentOpacityHit)
                }
            }
        ]
    }

    Rectangle {
        id: separator

        width: parent.width
        height: root.compactStrip ? 0 : 1

        color: root.useNotationPreview ? ui.theme.strokeColor : ui.theme.accentColor
    }

    RoundedRectangle {
        id: footerArea

        width: parent.width
        bottomLeftRadius: root.cornerRadius
        bottomRightRadius: root.cornerRadius

        color: root.compactStrip ? "transparent" : Utils.colorWithAlpha(ui.theme.buttonColor, ui.theme.buttonOpacityNormal)

        MouseArea {
            id: footerMouseArea

            anchors.fill: parent
            enabled: root.panelMode !== PanelMode.EDIT_LAYOUT
            hoverEnabled: true

            acceptedButtons: Qt.LeftButton | Qt.RightButton

            onPressed: function (event) {
                let pos = menuLoader.parent.mapFromItem(footerMouseArea, event.x, event.y);
                root.openContextMenu(pos);
            }
        }

        StyledTextLabel {
            id: shortcutLabel

            anchors.verticalCenter: parent.verticalCenter
            anchors.left: root.compactStrip ? undefined : parent.left
            anchors.horizontalCenter: root.compactStrip ? parent.horizontalCenter : undefined
            anchors.margins: root.compactStrip ? 0 : 6
            width: root.compactStrip ? parent.width : implicitWidth

            font: ui.theme.bodyFont
            font.pixelSize: root.compactStrip ? 9 : ui.theme.bodyFont.pixelSize
            color: root.compactStrip ? "#53565a" : ui.theme.fontPrimaryColor

            text: Boolean(root.padModel) ? root.padModel.keyboardShortcut : ""
        }

        StyledIconLabel {
            id: midiNoteIcon
            visible: !root.compactStrip

            anchors.verticalCenter: parent.verticalCenter
            anchors.right: midiNoteLabel.left

            color: ui.theme.fontPrimaryColor

            iconCode: IconCode.SINGLE_NOTE
        }

        StyledTextLabel {
            id: midiNoteLabel
            visible: !root.compactStrip

            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.margins: 6

            font: ui.theme.bodyFont
            color: ui.theme.fontPrimaryColor

            text: Boolean(root.padModel) ? root.padModel.midiNote : ""
        }
    }

    ContextMenuLoader {
        id: menuLoader

        onHandleMenuItem: function (itemId) {
            root.padModel.handleMenuItem(itemId);
        }

        states: [
            State {
                name: "MOUSE_HOVERED"
                when: footerMouseArea.containsMouse && !footerMouseArea.pressed
                PropertyChanges {
                    target: footerArea
                    color: Utils.colorWithAlpha(root.compactStrip ? ui.theme.accentColor : ui.theme.buttonColor, root.compactStrip ? 0.12 : ui.theme.buttonOpacityHover)
                }
            },
            State {
                name: "MOUSE_HIT"
                when: footerMouseArea.pressed
                PropertyChanges {
                    target: footerArea
                    color: Utils.colorWithAlpha(root.compactStrip ? ui.theme.accentColor : ui.theme.buttonColor, root.compactStrip ? 0.25 : ui.theme.buttonOpacityHit)
                }
            }
        ]
    }
}
