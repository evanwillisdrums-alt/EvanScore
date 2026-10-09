/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import Muse.Ui
import Muse.UiComponents
import MuseScore.NotationScene

StyledDialogView {
    id: root
    objectName: "feature-debug-dialog"
    title: qsTrc("notation", "EvanScore feature diagnostics")
    contentWidth: 760
    contentHeight: 580
    resizable: true
    margins: 16

    FeatureDebugModel { id: debugModel; Component.onCompleted: load() }
    NavigationPanel {
        id: controlsNavigation
        name: "Feature diagnostics controls"
        section: root.navigationSection
        order: 1
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        StyledTextLabel {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
            text: qsTrc("notation", "Select notes to inspect their sticking. This report distinguishes recognized markings from features still awaiting playback integration.")
        }
        RowLayout {
            Layout.fillWidth: true
            CheckBox {
                objectName: "feature-debug-full-score"
                text: qsTrc("notation", "Full score")
                checked: debugModel.fullScore
                navigation.panel: controlsNavigation
                navigation.order: 1
                onClicked: debugModel.fullScore = !debugModel.fullScore
            }
            Item { Layout.fillWidth: true }
            FlatButton {
                objectName: "feature-debug-refresh"
                text: qsTrc("global", "Refresh")
                backgroundRadius: 8
                navigation.panel: controlsNavigation
                navigation.order: 2
                onClicked: debugModel.refresh()
            }
            FlatButton {
                objectName: "feature-debug-copy"
                text: qsTrc("notation", "Copy report")
                Layout.minimumWidth: 148
                backgroundRadius: 8
                navigation.panel: controlsNavigation
                navigation.order: 3
                onClicked: debugModel.copyReport()
            }
        }
        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            Controls.TextArea {
                objectName: "feature-debug-report"
                text: debugModel.report
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.WrapAnywhere
                font: ui.theme.bodyFont
                color: ui.theme.fontPrimaryColor
                padding: 12
                background: Rectangle {
                    radius: 10
                    color: ui.theme.backgroundPrimaryColor
                    border.color: ui.theme.strokeColor
                }
            }
        }
    }
}
