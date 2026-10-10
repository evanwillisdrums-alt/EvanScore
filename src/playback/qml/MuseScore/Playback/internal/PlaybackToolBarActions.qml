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

import Muse.UiComponents
import Muse.Ui

import MuseScore.NotationScene
import MuseScore.Playback

Item {
    id: root

    property PlaybackToolBarModel playbackModel: null

    property NavigationPanel navPanel: null
    readonly property int navigationOrderEnd: buttonsListView.count + 10
    readonly property color displayColor: (ui.theme.borderWidth > 1) ? ui.theme.backgroundPrimaryColor : "#28303c"
    readonly property color displayTextColor: (ui.theme.borderWidth > 1) ? ui.theme.fontPrimaryColor : "#edf2f8"
    readonly property color captionColor: (ui.theme.borderWidth > 1) ? ui.theme.fontPrimaryColor : "#b8c3d1"
    readonly property real valueTop: 1
    readonly property real valueHeight: 22

    property bool floating: false

    width: buttonsListView.contentWidth + 8
    height: 34

    Rectangle {
        id: lcd
        objectName: "playback-control-display"
        // Native transport actions precede the LCD; options follow it.
        x: 126
        y: 0
        width: 380
        height: 34
        radius: 5
        color: root.displayColor
        border.width: 1
        border.color: (ui.theme.borderWidth > 1) ? ui.theme.strokeColor : "#4d5664"

        Row {
            x: 6
            y: 23
            spacing: 0
            Repeater {
                model: [
                    {
                        label: root.playbackModel.musicalTime ? qsTrc("playback", "POSITION") : qsTrc("playback", "TIME"),
                        size: 104
                    },
                    {
                        label: qsTrc("playback", "BAR · BEAT"),
                        size: 72
                    },
                    {
                        label: qsTrc("playback", "TEMPO"),
                        size: 70
                    },
                    {
                        label: qsTrc("playback", "METER"),
                        size: 42
                    },
                    {
                        label: qsTrc("playback", "KEY"),
                        size: 80
                    }
                ]
                Text {
                    required property var modelData
                    width: modelData.size
                    text: modelData.label
                    color: root.captionColor
                    font.family: ui.theme.bodyFont.family
                    font.pixelSize: 9
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
        Repeater {
            model: [110, 182, 252, 294]
            Rectangle {
                required property int modelData
                x: modelData
                y: 5
                width: 1
                height: 24
                color: root.captionColor
                opacity: 0.18
            }
        }
        Text {
            objectName: "transport-meter-key"
            x: 296
            y: root.valueTop
            width: 78
            height: root.valueHeight
            text: root.playbackModel.scoreInfo.key
            elide: Text.ElideRight
            color: root.displayTextColor
            font: timeField.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        Text {
            objectName: "transport-meter"
            x: 254
            y: root.valueTop
            width: 38
            height: root.valueHeight
            text: root.playbackModel.scoreInfo.timeSignature
            color: root.displayTextColor
            font: timeField.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        MouseArea {
            x: 294
            width: 86
            height: parent.height
            hoverEnabled: true
            acceptedButtons: Qt.NoButton
            onEntered: ui.tooltip.show(lcd, root.playbackModel.scoreInfo.keyDescription)
            onExited: ui.tooltip.hide(lcd)
        }
    }

    ListView {
        id: buttonsListView
        enabled: root.playbackModel.isPlayAllowed

        anchors.left: parent.left
        anchors.leftMargin: 4
        anchors.verticalCenter: parent.verticalCenter

        width: contentWidth
        height: contentHeight

        contentHeight: root.height
        spacing: 2

        model: root.playbackModel

        orientation: Qt.Horizontal
        interactive: false

        readonly property int navigationOrderEnd: count

        delegate: Item {
            id: actionSlot
            required property MenuItem item
            required property int index
            width: 28 + (index === 3 ? lcd.width + 8 : 0)
            height: root.height
            FlatButton {
                id: btn
                readonly property MenuItem item: actionSlot.item
                readonly property int index: actionSlot.index
                anchors.verticalCenter: parent.verticalCenter
                width: 28
                height: width
                backgroundRadius: 5

                icon: Boolean(item) ? item.icon : IconCode.NONE

                toolTipTitle: Boolean(item) ? item.title : ""
                toolTipDescription: Boolean(item) ? item.description : ""
                toolTipShortcut: Boolean(item) ? item.shortcuts : ""

                iconFont: ui.theme.toolbarIconsFont

                accentButton: (Boolean(item) && item.checked) || menuLoader.isMenuOpened
                transparent: !accentButton

                navigation.panel: root.navPanel
                navigation.name: toolTipTitle
                navigation.order: index < 4 ? index : index + 10
                accessible.name: (item.checkable ? (item.checked ? item.title + "  " + qsTrc("global", "On") : item.title + "  " + qsTrc("global", "Off")) : item.title)

                onClicked: {
                    if (menuLoader.isMenuOpened || item.subitems.length) {
                        menuLoader.toggleOpened(item.subitems);
                        return;
                    }

                    Qt.callLater(root.playbackModel.handleMenuItem, item.id);
                }

                StyledMenuLoader {
                    id: menuLoader

                    onHandleMenuItem: function (itemId) {
                        root.playbackModel.handleMenuItem(itemId);
                    }
                }
            }
        }
    }

    FlatButton {
        id: timeField
        objectName: "transport-time-format"

        x: lcd.x + 8
        y: root.valueTop
        width: 100
        height: root.valueHeight
        margins: 0
        minWidth: 0
        transparent: true
        backgroundRadius: 5
        property font font: Qt.font({
            family: "Consolas",
            pixelSize: 15
        })
        readonly property int navigationOrderEnd: navigation.order
        navigation.panel: root.navPanel
        navigation.order: 4
        accessible.name: root.playbackModel.musicalTime ? qsTrc("playback", "Musical position") : qsTrc("playback", "Elapsed time")
        toolTipTitle: qsTrc("playback", "Switch time format")
        toolTipDescription: qsTrc("playback", "Click to switch between elapsed time and bar.beat. A third field shows thousandths of a beat; 1.2.500 is halfway through beat 2. Use Bar · Beat to move playback.")
        contentItem: Text {
            objectName: "transport-time-value"
            width: timeField.width
            height: timeField.height
            text: root.playbackModel.musicalTime ? root.playbackModel.musicalPosition : root.playbackModel.elapsedPosition
            color: root.displayTextColor
            font: timeField.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            fontSizeMode: Text.Fit
            minimumPixelSize: 11
        }
        onClicked: root.playbackModel.musicalTime = !root.playbackModel.musicalTime
    }

    MeasureAndBeatFields {
        id: measureAndBeatFields
        objectName: "transport-bar-beat"
        enabled: root.playbackModel.isPlayAllowed

        x: lcd.x + 112 + (68 - width) / 2
        y: root.valueTop
        height: root.valueHeight
        foregroundColor: root.displayTextColor

        measureNumber: root.playbackModel.measureNumber
        maxMeasureNumber: root.playbackModel.maxMeasureNumber
        beatNumber: root.playbackModel.beatNumber
        maxBeatNumber: root.playbackModel.maxBeatNumber

        font: timeField.font

        navigationPanel: root.navPanel
        navigationOrderStart: timeField.navigationOrderEnd + 1

        onMeasureNumberEdited: function (newValue) {
            root.playbackModel.measureNumber = newValue;
        }

        onBeatNumberEdited: function (newValue) {
            root.playbackModel.beatNumber = newValue;
        }
    }

    Loader {
        id: tempoLoader
        objectName: "transport-tempo"
        enabled: root.playbackModel.isPlayAllowed

        x: lcd.x + 184
        y: root.valueTop
        height: root.valueHeight

        readonly property int navigationOrderEnd: item?.navigation?.order ?? measureAndBeatFields.navigationOrderEnd

        // Fixed width prevents items from jumping around; but we
        // scale it according to the font size to prevent clipping
        readonly property real tempoViewWidth: 66

        sourceComponent: root.floating ? tempoViewComponent : tempoButtonComponent

        Component {
            id: tempoViewComponent

            Item {
                implicitWidth: tempoLoader.tempoViewWidth
                implicitHeight: root.valueHeight

                TempoView {
                    id: tempoView
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter

                    noteSymbol: root.playbackModel.tempo.noteSymbol
                    tempoValue: root.playbackModel.tempo.value

                    noteSymbolFont.pixelSize: ui.theme.iconsFont.pixelSize
                    tempoValueFont: timeField.font
                    foregroundColor: root.displayTextColor
                }
            }
        }

        Component {
            id: tempoButtonComponent

            PopupButton {
                id: playbackSpeedButton

                backgroundRadius: 5

                implicitWidth: tempoLoader.tempoViewWidth
                implicitHeight: root.valueHeight

                transparent: !isPopupOpened
                iconColor: root.displayTextColor

                toolTipTitle: qsTrc("playback", "Speed")

                navigation.panel: root.navPanel
                navigation.order: measureAndBeatFields.navigationOrderEnd + 1

                contentItem: Text {
                    width: tempoLoader.tempoViewWidth
                    height: root.valueHeight
                    text: String(root.playbackModel.tempo.value)
                    font: timeField.font
                    color: root.displayTextColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                property PlaybackToolBarModel playbackModel: root.playbackModel

                popupComponent: PlaybackSpeedPopup {
                    playbackModel: playbackSpeedButton.playbackModel
                }
            }
        }
    }
}
