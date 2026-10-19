/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#pragma once
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <QVariantMap>
#include <memory>

class QNetworkAccessManager;
class QNetworkReply;
class QProcess;
class QTemporaryDir;
namespace mu::notation {
// Optional local-only client. Construction, score navigation and model discovery
// never start inference. This object has no score-editing capability.
class MalletAssistant : public QObject {
    Q_OBJECT
public:
    explicit MalletAssistant(QObject* parent = nullptr, QUrl endpoint = QUrl("http://127.0.0.1:11434"));
    ~MalletAssistant() override;
    QVariantMap state() const;
    void setEnabled(bool enabled);
    void discover();
    void setModel(const QString& name);
    void ask(const QVariantMap& context);
    void invalidate();
    void setup();
    void cancel();
    static QByteArray requestBody(const QString& model, const QVariantMap& context);
    static QVariantMap validateResponse(const QByteArray& response, const QVariantMap& context);
signals:
    void changed();
private:
    void request(const QString& path, const QByteArray& body = {});
    void finish(QNetworkReply*, quint64 generation, bool discovery);
    void startManagedService();
    void stopManagedService();
    bool ownChildProcess(QProcess* process);
    bool busy() const;
    QNetworkAccessManager* m_network = nullptr;
    QPointer<QNetworkReply> m_reply;
    QTimer m_timeout;
    QUrl m_endpoint;
    QVariantList m_models;
    QVariantMap m_context, m_result;
    QVariantMap m_pendingContext;
    QByteArray m_response;
    QString m_model, m_status;
    bool m_enabled = false;
    quint64 m_generation = 0;
    QPointer<QProcess> m_setup, m_server;
    std::unique_ptr<QTemporaryDir> m_setupDirectory;
    QTimer m_startTimer, m_setupTimer;
    int m_startAttempts = 0;
    bool m_managed = false, m_starting = false;
    QString m_backendLog;
    void* m_processJob = nullptr; // Windows job: private children die with this app.
};
}
