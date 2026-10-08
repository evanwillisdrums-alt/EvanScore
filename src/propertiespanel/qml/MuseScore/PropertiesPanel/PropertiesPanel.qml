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
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents
import MuseScore.PropertiesPanel

Rectangle {
    id: root

    property alias model: sectionList.model
    property var notationView: null

    property NavigationSection navigationSection: null
    property int navigationOrderStart: 1

    color: ui.theme.backgroundPrimaryColor

    onVisibleChanged: {
        propertiesPanelListModel.setPropertiesPanelVisible(root.visible)
    }

    function focusFirstItem() {
        var item = sectionList.itemAtIndex(0)
        if (item) {
            item.navigationPanel.requestActive()
        }
    }

    PropertiesPanelPopupControllerModel {
        id: popupController
    }

    onNotationViewChanged: {
        popupController.setNotationView(root.notationView)
    }

    StyledListView {
        id: sectionList
        anchors.fill: parent

        topMargin: 12
        bottomMargin: 12

        spacing: 8

        interactive: !popupController.isAnyPopupOpen

        function ensureContentVisible(invisibleContentHeight) {
            if (sectionList.contentY + invisibleContentHeight > 0) {
                sectionList.contentY += invisibleContentHeight
            } else {
                sectionList.contentY = 0
            }
        }

        Behavior on contentY {
            NumberAnimation { duration: 250 }
        }

        model: PropertiesPanelListModel {
            id: propertiesPanelListModel
        }

        onContentHeightChanged: {
            returnToBounds()

            if (contentHeight > cacheBuffer) {
                cacheBuffer = contentHeight
            }
        }

        onContentYChanged: {
            popupController.repositionPopupIfNeed()
        }

        delegate: Item {
            id: delegateItem

            required property PropertiesPanelAbstractModel propertiesPanelSectionModel
            required property int index

            width: ListView.view.width
            height: _item.implicitHeight + 20

            property var navigationPanel: _item.navigationPanel

            Rectangle {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                radius: 10
                color: ui.theme.backgroundSecondaryColor
                opacity: 0.45
            }

            PropertiesPanelSectionDelegate {
                id: _item

                anchors.top: parent.top
                anchors.topMargin: 10
                anchors.left: parent.left
                anchors.leftMargin: 18
                anchors.right: parent.right
                anchors.rightMargin: 18

                sectionModel: delegateItem.propertiesPanelSectionModel
                anchorItem: root
                navigationPanel.section: root.navigationSection
                navigationPanel.order: root.navigationOrderStart + delegateItem.index

                onEnsureContentVisibleRequested: function(invisibleContentHeight) {
                    sectionList.ensureContentVisible(invisibleContentHeight)
                }
            }
        }
    }
}
