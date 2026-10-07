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
import QtQuick
import QtQuick.Controls

import MuseScore.Preferences
import Muse.Shortcuts
import Muse.Ui
import Muse.UiComponents
import MuseScore.NotationScene

PreferencesPage {
    id: root

    contentFillsAvailableHeight: true

    property alias shortcutCodeKey: page.shortcutCodeKey

    function apply() {
        mouseModel.cancelCapture();
        return page.apply();
    }

    function reset() {
        mouseModel.cancelCapture();
        page.reset();
    }

    ShortcutsPage {
        id: page

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: mouseSection.top
        anchors.bottomMargin: 12

        navigationSection: root.navigationSection
        navigationOrderStart: root.navigationOrderStart
    }

    PercussionWorkspaceModel {
        id: mouseModel
        Component.onCompleted: load()
    }
    NavigationPanel {
        id: mouseNav
        name: "MouseBindings"
        section: root.navigationSection
        order: root.navigationOrderStart + 30
    }
    Item {
        id: mouseSection
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 220
        Rectangle {
            width: parent.width
            height: 1
            color: ui.theme.strokeColor
        }
        Column {
            anchors.fill: parent
            anchors.topMargin: 10
            spacing: 6
            StyledTextLabel {
                text: qsTrc("preferences", "Mouse button shortcuts")
            }
            StyledTextLabel {
                width: parent.width
                horizontalAlignment: Text.AlignLeft
                text: mouseModel.capturing ? qsTrc("preferences", "Press a side or middle mouse button, with any Ctrl/Alt/Shift modifiers.") : qsTrc("preferences", "Assign score commands to side or middle buttons. Left and right click retain their normal behavior.")
                wrapMode: Text.WordWrap
            }
            Row {
                spacing: 8
                SearchField {
                    id: mouseSearch
                    width: 140
                    hint: qsTrc("preferences", "Find an action")
                    navigation.panel: mouseNav
                    navigation.order: 1
                    onTextChanged: mouseActions.model = mouseModel.actionsFor(text)
                }
                StyledDropdown {
                    id: mouseActions
                    width: Math.max(120, mouseSection.width - mouseSearch.width - recordButton.width - 16)
                    model: mouseModel.actionsFor("")
                    currentIndex: count > 0 ? 0 : -1
                    onModelChanged: currentIndex = model && model.length > 0 ? 0 : -1
                    onActivated: function (index, value) {
                        currentIndex = index;
                    }
                    navigation.panel: mouseNav
                    navigation.order: 2
                }
                FlatButton {
                    id: recordButton
                    text: mouseModel.capturing ? qsTrc("preferences", "Cancel") : qsTrc("preferences", "Record button")
                    navigation.panel: mouseNav
                    navigation.order: 3
                    onClicked: {
                        if (mouseModel.capturing)
                            mouseModel.cancelCapture();
                        else if (mouseActions.currentIndex >= 0)
                            mouseModel.captureMouse(mouseActions.model[mouseActions.currentIndex].value);
                    }
                }
            }
            StyledFlickable {
                width: parent.width
                height: 82
                clip: true
                contentHeight: bindingRows.height
                Column {
                    id: bindingRows
                    width: parent.width
                    spacing: 4
                    Repeater {
                        model: mouseModel.mouseBindings
                        delegate: Row {
                            required property var modelData
                            required property int index
                            width: bindingRows.width
                            spacing: 8
                            StyledTextLabel {
                                width: 150
                                text: modelData.button
                                horizontalAlignment: Text.AlignLeft
                            }
                            StyledTextLabel {
                                width: bindingRows.width - 210
                                text: modelData.text
                                horizontalAlignment: Text.AlignLeft
                            }
                            FlatButton {
                                text: qsTrc("preferences", "Remove")
                                navigation.panel: mouseNav
                                navigation.order: index + 4
                                onClicked: mouseModel.removeMouseBinding(modelData.key)
                            }
                        }
                    }
                }
            }
        }
    }
    Component.onDestruction: mouseModel.cancelCapture()
}
