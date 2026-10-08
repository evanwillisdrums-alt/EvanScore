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

#include <QAbstractListModel>
#include <qqmlintegration.h>

#include "async/asyncable.h"

#include "modularity/ioc.h"
#include "context/iglobalcontext.h"
#include "iglobalconfiguration.h"
#include "project/iopenprojectscenario.h"
#include "actions/iactionsdispatcher.h"
#include "interactive/iinteractive.h"

namespace mu::appshell {
class MainToolBarModel : public QAbstractListModel, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(QString currentUri READ currentUri WRITE setCurrentUri NOTIFY currentUriChanged)

    Q_PROPERTY(bool hasProject READ hasProject NOTIFY hasProjectChanged)

    QML_ELEMENT

    muse::ContextInject<context::IGlobalContext> context = { this };
    muse::GlobalInject<muse::IGlobalConfiguration> globalConfiguration;
    muse::ContextInject<project::IOpenProjectScenario> openProjectScenario = { this };

    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher = { this };
    muse::ContextInject<muse::IInteractive> interactive = { this };

public:
    explicit MainToolBarModel(QObject* parent = nullptr);

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void load();
    Q_INVOKABLE void fileAction(const QString& action);
    Q_INVOKABLE void openRecentScores();
    bool hasProject() const;

    QString currentUri() const;
    void setCurrentUri(const QString& uri);

signals:
    void currentUriChanged();
    void hasProjectChanged();

private:
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        UriRole,
        IsTitleBoldRole,
        IsCheckedRole
    };

    void updateNotationPageItem();
    void updateCheckedState();

    QList<QVariantMap> m_items;
    QString m_currentUri;
};
}
