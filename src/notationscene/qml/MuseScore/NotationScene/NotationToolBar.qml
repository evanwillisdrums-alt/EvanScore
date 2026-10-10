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

import MuseScore.NotationScene

Row {
    id: root
    property alias isCompactMode: toolBarModel.isCompactMode
    property alias navigationPanel: toolBar.navigationPanel
    spacing: 2
    PercussionWorkspaceModel {
        id: workspace
        Component.onCompleted: load(true)
    }
    FlatButton {
        width: 28
        height: 28
        icon: IconCode.PERCUSSION
        transparent: !workspace.percussionMode
        accentButton: workspace.percussionMode
        toolTipTitle: qsTrc("notation", "Percussion mode")
        toolTipDescription: workspace.percussionMode ? qsTrc("notation", "On — click to use regular mode") : qsTrc("notation", "Off — click to enable percussion mode")
        accessible.name: toolTipTitle + ": " + (workspace.percussionMode ? qsTrc("global", "On") : qsTrc("global", "Off"))
        backgroundRadius: 5
        navigation.panel: toolBar.navigationPanel
        navigation.order: 0
        onClicked: workspace.percussionMode = !workspace.percussionMode
    }
    StyledToolBarView {
        id: toolBar

        navigationPanel.name: "NotationToolBar"
        navigationPanel.accessible.name: qsTrc("notation", "Notation toolbar")

        spacing: 2

        NotationToolBarModel {
            id: toolBarModel
        }

        model: toolBarModel

        sourceComponentCallback: function (type) {
            return type === ToolBarItemType.ACTION ? roundedActionComponent : null;
        }

        Component {
            id: roundedActionComponent

            StyledToolBarItem {
                width: 28
                height: 28
                backgroundRadius: 5
            }
        }
    }
}
