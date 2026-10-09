/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#pragma once
#include <QObject>
#include <QString>
#include <QTimer>
#include <qqmlintegration.h>
#include "modularity/ioc.h"
#include "async/asyncable.h"
#include "context/iglobalcontext.h"
#include "notation/inotation.h"

namespace mu::notation {
class FeatureDebugModel : public QObject, public muse::Contextable, public muse::async::Asyncable {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString report READ report NOTIFY reportChanged)
    Q_PROPERTY(bool fullScore READ fullScore WRITE setFullScore NOTIFY fullScoreChanged)
    muse::ContextInject<context::IGlobalContext> context = { this };
public:
    explicit FeatureDebugModel(QObject* parent = nullptr);
    Q_INVOKABLE void load();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void copyReport();
    QString report() const { return m_report; }
    bool fullScore() const { return m_fullScore; }
    void setFullScore(bool value);
signals:
    void reportChanged();
    void fullScoreChanged();
private:
    void onNotationChanged();
    INotationPtr m_notation;
    muse::async::Asyncable m_receiver;
    QTimer m_refreshTimer;
    QString m_report;
    bool m_fullScore = false;
    bool m_loaded = false;
};
}
