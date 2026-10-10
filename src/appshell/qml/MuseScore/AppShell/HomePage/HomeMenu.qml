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
import QtQuick.Layouts
import Muse.Ui
import Muse.UiComponents

Item {
    id: root
    property string currentPageName: ""
    property bool iconsOnly: false
    signal selected(string name)
    NavigationSection {
        id: navSec
        name: "HomeMenuSection"
        enabled: root.enabled && root.visible
        order: 2
    }
    NavigationPanel {
        id: navPanel
        name: "HomeMenuPanel"
        enabled: root.enabled && root.visible
        section: navSec
        order: 1
        direction: NavigationPanel.Vertical
        accessible.name: qsTrc("appshell", "Home menu") + " " + navPanel.directionInfo
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 6
        Column {
            Layout.fillWidth: true
            Layout.topMargin: 18
            Layout.bottomMargin: 20
            spacing: 6
            visible: !root.iconsOnly
            StyledTextLabel {
                text: "EvanScore"
                font: ui.theme.headerBoldFont
                horizontalAlignment: Text.AlignLeft
            }
            StyledTextLabel {
                text: qsTrc("appshell", "Score workspace")
                color: ui.theme.fontSecondaryColor
                horizontalAlignment: Text.AlignLeft
            }
        }
        Repeater {
            model: [
                {
                    name: "scores",
                    title: qsTrc("appshell", "Scores"),
                    icon: IconCode.MUSIC_NOTES
                },
                {
                    name: "extensions",
                    title: qsTrc("appshell", "Plugins"),
                    icon: IconCode.PLUGIN
                },
                {
                    name: "musesounds",
                    title: qsTrc("appshell", "Sounds"),
                    icon: IconCode.PLAY
                }
            ]
            FlatButton {
                required property var modelData
                required property int index
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                icon: modelData.icon
                text: root.iconsOnly ? "" : modelData.title
                orientation: Qt.Horizontal
                transparent: !accentButton
                accentButton: root.currentPageName === modelData.name
                backgroundRadius: 6
                toolTipTitle: modelData.title
                navigation.panel: navPanel
                navigation.order: index
                onClicked: root.selected(modelData.name)
            }
        }
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        SeparatorLine {
            Layout.fillWidth: true
        }
        FlatButton {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            Layout.bottomMargin: 4
            icon: IconCode.ACCOUNT
            text: root.iconsOnly ? "" : qsTrc("appshell", "Account")
            orientation: Qt.Horizontal
            transparent: !accentButton
            accentButton: root.currentPageName === "account"
            backgroundRadius: 6
            toolTipTitle: qsTrc("appshell", "Account")
            navigation.panel: navPanel
            navigation.order: 4
            onClicked: root.selected("account")
        }
    }
}
