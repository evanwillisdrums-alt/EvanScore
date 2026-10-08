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
#include <QVariantList>
#include <qqmlintegration.h>
#include "modularity/ioc.h"
#include "async/asyncable.h"
#include "context/iglobalcontext.h"
#include "notation/inotationconfiguration.h"
#include "rcommand/icommandsregister.h"
#include "rcommand/icommandsstate.h"
#include "rcommand/icommanddispatcher.h"

namespace mu::notation {
class PercussionWorkspaceModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool percussionMode READ percussionMode WRITE setPercussionMode NOTIFY percussionModeChanged)
    Q_PROPERTY(bool capturing READ capturing NOTIFY capturingChanged)
    Q_PROPERTY(QVariantList mouseBindings READ mouseBindings NOTIFY mouseBindingsChanged)
    muse::GlobalInject<INotationConfiguration> notationConfiguration;
    muse::ContextInject<context::IGlobalContext> context = { this };
    muse::GlobalInject<muse::rcommand::ICommandsRegister> commandsRegister;
    muse::ContextInject<muse::rcommand::ICommandsState> commandsState = { this };
    muse::ContextInject<muse::rcommand::ICommandDispatcher> dispatcher = { this };
public:
    explicit PercussionWorkspaceModel(QObject* parent = nullptr);
    ~PercussionWorkspaceModel() override;
    Q_INVOKABLE void load(bool handleMouse = false);
    bool percussionMode() const;
    void setPercussionMode(bool enabled);
    bool capturing() const { return m_capturing; }
    QVariantList mouseBindings() const;
    Q_INVOKABLE QVariantList actionsFor(const QString& search) const;
    Q_INVOKABLE void captureMouse(const QString& command);
    Q_INVOKABLE void cancelCapture();
    Q_INVOKABLE void removeMouseBinding(const QString& key);
signals:
    void percussionModeChanged();
    void capturingChanged();
    void mouseBindingsChanged();
    void styleLoadFailed();
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    void applyWorkspace();
    bool m_loaded = false;
    bool m_handleMouse = false;
    bool m_capturing = false;
    QString m_captureCommand;
    int m_consumedButton = 0;
};
}
