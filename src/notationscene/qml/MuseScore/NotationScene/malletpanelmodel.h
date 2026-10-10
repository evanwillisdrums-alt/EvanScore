/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#pragma once
#include <QObject>
#include <QVariantMap>
#include <QTimer>
#include <qqmlintegration.h>
#include "modularity/ioc.h"
#include "async/asyncable.h"
#include "context/iglobalcontext.h"
#include "notation/inotation.h"
#include "playback/iplaybackcontroller.h"
#include "malletplacement.h"
namespace mu::engraving { class Chord; class Note; }
namespace mu::notation {
class MalletPanelModel : public QObject, public muse::Contextable, public muse::async::Asyncable {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantMap state READ state NOTIFY stateChanged)
    Q_PROPERTY(QVariantList alternatives READ alternativeRows NOTIFY stateChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    muse::ContextInject<context::IGlobalContext> context = { this };
    muse::ContextInject<playback::IPlaybackController> playbackController = { this };
public:
    explicit MalletPanelModel(QObject* parent = nullptr);
    QVariantMap state() const { return m_state; }
    QVariantList alternativeRows() const { return m_alternativeRows; }
    bool active() const { return m_active; }
    void setActive(bool value);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setPickMode(bool value);
    Q_INVOKABLE void togglePitch(int pitch);
    Q_INVOKABLE void clearPicked();
    Q_INVOKABLE void selectAlternative(int index);
    Q_INVOKABLE void showOriginal();
    Q_INVOKABLE void setOption(const QString& key, const QVariant& value);
    Q_INVOKABLE void setStrikePoint(int mallet, double fraction);
    Q_INVOKABLE void audition();
    Q_INVOKABLE void compare();
    Q_INVOKABLE void commit();
    Q_INVOKABLE QVariantMap diagnostics() const;
signals:
    void stateChanged();
    void activeChanged();
private:
    void onNotationChanged();
    std::vector<mu::engraving::Chord*> selectedOnset() const;
    std::vector<mu::engraving::Note*> sourceNotes() const;
    QString sourceKey() const;
    void analyze();
    void publish();
    void playPitches(const std::vector<int>& pitches);
    mallet::Player m_player;
    mallet::Keyboard m_keyboard;
    mallet::Pose m_original;
    mallet::Pose m_previous;
    std::vector<mallet::Pose> m_candidates;
    std::vector<int> m_pitches, m_required, m_held;
    INotationPtr m_notation;
    muse::async::Asyncable m_receiver;
    QTimer m_refreshTimer, m_compareTimer;
    QVariantMap m_state;
    QVariantList m_alternativeRows;
    QString m_sourceKey, m_notice, m_sticking, m_instrument = "Marimba";
    QString m_skin = "#d99d78", m_hair = "#382b27";
    bool m_active = false, m_pick = false, m_supported = false, m_editing = false, m_metal = false, m_side = false;
    int m_selected = -1;
};
}
