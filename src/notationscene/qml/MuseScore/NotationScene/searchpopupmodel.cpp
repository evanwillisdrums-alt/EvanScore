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

#include "searchpopupmodel.h"
#include <QPointer>
#include <algorithm>
#include <set>

#include "notation/inotation.h"
#include "notation/inotationelements.h" // IWYU pragma: keep
#include "notation/inotationinteraction.h" // IWYU pragma: keep
#include "notation/inotationnoteinput.h" // IWYU pragma: keep

using namespace mu::notation;

SearchPopupModel::SearchPopupModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void SearchPopupModel::classBegin()
{
    init();
}

void SearchPopupModel::init()
{
}

void SearchPopupModel::search(const QString& text)
{
    if (text.trimmed().startsWith('>')) {
        searchCommands(text.trimmed().mid(1));
        return;
    }
    if (!m_commandResults.empty()) {
        m_commandResults.clear();
        emit commandResultsChanged();
    }
    if (text.trimmed().isEmpty()) return;
    INotationPtr notation = globalContext()->currentNotation();
    if (!notation) {
        return;
    }

    notation->interaction()->clearSelection();

    std::vector<EngravingItem*> elements = notation->elements()->search(text);
    if (!elements.empty()) {
        const NoteInputState& inputState = notation->interaction()->noteInput()->state();
        notation->interaction()->select(elements, elements.size() == 1 ? SelectType::SINGLE : SelectType::RANGE,
                                        inputState.isValid() ? inputState.staffIdx() : 0);
        notation->interaction()->showItem(elements.front());
    }
}

void SearchPopupModel::searchCommands(const QString& text)
{
    const auto registered = actionsDispatcher()->actionList();
    const std::set<muse::actions::ActionCode> codes(registered.begin(), registered.end());
    const auto words = text.simplified().split(' ', Qt::SkipEmptyParts);
    struct Match { QString title; QString code; bool enabled; int rank; };
    std::vector<Match> matches;
    for (const auto& action : uiActionsRegister()->actionList()) {
        if (!codes.contains(action.code)) continue;
        const QString title = action.title.qTranslatedWithoutMnemonic();
        if (title.isEmpty()) continue;
        const QString code = QString::fromStdString(action.code);
        const QString searchable = title + ' ' + action.description.qTranslated() + ' ' + code;
        if (!std::all_of(words.begin(), words.end(), [&](const QString& word) {
            return searchable.contains(word, Qt::CaseInsensitive);
        })) continue;
        const bool enabled = uiActionsRegister()->actionState(action.code).enabled;
        const int rank = (title.startsWith(text.trimmed(), Qt::CaseInsensitive) ? 0 : 2) + (enabled ? 0 : 1);
        matches.push_back({ title, code, enabled, rank });
    }
    std::sort(matches.begin(), matches.end(), [](const Match& a, const Match& b) {
        if (a.rank != b.rank) return a.rank < b.rank;
        const int order = QString::localeAwareCompare(a.title, b.title);
        return order == 0 ? a.code < b.code : order < 0;
    });
    m_commandResults.clear();
    std::set<QString> seen;
    for (const auto& match : matches) {
        if (!seen.insert(match.code).second) continue;
        m_commandResults.append(QVariantMap { { "title", match.title }, { "code", match.code }, { "enabled", match.enabled } });
        if (m_commandResults.size() == 8) break;
    }
    emit commandResultsChanged();
}

bool SearchPopupModel::runCommand(const QString& code)
{
    // Only a displayed result can run, and its state is checked again after
    // focus returns to the score. Searching itself never dispatches an action.
    const bool listed = std::any_of(m_commandResults.begin(), m_commandResults.end(), [&](const QVariant& item) {
        return item.toMap().value("code").toString() == code;
    });
    if (!listed || !uiActionsRegister()->actionState(code.toStdString()).enabled) return false;
    const auto registered = actionsDispatcher()->actionList();
    if (std::find(registered.begin(), registered.end(), code.toStdString()) == registered.end()) return false;
    QPointer<SearchPopupModel> guard(this);
    actionsDispatcher()->dispatch(code.toStdString());
    // Close/Quit can destroy the score window and this model synchronously.
    if (guard) emit guard->commandExecuted(code);
    return true;
}
