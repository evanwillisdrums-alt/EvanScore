/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#pragma once

#include <QObject>
#include <QTimer>
#include <QVariant>
#include <utility>

namespace mu::appshell {
// KDDockWidgets rejects nested layout signals. Read the latest constraints
// after the current geometry notification unwinds, and cancel with the dock.
template<typename Update>
void deferDockUpdate(QObject* dock, const char* pendingKey, Update update)
{
    if (dock->property(pendingKey).toBool()) {
        return;
    }
    dock->setProperty(pendingKey, true);
    QTimer::singleShot(0, dock, [dock, pendingKey, update = std::move(update)]() mutable {
        dock->setProperty(pendingKey, false);
        update();
    });
}
}
