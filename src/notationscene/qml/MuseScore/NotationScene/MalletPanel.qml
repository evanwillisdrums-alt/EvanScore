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
    property int selectedMallet: 1
    property bool showOtherOptions: false
    property bool showMeasurements: false
    property string shownSource: ""
    property bool shownPickMode: false
    function previewAlternative(index) {
        root.lastAlternative = index;
        root.model.selectAlternative(index);
    }
    onPanelStateChanged: {
        if (shownSource !== (panelState.sourceKey || "") || shownPickMode !== !!panelState.pickMode) {
            shownSource = panelState.sourceKey || "";
            shownPickMode = !!panelState.pickMode;
            lastAlternative = 0;
            showOtherOptions = false;
            info.contentY = 0;
        }
        const ids = panelState.pose?.mallets || [];
        if (ids.indexOf(selectedMallet) < 0) selectedMallet = ids.length ? ids[0] : 1;
    }
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
        id: control
        height: 28
        orientation: Qt.Horizontal
        minWidth: 28
        implicitWidth: Math.max(minWidth, buttonMetrics.advanceWidth(text)
                                + (icon === IconCode.NONE ? 0 : iconFont.pixelSize + (text.length ? 6 : 0))
                                + 2 * margins)
        margins: 6
        backgroundRadius: 6
        navigation.panel: panelNavigation
        FontMetrics {
            id: buttonMetrics
            font: control.textFont
        }
        // FlatButton's wrapping RowLayout can retain a squeezed implicit
        // caption width after dock recreation. Size this single-line native
        // content from its font, independently of the previous layout width.
        contentItem: Row {
            spacing: 6
            StyledIconLabel {
                visible: control.icon !== IconCode.NONE
                iconCode: control.icon
                font: control.iconFont
                color: control.iconColor
                width: visible ? control.iconFont.pixelSize : 0
                height: 28
            }
            StyledTextLabel {
                visible: control.text.length > 0
                text: control.text
                font: control.textFont
                textFormat: control.textFormat
                width: buttonMetrics.advanceWidth(text)
                height: 28
                wrapMode: Text.NoWrap
            }
        }
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        GridLayout {
            id: toolbar
            Layout.fillWidth: true
            columns: root.width < 760 ? 1 : 2
            Layout.preferredHeight: columns === 1 ? 62 : 34
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            rowSpacing: 3
            columnSpacing: 6
            RowLayout {
                Layout.fillWidth: toolbar.columns === 1
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
                    visible: root.panelState.pickMode
                    enabled: root.panelState.pickMode
                    transparent: true
                    navigation.order: 3
                    onClicked: root.model.clearPicked()
                }
                Item {
                    Layout.fillWidth: true
                    visible: toolbar.columns === 1
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 5
                Item { Layout.fillWidth: true }
                Button {
                    objectName: "mallet-original"
                    text: qsTrc("notation", "Original")
                    toolTipTitle: qsTrc("notation", "Original placement")
                    accentButton: root.panelState.selectedAlternative < 0
                    navigation.order: 4
                    onClicked: root.model.showOriginal()
                }
                Button {
                    text: qsTrc("notation", "Preview")
                    toolTipTitle: qsTrc("notation", "Preview alternative")
                    enabled: root.model.alternatives.length > 0
                    accentButton: root.panelState.selectedAlternative >= 0
                    navigation.order: 5
                    onClicked: root.previewAlternative(Math.min(root.lastAlternative, root.model.alternatives.length - 1))
                }
                Button {
                    objectName: "mallet-audition"
                    icon: IconCode.PLAY
                    toolTipTitle: qsTrc("notation", "Audition this chord")
                    toolTipDescription: root.panelState.audioStatus || ""
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
                    text: qsTrc("notation", "Apply to score")
                    toolTipDescription: root.panelState.commitReason || ""
                    enabled: root.panelState.canCommit
                    accentButton: true
                    navigation.order: 8
                    onClicked: root.model.commit()
                }
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
                            text: qsTrc("notation", "Focus")
                            toolTipDescription: qsTrc("notation", "Enlarge the active register and player. The overview shows the full instrument.")
                            transparent: !accentButton
                            accentButton: scene.focusPlacement
                            navigation.order: 9
                            onClicked: { scene.focusPlacement = true; scene.fit(); }
                        }
                        Button {
                            text: qsTrc("notation", "Full")
                            transparent: !accentButton
                            accentButton: !scene.focusPlacement
                            navigation.order: 10
                            onClicked: { scene.focusPlacement = false; scene.fit(); }
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
                            onClicked: scene.fit()
                        }
                    }
                    MalletSceneView {
                        id: scene
                        objectName: "mallet-instrument-view"
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 80
                        scene: root.panelState
                        backgroundColor: ui.theme.textFieldColor
                        Accessible.name: qsTrc("notation", "Mallet instrument and overhead player")
                        Accessible.description: qsTrc("notation", "In Pick on bars, click a bar to add or remove its note; the top overview also selects notes across the full range. Drag an occupied bar to move its strike point. Arrow keys explore; Space selects. Focus or Fit reframes the view; scroll zooms.")
                        onPitchClicked: function (pitch) {
                            root.model.togglePitch(pitch);
                        }
                        onMalletSelected: function(mallet) { root.selectedMallet = mallet; }
                        onStrikePointDragged: function (mallet, fraction) {
                            root.model.setStrikePoint(mallet, fraction);
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: scene.hoveredPitch >= 0 ? (root.panelState.bars || []).find(b => b.pitch === scene.hoveredPitch)?.label || "" : root.panelState.displayedChord || qsTrc("notation", "Click a chord")
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            wrapMode: Text.NoWrap
                            opacity: .8
                        }
                        Button {
                            objectName: "mallet-strike-center"
                            text: qsTrc("notation", "Center")
                            enabled: (root.panelState.pose?.mallets || []).indexOf(root.selectedMallet) >= 0
                            transparent: true
                            toolTipDescription: qsTrc("notation", "Move the selected mallet to the center of its bar.")
                            onClicked: root.model.setStrikePoint(root.selectedMallet, .5)
                        }
                        Button {
                            objectName: "mallet-strike-edge"
                            text: qsTrc("notation", "Near edge")
                            enabled: (root.panelState.pose?.mallets || []).indexOf(root.selectedMallet) >= 0
                            transparent: true
                            toolTipDescription: qsTrc("notation", "Use the end nearest the player. Access may improve, with a tone tradeoff; not a routine chord default.")
                            onClicked: root.model.setStrikePoint(root.selectedMallet, .98)
                        }
                        Button {
                            icon: IconCode.UNDO
                            toolTipTitle: qsTrc("notation", "Reset strike points")
                            transparent: true
                            onClicked: root.model.resetStrikePoints()
                        }
                    }
                }
            }
            Item {
                Controls.SplitView.preferredWidth: 365
                Controls.SplitView.minimumWidth: 312
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
                        objectName: "mallet-information-scroll"
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
                                visible: !!root.panelState.displayedChord
                                text: (root.panelState.selectedAlternative >= 0 ? qsTrc("notation", "Suggestion %1 · ").arg(root.panelState.selectedAlternative + 1) : qsTrc("notation", "Original · ")) + (root.panelState.displayedChord || "")
                                font: ui.theme.bodyBoldFont
                            }
                            Label {
                                Layout.fillWidth: true
                                text: root.panelState.status || ""
                                font: ui.theme.bodyBoldFont
                                color: root.panelState.pose?.uncertain ? "#afb6be" : root.panelState.pose?.severity === 2 ? "#ffb0ad" : root.panelState.pose?.severity === 1 ? "#f3d290" : ui.theme.fontPrimaryColor
                            }
                            Label {
                                objectName: "mallet-next-step"
                                Layout.fillWidth: true
                                text: root.panelState.nextStep || ""
                                opacity: .85
                            }
                            Label {
                                visible: (root.panelState.pose?.pitches || []).length > 0 && !root.panelState.canAudition
                                Layout.fillWidth: true
                                text: root.panelState.audioStatus || ""
                                opacity: .75
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
                                    text: root.panelState.selectionHelp || ""
                                    opacity: .75
                                }
                                Label {
                                    Layout.fillWidth: true
                                    visible: (root.panelState.pose?.pitches || []).length > 0
                                    text: root.panelState.sticking ? qsTrc("notation", "Written sticking: ") + root.panelState.sticking : qsTrc("notation", "No written sticking • suggested assignment")
                                    opacity: .8
                                }
                                Label {
                                    Layout.fillWidth: true
                                    visible: root.panelState.recommendKeep === true
                                    text: qsTrc("notation", "Keep the original. No modeled comfort conflict; alternatives are optional.")
                                    color: "#a5d6b8"
                                }
                                Repeater {
                                    model: (root.panelState.pose?.pitches || []).length ? root.panelState.pose?.cons || [] : []
                                    Label {required property string modelData; Layout.fillWidth: true; text: "• " + modelData; color: root.panelState.pose?.uncertain ? "#afb6be" : root.panelState.pose?.severity === 2 ? "#ffb0ad" : root.panelState.pose?.severity === 1 ? "#f3d290" : ui.theme.fontPrimaryColor}
                                }
                                Repeater {
                                    model: (root.panelState.pose?.pitches || []).length ? root.panelState.pose?.pros || [] : []
                                    Label {required property string modelData; Layout.fillWidth: true; text: "+ " + modelData; color: "#a5d6b8"}
                                }
                                Label {
                                    Layout.fillWidth: true
                                    visible: root.panelState.selectedAlternative >= 0
                                    text: qsTrc("notation", "Original: %1 · Preview: %2").arg(root.panelState.originalPose?.status || "").arg(root.panelState.pose?.status || "")
                                }
                                Repeater {
                                    model: root.panelState.pose?.voices || []
                                    RowLayout {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        Rectangle {
                                            implicitWidth: 8; implicitHeight: 8; radius: 4
                                            color: ["#5da9f4", "#72d0cf", "#f3c560", "#ed8a9e"][parent.modelData.mallet - 1]
                                        }
                                        Label {
                                            Layout.fillWidth: true
                                            text: (root.panelState.reverseNumbering ? 5-parent.modelData.mallet : parent.modelData.mallet) + " · " + (parent.modelData.position || (parent.modelData.mallet < 3 ? "L" : "R")) + " · " + parent.modelData.name + " · " + parent.modelData.zone
                                        }
                                        Button {
                                            text: qsTrc("notation", "Select")
                                            transparent: true
                                            accentButton: root.selectedMallet === parent.modelData.mallet
                                            onClicked: root.selectedMallet = parent.modelData.mallet
                                        }
                                    }
                                }
                                Label {
                                    Layout.fillWidth: true
                                    visible: (root.panelState.pose?.voices || []).length > 0
                                    text: qsTrc("notation", "Strike zones: green center · orange end access · dashed estimated nodes")
                                    opacity: .75
                                }
                                Button {
                                    objectName: "mallet-measurements-toggle"
                                    visible: (root.panelState.pose?.voices || []).length > 0
                                    text: root.showMeasurements ? qsTrc("notation", "Hide measurements and limits") : qsTrc("notation", "Measurements and limits")
                                    transparent: true
                                    onClicked: root.showMeasurements = !root.showMeasurements
                                }
                                Repeater {
                                    model: root.showMeasurements ? root.panelState.checks || [] : []
                                    Label {required property var modelData;Layout.fillWidth: true;text:modelData.label + ": " + modelData.detail;color:modelData.warning ? "#f3d290" : ui.theme.fontPrimaryColor}
                                }
                                Label {
                                    Layout.fillWidth: true
                                    visible: root.showMeasurements && (root.panelState.pose?.voices || []).length > 0
                                    text: qsTrc("notation", "Arm reach L %1 · R %2 cm\nHand clearance %3 · Shaft clearance %4 cm\nOuter 1–4 spread %5 cm").arg(Number(root.panelState.pose?.leftReach || 0).toFixed(1)).arg(Number(root.panelState.pose?.rightReach || 0).toFixed(1)).arg(Number(root.panelState.pose?.handClearance || 0).toFixed(1)).arg(Number(root.panelState.pose?.shaftClearance ?? -1) < 0 ? "—" : Number(root.panelState.pose.shaftClearance).toFixed(1)).arg(Number(root.panelState.pose?.outerSpread || 0).toFixed(1))
                                    opacity: .8
                                }
                                Label {
                                    Layout.fillWidth: true
                                    visible: !!root.panelState.previousPitches || !!root.panelState.nextPitches
                                    text: qsTrc("notation", "Before: %1 · After: %2\nTravel %3 cm · estimated preparation %4 ms").arg(root.panelState.previousPitches || "—").arg(root.panelState.nextPitches || "—").arg(Number(root.panelState.pose?.movement || 0).toFixed(1)).arg(Math.round(Number(root.panelState.pose?.preparation || 0) * 1000))
                                    opacity: .8
                                }
                                Repeater {
                                    model: root.panelState.contextNotes || []
                                    Label {required property string modelData;Layout.fillWidth:true;text:modelData;opacity:.7}
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
                                    text: qsTrc("notation", "Click any suggestion card to preview it. Apply to score writes the displayed notes and sticking in one Undo step. Strike points remain a preview.") + "\n" + (root.panelState.searchOrder || "")
                                    opacity: .8
                                }
                                Button {
                                    objectName: "mallet-explore-alternatives"
                                    visible: root.panelState.recommendKeep === true
                                    text: root.showOtherOptions ? qsTrc("notation", "Hide optional alternatives") : qsTrc("notation", "Explore optional alternatives")
                                    onClicked: root.showOtherOptions = !root.showOtherOptions
                                }
                                Label {
                                    visible: !root.panelState.canCommit
                                    Layout.fillWidth: true
                                    text: root.panelState.commitReason || ""
                                    opacity: .8
                                }
                                Label {
                                    visible: root.model.alternatives.length === 0
                                    Layout.fillWidth: true
                                    text: (root.panelState.pose?.pitches || []).length ? qsTrc("notation", "No valid alternatives under the current constraints. Review Diagnostics and Player: written sticking, protected bass/top notes, mallet count and octave permissions affect the choices.") : qsTrc("notation", "Select a score chord, or pick notes on the bars, to see suggestions.")
                                    opacity: .8
                                }
                                Repeater {
                                    model: root.panelState.recommendKeep && !root.showOtherOptions ? [] : root.model.alternatives
                                    Rectangle {
                                        required property var modelData
                                        objectName: "mallet-alternative-card-" + modelData.index
                                        Layout.fillWidth: true
                                        implicitHeight: candidateColumn.implicitHeight + 16
                                        radius: 7
                                        color: ui.theme.backgroundSecondaryColor
                                        border.width: root.panelState.selectedAlternative === modelData.index ? 2 : 0
                                        border.color: ui.theme.accentColor
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.previewAlternative(parent.modelData.index)
                                        }
                                        ColumnLayout {
                                            id: candidateColumn
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            anchors.top: parent.top
                                            anchors.margins: 8
                                            Button {
                                                objectName: "mallet-preview-" + modelData.index
                                                Layout.fillWidth: true
                                                text: qsTrc("notation", "Preview %1 · ").arg(modelData.index + 1) + modelData.pitches
                                                toolTipDescription: modelData.description
                                                accentButton: root.panelState.selectedAlternative === modelData.index
                                                transparent: !accentButton
                                                onClicked: {
                                                    root.previewAlternative(modelData.index);
                                                }
                                            }
                                            Label {
                                                Layout.fillWidth: true
                                                text: modelData.description
                                                opacity: .8
                                            }
                                            Label {
                                                Layout.fillWidth: true
                                                text: modelData.status + " · " + (modelData.mallets || []).map(m => root.panelState.reverseNumbering ? 5-m : m).join(" ")
                                                color: modelData.uncertain ? "#afb6be" : modelData.severity === 2 ? "#ffb0ad" : modelData.severity ? "#f3d290" : "#a5d6b8"
                                            }
                                            Repeater {
                                                model: modelData.changes || []
                                                Label { required property string modelData; Layout.fillWidth: true; text: modelData; opacity: .85 }
                                            }
                                            Repeater {
                                                model: modelData.pros || []
                                                Label { required property string modelData; Layout.fillWidth: true; text: "+ " + modelData; color: "#a5d6b8" }
                                            }
                                            Repeater {
                                                model: modelData.cons || []
                                                Label { required property string modelData; Layout.fillWidth: true; text: "• " + modelData; opacity: .8 }
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
                                    Layout.fillWidth: true
                                    text: qsTrc("notation", "Respect written sticking")
                                    checked: root.panelState.respectSticking === true
                                    onClicked: root.model.setOption("respectSticking", !checked)
                                    navigation.panel: panelNavigation
                                    navigation.order: 17
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: qsTrc("notation", "Physical positions: outer/inner left, inner/outer right. Written numbers are read from low to high within each chord, then follow their original voices through revoicing. ? denotes an unknown slot.")
                                    opacity: .8
                                }
                                Repeater {
                                    model: [
                                        {key:"reverseNumbering", text:qsTrc("notation", "Reverse numbered convention (4–3 | 2–1)")},
                                        {key:"optimizeStrikes", text:qsTrc("notation", "Suggest edge access when substantially helpful")},
                                        {key:"allowOctaves", text:qsTrc("notation", "Allow octave revoicing alternatives")},
                                        {key:"keepBass", text:qsTrc("notation", "Protect the original bass pitch")},
                                        {key:"keepMelody", text:qsTrc("notation", "Protect the original top melody pitch")},
                                        {key:"allowInversion", text:qsTrc("notation", "Allow a different bass pitch class")}
                                    ]
                                    UiComponents.CheckBox {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        text: modelData.text
                                        checked: root.panelState[modelData.key] === true
                                        onClicked: root.model.setOption(modelData.key, !checked)
                                        navigation.panel: panelNavigation
                                    }
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
                                        },
                                        {text:qsTrc("notation", "Three mallets (two left, one right)"),value:3}
                                    ]
                                    currentIndex: root.panelState.malletCount === 2 ? 1 : root.panelState.malletCount === 3 ? 2 : 0
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
                                        },
                                        {key:"rotation", title:qsTrc("notation", "Comfortable pair tilt (degrees)"), min:5, max:90},
                                        {key:"bodyDistance", title:qsTrc("notation", "Body distance from bar front (cm)"), min:15, max:60},
                                        {key:"accidentalHeight", title:qsTrc("notation", "Accidental elevation above naturals (cm)"), min:0, max:15},
                                        {key:"handWidth", title:qsTrc("notation", "Hand width (cm)"), min:4, max:15},
                                        {key:"travelSpeed", title:qsTrc("notation", "Estimated travel speed (cm/s)"), min:30, max:600}
                                    ]
                                    RowLayout {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        Label {
                                            text: modelData.title
                                            Layout.fillWidth: true
                                        }
                                        Controls.SpinBox {
                                            objectName: "mallet-player-" + modelData.key
                                            implicitWidth: 84
                                            from: modelData.min
                                            to: modelData.max
                                            editable: true
                                            value: root.panelState[modelData.key] || modelData.min
                                            onValueModified: root.model.setOption(modelData.key, value)
                                            Keys.onShortcutOverride: function (event) {
                                                if (!activeFocus && !contentItem.activeFocus)
                                                    return;
                                                if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter
                                                    || event.key === Qt.Key_Escape || event.key === Qt.Key_Backspace
                                                    || event.key === Qt.Key_Delete || event.key === Qt.Key_Left
                                                    || event.key === Qt.Key_Right || event.key === Qt.Key_Up
                                                    || event.key === Qt.Key_Down || event.key === Qt.Key_Home
                                                    || event.key === Qt.Key_End || (event.text.length > 0
                                                        && !(event.modifiers & (Qt.ControlModifier | Qt.MetaModifier | Qt.AltModifier)))) {
                                                    event.accepted = true;
                                                }
                                            }
                                            palette.text: ui.theme.fontPrimaryColor
                                            background: Rectangle {
                                                color: ui.theme.textFieldColor
                                                radius: 6
                                            }
                                        }
                                    }
                                }
                                Label {
                                    text: qsTrc("notation", "Body position • offset from automatic follow")
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
                                    text: qsTrc("notation", "Grip and comfort limits save automatically. Appearance changes only when this panel opens.")
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
