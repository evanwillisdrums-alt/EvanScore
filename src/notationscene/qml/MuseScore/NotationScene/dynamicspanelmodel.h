/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#pragma once
#include <QObject>
#include <QVariantMap>
#include <QVariantList>
#include <qqmlintegration.h>
#include "modularity/ioc.h"
#include "async/asyncable.h"
#include "context/iglobalcontext.h"
#include "notation/inotation.h"
#include "engraving/style/styledef.h"
#include "interactive/iinteractive.h"
#include "iglobalconfiguration.h"
namespace mu::engraving { class Dynamic; class Hairpin; class Note; class Score; }
namespace mu::notation {
class DynamicsPanelModel : public QObject, public muse::Contextable, public muse::async::Asyncable {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantMap state READ state NOTIFY stateChanged)
    Q_PROPERTY(QVariantList mappings READ mappings NOTIFY stateChanged)
    Q_PROPERTY(QVariantList dynamicChoices READ dynamicChoices CONSTANT)
    muse::ContextInject<context::IGlobalContext> context = { this };
    muse::ContextInject<muse::IInteractive> interactive = { this };
    muse::GlobalInject<muse::IGlobalConfiguration> globalConfiguration;
public:
    explicit DynamicsPanelModel(QObject* parent = nullptr);
    Q_INVOKABLE void load();
    QVariantMap state() const;
    QVariantList mappings() const;
    QVariantList dynamicChoices() const;
    Q_INVOKABLE void showScore();
    Q_INVOKABLE void followSelection();
    Q_INVOKABLE void setEnabled(bool enabled);
    Q_INVOKABLE void setBattery(bool enabled);
    Q_INVOKABLE void setMapping(int dynamic, int role, int velocity);
    Q_INVOKABLE void resetMappings();
    Q_INVOKABLE void savePreset();
    Q_INVOKABLE void loadPreset();
    Q_INVOKABLE void makeDefault();
    Q_INVOKABLE void applyDefault();
    Q_INVOKABLE void setCurve(int shape, double bend);
    Q_INVOKABLE void setEndpoint(bool end, int dynamic, int role, int velocity);
    Q_INVOKABLE void setNoteVelocity(int velocity);
    Q_INVOKABLE void setNotePlayback(bool play);
    Q_INVOKABLE void adjustNotes(int operation, double amount, int category);
    Q_INVOKABLE void resetSelection();
signals:
    void stateChanged();
private:
    void onNotationChanged();
    mu::engraving::Score* score() const;
    mu::engraving::Dynamic* dynamic() const;
    mu::engraving::Hairpin* hairpin() const;
    void setScoreStyle(mu::engraving::Sid id, const mu::engraving::PropertyValue& value);
    std::vector<mu::engraving::Note*> notes() const;
    void edit(const std::function<void()>& change, const char* label);
    muse::async::Asyncable m_notationReceiver;
    INotationPtr m_notation;
    bool writePreset(const muse::io::path_t& path);
    void readPreset(const muse::io::path_t& path);
    QString m_notice;
    bool m_forceScore = true;
    bool m_loaded = false;
    bool m_editing = false;
};
}
