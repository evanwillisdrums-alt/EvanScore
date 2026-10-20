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
#include "articulationsettingsmodel.h"

#include "engraving/dom/articulation.h"
#include "engraving/dom/tapping.h"
#include "engraving/dom/chord.h"
#include "engraving/editing/editvoice.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationselection.h"

#include "log.h"
#include "translation.h"

using namespace mu::propertiespanel;
using namespace mu::engraving;

ArticulationSettingsModel::ArticulationSettingsModel(QObject* parent, const muse::modularity::ContextPtr& iocCtx,
                                                     IElementRepositoryService* repository, PropertiesPanelModelType type)
    : PropertiesPanelAbstractModel(parent, iocCtx, repository)
{
    setModelType(type);
    setTitle(type == PropertiesPanelModelType::TYPE_ARTICULATION ? muse::qtrc("propertiespanel",
                                                                              "Articulation") : muse::qtrc("propertiespanel", "Tapping"));
    setIcon(muse::ui::IconCode::Code::ARTICULATION);
    createProperties();
}

void ArticulationSettingsModel::createProperties()
{
    m_placement = buildPropertyItem(mu::engraving::Pid::ARTICULATION_ANCHOR);
    m_direction = buildPropertyItem(mu::engraving::Pid::DIRECTION);
}

void ArticulationSettingsModel::requestElements()
{
    m_elementList = m_repository->findElementsByType(mu::engraving::ElementType::ARTICULATION);

    QList<mu::engraving::EngravingItem*> tappings = m_repository->findElementsByType(mu::engraving::ElementType::TAPPING);
    for (mu::engraving::EngravingItem* tapping : tappings) {
        m_elementList.push_back(tapping);
    }

    QList<mu::engraving::EngravingItem*> halfSlurSegs
        = m_repository->findElementsByType(mu::engraving::ElementType::TAPPING_HALF_SLUR_SEGMENT);
    for (mu::engraving::EngravingItem* halfSlurSeg : halfSlurSegs) {
        TappingHalfSlur* halfSlur = toTappingHalfSlur(toTappingHalfSlurSegment(halfSlurSeg)->spanner());
        Tapping* tapping = halfSlur->tapping();
        m_elementList.push_back(tapping);
    }
}

void ArticulationSettingsModel::loadProperties()
{
    loadPropertyItem(m_placement);
    loadPropertyItem(m_direction);
    updateIsPlacementAvailable();
    updateVoiceChoices();
}

void ArticulationSettingsModel::onNotationChanged(const PropertyIdSet&, const StyleIdSet&)
{
    updateProperties();
}

void ArticulationSettingsModel::updateVoiceChoices()
{
    QVariantList choices;
    int current = -1;
    if (m_elementList.size() == 1 && m_elementList.front()->isArticulation()) {
        auto* articulation = toArticulation(m_elementList.front());
        if ((articulation->isAccent() || articulation->isMarcato()) && articulation->chordRest()
            && articulation->chordRest()->isChord() && !toChord(articulation->chordRest())->isGrace()) {
            current = int(articulation->voice());
            for (int voice = 0; voice < VOICES; ++voice) {
                choices << QVariantMap { { "voice", voice }, { "text", QString::number(voice + 1) },
                    { "available", EditVoice::canMoveArticulationToVoice(articulation, voice) } };
            }
        }
    }
    if (choices != m_voiceChoices || current != m_currentVoice) {
        m_voiceChoices = choices;
        m_currentVoice = current;
        emit voiceChoicesChanged();
    }
}

void ArticulationSettingsModel::moveToVoice(int voice)
{
    const auto notation = currentNotation();
    if (!notation || voice < 0 || voice >= VOICES || m_elementList.size() != 1
        || !m_elementList.front()->isArticulation()) {
        return;
    }
    const auto& selected = notation->interaction()->selection()->elements();
    if (selected.size() != 1 || selected.front() != m_elementList.front()
        || !EditVoice::canMoveArticulationToVoice(toArticulation(selected.front()), voice)) {
        return;
    }
    notation->interaction()->changeSelectedElementsVoice(voice);
}

void ArticulationSettingsModel::updateIsPlacementAvailable()
{
    bool available = false;
    for (EngravingItem* item : m_elementList) {
        if (!item->isTapping()) {
            available = true;
            break;
        }

        Tapping* tapping = toTapping(item);
        bool hasSymbol = tapping->hand() == TappingHand::RIGHT || tapping->lhShowItems() != LHTappingShowItems::HALF_SLUR;
        if (hasSymbol) {
            available = true;
            break;
        }
    }

    if (available != m_isPlacementAvailable) {
        m_isPlacementAvailable = available;
        emit isPlacementAvailableChanged(m_isPlacementAvailable);
    }
}

PropertyItem* ArticulationSettingsModel::placement() const
{
    return m_placement;
}

bool ArticulationSettingsModel::isPlacementAvailable() const
{
    return m_isPlacementAvailable;
}
