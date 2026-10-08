// SPDX-License-Identifier: GPL-3.0-only
import QtQuick
import QtQuick.Controls
import QtQuick.Window
import MuseScore 3.0

MuseScore {
    id: root
    title: "EvanScore Note Input"
    description: "A floating, nonmodal note-input keypad with notation previews."
    version: "1.1"
    categoryCode: "composing-arranging-tools"
    thumbnailName: ""
    pluginType: ""
    requiresScore: true
    onRun: noteWindow.show()
    // Follow the host theme, including live light/dark changes.
    property var theme: typeof ui !== "undefined" ? ui.theme : null
    property color panelColor: theme ? theme.backgroundSecondaryColor : "#e2e1dd"
    property color windowColor: theme ? theme.backgroundPrimaryColor : "#cfcecb"
    property color buttonColor: theme ? theme.buttonColor : "#cfcecb"
    property color inkColor: theme ? theme.fontPrimaryColor : "#252525"
    property color accentColor: theme ? theme.accentColor : "#426fb5"
    // UI symbols only: the score's music-font settings remain unchanged.
    property url musicFontSource: "qrc:/fonts/leland/Leland.otf"
    FontLoader {
        id: musicFont
        source: root.musicFontSource
    }

    Window {
        id: noteWindow
        objectName: "EvanScoreNoteInputWindow"
        title: "Note input"
        width: 260
        height: 346
        minimumWidth: 260
        maximumWidth: 260
        minimumHeight: 346
        maximumHeight: 346
        flags: Qt.Tool | Qt.WindowStaysOnTopHint
        modality: Qt.NonModal
        color: root.windowColor
        onClosing: root.quit()
        Rectangle {
            anchors.fill: parent
            anchors.margins: 8
            radius: 14
            color: root.panelColor
            Column {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8
                Button {
                    id: inputButton
                    text: "Note input"
                    width: parent.width
                    height: 28
                    contentItem: Text {
                        text: inputButton.text
                        color: root.inkColor
                        font: root.theme ? root.theme.bodyFont : Qt.font({
                            family: "Arial",
                            pixelSize: 12
                        })
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.cmd("note-input")
                    background: Rectangle {
                        radius: 8
                        color: inputButton.down ? root.accentColor : root.buttonColor
                    }
                }
                Grid {
                    columns: 3
                    spacing: 6
                    Repeater {
                        model: [
                            {
                                "label": "Quarter",
                                "action": "pad-note-4",
                                "key": "7",
                                "glyphs": [
                                    {
                                        "symbol": "\ue1d5",
                                        "x": 0,
                                        "y": 0,
                                        "factor": 1
                                    }
                                ],
                                "bounds": [0, -131, 325, 875]
                            },
                            {
                                "label": "Eighth",
                                "action": "pad-note-8",
                                "key": "8",
                                "glyphs": [
                                    {
                                        "symbol": "\ue1d7",
                                        "x": 0,
                                        "y": 0,
                                        "factor": 1
                                    }
                                ],
                                "bounds": [0, -133, 589, 888]
                            },
                            {
                                "label": "16th",
                                "action": "pad-note-16",
                                "key": "9",
                                "glyphs": [
                                    {
                                        "symbol": "\ue1d9",
                                        "x": 0,
                                        "y": 0,
                                        "factor": 1
                                    }
                                ],
                                "bounds": [0, -133, 579, 887]
                            },
                            {
                                "label": "Half",
                                "action": "pad-note-2",
                                "key": "4",
                                "glyphs": [
                                    {
                                        "symbol": "\ue1d3",
                                        "x": 0,
                                        "y": 0,
                                        "factor": 1
                                    }
                                ],
                                "bounds": [0, -132, 325, 875]
                            },
                            {
                                "label": "Rest",
                                "action": "pad-rest",
                                "key": "5",
                                "glyphs": [
                                    {
                                        "symbol": "\ue4e5",
                                        "x": 0,
                                        "y": 0,
                                        "factor": 1
                                    }
                                ],
                                "bounds": [0, -331, 235, 401]
                            },
                            {
                                "label": "Dot",
                                "action": "pad-dot",
                                "key": "6",
                                "glyphs": [
                                    {
                                        "symbol": "\ue1d5",
                                        "x": 0,
                                        "y": 0,
                                        "factor": 1
                                    },
                                    {
                                        "symbol": "\ue1e7",
                                        "x": 460,
                                        "y": 0,
                                        "factor": 1
                                    }
                                ],
                                "bounds": [0, -131, 560, 875]
                            },
                            {
                                "label": "Flat",
                                "action": "flat",
                                "key": "1",
                                "glyphs": [
                                    {
                                        "symbol": "\ue260",
                                        "x": 0,
                                        "y": 0,
                                        "factor": 1
                                    }
                                ],
                                "bounds": [0, -176, 203, 453]
                            },
                            {
                                "label": "Natural",
                                "action": "nat",
                                "key": "2",
                                "glyphs": [
                                    {
                                        "symbol": "\ue261",
                                        "x": 0,
                                        "y": 0,
                                        "factor": 1
                                    }
                                ],
                                "bounds": [0, -323, 171, 325]
                            },
                            {
                                "label": "Sharp",
                                "action": "sharp",
                                "key": "3",
                                "glyphs": [
                                    {
                                        "symbol": "\ue262",
                                        "x": 0,
                                        "y": 0,
                                        "factor": 1
                                    }
                                ],
                                "bounds": [0, -333, 244, 334]
                            },
                            {
                                "label": "Flam",
                                "action": "acciaccatura",
                                "key": "0",
                                "glyphs": [
                                    {
                                        "symbol": "\ue560",
                                        "x": -450,
                                        "y": 120,
                                        "factor": 0.8
                                    },
                                    {
                                        "symbol": "\ue1d5",
                                        "x": 0,
                                        "y": 0,
                                        "factor": 1
                                    }
                                ],
                                "bounds": [-450.0, -131, 325, 875]
                            },
                            {
                                "label": "Diddle",
                                "action": "add-diddle",
                                "key": ".",
                                "glyphs": [
                                    {
                                        "symbol": "\ue1d5",
                                        "x": 0,
                                        "y": 0,
                                        "factor": 1
                                    },
                                    {
                                        "symbol": "\ue220",
                                        "x": 310,
                                        "y": 500,
                                        "factor": 1
                                    }
                                ],
                                "bounds": [0, -131, 455, 875]
                            },
                            {
                                "label": "Accent",
                                "action": "add-sforzato",
                                "key": "Return",
                                "glyphs": [
                                    {
                                        "symbol": "\ue1d5",
                                        "x": 0,
                                        "y": 0,
                                        "factor": 1
                                    },
                                    {
                                        "symbol": "\ue4a0",
                                        "x": -18,
                                        "y": 1050,
                                        "factor": 1
                                    }
                                ],
                                "bounds": [-18, -131, 343, 1285]
                            },
                            {
                                "label": "Roll",
                                "action": "add-roll",
                                "key": "",
                                "glyphs": [
                                    {
                                        "symbol": "\ue1d5",
                                        "x": 0,
                                        "y": 0,
                                        "factor": 1
                                    },
                                    {
                                        "symbol": "\ue222",
                                        "x": 310,
                                        "y": 500,
                                        "factor": 1
                                    }
                                ],
                                "bounds": [0, -131, 455, 875]
                            },
                            {
                                "label": "Tie",
                                "action": "tie",
                                "key": "",
                                "glyphs": [
                                    {
                                        "symbol": "\ue1d5",
                                        "x": 0,
                                        "y": 0,
                                        "factor": 1
                                    },
                                    {
                                        "symbol": "\ue1d5",
                                        "x": 750,
                                        "y": 0,
                                        "factor": 1
                                    }
                                ],
                                "bounds": [0, -340, 1075, 875]
                            },
                            {
                                "label": "Marcato",
                                "action": "add-marcato",
                                "key": "",
                                "glyphs": [
                                    {
                                        "symbol": "\ue1d5",
                                        "x": 0,
                                        "y": 0,
                                        "factor": 1
                                    },
                                    {
                                        "symbol": "\ue4ac",
                                        "x": 50,
                                        "y": 1050,
                                        "factor": 1
                                    }
                                ],
                                "bounds": [0, -131, 346, 1303]
                            }
                        ]
                        delegate: Button {
                            id: keyButton
                            required property var modelData
                            objectName: "key-" + modelData.action
                            width: 68
                            height: 47
                            Accessible.name: modelData.label
                            ToolTip.visible: hovered
                            ToolTip.delay: 600
                            ToolTip.text: modelData.label + (modelData.key ? " (" + modelData.key + ")" : "")
                            contentItem: Item {
                                id: symbolArea
                                readonly property var bounds: keyButton.modelData.bounds
                                readonly property real scale: Math.min(34 / (bounds[3] - bounds[1]), 38 / (bounds[2] - bounds[0]))
                                Repeater {
                                    model: keyButton.modelData.glyphs
                                    delegate: Text {
                                        required property var modelData
                                        objectName: "notation-glyph"
                                        text: modelData.symbol
                                        font.family: musicFont.name
                                        font.pixelSize: Math.max(1, symbolArea.scale * 1000 * modelData.factor)
                                        color: root.inkColor
                                        // Center the ink, rather than the font's large ascent/descent.
                                        x: (symbolArea.width - (symbolArea.bounds[0] + symbolArea.bounds[2]) * symbolArea.scale) / 2 + modelData.x * symbolArea.scale
                                        y: (symbolArea.height + (symbolArea.bounds[1] + symbolArea.bounds[3]) * symbolArea.scale) / 2 - modelData.y * symbolArea.scale - baselineOffset
                                    }
                                }
                                Canvas {
                                    anchors.centerIn: parent
                                    width: 42
                                    height: 34
                                    visible: keyButton.modelData.action === "tie"
                                    property color ink: root.inkColor
                                    onInkChanged: requestPaint()
                                    onPaint: {
                                        let ctx = getContext("2d");
                                        ctx.clearRect(0, 0, width, height);
                                        ctx.fillStyle = ink;
                                        ctx.beginPath();
                                        ctx.moveTo(10, 28);
                                        ctx.bezierCurveTo(16, 32, 26, 32, 32, 28);
                                        ctx.bezierCurveTo(26, 35, 16, 35, 10, 28);
                                        ctx.fill();
                                    }
                                }
                            }
                            onClicked: root.cmd(keyButton.modelData.action)
                            background: Rectangle {
                                radius: 8
                                color: keyButton.down ? root.accentColor : root.buttonColor
                                border.width: keyButton.activeFocus ? 1 : 0
                                border.color: root.accentColor
                                Rectangle {
                                    anchors.fill: parent
                                    radius: parent.radius
                                    color: root.inkColor
                                    opacity: keyButton.hovered && !keyButton.down ? 0.06 : 0
                                }
                            }
                            Shortcut {
                                sequence: keyButton.modelData.key
                                enabled: keyButton.modelData.key !== "" && noteWindow.active
                                context: Qt.WindowShortcut
                                onActivated: root.cmd(keyButton.modelData.action)
                            }
                        }
                    }
                }
            }
        }
    }
}
