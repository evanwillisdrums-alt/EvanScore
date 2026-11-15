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
import MuseScore.NotationScene

Rectangle {
    id: root
    property int navigationOrderStart: 0
    property int navigationOrderEnd: navigationPanel.order
    property string query: ""
    property bool commandsSelected: false
    readonly property bool commandsMode: commandsSelected || query.trim().startsWith(">")
    property int currentResult: 0
    visible: false
    implicitHeight: 50 + (commandsMode ? resultColumn.implicitHeight + 12 : 0)
    height: implicitHeight
    color: ui.theme.backgroundPrimaryColor
    signal closed()

    function show() {
        root.visible = true
        Qt.callLater(textInputField.navigation.requestActive)
    }
    function hide() {
        root.visible = false
        root.closed()
    }
    function runResult(index) {
        if (index < 0 || index >= searchModel.commandResults.length) return false
        const item = searchModel.commandResults[index]
        if (!item.enabled) return false
        root.hide()
        // Restore notation focus before invoking its normal action handler.
        Qt.callLater(function() { searchModel.runCommand(item.code) })
        return true
    }

    property NavigationPanel navigationPanel: NavigationPanel {
        name: "SearchPopup"
        enabled: root.visible
        direction: NavigationPanel.Both
        order: root.navigationOrderStart
        accessible.name: qsTrc("notation", "Find and commands")
    }
    SearchPopupModel {
        id: searchModel
        objectName: "evanscore-command-search-model"
        onCommandResultsChanged: root.currentResult = 0
    }

    RowLayout {
        id: searchRow
        height: 50
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8
        spacing: 8
        FlatButton {
            icon: IconCode.CLOSE_X_ROUNDED
            navigation.panel: root.navigationPanel
            navigation.column: 0
            navigation.accessible.name: qsTrc("notation", "Close search")
            onClicked: root.hide()
        }
        FlatButton {
            objectName: "search-commands-mode"
            text: root.commandsMode ? qsTrc("notation", "Commands") : qsTrc("notation", "Find / Go to")
            toolTipTitle: qsTrc("notation", "Switch search mode")
            toolTipDescription: qsTrc("notation", "Type > followed by a command name, or switch back to find notes and measures.")
            orientation: Qt.Horizontal
            minWidth: 92
            navigation.panel: root.navigationPanel
            navigation.column: 1
            onClicked: {
                root.commandsSelected = !root.commandsMode
                textInputField.clear()
                root.query = ""
                searchModel.search(root.commandsSelected ? "> " : "")
                Qt.callLater(textInputField.navigation.requestActive)
            }
        }
        TextInputField {
            id: textInputField
            objectName: "notation-find-command-input"
            Layout.fillWidth: true
            hint: root.commandsMode ? qsTrc("notation", "Search commands…") : qsTrc("notation", "Find in score, or > command name")
            navigation.panel: root.navigationPanel
            navigation.column: 2
            onTextChanged: function(value) {
                root.query = value
                searchModel.search(root.commandsSelected && !value.trim().startsWith(">") ? "> " + value : value)
            }
            Keys.onShortcutOverride: function(event) {
                if (root.commandsMode && (event.key === Qt.Key_Up || event.key === Qt.Key_Down)) event.accepted = true
            }
            Keys.onPressed: function(event) {
                if (!root.commandsMode || (event.key !== Qt.Key_Up && event.key !== Qt.Key_Down)) return
                const last = searchModel.commandResults.length - 1
                root.currentResult = Math.max(0, Math.min(last, root.currentResult + (event.key === Qt.Key_Down ? 1 : -1)))
                event.accepted = true
            }
            onAccepted: {
                if (root.commandsMode) {
                    if (!root.runResult(root.currentResult)) Qt.callLater(textInputField.navigation.requestActive)
                } else Qt.callLater(root.hide)
            }
            onEscaped: Qt.callLater(root.hide)
        }
    }
    Column {
        id: resultColumn
        visible: root.commandsMode
        anchors.top: searchRow.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8
        spacing: 2
        Repeater {
            model: searchModel.commandResults
            delegate: FlatButton {
                required property var modelData
                required property int index
                objectName: "command-result-" + index
                width: resultColumn.width
                height: 28
                text: modelData.title + (modelData.enabled ? "" : " · " + qsTrc("notation", "Unavailable for this selection"))
                enabled: modelData.enabled
                accentButton: root.currentResult === index
                orientation: Qt.Horizontal
                navigation.panel: root.navigationPanel
                navigation.row: index + 1
                navigation.column: 0
                onClicked: root.runResult(index)
            }
        }
        StyledTextLabel {
            width: parent.width
            height: 28
            visible: searchModel.commandResults.length === 0
            text: qsTrc("notation", "No matching commands. Try a shorter name.")
        }
    }
}
