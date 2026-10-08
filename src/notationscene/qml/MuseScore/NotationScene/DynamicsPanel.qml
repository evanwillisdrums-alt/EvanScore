/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Muse.Ui
import MuseScore.NotationScene
Item {
    id: root
    objectName: "DynamicsPanel"
    property var navigationSection: null
    property int navigationOrderStart: 0
    property alias model: dynamicsModel
    readonly property var state: model.state
    property string lastScope: ""
    onStateChanged: {
        if (state.scope !== lastScope) {
            if (scroll.contentItem) scroll.contentItem.contentY = 0
            lastScope = state.scope || ""
        }
    }
    Rectangle { anchors.fill: parent; color: ui.theme.backgroundPrimaryColor }
    function dynamicIndex(value) {
        const choices = model.dynamicChoices
        for (let i = 0; i < choices.length; ++i) if (choices[i].value === value) return i
        return 0
    }
    DynamicsPanelModel { id: dynamicsModel }
    Component.onCompleted: model.load()
    component PanelButton: Button {
        font.pixelSize: 12
        implicitHeight: 30
        leftPadding: 9; rightPadding: 9
        palette.buttonText: checked ? "white" : ui.theme.fontPrimaryColor
        background: Rectangle { radius: 8; color: parent.checked ? ui.theme.accentColor : ui.theme.buttonColor; opacity: parent.down ? .65 : 1 }
        Accessible.name: text
    }
    component PanelCombo: ComboBox {
        implicitHeight: 30
        font.pixelSize: 12
        background: Rectangle { radius: 8; color: ui.theme.buttonColor }
        contentItem: Text { text: parent.displayText; color: ui.theme.fontPrimaryColor; font: parent.font; leftPadding: 9; rightPadding: 24; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
        indicator: Text { text: "⌄"; color: ui.theme.fontPrimaryColor; font.pixelSize: 14; x: parent.width - 20; y: 5 }
        delegate: ItemDelegate {
            required property var modelData
            width: parent ? parent.width : 100
            text: typeof modelData === "string" ? modelData : modelData.text
            palette.text: ui.theme.fontPrimaryColor
            background: Rectangle { radius: 6; color: parent.highlighted ? ui.theme.accentColor : ui.theme.backgroundSecondaryColor }
        }
        popup.background: Rectangle { radius: 8; color: ui.theme.backgroundSecondaryColor }
    }
    component LevelSpin: SpinBox {
        from: 0; to: 127; editable: true
        implicitHeight: 30; implicitWidth: 62
        font.pixelSize: 12
        palette.text: ui.theme.fontPrimaryColor
        palette.buttonText: ui.theme.fontPrimaryColor
        background: Rectangle { radius: 7; color: ui.theme.backgroundPrimaryColor }
    }
    component Caption: Text {
        color: ui.theme.fontPrimaryColor; font.pixelSize: 12
        wrapMode: Text.WordWrap
    }
    ScrollView {
        id: scroll
        objectName: "dynamics-scroll"
        anchors.fill: parent
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ColumnLayout {
            width: root.width - 24
            x: 12
            spacing: 12
            Item { Layout.preferredHeight: 1 }
            RowLayout {
                Layout.fillWidth: true
                PanelButton { text: qsTrc("notation", "Full score"); checked: root.state.scope === "score"; onClicked: root.model.showScore() }
                PanelButton { text: qsTrc("notation", "Selection"); checked: root.state.scope !== "score"; onClicked: root.model.followSelection() }
            }
            Caption { text: root.state.title || ""; font.bold: true; font.pixelSize: 16; Layout.fillWidth: true }
            Caption {
                Layout.fillWidth: true; opacity: .7
                text: root.state.scope === "score" ? qsTrc("notation", "Score defaults. Click a note or hairpin to edit just that selection.") : qsTrc("notation", "Only the selected music changes. Score mappings remain separate.")
            }
            ColumnLayout {
                visible: root.state.available
                Layout.fillWidth: true
                spacing: 10
                CheckBox {
                    objectName: "dynamics-enabled"
                    visible: root.state.scope === "score"
                    text: qsTrc("notation", "Use custom playback dynamics")
                    checked: root.state.enabled || false
                    onClicked: root.model.setEnabled(checked)
                    palette.windowText: ui.theme.fontPrimaryColor; font.pixelSize: 12
                }
                CheckBox {
                    visible: root.state.scope === "score"
                    text: qsTrc("notation", "Separate battery accents and taps")
                    checked: root.state.battery || false
                    onClicked: root.model.setBattery(checked)
                    palette.windowText: ui.theme.fontPrimaryColor; font.pixelSize: 12
                }
                Rectangle {
                    visible: root.state.scope === "score"
                    Layout.fillWidth: true
                    implicitHeight: presetCard.implicitHeight + 20
                    radius: 10; color: ui.theme.backgroundSecondaryColor
                    ColumnLayout {
                        id: presetCard
                        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                        anchors.margins: 10
                        spacing: 7
                        Caption { text: qsTrc("notation", "Presets & defaults"); font.bold: true }
                        RowLayout {
                            PanelButton { objectName: "dynamics-save-preset"; text: qsTrc("notation", "Save Preset"); Layout.fillWidth: true; onClicked: root.model.savePreset() }
                            PanelButton { objectName: "dynamics-load-preset"; text: qsTrc("notation", "Load Preset"); Layout.fillWidth: true; onClicked: root.model.loadPreset() }
                        }
                        PanelButton { objectName: "dynamics-make-default"; text: qsTrc("notation", "Make Default Dynamics"); Layout.fillWidth: true; onClicked: root.model.makeDefault() }
                        PanelButton { objectName: "dynamics-apply-default"; text: qsTrc("notation", "Apply Default Dynamics"); enabled: root.state.hasDefault === true; Layout.fillWidth: true; onClicked: root.model.applyDefault() }
                        Caption { text: root.state.notice || qsTrc("notation", "Defaults apply to new scores. Presets contain playback settings only."); Layout.fillWidth: true; opacity: .7; font.pixelSize: 11; Accessible.role: Accessible.StaticText }
                    }
                }
                Rectangle {
                    visible: (root.state.scope === "score" || root.state.scope === "hairpin")
                    Layout.fillWidth: true
                    implicitHeight: curveCard.implicitHeight + 20
                    radius: 10; color: ui.theme.backgroundSecondaryColor
                    ColumnLayout {
                        id: curveCard
                        anchors.left: parent.left; anchors.right: parent.right
                        anchors.margins: 10; anchors.top: parent.top
                        spacing: 8
                        Caption { text: root.state.scope === "hairpin" ? qsTrc("notation", "Playback curve") : qsTrc("notation", "Default hairpin curve"); font.bold: true }
                        DynamicsCurve {
                            id: curve
                            Layout.fillWidth: true
                            shape: root.state.shape || 0
                            bend: root.state.bend || 0
                            startLevel: root.state.startLevel === undefined ? 49 : root.state.startLevel
                            endLevel: root.state.endLevel === undefined ? 112 : root.state.endLevel
                            tapStartLevel: root.state.tapStartLevel === undefined ? 49 : root.state.tapStartLevel
                            tapEndLevel: root.state.tapEndLevel === undefined ? 49 : root.state.tapEndLevel
                            showTapCurve: root.state.scope === "hairpin" && root.state.batteryHairpin
                            endpointsEditable: root.state.scope === "hairpin"
                            onCurveEdited: function(shape, bend) { root.model.setCurve(shape, bend) }
                            onEndpointEdited: function(end, velocity) { root.model.setEndpoint(end, end ? root.state.endDynamic : root.state.startDynamic, end ? root.state.endRole : root.state.startRole, velocity) }
                        }
                        Caption { visible: curve.showTapCurve; text: qsTrc("notation", "Blue: endpoint curve  •  Dashed: tap levels"); opacity: .7; font.pixelSize: 10; Layout.fillWidth: true }
                        Flow {
                            Layout.fillWidth: true
                            spacing: 5
                            Repeater {
                                model: [qsTrc("notation", "Linear"), qsTrc("notation", "Early"), qsTrc("notation", "Late"), qsTrc("notation", "S-curve"), qsTrc("notation", "Delayed")]
                                PanelButton {
                                    required property string modelData
                                    required property int index
                                    objectName: "dynamics-preset-" + index
                                    text: modelData
                                    onClicked: root.model.setCurve(index < 3 ? 0 : index === 3 ? 1 : 2, index === 1 ? -.7 : index === 2 ? .7 : 0)
                                }
                            }
                        }
                        Caption { text: qsTrc("notation", "Drag the curve • arrows fine-tune • double-click resets"); Layout.fillWidth: true; opacity: .6; font.pixelSize: 10 }
                        Slider {
                            objectName: "dynamics-bend-slider"
                            Layout.fillWidth: true
                            from: -2; to: 2; stepSize: .01
                            value: root.state.bend || 0
                            onMoved: curve.previewBend = value
                            onPressedChanged: if (!pressed) root.model.setCurve(curve.shape, value)
                            Accessible.name: qsTrc("notation", "Curve bend")
                        }
                        Caption { text: qsTrc("notation", "Bend: %1").arg(Number(curve.previewBend).toFixed(2)); opacity: .7 }
                        Caption {
                            visible: root.state.scope === "hairpin" && ((root.state.startLevel > root.state.endLevel && root.state.isCrescendo) || (root.state.startLevel < root.state.endLevel && !root.state.isCrescendo))
                            text: qsTrc("notation", "The endpoint levels reverse this hairpin's direction."); color: "#d09544"; Layout.fillWidth: true
                        }
                    }
                }
                Repeater {
                    model: root.state.scope === "hairpin" ? 2 : 0
                    ColumnLayout {
                        id: endpoint
                        required property int index
                        readonly property bool end: index === 1
                        readonly property int selectedDynamic: end ? root.state.endDynamic : root.state.startDynamic
                        readonly property int selectedRole: end ? root.state.endRole : root.state.startRole
                        function save(dynamic, role, velocity) { root.model.setEndpoint(end, dynamic, role, velocity) }
                        Layout.fillWidth: true
                        Caption { text: endpoint.end ? qsTrc("notation", "End level") : qsTrc("notation", "Start level"); font.bold: true }
                        RowLayout {
                            PanelCombo {
                                id: marking
                                objectName: "dynamics-endpoint-marking-" + endpoint.index
                                Layout.fillWidth: true
                                model: root.model.dynamicChoices; textRole: "text"; valueRole: "value"
                                currentIndex: root.dynamicIndex(endpoint.selectedDynamic)
                                onActivated: endpoint.save(root.model.dynamicChoices[currentIndex].value, endpoint.selectedRole, -1)
                                Accessible.name: endpoint.end ? qsTrc("notation", "End dynamic") : qsTrc("notation", "Start dynamic")
                            }
                            PanelCombo {
                                Layout.preferredWidth: 92
                                model: [qsTrc("notation", "Auto"), qsTrc("notation", "Normal"), qsTrc("notation", "Tap"), qsTrc("notation", "Accent"), qsTrc("notation", "Tenuto"), qsTrc("notation", "Marcato"), qsTrc("notation", "Ghost"), qsTrc("notation", "Soft accent"), qsTrc("notation", "Stress"), qsTrc("notation", "Unstress")]
                                currentIndex: endpoint.selectedRole || 0
                                onActivated: endpoint.save(endpoint.selectedDynamic, currentIndex, -1)
                                Accessible.name: qsTrc("notation", "Endpoint note category")
                            }
                        }
                        RowLayout {
                            Caption { text: qsTrc("notation", "Velocity"); Layout.fillWidth: true }
                            LevelSpin {
                                value: endpoint.end ? root.state.endLevel : root.state.startLevel
                                onValueModified: endpoint.save(endpoint.selectedDynamic, endpoint.selectedRole, value)
                                Accessible.name: endpoint.end ? qsTrc("notation", "End velocity") : qsTrc("notation", "Start velocity")
                            }
                            PanelButton { text: qsTrc("notation", "Auto"); onClicked: endpoint.save(-1, 0, -1) }
                        }
                    }
                }
                ColumnLayout {
                    visible: (root.state.scope === "notes" || root.state.scope === "dynamic")
                    Layout.fillWidth: true
                    Caption { text: qsTrc("notation", "Note playback"); font.bold: true }
                    Caption { text: root.state.mixed ? qsTrc("notation", "Mixed values — editing applies to all selected notes.") : root.state.localOverride ? qsTrc("notation", "Local override") : qsTrc("notation", "Following score dynamics"); Layout.fillWidth: true; opacity: .7 }
                    Caption { visible: root.state.scope === "notes"; text: qsTrc("notation", "Category: %1").arg(root.state.noteCategory || ""); opacity: .7 }
                    RowLayout {
                        Caption { text: qsTrc("notation", "Velocity"); Layout.fillWidth: true }
                        LevelSpin { objectName: "dynamics-note-velocity"; value: root.state.effectiveVelocity === undefined ? 80 : root.state.effectiveVelocity; onValueModified: root.model.setNoteVelocity(value); Accessible.name: qsTrc("notation", "Selected note velocity") }
                    }
                    Slider {
                        objectName: "dynamics-note-slider"
                        Layout.fillWidth: true
                        from: 0; to: 127; stepSize: 1; value: root.state.effectiveVelocity === undefined ? 80 : root.state.effectiveVelocity
                        onPressedChanged: if (!pressed) root.model.setNoteVelocity(Math.round(value))
                        Accessible.name: qsTrc("notation", "Selected note velocity")
                    }
                    CheckBox { text: qsTrc("notation", "Play selected notes"); checked: root.state.notePlay === true; onClicked: root.model.setNotePlayback(checked); palette.windowText: ui.theme.fontPrimaryColor }
                    ColumnLayout {
                        visible: root.state.scope === "notes"
                        Layout.fillWidth: true
                        Caption { text: qsTrc("notation", "Adjust selection"); font.bold: true }
                        PanelCombo {
                            id: batchCategory; Layout.fillWidth: true
                            model: [qsTrc("notation", "All selected notes"), qsTrc("notation", "Selected taps"), qsTrc("notation", "Selected tenuto"), qsTrc("notation", "Selected accents"), qsTrc("notation", "Selected marcato"), qsTrc("notation", "Selected ghost notes"), qsTrc("notation", "Selected normal notes"), qsTrc("notation", "Selected soft accents"), qsTrc("notation", "Selected stress"), qsTrc("notation", "Selected unstress")]
                        }
                        RowLayout {
                            PanelCombo {
                                id: batchOperation; Layout.fillWidth: true
                                model: [qsTrc("notation", "Add / subtract"), qsTrc("notation", "Scale %"), qsTrc("notation", "Follow score ±%")]
                            }
                            SpinBox { id: batchAmount; from: batchOperation.currentIndex === 1 ? 0 : -100; to: batchOperation.currentIndex === 0 ? 127 : 500; value: batchOperation.currentIndex === 1 ? 100 : 0; editable: true; implicitWidth: 76; palette.text: ui.theme.fontPrimaryColor; background: Rectangle { radius: 7; color: ui.theme.backgroundPrimaryColor } }
                        }
                        PanelButton {
                            text: qsTrc("notation", "Apply to selection"); Layout.fillWidth: true
                            onClicked: root.model.adjustNotes(batchOperation.currentIndex, batchAmount.value, [0, 2, 4, 3, 5, 6, 1, 7, 8, 9][batchCategory.currentIndex])
                        }
                    }
                }
                PanelButton {
                    visible: root.state.scope === "notes" || root.state.scope === "hairpin" || root.state.scope === "dynamic"
                    text: qsTrc("notation", "Use score defaults")
                    Layout.fillWidth: true
                    onClicked: root.model.resetSelection()
                }
                ColumnLayout {
                    visible: root.state.scope === "score"
                    Layout.fillWidth: true
                    spacing: 6
                    RowLayout {
                        Caption { text: qsTrc("notation", "Dynamic mappings"); font.bold: true; Layout.fillWidth: true }
                        PanelButton { text: qsTrc("notation", "Reset"); onClicked: root.model.resetMappings() }
                    }
                    TextField { id: search; Layout.fillWidth: true; placeholderText: qsTrc("notation", "Find a dynamic, e.g. mp or ff"); Accessible.name: placeholderText }
                    Repeater {
                        // Stable row identities preserve focus while the velocity data changes.
                        model: root.model.dynamicChoices.slice(1).map(function(choice) { return {name: choice.text, dynamic: choice.value}; })
                        Rectangle {
                            id: mapping
                            required property var modelData
                            required property int index
                            readonly property var values: root.model.mappings[index] || ({})
                            visible: search.text === "" || modelData.name.indexOf(search.text.toLowerCase()) >= 0
                            Layout.fillWidth: true
                            implicitHeight: mappingCard.implicitHeight + 20
                            radius: 10; color: ui.theme.backgroundSecondaryColor
                            ColumnLayout {
                                id: mappingCard
                                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                                anchors.margins: 10
                                spacing: 7
                                Caption { text: mapping.modelData.name; font.bold: true; font.italic: true; font.pixelSize: 17; Accessible.name: qsTrc("notation", "%1 dynamic velocities").arg(text) }
                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 3; columnSpacing: 6; rowSpacing: 7
                                    Repeater {
                                        model: [
                                            {key: "tap", role: 2, label: qsTrc("notation", "Tap")},
                                            {key: "tenuto", role: 4, label: qsTrc("notation", "Tenuto")},
                                            {key: "accent", role: 3, label: qsTrc("notation", "Accent")},
                                            {key: "marcato", role: 5, label: qsTrc("notation", "Marcato")},
                                            {key: "ghost", role: 6, label: qsTrc("notation", "Ghost")},
                                            {key: "normal", role: 1, label: qsTrc("notation", "Normal")},
                                            {key: "softAccent", role: 7, label: qsTrc("notation", "Soft accent")},
                                            {key: "stress", role: 8, label: qsTrc("notation", "Stress")},
                                            {key: "unstress", role: 9, label: qsTrc("notation", "Unstress")}
                                        ]
                                        ColumnLayout {
                                            id: mappingValue
                                            required property var modelData
                                            Layout.fillWidth: true
                                            spacing: 3
                                            Caption { text: mappingValue.modelData.label; font.pixelSize: 11; opacity: .7 }
                                            LevelSpin {
                                                objectName: "dynamics-mapping-" + mapping.modelData.name + "-" + mappingValue.modelData.key
                                                Layout.fillWidth: true
                                                value: mapping.values[mappingValue.modelData.key] || 0
                                                onValueModified: root.model.setMapping(mapping.modelData.dynamic, mappingValue.modelData.role, value)
                                                Accessible.name: mapping.modelData.name + " " + mappingValue.modelData.label + " " + qsTrc("notation", "velocity")
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Caption { Layout.fillWidth: true; text: qsTrc("notation", "Values are MIDI velocities, 0–127 (0 is silent). Battery taps default to piano; ghost notes use quiet levels. Accent and marcato are separate; combined articulations use marcato, then accent, then tenuto. Staccato and staccatissimo retain their note-length behavior. Score settings and local edits save with your score."); opacity: .6; font.pixelSize: 11 }
                }
            }
            Item { Layout.preferredHeight: 12 }
        }
    }
}
