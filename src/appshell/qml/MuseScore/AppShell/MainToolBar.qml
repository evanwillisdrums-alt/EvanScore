/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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
import QtQuick.Controls

import Muse.Ui
import Muse.UiComponents
import MuseScore.AppShell

Item {
    id: root

    width: Math.ceil(fileControls.width + 10 + radioButtonList.width)
    height: 36

    property alias navigation: navPanel

    property alias currentUri: toolBarModel.currentUri

    signal selected(string uri)

    function select(uri) {
        root.selected(uri);
    }

    function focusOnFirst() {
        var btn = radioButtonList.itemAtIndex(0) as RadioDelegate;
        if (btn) {
            btn.navigation.requestActive();
        }
    }

    MainToolBarModel {
        id: toolBarModel
    }

    Component.onCompleted: {
        toolBarModel.load();
    }

    NavigationPanel {
        id: navPanel
        name: "MainToolBar"
        enabled: root.enabled && root.visible
        accessible.name: qsTrc("appshell", "Main toolbar") + " " + navPanel.directionInfo
    }

    Row {
        id: fileControls
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2
        Repeater {
            model: [
                {action: "file-new", title: qsTrc("appshell", "New score"), icon: IconCode.NEW_FILE, shortcut: "Ctrl+N"},
                {action: "file-open", title: qsTrc("appshell", "Open score"), icon: IconCode.OPEN_FILE, shortcut: "Ctrl+O"},
                {action: "file-save", title: qsTrc("appshell", "Save score"), icon: IconCode.SAVE, shortcut: "Ctrl+S"}
            ]
            FlatButton {
                required property var modelData
                required property int index
                objectName: "workspace-" + modelData.action
                width: 32; height: 32
                transparent: true
                backgroundRadius: 9
                enabled: modelData.action !== "file-save" || toolBarModel.hasProject
                icon: modelData.icon
                toolTipTitle: modelData.title
                toolTipShortcut: modelData.shortcut
                navigation.panel: navPanel
                navigation.name: modelData.action
                navigation.order: index
                accessible.name: modelData.title
                onClicked: toolBarModel.fileAction(modelData.action)
            }
        }
    }
    RadioButtonGroup {
        id: radioButtonList
        anchors.left: fileControls.right
        anchors.leftMargin: 10
        spacing: 2

        model: toolBarModel

        width: Math.max(1, contentItem.childrenRect.width)
        height: Math.max(1, contentItem.childrenRect.height)

        delegate: RadioDelegate {
            id: tabButton
            required property bool isTitleBold
            required property bool isChecked
            required property string title
            required property string uri
            required property int index

            ButtonGroup.group: radioButtonList.radioButtonGroup

            spacing: 0
            leftPadding: 12
            rightPadding: 12

            property alias navigation: tabNavigation
            height: 32
            // A checked label can become bold without resizing its dock. A
            // synchronous dock resize during layout re-enters KDDockWidgets'
            // size signal, which throws and leaves startup unresponsive.
            width: Math.ceil(Math.max(64, tabTitleMetrics.advanceWidth + leftPadding + rightPadding))
            TextMetrics {
                id: tabTitleMetrics
                font: ui.theme.bodyBoldFont
                text: tabButton.title
            }
            indicator: Item {}
            contentItem: StyledTextLabel {
                text: tabButton.title
                font: tabButton.checked || tabButton.isTitleBold ? ui.theme.bodyBoldFont : ui.theme.bodyFont
                color: tabButton.checked ? ui.theme.accentColor : ui.theme.fontPrimaryColor
            }
            background: Rectangle {
                color: tabButton.checked ? Utils.colorWithAlpha(ui.theme.buttonColor, 0.35)
                    : tabButton.hovered ? Utils.colorWithAlpha(ui.theme.buttonColor, 0.2) : "transparent"
                radius: 9
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 16
                    height: 2
                    radius: 1
                    color: ui.theme.accentColor
                    visible: tabButton.checked || tabNavigation.highlight
                }
            }
            NavigationControl {
                id: tabNavigation
                name: title
                panel: navPanel
                order: index + 10
                enabled: root.enabled && root.visible
                accessible.role: MUAccessible.RadioButton
                accessible.name: title
                accessible.checked: tabButton.checked
                onActiveChanged: {
                    if (active)
                        tabButton.forceActiveFocus();
                }
                onTriggered: tabButton.toggled()
            }

            checked: isChecked

            onToggled: {
                tabNavigation.requestActiveByInteraction();
                root.selected(uri);
            }
        }
    }
}
