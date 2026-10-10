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
import Muse.Ui
import Muse.UiComponents
import MuseScore.AppShell

Item {
    id: root
    width: controls.width
    height: 28
    property alias navigation: navPanel
    property alias currentUri: toolBarModel.currentUri
    signal selected(string uri)
    function select(uri) {
        root.selected(uri);
    }
    function focusOnFirst() {
        controls.children[0].navigation.requestActive();
    }

    MainToolBarModel {
        id: toolBarModel
    }
    Component.onCompleted: toolBarModel.load()
    NavigationPanel {
        id: navPanel
        name: "MainToolBar"
        enabled: root.enabled && root.visible
        accessible.name: qsTrc("appshell", "Main toolbar") + " " + navPanel.directionInfo
    }
    Row {
        id: controls
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2
        FlatButton {
            objectName: "workspace-home"
            width: 28
            height: 28
            icon: IconCode.GRID
            transparent: !accentButton
            accentButton: root.currentUri === "musescore://home"
            backgroundRadius: 5
            toolTipTitle: qsTrc("appshell", "Home")
            toolTipDescription: qsTrc("appshell", "Recent scores, New and Open")
            navigation.panel: navPanel
            navigation.order: 0
            onClicked: root.selected("musescore://home")
        }
        Repeater {
            model: toolBarModel
            FlatButton {
                required property string title
                required property string uri
                required property bool isChecked
                required property int index
                visible: index === 1
                width: visible ? 28 : 0
                height: 28
                icon: IconCode.MUSIC_NOTES
                accentButton: isChecked
                transparent: !accentButton
                backgroundRadius: 5
                toolTipTitle: title
                navigation.panel: navPanel
                navigation.order: 1
                onClicked: root.selected(uri)
            }
        }
        Item {
            width: 8
            height: 28
            Rectangle {
                anchors.centerIn: parent
                width: 1
                height: 18
                color: Utils.colorWithAlpha(ui.theme.strokeColor, 0.55)
            }
        }
        Repeater {
            model: [
                {
                    action: "file-new",
                    title: qsTrc("appshell", "New score"),
                    icon: IconCode.NEW_FILE,
                    shortcut: "Ctrl+N"
                },
                {
                    action: "file-open",
                    title: qsTrc("appshell", "Open score"),
                    icon: IconCode.OPEN_FILE,
                    shortcut: "Ctrl+O"
                },
                {
                    action: "file-save",
                    title: qsTrc("appshell", "Save score"),
                    icon: IconCode.SAVE,
                    shortcut: "Ctrl+S"
                }
            ]
            FlatButton {
                required property var modelData
                required property int index
                objectName: "workspace-" + modelData.action
                width: 28
                height: 28
                transparent: true
                backgroundRadius: 5
                enabled: modelData.action !== "file-save" || toolBarModel.hasProject
                icon: modelData.icon
                toolTipTitle: modelData.title
                toolTipShortcut: modelData.shortcut
                navigation.panel: navPanel
                navigation.name: modelData.action
                navigation.order: index + 2
                onClicked: toolBarModel.fileAction(modelData.action)
            }
        }
    }
}
