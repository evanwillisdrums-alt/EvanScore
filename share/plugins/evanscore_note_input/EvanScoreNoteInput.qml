// SPDX-License-Identifier: GPL-3.0-only
import QtQuick
import QtQuick.Controls
import QtQuick.Window
import MuseScore 3.0

MuseScore {
    id: root
    title: "EvanScore Note Input"
    description: "A floating, nonmodal note-input keypad for EvanScore."
    version: "1.0"
    category: "composing-arranging-tools"
    thumbnailName: ""
    pluginType: ""
    requiresScore: true
    onRun: noteWindow.show()
    Window {
        id: noteWindow
        objectName: "EvanScoreNoteInputWindow"
        title: "Note input"
        width: 260
        height: 310
        minimumWidth: 260
        maximumWidth: 260
        minimumHeight: 310
        maximumHeight: 310
        flags: Qt.Tool | Qt.WindowStaysOnTopHint
        modality: Qt.NonModal
        color: "#cfcecb"
        onClosing: root.quit()
        Rectangle {
            anchors.fill: parent
            anchors.margins: 8
            radius: 14
            color: "#e2e1dd"
            Column {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8
                Button {
                    text: "Note input"
                    font.family: "Arial"
                    width: parent.width
                    onClicked: root.cmd("note-input")
                    background: Rectangle {
                        radius: 8
                        color: parent.down ? "#bdbdba" : "#cfcecb"
                    }
                }
                Grid {
                    columns: 3
                    spacing: 6
                    Repeater {
                        model: [
                            {
                                label: "Quarter",
                                action: "pad-note-4",
                                key: "7"
                            },
                            {
                                label: "Eighth",
                                action: "pad-note-8",
                                key: "8"
                            },
                            {
                                label: "16th",
                                action: "pad-note-16",
                                key: "9"
                            },
                            {
                                label: "Half",
                                action: "pad-note-2",
                                key: "4"
                            },
                            {
                                label: "Rest",
                                action: "pad-rest",
                                key: "5"
                            },
                            {
                                label: "Dot",
                                action: "pad-dot",
                                key: "6"
                            },
                            {
                                label: "Flat",
                                action: "flat",
                                key: "1"
                            },
                            {
                                label: "Natural",
                                action: "nat",
                                key: "2"
                            },
                            {
                                label: "Sharp",
                                action: "sharp",
                                key: "3"
                            },
                            {
                                label: "Flam",
                                action: "acciaccatura",
                                key: "0"
                            },
                            {
                                label: "Diddle",
                                action: "add-diddle",
                                key: "."
                            },
                            {
                                label: "Accent",
                                action: "add-sforzato",
                                key: "Return"
                            },
                            {
                                label: "Roll",
                                action: "add-roll",
                                key: ""
                            },
                            {
                                label: "Tie",
                                action: "tie",
                                key: ""
                            },
                            {
                                label: "Marcato",
                                action: "add-marcato",
                                key: ""
                            }
                        ]
                        delegate: Button {
                            required property var modelData
                            objectName: "key-" + modelData.action
                            width: 68
                            height: 39
                            text: modelData.label
                            font.family: "Arial"
                            font.pixelSize: 12
                            onClicked: root.cmd(modelData.action)
                            background: Rectangle {
                                radius: 8
                                color: parent.down ? "#bdbdba" : "#cfcecb"
                            }
                            Shortcut {
                                sequence: modelData.key
                                enabled: modelData.key !== "" && noteWindow.active
                                context: Qt.WindowShortcut
                                onActivated: root.cmd(modelData.action)
                            }
                        }
                    }
                }
            }
        }
    }
}
