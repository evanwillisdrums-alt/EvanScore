/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Muse.Ui
import Muse.UiComponents
import Muse.UiComponents as UiComponents
import MuseScore.NotationScene

Item {
    id: root
    objectName: "MalletPanel"
    property NavigationSection navigationSection: null
    property int contentNavigationPanelOrderStart: 0
    property alias model: malletModel
    readonly property var panelState: model.state
    property int currentTab: 0
    property int lastAlternative: 0
    MalletPanelModel {
        id: malletModel
        active: root.visible
    }
    NavigationPanel {
        id: panelNavigation
        name: "MalletVisualizer"
        section: root.navigationSection
        order: root.contentNavigationPanelOrderStart
        enabled: root.visible
    }
    component Label: StyledTextLabel {
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
    }
    component Button: FlatButton {
        height: 28
        minWidth: 0
        margins: 6
        backgroundRadius: 6
        navigation.panel: panelNavigation
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            spacing: 5
            Button {
                objectName: "mallet-score-selection"
                text: qsTrc("notation", "Score selection")
                accentButton: !root.panelState.pickMode
                transparent: !accentButton
                navigation.order: 1
                onClicked: root.model.setPickMode(false)
            }
            Button {
                objectName: "mallet-pick-bars"
                text: qsTrc("notation", "Pick on bars")
                accentButton: root.panelState.pickMode
                transparent: !accentButton
                navigation.order: 2
                onClicked: root.model.setPickMode(true)
            }
            Button {
                text: qsTrc("notation", "Clear")
                enabled: root.panelState.pickMode
                transparent: true
                navigation.order: 3
                onClicked: root.model.clearPicked()
            }
            Item {
                Layout.fillWidth: true
            }
            Button {
                text: "A"
                toolTipTitle: qsTrc("notation", "Original placement")
                accentButton: root.panelState.selectedAlternative < 0
                navigation.order: 4
                onClicked: root.model.showOriginal()
            }
            Button {
                text: "B"
                toolTipTitle: qsTrc("notation", "Preview alternative")
                enabled: root.model.alternatives.length > 0
                accentButton: root.panelState.selectedAlternative >= 0
                navigation.order: 5
                onClicked: root.model.selectAlternative(root.lastAlternative)
            }
            Button {
                objectName: "mallet-audition"
                icon: IconCode.PLAY
                toolTipTitle: qsTrc("notation", "Audition preview")
                enabled: root.panelState.canAudition
                transparent: true
                navigation.order: 6
                onClicked: root.model.audition()
            }
            Button {
                objectName: "mallet-compare"
                text: qsTrc("notation", "Compare")
                enabled: root.panelState.canAudition && root.panelState.selectedAlternative >= 0
                navigation.order: 7
                onClicked: root.model.compare()
            }
            Button {
                objectName: "mallet-commit"
                text: qsTrc("notation", "Commit")
                toolTipDescription: qsTrc("notation", "Apply the preview's pitches and numbered sticking to this onset in one Undo step.")
                enabled: root.panelState.canCommit
                accentButton: true
                navigation.order: 8
                onClicked: root.model.commit()
            }
        }
        Controls.SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal
            handle: Rectangle {
                implicitWidth: 4
                color: ui.theme.strokeSecondaryColor
                opacity: .45
            }
            Item {
                Controls.SplitView.fillWidth: true
                Controls.SplitView.minimumWidth: 210
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 2
                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: (root.panelState.instrument || "Marimba") + "  ·  " + (root.panelState.rangeLabel || "C2  C7")
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            wrapMode: Text.NoWrap
                        }
                        Button {
                            text: qsTrc("notation", "Top")
                            transparent: !accentButton
                            accentButton: !root.panelState.sideView
                            navigation.order: 9
                            onClicked: root.model.setOption("sideView", false)
                        }
                        Button {
                            text: qsTrc("notation", "Front")
                            transparent: !accentButton
                            accentButton: root.panelState.sideView
                            navigation.order: 10
                            onClicked: root.model.setOption("sideView", true)
                        }
                        Button {
                            text: "−"
                            toolTipTitle: qsTrc("notation", "Zoom out")
                            transparent: true
                            navigation.order: 11
                            onClicked: scene.zoom -= .25
                        }
                        Button {
                            text: "+"
                            toolTipTitle: qsTrc("notation", "Zoom in")
                            transparent: true
                            navigation.order: 12
                            onClicked: scene.zoom += .25
                        }
                        Button {
                            text: qsTrc("notation", "Fit")
                            transparent: true
                            navigation.order: 13
                            onClicked: scene.zoom = 1
                        }
                    }
                    MalletSceneView {
                        id: scene
                        objectName: "mallet-instrument-view"
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        scene: root.panelState
                        backgroundColor: ui.theme.textFieldColor
                        Accessible.name: qsTrc("notation", "Mallet instrument and overhead player")
                        Accessible.description: qsTrc("notation", "Arrow keys explore bars; Space selects a bar in Pick on bars mode. Scroll to zoom.")
                        onPitchClicked: function (pitch) {
                            root.model.togglePitch(pitch);
                        }
                        onStrikePointDragged: function (mallet, fraction) {
                            root.model.setStrikePoint(mallet, fraction);
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: scene.hoveredPitch >= 0 ? qsTrc("notation", "MIDI pitch") + " " + scene.hoveredPitch : root.panelState.pose?.pitchText || qsTrc("notation", "Click a chord in the score")
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            wrapMode: Text.NoWrap
                            opacity: .8
                        }
                        Label {
                            text: qsTrc("notation", "1–2 left · 3–4 right")
                            opacity: .8
                        }
                    }
                }
            }
            Item {
                Controls.SplitView.preferredWidth: 330
                Controls.SplitView.minimumWidth: 250
                Controls.SplitView.maximumWidth: 580
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 5
                    RowLayout {
                        spacing: 4
                        Repeater {
                            model: [qsTrc("notation", "Diagnostics"), qsTrc("notation", "Alternatives"), qsTrc("notation", "Player")]
                            Button {
                                required property int index
                                required property string modelData
                                text: modelData
                                accentButton: root.currentTab === index
                                transparent: !accentButton
                                navigation.order: index + 14
                                onClicked: root.currentTab = index
                            }
                        }
                    }
                    Flickable {
                        id: info
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        contentWidth: width
                        contentHeight: details.height
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds
                        Controls.ScrollBar.vertical: StyledScrollBar {}
                        ColumnLayout {
                            id: details
                            width: info.width - 10
                            spacing: 8
                            Label {
                                Layout.fillWidth: true
                                text: root.panelState.status || ""
                                font: ui.theme.bodyBoldFont
                                color: root.panelState.pose?.severity === 2 ? "#ffb0ad" : root.panelState.pose?.severity === 1 ? "#f3d290" : ui.theme.fontPrimaryColor
                            }
                            Label {
                                visible: !!root.panelState.notice
                                Layout.fillWidth: true
                                text: root.panelState.notice || ""
                                color: "#f3d290"
                            }
                            ColumnLayout {
                                visible: root.currentTab === 0
                                Layout.fillWidth: true
                                spacing: 6
                                Label {
                                    Layout.fillWidth: true
                                    text: root.panelState.sticking ? qsTrc("notation", "Written sticking: ") + root.panelState.sticking : qsTrc("notation", "No written sticking • suggested assignment")
                                    opacity: .8
                                }
                                Repeater {
                                    model: 4
                                    RowLayout {
                                        required property int index
                                        Layout.fillWidth: true
                                        Rectangle {
                                            implicitWidth: 8
                                            implicitHeight: 8
                                            radius: 4
                                            color: ["#5da9f4", "#72d0cf", "#f3c560", "#ed8a9e"][parent.index]
                                        }
                                        Label {
                                            Layout.fillWidth: true
                                            text: {
                                                const ids = root.panelState.pose?.mallets || [];
                                                const n = ids.indexOf(parent.index + 1);
                                                const pitches = root.panelState.pose?.pitches || [];
                                                return (parent.index + 1) + (parent.index < 2 ? "  L  ·  " : "  R  ·  ") + (n >= 0 ? "MIDI " + pitches[n] : qsTrc("notation", "resting"));
                                            }
                                        }
                                    }
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: qsTrc("notation", "Estimated hand openings: L %1 cm · R %2 cm").arg(Number(root.panelState.pose?.leftOpening || 0).toFixed(1)).arg(Number(root.panelState.pose?.rightOpening || 0).toFixed(1))
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: qsTrc("notation", "Hand rotation: L %1° · R %2°").arg(Number(root.panelState.pose?.leftRotation || 0).toFixed(0)).arg(Number(root.panelState.pose?.rightRotation || 0).toFixed(0))
                                    opacity: .8
                                }
                                Repeater {
                                    model: root.panelState.pose?.issues || []
                                    Label {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        text: (modelData.severity > 0 ? "• " : "✓ ") + modelData.message
                                        color: modelData.severity === 2 ? "#ffb0ad" : modelData.severity === 1 ? "#f3d290" : ui.theme.fontPrimaryColor
                                    }
                                }
                                Label {
                                    visible: (root.panelState.heldPitches || []).length > 0
                                    Layout.fillWidth: true
                                    text: qsTrc("notation", "Tied notes remain sounding; only new attacks use mallets.")
                                    opacity: .8
                                }
                            }
                            ColumnLayout {
                                visible: root.currentTab === 1
                                Layout.fillWidth: true
                                spacing: 7
                                Label {
                                    Layout.fillWidth: true
                                    text: qsTrc("notation", "Preview changes only this view. Commit writes pitches and sticking; Undo restores both.")
                                    opacity: .8
                                }
                                Repeater {
                                    model: root.model.alternatives
                                    Rectangle {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        implicitHeight: candidateColumn.implicitHeight + 16
                                        radius: 7
                                        color: ui.theme.backgroundSecondaryColor
                                        border.width: modelData.selected ? 1 : 0
                                        border.color: ui.theme.accentColor
                                        ColumnLayout {
                                            id: candidateColumn
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            anchors.top: parent.top
                                            anchors.margins: 8
                                            Button {
                                                Layout.fillWidth: true
                                                text: modelData.pitches
                                                accentButton: modelData.selected
                                                transparent: !accentButton
                                                onClicked: {
                                                    root.lastAlternative = modelData.index;
                                                    root.model.selectAlternative(modelData.index);
                                                }
                                            }
                                            Label {
                                                Layout.fillWidth: true
                                                text: modelData.description
                                                opacity: .8
                                            }
                                            Label {
                                                Layout.fillWidth: true
                                                text: qsTrc("notation", "Opening L %1 · R %2 cm").arg(Number(modelData.leftOpening).toFixed(1)).arg(Number(modelData.rightOpening).toFixed(1))
                                                opacity: .7
                                            }
                                        }
                                    }
                                }
                            }
                            ColumnLayout {
                                visible: root.currentTab === 2
                                Layout.fillWidth: true
                                spacing: 8
                                UiComponents.CheckBox {
                                    text: qsTrc("notation", "Respect written sticking")
                                    checked: root.panelState.respectSticking === true
                                    onClicked: root.model.setOption("respectSticking", !checked)
                                    navigation.panel: panelNavigation
                                    navigation.order: 17
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: qsTrc("notation", "Numbering stays physical: 1 outer left, 2 inner left, 3 inner right, 4 outer right. Written numbers follow ascending pitches within each chord.")
                                    opacity: .8
                                }
                                StyledDropdown {
                                    Layout.fillWidth: true
                                    model: [
                                        {
                                            text: qsTrc("notation", "Four mallets"),
                                            value: 4
                                        },
                                        {
                                            text: qsTrc("notation", "Two mallets"),
                                            value: 2
                                        }
                                    ]
                                    currentIndex: root.panelState.malletCount === 2 ? 1 : 0
                                    onActivated: function (index, value) {
                                        root.model.setOption("malletCount", value);
                                    }
                                }
                                StyledDropdown {
                                    Layout.fillWidth: true
                                    model: [
                                        {
                                            text: "Stevens",
                                            value: 0
                                        },
                                        {
                                            text: "Burton",
                                            value: 1
                                        },
                                        {
                                            text: qsTrc("notation", "Traditional cross grip"),
                                            value: 2
                                        }
                                    ]
                                    currentIndex: root.panelState.grip || 0
                                    onActivated: function (index, value) {
                                        root.model.setOption("grip", value);
                                    }
                                }
                                Repeater {
                                    model: [
                                        {
                                            key: "opening",
                                            title: qsTrc("notation", "Comfortable hand opening (cm)"),
                                            min: 5,
                                            max: 50
                                        },
                                        {
                                            key: "reach",
                                            title: qsTrc("notation", "Comfortable arm reach (cm)"),
                                            min: 30,
                                            max: 100
                                        },
                                        {
                                            key: "shaft",
                                            title: qsTrc("notation", "Mallet shaft length (cm)"),
                                            min: 20,
                                            max: 60
                                        },
                                        {
                                            key: "head",
                                            title: qsTrc("notation", "Mallet head diameter (cm)"),
                                            min: 1,
                                            max: 8
                                        }
                                    ]
                                    RowLayout {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        Label {
                                            text: modelData.title
                                            Layout.fillWidth: true
                                        }
                                        Controls.SpinBox {
                                            implicitWidth: 84
                                            from: modelData.min
                                            to: modelData.max
                                            editable: true
                                            value: root.panelState[modelData.key] || modelData.min
                                            onValueModified: root.model.setOption(modelData.key, value)
                                            palette.text: ui.theme.fontPrimaryColor
                                            background: Rectangle {
                                                color: ui.theme.textFieldColor
                                                radius: 6
                                            }
                                        }
                                    }
                                }
                                Label {
                                    text: qsTrc("notation", "Body position • move left / right")
                                    Layout.fillWidth: true
                                }
                                Controls.Slider {
                                    Layout.fillWidth: true
                                    from: -50
                                    to: 50
                                    value: root.panelState.bodyOffset || 0
                                    onMoved: root.model.setOption("bodyOffset", value)
                                }
                                Label {
                                    text: qsTrc("notation", "Player options save automatically. Appearance changes only when this panel opens.")
                                    Layout.fillWidth: true
                                    opacity: .8
                                }
                            }
                            Label {
                                Layout.fillWidth: true
                                text: root.panelState.geometryNote || ""
                                opacity: .65
                            }
                        }
                    }
                }
            }
        }
    }
}
