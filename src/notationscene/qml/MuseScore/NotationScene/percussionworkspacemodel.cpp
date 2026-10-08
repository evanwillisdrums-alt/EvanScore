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

#include "percussionworkspacemodel.h"
#include <algorithm>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QWindow>
#include "settings.h"
#include "notation/inotation.h"
#include "notation/inotationstyle.h"
#include "notation/styledefaultssettings.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationundostack.h"
#include "engraving/style/styledef.h"

using namespace mu::notation;
using namespace muse;
static const Settings::Key MOUSE("notation", "input/mouseBindings");

static QJsonObject bindings()
{
    return QJsonDocument::fromJson(QByteArray::fromStdString(settings()->value(MOUSE).toString())).object();
}
static void saveBindings(const QJsonObject& value)
{
    settings()->setSharedValue(MOUSE, Val(QJsonDocument(value).toJson(QJsonDocument::Compact).toStdString()));
}
static QString bindingLabel(const QString& key)
{
    const auto parts = key.split(':');
    const int button = parts.value(0).toInt();
    const int mods = parts.value(1).toInt();
    QString label;
    if (mods & Qt::ControlModifier) label += "Ctrl+";
    if (mods & Qt::AltModifier) label += "Alt+";
    if (mods & Qt::ShiftModifier) label += "Shift+";
    if (mods & Qt::MetaModifier) label += "Meta+";
    if (button == Qt::MiddleButton) label += QObject::tr("Middle mouse");
    else if (button == Qt::BackButton) label += QObject::tr("Mouse back");
    else if (button == Qt::ForwardButton) label += QObject::tr("Mouse forward");
    else label += QObject::tr("Mouse button %1").arg(button);
    return label;
}
PercussionWorkspaceModel::PercussionWorkspaceModel(QObject* parent)
    : QObject(parent), Contextable(muse::iocCtxForQmlObject(this)) {}
