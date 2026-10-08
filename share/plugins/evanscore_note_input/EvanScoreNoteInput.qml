// SPDX-License-Identifier: GPL-3.0-only
import QtQuick
import QtQuick.Controls
import QtQuick.Window
import MuseScore 3.0
import MuseScore.NotationScene

MuseScore {
    id: root
    title: "EvanScore Note Input"
    description: "A translucent floating keypad with notation controls and Windows window buttons."
    version: "1.2"
    categoryCode: "composing-arranging-tools"
    thumbnailName: ""
    pluginType: ""
    requiresScore: true
    onRun: {
        voicesFilter.load();
        syncSelection();
        noteWindow.show();
    }
    onScoreStateChanged: Qt.callLater(syncSelection)

    property url musicFontSource: "qrc:/fonts/leland/Leland.otf"
    property var elementTypes: typeof Element !== "undefined" ? Element : ({})
    property var symbolTypes: typeof SymId !== "undefined" ? SymId : ({})
    property int activePage: 0
    property int activeVoice: 0
    property string activeDuration: ""
    property string activeTool: "escape"
    property string notice: ""
    readonly property color accentColor: typeof ui !== "undefined" ? ui.theme.accentColor : "#008edb"
    readonly property var pages: [[
            {
                "label": "Selection tool",
                "action": "escape",
                "col": 0,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [],
                "bounds": [0, 0, 1, 1],
                "text": "pointer",
                "shortcut": ""
            },
            {
                "label": "Accent",
                "action": "add-sforzato",
                "col": 1,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue4a0",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, 0, 361, 235],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Staccato",
                "action": "add-staccato",
                "col": 2,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue4a2",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [-0.25, 0, 78.25, 78],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Tenuto",
                "action": "add-tenuto",
                "col": 3,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue4a4",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, 0, 316, 46],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Natural",
                "action": "nat",
                "col": 0,
                "row": 1,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue261",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -323, 171, 325],
                "text": "",
                "shortcut": "2"
            },
            {
                "label": "Sharp",
                "action": "sharp",
                "col": 1,
                "row": 1,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue262",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -333, 244, 334],
                "text": "",
                "shortcut": "3"
            },
            {
                "label": "Flat",
                "action": "flat",
                "col": 2,
                "row": 1,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue260",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -176, 203, 453],
                "text": "",
                "shortcut": "1"
            },
            {
                "label": "Flip stem / direction",
                "action": "flip",
                "col": 3,
                "row": 1,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [],
                "bounds": [0, 0, 1, 1],
                "text": "flip",
                "shortcut": ""
            },
            {
                "label": "Quarter note",
                "action": "pad-note-4",
                "col": 0,
                "row": 2,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue1d5",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -131, 325, 875],
                "text": "",
                "shortcut": "7"
            },
            {
                "label": "Half note",
                "action": "pad-note-2",
                "col": 1,
                "row": 2,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue1d3",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -132, 325, 875],
                "text": "",
                "shortcut": "4"
            },
            {
                "label": "Whole note",
                "action": "pad-note-1",
                "col": 2,
                "row": 2,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue1d2",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -134, 373, 136],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Double dot",
                "action": "pad-dot2",
                "col": 3,
                "row": 2,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue1d5",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    },
                    {
                        "symbol": "\ue1e7",
                        "x": 470,
                        "y": 0,
                        "factor": 1
                    },
                    {
                        "symbol": "\ue1e7",
                        "x": 680,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -131, 780, 875],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "32nd note",
                "action": "pad-note-32",
                "col": 0,
                "row": 3,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue1db",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -133, 579, 1064],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "16th note",
                "action": "pad-note-16",
                "col": 1,
                "row": 3,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue1d9",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -133, 579, 887],
                "text": "",
                "shortcut": "9"
            },
            {
                "label": "Eighth note",
                "action": "pad-note-8",
                "col": 2,
                "row": 3,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue1d7",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -133, 589, 888],
                "text": "",
                "shortcut": "8"
            },
            {
                "label": "Tie",
                "action": "tie",
                "col": 3,
                "row": 3,
                "colSpan": 1,
                "rowSpan": 2,
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
                "bounds": [0, -340, 1075, 875],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Rest",
                "action": "pad-rest",
                "col": 0,
                "row": 4,
                "colSpan": 2,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue4e5",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -331, 235, 401],
                "text": "",
                "shortcut": "5"
            },
            {
                "label": "Dot",
                "action": "pad-dot",
                "col": 2,
                "row": 4,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue1d5",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    },
                    {
                        "symbol": "\ue1e7",
                        "x": 470,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -131, 570, 875],
                "text": "",
                "shortcut": "6"
            }
        ], [
            {
                "label": "Flam",
                "action": "acciaccatura",
                "col": 0,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
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
                "bounds": [-450.0, -131, 325, 875],
                "text": "",
                "shortcut": "0"
            },
            {
                "label": "Diddle / double stroke",
                "action": "add-diddle",
                "col": 1,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
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
                "bounds": [0, -131, 455, 875],
                "text": "",
                "shortcut": "."
            },
            {
                "label": "Roll",
                "action": "add-roll",
                "col": 2,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
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
                "bounds": [0, -131, 455, 875],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Appoggiatura",
                "action": "appoggiatura",
                "col": 3,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue562",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -83, 370, 567],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Quarter grace note",
                "action": "grace4",
                "col": 0,
                "row": 1,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue1d5",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -131, 325, 875],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "16th grace note",
                "action": "grace16",
                "col": 1,
                "row": 1,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue1d9",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -133, 579, 887],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "32nd grace note",
                "action": "grace32",
                "col": 2,
                "row": 1,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue1db",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -133, 579, 1064],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Marcato",
                "action": "add-marcato",
                "col": 3,
                "row": 1,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue4ac",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [1, 0, 296, 253],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Accent",
                "action": "add-sforzato",
                "col": 0,
                "row": 2,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue4a0",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, 0, 361, 235],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Staccato",
                "action": "add-staccato",
                "col": 1,
                "row": 2,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue4a2",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [-0.25, 0, 78.25, 78],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Tenuto",
                "action": "add-tenuto",
                "col": 2,
                "row": 2,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue4a4",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, 0, 316, 46],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Tie",
                "action": "tie",
                "col": 3,
                "row": 2,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [],
                "bounds": [0, 0, 1, 1],
                "text": "tie",
                "shortcut": ""
            }
        ], [
            {
                "label": "Automatic beaming",
                "action": "beam-auto",
                "col": 0,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [],
                "bounds": [0, 0, 1, 1],
                "text": "AUTO",
                "shortcut": ""
            },
            {
                "label": "Start beam",
                "action": "beam-break-left",
                "col": 1,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [],
                "bounds": [0, 0, 1, 1],
                "text": "Start",
                "shortcut": ""
            },
            {
                "label": "Continue beam",
                "action": "beam-join",
                "col": 2,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [],
                "bounds": [0, 0, 1, 1],
                "text": "Middle",
                "shortcut": ""
            },
            {
                "label": "Break secondary beam",
                "action": "beam-break-inner-8th",
                "col": 3,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [],
                "bounds": [0, 0, 1, 1],
                "text": "Break",
                "shortcut": ""
            },
            {
                "label": "Remove beam",
                "action": "beam-none",
                "col": 0,
                "row": 1,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [],
                "bounds": [0, 0, 1, 1],
                "text": "None",
                "shortcut": ""
            },
            {
                "label": "Quarter note",
                "action": "pad-note-4",
                "col": 1,
                "row": 1,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue1d5",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -131, 325, 875],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Eighth note",
                "action": "pad-note-8",
                "col": 2,
                "row": 1,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue1d7",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -133, 589, 888],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "16th note",
                "action": "pad-note-16",
                "col": 3,
                "row": 1,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue1d9",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -133, 579, 887],
                "text": "",
                "shortcut": ""
            }
        ], [
            {
                "label": "Fermata",
                "action": "add-fermata",
                "col": 0,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue4c0",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, 0, 622, 368],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Accent",
                "action": "add-sforzato",
                "col": 1,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue4a0",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, 0, 361, 235],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Staccato",
                "action": "add-staccato",
                "col": 2,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue4a2",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [-0.25, 0, 78.25, 78],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Tenuto",
                "action": "add-tenuto",
                "col": 3,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue4a4",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, 0, 316, 46],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Marcato",
                "action": "add-marcato",
                "col": 0,
                "row": 1,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue4ac",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [1, 0, 296, 253],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Flip direction",
                "action": "flip",
                "col": 1,
                "row": 1,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [],
                "bounds": [0, 0, 1, 1],
                "text": "flip",
                "shortcut": ""
            }
        ], [
            {
                "label": "Double flat",
                "action": "flat2",
                "col": 0,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue264",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -176, 371, 453],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Flat",
                "action": "flat",
                "col": 1,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue260",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -176, 203, 453],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Natural",
                "action": "nat",
                "col": 2,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue261",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -323, 171, 325],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Sharp",
                "action": "sharp",
                "col": 3,
                "row": 0,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue262",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -333, 244, 334],
                "text": "",
                "shortcut": ""
            },
            {
                "label": "Double sharp",
                "action": "sharp2",
                "col": 0,
                "row": 1,
                "colSpan": 1,
                "rowSpan": 1,
                "glyphs": [
                    {
                        "symbol": "\ue263",
                        "x": 0,
                        "y": 0,
                        "factor": 1
                    }
                ],
                "bounds": [0, -137, 275, 138],
                "text": "",
                "shortcut": ""
            }
        ]]
    readonly property var tabs: [
        {
            "label": "Notes",
            "action": "",
            "col": 0,
            "row": 0,
            "colSpan": 1,
            "rowSpan": 1,
            "glyphs": [
                {
                    "symbol": "\ue1d2",
                    "x": 0,
                    "y": 0,
                    "factor": 1
                }
            ],
            "bounds": [0, -134, 373, 136],
            "text": "",
            "shortcut": ""
        },
        {
            "label": "Grace notes and percussion",
            "action": "",
            "col": 1,
            "row": 0,
            "colSpan": 1,
            "rowSpan": 1,
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
            "bounds": [-450.0, -131, 325, 875],
            "text": "",
            "shortcut": ""
        },
        {
            "label": "Beaming",
            "action": "",
            "col": 2,
            "row": 0,
            "colSpan": 1,
            "rowSpan": 1,
            "glyphs": [],
            "bounds": [0, 0, 1, 1],
            "text": "AUTO",
            "shortcut": ""
        },
        {
            "label": "Articulations",
            "action": "",
            "col": 3,
            "row": 0,
            "colSpan": 1,
            "rowSpan": 1,
            "glyphs": [
                {
                    "symbol": "\ue4c0",
                    "x": 0,
                    "y": 0,
                    "factor": 1
                }
            ],
            "bounds": [0, 0, 622, 368],
            "text": "",
            "shortcut": ""
        },
        {
            "label": "Accidentals",
            "action": "",
            "col": 4,
            "row": 0,
            "colSpan": 1,
            "rowSpan": 1,
            "glyphs": [
                {
                    "symbol": "\ue264",
                    "x": 0,
                    "y": 0,
                    "factor": 1
                }
            ],
            "bounds": [0, -176, 371, 453],
            "text": "",
            "shortcut": ""
        }
    ]
    FontLoader {
        id: musicFont
        source: root.musicFontSource
    }

    VoicesSelectionFilterModel {
        id: voicesFilter
        objectName: "keypad-voices-filter"
    }

    function syncSelection() {
        activeDuration = "";
        if (!curScore || !curScore.selection)
            return;
        let selected = curScore.selection.elements;
        if (!selected.length)
            return;
        let item = selected[0];
        if (item.type === root.elementTypes.NOTE)
            item = item.parent;
        if (!item)
            return;
        if (item.track >= 0)
            activeVoice = item.track % 4;
        if (item.duration) {
            let ratio = item.duration.numerator / item.duration.denominator;
            let lengths = [1, 2, 4, 8, 16, 32];
            for (let n of lengths)
                for (let dots = 0; dots <= 3; ++dots)
                    if (Math.abs(ratio - (2 - Math.pow(0.5, dots)) / n) < 0.000001)
                        activeDuration = "pad-note-" + n;
        }
    }
    function addFermata() {
        if (!curScore || !curScore.selection)
            return false;
        let items = curScore.selection.elements;
        let targets = [];
        let seen = {};
        for (let item of items) {
            let cr = item.type === root.elementTypes.NOTE ? item.parent : item;
            if (!cr || (cr.type !== root.elementTypes.CHORD && cr.type !== root.elementTypes.REST))
                continue;
            let segment = cr.parent;
            let key = segment.tick + ":" + cr.track;
            if (seen[key])
                continue;
            seen[key] = true;
            let existing = [];
            for (let annotation of segment.annotations)
                if (annotation.type === root.elementTypes.FERMATA && annotation.track === cr.track)
                    existing.push(annotation);
            targets.push({ tick: segment.tick, track: cr.track, existing: existing });
        }
        if (!targets.length)
            return false;
        let remove = targets.every(function(target) { return target.existing.length > 0; });
        curScore.startCmd();
        try {
            let cursor = curScore.newCursor();
            for (let target of targets) {
                if (remove) {
                    for (let existing of target.existing)
                        removeElement(existing);
                    continue;
                }
                if (target.existing.length)
                    continue;
                cursor.track = target.track;
                cursor.rewindToTick(target.tick);
                let fermata = newElement(root.elementTypes.FERMATA);
                fermata.symbol = root.symbolTypes.fermataAbove;
                cursor.add(fermata);
            }
        } finally {
            curScore.endCmd();
        }
        return true;
    }
    function activate(spec) {
        if (spec.action === "add-fermata") {
            if (!addFermata()) {
                notice = "Select a note or rest first.";
                noticeTimer.restart();
            }
            return;
        }
        if (spec.action.indexOf("pad-note-") === 0)
            activeDuration = spec.action;
        if (spec.action === "escape")
            activeTool = "escape";
        cmd(spec.action);
    }
    Timer {
        id: noticeTimer
        interval: 2000
        onTriggered: root.notice = ""
    }

    Window {
        id: noteWindow
        objectName: "EvanScoreNoteInputWindow"
        title: "Keypad"
        width: 320
        height: 510
        minimumWidth: 320
        maximumWidth: 320
        minimumHeight: 510
        maximumHeight: 510
        // A taskbar window allows Windows to restore the minimized keypad.
        flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
        modality: Qt.NonModal
        color: "transparent"
        onClosing: root.quit()

        Rectangle {
            id: panel
            anchors.fill: parent
            radius: 16
            antialiasing: true
            border.width: 1
            border.color: "#5077797c"
            gradient: Gradient {
                GradientStop {
                    position: 0
                    color: "#e52b2e32"
                }
                GradientStop {
                    position: 1
                    color: "#d934373b"
                }
            }
            Item {
                id: titleBar
                width: parent.width
                height: 32
                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: 74
                    onPressed: noteWindow.startSystemMove()
                }
                Text {
                    anchors.centerIn: parent
                    text: "Keypad"
                    color: "#f3f3f3"
                    font.family: "Segoe UI"
                    font.pixelSize: 13
                    font.bold: true
                }
                Row {
                    anchors.right: parent.right
                    anchors.rightMargin: 5
                    spacing: 0
                    Button {
                        objectName: "keypad-minimize"
                        width: 32
                        height: 28
                        Accessible.name: "Minimize keypad"
                        onClicked: noteWindow.showMinimized()
                        contentItem: Text {
                            text: "\u2013"
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: 6
                            color: parent.hovered ? "#30ffffff" : "transparent"
                        }
                    }
                    Button {
                        objectName: "keypad-close"
                        width: 32
                        height: 28
                        Accessible.name: "Close keypad"
                        onClicked: noteWindow.close()
                        contentItem: Text {
                            text: "\u00d7"
                            color: "white"
                            font.pixelSize: 20
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: 6
                            color: parent.hovered ? "#c42b1c" : "transparent"
                        }
                    }
                }
            }
            Column {
                x: 10
                y: 40
                width: parent.width - 20
                spacing: 6
                Row {
                    width: parent.width
                    spacing: 6
                    Repeater {
                        model: [
                            {
                                action: "delete",
                                label: "Delete"
                            },
                            {
                                action: "undo",
                                label: "Undo"
                            },
                            {
                                action: "redo",
                                label: "Redo"
                            }
                        ]
                        delegate: KeyButton {
                            required property var modelData
                            spec: modelData
                            width: (parent.width - 12) / 3
                            height: 40
                            onClicked: root.cmd(spec.action)
                        }
                    }
                }
                Row {
                    width: parent.width
                    spacing: 5
                    Repeater {
                        model: root.tabs
                        delegate: KeyButton {
                            required property var modelData
                            required property int index
                            spec: modelData
                            width: (parent.width - 20) / 5
                            height: 42
                            checked: root.activePage === index
                            objectName: "keypad-tab-" + index
                            onClicked: root.activePage = index
                        }
                    }
                }
                Item {
                    id: grid
                    width: parent.width
                    height: 324
                    readonly property real keyWidth: (width - 18) / 4
                    Repeater {
                        model: root.pages[root.activePage]
                        delegate: KeyButton {
                            id: gridKey
                            required property var modelData
                            spec: modelData
                            x: spec.col * (grid.keyWidth + 6)
                            y: spec.row * 66
                            width: spec.colSpan * (grid.keyWidth + 6) - 6
                            height: spec.rowSpan * 66 - 6
                            checked: (spec.action === "escape" && root.activeTool === "escape") || spec.action === root.activeDuration
                            onClicked: root.activate(spec)
                            Shortcut {
                                sequence: gridKey.spec.shortcut || ""
                                enabled: sequence !== "" && noteWindow.active
                                context: Qt.WindowShortcut
                                onActivated: root.activate(gridKey.spec)
                            }
                        }
                    }
                }
                Row {
                    width: parent.width
                    spacing: 5
                    Repeater {
                        model: ["1", "2", "3", "4", "All"]
                        delegate: KeyButton {
                            required property string modelData
                            required property int index
                            spec: ({
                                    action: index < 4 ? "voice-" + modelData : "select-all-voices",
                                    label: index < 4 ? "Voice " + modelData : "Include all voices in range selections",
                                    text: modelData
                                })
                            width: (parent.width - 20) / 5
                            height: 36
                            checked: root.activeVoice === index
                            onClicked: {
                                root.activeVoice = index;
                                if (index < 4)
                                    root.cmd(spec.action);
                                else
                                    voicesFilter.selectAll();
                            }
                            Rectangle {
                                anchors.bottom: parent.bottom
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: parent.width - 12
                                height: 2
                                radius: 1
                                color: ["#2b9ee9", "#69bd62", "#f39a45", "#d776d7", "#a2a5aa"][index]
                                opacity: 0.8
                            }
                        }
                    }
                }
            }
            Text {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 42
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.notice
                color: "#ffffff"
                font.pixelSize: 10
            }
        }
    }

    component KeyButton: Button {
        id: key
        property var spec: ({})
        objectName: "key-" + (spec.action || "")
        hoverEnabled: true
        Accessible.name: spec.label || ""
        ToolTip.visible: hovered
        ToolTip.delay: 650
        ToolTip.text: spec.label || ""
        padding: 5
        background: Rectangle {
            radius: 11
            antialiasing: true
            border.width: 1
            border.color: key.checked ? "#55bafbff" : "#287c7f84"
            gradient: Gradient {
                GradientStop {
                    position: 0
                    color: key.checked ? root.accentColor : key.down ? "#c36b7077" : key.hovered ? "#bd767b82" : "#a6666a70"
                }
                GradientStop {
                    position: 1
                    color: key.checked ? Qt.darker(root.accentColor, 1.15) : key.down ? "#b351555b" : "#985b5f65"
                }
            }
        }
        contentItem: Item {
            id: ink
            readonly property var bounds: key.spec.bounds || [0, 0, 1, 1]
            readonly property real scale: Math.min(["add-sforzato", "add-tenuto"].indexOf(key.spec.action) >= 0 ? 0.085 : key.spec.action === "add-staccato" ? 0.075 : 0.04, Math.max(0, height - 8) / (bounds[3] - bounds[1]), Math.max(0, width - 8) / (bounds[2] - bounds[0]))
            Repeater {
                model: key.spec.glyphs || []
                delegate: Text {
                    required property var modelData
                    text: modelData.symbol
                    font.family: musicFont.name
                    font.pixelSize: Math.max(1, ink.scale * 1000 * modelData.factor)
                    color: "#fafafa"
                    x: (ink.width - (ink.bounds[0] + ink.bounds[2]) * ink.scale) / 2 + modelData.x * ink.scale
                    y: (ink.height + (ink.bounds[1] + ink.bounds[3]) * ink.scale) / 2 - modelData.y * ink.scale - baselineOffset
                }
            }
            Text {
                anchors.centerIn: parent
                text: key.spec.text && ["pointer", "flip", "tie"].indexOf(key.spec.text) < 0 ? key.spec.text : ""
                color: "#fafafa"
                font.family: "Segoe UI"
                font.pixelSize: key.spec.text === "AUTO" ? 10 : 16
                font.bold: true
            }
            Canvas {
                anchors.horizontalCenter: parent.horizontalCenter
                y: key.spec.action === "tie" && (key.spec.glyphs || []).length ? (ink.height + (ink.bounds[1] + ink.bounds[3]) * ink.scale) / 2 + 4 : (ink.height - height) / 2
                width: 40
                height: 40
                visible: ["delete", "undo", "redo", "escape", "flip", "tie"].indexOf(key.spec.action) >= 0
                onPaint: {
                    let ctx = getContext("2d");
                    ctx.clearRect(0, 0, 40, 40);
                    ctx.strokeStyle = "#fafafa";
                    ctx.fillStyle = "#fafafa";
                    ctx.lineWidth = 2.6;
                    ctx.lineJoin = "round";
                    ctx.lineCap = "round";
                    if (key.spec.action === "delete") {
                        ctx.strokeRect(11, 12, 18, 22);
                        ctx.strokeRect(16, 6, 8, 4);
                        ctx.beginPath();
                        ctx.moveTo(8, 10);
                        ctx.lineTo(32, 10);
                        ctx.moveTo(17, 16);
                        ctx.lineTo(17, 29);
                        ctx.moveTo(23, 16);
                        ctx.lineTo(23, 29);
                        ctx.stroke();
                    } else if (key.spec.action === "undo" || key.spec.action === "redo") {
                        if (key.spec.action === "redo") {
                            ctx.translate(40, 0);
                            ctx.scale(-1, 1);
                        }
                        ctx.beginPath();
                        ctx.arc(21, 22, 12, -2.9, 1.2);
                        ctx.stroke();
                        ctx.beginPath();
                        ctx.moveTo(8, 17);
                        ctx.lineTo(8, 8);
                        ctx.lineTo(17, 14);
                        ctx.fill();
                    } else if (key.spec.action === "escape") {
                        ctx.beginPath();
                        ctx.moveTo(12, 5);
                        ctx.lineTo(12, 32);
                        ctx.lineTo(19, 26);
                        ctx.lineTo(25, 35);
                        ctx.lineTo(29, 32);
                        ctx.lineTo(23, 23);
                        ctx.lineTo(32, 23);
                        ctx.closePath();
                        ctx.fill();
                    } else if (key.spec.action === "flip") {
                        ctx.beginPath();
                        ctx.ellipse(14, 16, 12, 8);
                        ctx.fill();
                        ctx.beginPath();
                        ctx.moveTo(15, 10);
                        ctx.lineTo(20, 5);
                        ctx.lineTo(25, 10);
                        ctx.moveTo(15, 30);
                        ctx.lineTo(20, 35);
                        ctx.lineTo(25, 30);
                        ctx.stroke();
                    } else if (key.spec.action === "tie") {
                        if (!(key.spec.glyphs || []).length) {
                            ctx.beginPath();
                            ctx.ellipse(4, 17, 9, 6);
                            ctx.ellipse(27, 17, 9, 6);
                            ctx.fill();
                        }
                        ctx.beginPath();
                        ctx.moveTo(9, 4);
                        ctx.bezierCurveTo(15, 10, 25, 10, 31, 4);
                        ctx.bezierCurveTo(25, 13, 15, 13, 9, 4);
                        ctx.fill();
                    }
                }
            }
        }
    }
}
