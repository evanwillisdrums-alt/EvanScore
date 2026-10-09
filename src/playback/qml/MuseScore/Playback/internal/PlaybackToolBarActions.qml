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
    readonly property int navigationOrderEnd: tempoLoader.navigationOrderEnd
    readonly property color displayColor: (ui.theme.borderWidth > 1) ? ui.theme.backgroundPrimaryColor : "#202b3b"
    readonly property color displayTextColor: (ui.theme.borderWidth > 1) ? ui.theme.fontPrimaryColor : "#edf2f8"
    readonly property color captionColor: (ui.theme.borderWidth > 1) ? ui.theme.fontPrimaryColor : "#a9b7c8"
    readonly property real valueTop: 13
    readonly property real valueHeight: 26

    property bool floating: false

    width: lcd.x + lcd.width + 4
    height: 42

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: ui.theme.backgroundSecondaryColor
        opacity: 0.55
    }

    Rectangle {
        id: lcd
        objectName: "playback-control-display"
        x: buttonsListView.x + buttonsListView.width + 8
        y: 1
        width: 400
        height: 40
        radius: 7
        color: root.displayColor
        border.width: 1
        border.color: (ui.theme.borderWidth > 1) ? ui.theme.strokeColor : "#39475a"

        Row {
            x: 8; y: 3
            spacing: 0
            Repeater {
                model: [
                    { label: root.playbackModel.musicalTime ? qsTrc("playback", "POSITION") : qsTrc("playback", "TIME"), size: 110 },
                    { label: qsTrc("playback", "BAR · BEAT"), size: 78 },
                    { label: qsTrc("playback", "TEMPO"), size: 86 },
                    { label: qsTrc("playback", "METER · KEY"), size: 110 }
                ]
                Text {
                    required property var modelData
                    width: modelData.size
                    text: modelData.label
                    color: root.captionColor
                    font.family: ui.theme.bodyFont.family
                    font.pixelSize: 10
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
        Repeater {
            model: [118, 196, 282]
            Rectangle {
                required property int modelData
                x: modelData; y: 6; width: 1; height: 28
                color: root.captionColor
                opacity: 0.18
            }
        }
        Text {
            objectName: "transport-meter-key"
            x: 286; y: root.valueTop
            width: 106
            height: root.valueHeight
            text: root.playbackModel.scoreInfo.timeSignature + "  " + root.playbackModel.scoreInfo.key
            elide: Text.ElideRight
            color: root.displayTextColor
            font: timeField.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        MouseArea {
            x: 284; width: 112; height: parent.height
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
        spacing: 4

        model: root.playbackModel

        orientation: Qt.Horizontal
        interactive: false

        readonly property int navigationOrderEnd: count

        delegate: FlatButton {
            id: btn

            required property MenuItem item
            required property int index

            width: 30
            height: width
            backgroundRadius: 8

            icon: Boolean(item) ? item.icon : IconCode.NONE

            toolTipTitle: Boolean(item) ? item.title : ""
            toolTipDescription: Boolean(item) ? item.description : ""
            toolTipShortcut: Boolean(item) ? item.shortcuts : ""

            iconFont: ui.theme.toolbarIconsFont

            accentButton: (Boolean(item) && item.checked) || menuLoader.isMenuOpened
            transparent: !accentButton

            navigation.panel: root.navPanel
            navigation.name: toolTipTitle
            navigation.order: index
            accessible.name: (item.checkable ? (item.checked ? item.title + "  " + qsTrc("global", "On") :
                                                               item.title + "  " + qsTrc("global", "Off")) : item.title)

            onClicked: {
                if (menuLoader.isMenuOpened || item.subitems.length) {
                    menuLoader.toggleOpened(item.subitems)
                    return
                }

                Qt.callLater(root.playbackModel.handleMenuItem, item.id)
            }

            StyledMenuLoader {
                id: menuLoader

                onHandleMenuItem: function(itemId) {
                    root.playbackModel.handleMenuItem(itemId)
                }
            }
        }
    }

    FlatButton {
        id: timeField
        objectName: "transport-time-format"

        x: lcd.x + 8 + (110 - width) / 2
        y: root.valueTop
        width: 108
        height: root.valueHeight
        margins: 0
        minWidth: 0
        transparent: true
        backgroundRadius: 5
        property font font: Qt.font({family: "Consolas", pixelSize: 15})
        readonly property int navigationOrderEnd: navigation.order
        navigation.panel: root.navPanel
        navigation.order: buttonsListView.navigationOrderEnd + 1
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

        x: lcd.x + 118 + (78 - width) / 2
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

        onMeasureNumberEdited: function(newValue) {
            root.playbackModel.measureNumber = newValue
        }

        onBeatNumberEdited: function(newValue) {
            root.playbackModel.beatNumber = newValue
        }
    }

    Loader {
        id: tempoLoader
        objectName: "transport-tempo"
        enabled: root.playbackModel.isPlayAllowed

        x: lcd.x + 198
        y: root.valueTop
        height: root.valueHeight

        readonly property int navigationOrderEnd: item?.navigation?.order ?? measureAndBeatFields.navigationOrderEnd

        // Fixed width prevents items from jumping around; but we
        // scale it according to the font size to prevent clipping
        readonly property real tempoViewWidth: 82

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

                backgroundRadius: 8

                implicitWidth: tempoLoader.tempoViewWidth
                implicitHeight: root.valueHeight

                transparent: !isPopupOpened
                iconColor: root.displayTextColor

                toolTipTitle: qsTrc("playback", "Speed")

                navigation.panel: root.navPanel
                navigation.order: measureAndBeatFields.navigationOrderEnd + 1

                contentItem: TempoView {
                    anchors.centerIn: parent

                    noteSymbol: root.playbackModel.tempo.noteSymbol
                    tempoValue: root.playbackModel.tempo.value

                    noteSymbolFont.pixelSize: ui.theme.iconsFont.pixelSize
                    tempoValueFont: timeField.font
                    foregroundColor: root.displayTextColor
                }

                property PlaybackToolBarModel playbackModel: root.playbackModel

                popupComponent: PlaybackSpeedPopup {
                    playbackModel: playbackSpeedButton.playbackModel
                }
            }
        }
    }

}
