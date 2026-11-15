/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2023 MuseScore Limited and others
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
import MuseScore.Playback

Item {
    id: root
    property alias navigation: navPanel
    required property MixerPanelContextMenuModel viewModel
    anchors.fill: parent
    readonly property var views: [
        { key: "mix", title: qsTrc("playback", "Mix"), help: qsTrc("playback", "Faders, pan, volume, mute and solo") },
        { key: "sounds", title: qsTrc("playback", "Sounds"), help: qsTrc("playback", "Choose sounds without changing the instrument or notation") },
        { key: "effects", title: qsTrc("playback", "Effects"), help: qsTrc("playback", "Effect inserts and sends; click a supported effect name to open its editor") },
        { key: "all", title: qsTrc("playback", "All"), help: qsTrc("playback", "Show every mixer section") }
    ]

    NavigationPanel {
        id: navPanel
        name: "MixerPanelToolbarPanel"
        enabled: root.enabled && root.visible
        direction: NavigationPanel.Horizontal
    }

    Row {
        id: viewButtons
        visible: root.width >= implicitWidth
        anchors.verticalCenter: parent.verticalCenter
        spacing: 3
        Repeater {
            model: root.views
            delegate: FlatButton {
                required property var modelData
                required property int index
                objectName: "mixer-view-" + modelData.key
                text: modelData.title
                orientation: Qt.Horizontal
                height: 26
                minWidth: 42
                margins: 9
                accentButton: root.viewModel.activeView === modelData.key
                toolTipTitle: modelData.title
                toolTipDescription: modelData.help
                navigation.panel: navPanel
                navigation.name: objectName
                navigation.column: index
                onClicked: root.viewModel.selectView(modelData.key)
            }
        }
    }
    StyledDropdown {
        objectName: "mixer-view-compact"
        visible: !viewButtons.visible
        anchors.verticalCenter: parent.verticalCenter
        width: Math.min(138, root.width)
        height: 26
        model: root.views
        textRole: "title"
        valueRole: "key"
        indeterminateText: qsTrc("playback", "Custom view")
        currentIndex: indexOfValue(root.viewModel.activeView)
        navigation.panel: navPanel
        navigation.name: objectName
        onActivated: function(index, value) { root.viewModel.selectView(value) }
    }

}