PercussionWorkspaceModel::~PercussionWorkspaceModel()
{
    if (QCoreApplication::instance()) QCoreApplication::instance()->removeEventFilter(this);
}
void PercussionWorkspaceModel::load(bool handleMouse)
{
    if (m_loaded) return;
    m_loaded = true;
    m_handleMouse = handleMouse;
    settings()->setDefaultValue(PERCUSSION_WORKSPACE_MODE, Val(false));
    settings()->setDefaultValue(MOUSE, Val("{}"));
    settings()->valueChanged(PERCUSSION_WORKSPACE_MODE).onReceive(this, [this](const Val&) {
        emit percussionModeChanged();
        if (m_handleMouse) applyWorkspace();
    });
    settings()->valueChanged(MOUSE).onReceive(this, [this](const Val&) { emit mouseBindingsChanged(); });
    if (m_handleMouse) {
        QCoreApplication::instance()->installEventFilter(this);
        context()->currentNotationChanged().onNotify(this, [this]() { applyWorkspace(); });
        applyWorkspace();
    }
}
bool PercussionWorkspaceModel::percussionMode() const { return settings()->value(PERCUSSION_WORKSPACE_MODE).toBool(); }
void PercussionWorkspaceModel::setPercussionMode(bool enabled) { settings()->setSharedValue(PERCUSSION_WORKSPACE_MODE, Val(enabled)); }
void PercussionWorkspaceModel::applyWorkspace()
{
    auto notation = context()->currentNotation();
    if (!notation) return;
    auto style = notation->style();
    const bool above = style->styleValue(engraving::Sid::percussionAccentsAbove).toBool();
    const auto savedDefault = settings()->value(SAVED_DEFAULT_STYLE_PATH).toPath();
    const bool hasSavedDefault = !savedDefault.empty() && savedDefault == notationConfiguration()->defaultStyleFilePath();
    if (percussionMode() && !hasSavedDefault) {
        if (!above && !style->loadStyle(io::path_t(":/configs/evanscore-percussion.mss"), false)) emit styleLoadFailed();
    } else if (above != percussionMode()) {
        auto undo = notation->undoStack();
        undo->prepareChanges(TranslatableString("undoableAction", "Change workspace accent placement"));
        style->setStyleValue(engraving::Sid::percussionAccentsAbove, percussionMode());
        undo->commitChanges();
    }
}
QVariantList PercussionWorkspaceModel::actionsFor(const QString& search) const
{
    QVariantList result;
    for (const auto& info : commandsRegister()->commandInfoList()) {
        if (!info.availabilities.testFlag(rcommand::Availability::Shortcut) || !info.inputSchema.args.empty()) continue;
        const QString title = info.description.isEmpty() ? info.title.qTranslatedWithoutMnemonic() : info.description.qTranslated();
        const QString command = QString::fromStdString(info.command.toString());
        if (!search.isEmpty() && !title.contains(search, Qt::CaseInsensitive) && !command.contains(search, Qt::CaseInsensitive)) continue;
        result.append(QVariantMap {{ "text", title }, { "value", command }});
    }
    std::sort(result.begin(), result.end(), [](const QVariant& a, const QVariant& b) {
        return a.toMap()["text"].toString().localeAwareCompare(b.toMap()["text"].toString()) < 0;
    });
    return result;
}
QVariantList PercussionWorkspaceModel::mouseBindings() const
{
    QVariantList result;
    const auto saved = bindings();
    for (auto i = saved.begin(); i != saved.end(); ++i) {
        const auto command = rcommand::Command(i.value().toString().toStdString());
        const auto& info = commandsRegister()->commandInfo(command);
        result.append(QVariantMap {{ "key", i.key() }, { "button", bindingLabel(i.key()) },
                                  { "text", info.description.isEmpty() ? info.title.qTranslatedWithoutMnemonic() : info.description.qTranslated() }});
    }
    return result;
}
void PercussionWorkspaceModel::captureMouse(const QString& command)
{
    const auto& info = commandsRegister()->commandInfo(rcommand::Command(command.toStdString()));
    if (!info.isValid() || !info.inputSchema.args.empty()) return;
    m_captureCommand = command;
    m_capturing = true;
    QCoreApplication::instance()->installEventFilter(this);
    emit capturingChanged();
}
void PercussionWorkspaceModel::cancelCapture()
{
    m_capturing = false;
    if (!m_handleMouse) QCoreApplication::instance()->removeEventFilter(this);
    emit capturingChanged();
}
void PercussionWorkspaceModel::removeMouseBinding(const QString& key)
{
    auto saved = bindings();
    saved.remove(key);
    saveBindings(saved);
}
bool PercussionWorkspaceModel::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != QGuiApplication::focusWindow()) return false;
    if (event->type() == QEvent::MouseButtonRelease) {
        auto mouse = static_cast<QMouseEvent*>(event);
        if (m_consumedButton == int(mouse->button())) {
            m_consumedButton = 0;
            if (!m_handleMouse && !m_capturing) QCoreApplication::instance()->removeEventFilter(this);
            return true;
        }
        return false;
    }
    if (event->type() != QEvent::MouseButtonPress) return false;
    auto mouse = static_cast<QMouseEvent*>(event);
    if (mouse->button() == Qt::LeftButton || mouse->button() == Qt::RightButton) return false;
    const int modifiers = int(mouse->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier | Qt::MetaModifier));
    const QString key = QString::number(int(mouse->button())) + ':' + QString::number(modifiers);
    if (m_capturing) {
        auto saved = bindings();
        saved.insert(key, m_captureCommand);
        saveBindings(saved);
        m_consumedButton = int(mouse->button());
        m_capturing = false;
        emit capturingChanged();
        return true;
    }
    if (!m_handleMouse || watched->objectName() != "ApplicationWindow") return false;
    QQuickItem* owner = nullptr;
    for (QObject* p = parent(); p && !owner; p = p->parent()) owner = qobject_cast<QQuickItem*>(p);
    if (!owner || owner->window() != watched) return false;
    auto notation = context()->currentNotation();
    if (!notation || notation->interaction()->isTextEditingStarted()) return false;
    auto focus = QGuiApplication::focusObject();
    if (focus && (focus->inherits("QQuickTextInput") || focus->inherits("QQuickTextEdit") || focus->inherits("QLineEdit") || focus->inherits("QTextEdit"))) return false;
    const QString mapped = bindings().value(key).toString();
    if (mapped.isEmpty()) return false;
    const rcommand::Command command(mapped.toStdString());
    if (!commandsState()->commandState(command).enabled) return false;
    dispatcher()->dispatch(command);
    m_consumedButton = int(mouse->button());
    return true;
}
