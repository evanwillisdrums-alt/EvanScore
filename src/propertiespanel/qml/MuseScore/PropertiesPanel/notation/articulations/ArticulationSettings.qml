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
import QtQuick

import Muse.Ui
import Muse.UiComponents
import MuseScore.PropertiesPanel

import "../../common"

Column {
    id: root

    required property ArticulationSettingsModel model

    property NavigationPanel navigationPanel: null
    property int navigationRowStart: 1

    objectName: "ArticulationSettings"

    spacing: 12

    function focusOnFirst() {
        placementSection.focusOnFirst()
    }

    PlacementSection {
        id: placementSection

        enabled: root.model ? root.model.isPlacementAvailable : null

        titleText: qsTrc("propertiespanel", "Placement")
        propertyItem: root.model ? root.model.placement : null

        navigationPanel: root.navigationPanel
        navigationRowStart: root.navigationRowStart

        model: [
            { text: qsTrc("propertiespanel", "Auto"), value: ArticulationTypes.TYPE_AUTO },
            { text: qsTrc("propertiespanel", "Above"), value: ArticulationTypes.TYPE_TOP },
            { text: qsTrc("propertiespanel", "Below"), value: ArticulationTypes.TYPE_BOTTOM }
        ]
    }

    DirectionSection {
        id: directionSection

        objectName: "articulation-symbol-direction"
        enabled: root.model && root.model.isPlacementAvailable
        titleText: qsTrc("propertiespanel", "Symbol direction")
        width: parent.width
        propertyItem: root.model ? root.model.direction : null
        navigationPanel: root.navigationPanel
        navigationRowStart: placementSection.navigationRowEnd + 1
    }

    PropertyView {
        id: voiceSection

        objectName: "articulation-voice-section"
        width: parent.width
        visible: root.model && root.model.voiceChoices.length > 0
        titleText: qsTrc("propertiespanel", "Attach accent to voice")
        showButton: false
        navigationPanel: root.navigationPanel
        navigationRowStart: directionSection.navigationRowEnd + 1
        navigationRowEnd: navigationRowStart + 4
        spacing: 6

        Row {
            width: parent.width
            spacing: 6

            Repeater {
                model: root.model ? root.model.voiceChoices : []

                FlatRadioButton {
                    required property var modelData
                    required property int index

                    objectName: "articulation-voice-" + (modelData.voice + 1)
                    width: (parent.width - 18) / 4
                    text: modelData.text
                    checked: root.model.currentVoice === modelData.voice
                    enabled: modelData.available
                    toolTipTitle: qsTrc("propertiespanel", "Voice %1").arg(modelData.voice + 1)
                    toolTipDescription: qsTrc("propertiespanel", "Move this accent to an existing chord at the same position. Notes stay in their current voices.")
                    navigation.name: objectName
                    navigation.panel: root.navigationPanel
                    navigation.row: voiceSection.navigationRowStart + 1 + index
                    onToggled: root.model.moveToVoice(modelData.voice)
                }
            }
        }

        StyledTextLabel {
            width: parent.width
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
            text: qsTrc("propertiespanel", "Choose a voice with a chord at this position. Existing accents and linked parts are preserved.")
        }
    }
}
