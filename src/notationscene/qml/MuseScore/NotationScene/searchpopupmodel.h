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

#pragma once

#include <QObject>
#include <QQmlParserStatus>
#include <QVariantList>
#include <qqmlintegration.h>

#include "modularity/ioc.h"
#include "context/iglobalcontext.h"
#include "ui/iuiactionsregister.h"
#include "actions/iactionsdispatcher.h"

namespace mu::notation {
class SearchPopupModel : public QObject, public QQmlParserStatus, public muse::Contextable
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus);
    QML_ELEMENT;
    Q_PROPERTY(QVariantList commandResults READ commandResults NOTIFY commandResultsChanged)

    muse::ContextInject<mu::context::IGlobalContext> globalContext = { this };
    muse::ContextInject<muse::ui::IUiActionsRegister> uiActionsRegister = { this };
    muse::ContextInject<muse::actions::IActionsDispatcher> actionsDispatcher = { this };

public:
    explicit SearchPopupModel(QObject* parent = nullptr);

    Q_INVOKABLE void search(const QString& text);
    Q_INVOKABLE bool runCommand(const QString& code);
    QVariantList commandResults() const { return m_commandResults; }

signals:
    void commandResultsChanged();
    void commandExecuted(const QString& code);

private:
    void classBegin() override;
    void componentComplete() override {}
    void init();
    void searchCommands(const QString& text);
    QVariantList m_commandResults;
};
}
