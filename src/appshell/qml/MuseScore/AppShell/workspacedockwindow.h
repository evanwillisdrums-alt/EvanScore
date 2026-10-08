/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 */
#pragma once

#include "dockwindow.h"

namespace mu::appshell {
class WorkspaceDockWindow : public muse::dock::DockWindow
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit WorkspaceDockWindow(QQuickItem* parent = nullptr)
        : muse::dock::DockWindow(parent) {}

    Q_INVOKABLE void loadPage(const QString& uri, const QVariantMap& params)
    {
        // The pinned dock framework captures the first URI by reference in its
        // deferred pageLoaded callback. QML invocation arguments are temporary;
        // retain this first argument until the window is destroyed. Later page
        // loads notify synchronously and can use their normal arguments.
        if (currentPageUri().isEmpty()) {
            m_firstPageUri = uri;
            muse::dock::DockWindow::loadPage(m_firstPageUri, params);
        } else {
            muse::dock::DockWindow::loadPage(uri, params);
        }
    }

private:
    QString m_firstPageUri;
};
}
