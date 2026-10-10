/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
import QtQuick
import QtQuick.Layouts
import Muse.Ui
import Muse.UiComponents
import MuseScore.Playback

StyledPopupView {
    id: root
    property InputResourceItem resourceItem: null
    property string soundName: ""
    contentWidth: 360
    contentHeight: column.implicitHeight

    ColumnLayout {
        id: column
        width: 360
        spacing: 12
        StyledTextLabel {
            Layout.fillWidth: true
            font: ui.theme.bodyBoldFont
            text: qsTrc("playback", "Save a prepared VDL sound")
            horizontalAlignment: Text.AlignLeft
        }
        StyledTextLabel {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignLeft
            text: qsTrc("playback", "First load one VDL patch in Kontakt and close its editor. Save it here to reuse its settings from the Mixer without opening Kontakt each time.")
        }
        TextInputField {
            Layout.fillWidth: true
            currentText: root.soundName
            hint: qsTrc("playback", "For example, VDL SnareLine Manual LITE")
            onTextChanged: function(text) { root.soundName = text }
            Keys.onShortcutOverride: function(event) {
                if (!(event.modifiers & (Qt.ControlModifier | Qt.MetaModifier | Qt.AltModifier))) event.accepted = true
            }
        }
        StyledTextLabel {
            Layout.fillWidth: true
            visible: text.length > 0
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignLeft
            text: root.resourceItem ? root.resourceItem.vdlStatus : ""
        }
        RowLayout {
            CheckBox {
                id: snareManual
                text: qsTrc("playback", "SnareLine Manual / LITE translation")
                checked: false
            }
        }
        StyledTextLabel {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignLeft
            text: qsTrc("playback", "Enable this only for SnareLine Manual or Manual LITE. Hits, shots, rims and backsticks use written R/L; center/halfway/edge, snares on/off and line/solo use staff text. Other patches retain their original MIDI mapping.")
        }
        RowLayout {
            Layout.alignment: Qt.AlignRight
            FlatButton { text: qsTrc("global", "Close"); onClicked: root.close() }
            FlatButton {
                text: qsTrc("global", "Save")
                enabled: root.resourceItem && root.soundName.trim().length > 0
                onClicked: root.resourceItem.savePreparedVdlSound(root.soundName, snareManual.checked)
            }
        }
    }
}
