/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include "featuredebugmodel.h"
#include <QClipboard>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickWindow>
#include <QQuickItem>
#include <QSettings>
#include "processmemory.h"
#include <algorithm>
#include <set>
#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationselection.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/sticking.h"
#include "engraving/dom/dynamicsplayback.h"
#include "engraving/playback/stickingresolver.h"

using namespace mu::notation;
using namespace mu::engraving;

static QJsonObject runtimeDiagnostics()
{
    QJsonObject result = processMemoryDiagnostics();
    result.insert("timeFormat", QSettings().value("evanscore/transport/musicalPosition", false).toBool() ? "bar.beat" : "elapsed");
    QJsonArray mixers;
    for (QWindow* window : QGuiApplication::allWindows()) {
        const auto* quick = qobject_cast<QQuickWindow*>(window);
        if (!quick) continue;
        for (QObject* object : quick->contentItem()->findChildren<QObject*>(QStringLiteral("mixer-panel-model"))) {
            QVariantMap data;
            if (QMetaObject::invokeMethod(object, "diagnostics", Qt::DirectConnection, Q_RETURN_ARG(QVariantMap, data))) {
                mixers.append(QJsonObject::fromVariantMap(data));
            }
        }
    }
    result.insert("loadedMixers", mixers);
    return result;
}

FeatureDebugModel::FeatureDebugModel(QObject* parent)
    : QObject(parent), Contextable(muse::iocCtxForQmlObject(this))
{
    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(50);
    connect(&m_refreshTimer, &QTimer::timeout, this, &FeatureDebugModel::refresh);
}

void FeatureDebugModel::load()
{
    if (m_loaded) return;
    m_loaded = true;
    context()->currentNotationChanged().onNotify(this, [this]() { onNotationChanged(); });
    onNotationChanged();
}

void FeatureDebugModel::onNotationChanged()
{
    m_receiver.async_disconnectAll();
    m_notation = context()->currentNotation();
    if (m_notation) {
        m_notation->interaction()->selectionChanged().onNotify(&m_receiver, [this]() { m_refreshTimer.start(); });
        m_notation->notationChanged().onReceive(&m_receiver, [this](const muse::RectF&) { m_refreshTimer.start(); });
    }
    refresh();
}

void FeatureDebugModel::setFullScore(bool value)
{
    if (value == m_fullScore) return;
    m_fullScore = value;
    emit fullScoreChanged();
    refresh();
}

void FeatureDebugModel::refresh()
{
    m_refreshTimer.stop();
    constexpr size_t limit = 500;
    const Score* score = m_notation ? m_notation->elements()->msScore() : nullptr;
    std::vector<const Chord*> chords;
    if (score && m_fullScore) {
        for (const Segment* segment = score->firstSegment(SegmentType::ChordRest);
             segment && chords.size() <= limit; segment = segment->next1(SegmentType::ChordRest)) {
            for (size_t track = 0; track < score->ntracks() && chords.size() <= limit; ++track) {
                const EngravingItem* item = segment->element(static_cast<track_idx_t>(track));
                if (item && item->isChord()) chords.push_back(toChord(item));
            }
        }
    } else if (score) {
        std::set<const Chord*> seen;
        for (const Note* note : m_notation->interaction()->selection()->notes()) {
            if (note && note->chord() && seen.insert(note->chord()).second) chords.push_back(note->chord());
            if (chords.size() > limit) break;
        }
        // Clicking a sticking label should inspect the note it belongs to.
        for (const EngravingItem* item : m_notation->interaction()->selection()->elements()) {
            if (chords.size() > limit) break;
            if (!item || !item->isSticking()) continue;
            const auto* segment = toSticking(item)->segment();
            const auto* target = segment ? segment->element(item->track()) : nullptr;
            if (target && target->isChord() && seen.insert(toChord(target)).second) chords.push_back(toChord(target));
        }
    }
    QJsonArray rows;
    for (size_t i = 0; i < chords.size() && i < limit; ++i) {
        const Chord* chord = chords[i];
        const auto assignment = StickingResolver::resolve(chord);
        const QStringList kinds { "missing", "hands", "mallets", "unsupported", "ambiguous" };
        QJsonArray strokes;
        for (const auto& stroke : assignment.strokes) {
            strokes.append(QJsonObject {
                { "hand", stroke.hand == StickingHand::Right ? "right" : stroke.hand == StickingHand::Left ? "left" : "unspecified" },
                { "mallet", stroke.mallet }
            });
        }
        QJsonArray pitches;
        QJsonArray dynamics;
        const QStringList roles { "auto", "normal", "tap", "accent", "tenuto", "marcato", "ghost", "soft accent", "stress", "unstress" };
        for (const Note* note : chord->notes()) {
            pitches.append(note->pitch());
            const bool play = note->getProperty(Pid::PLAY).toBool();
            const bool absolute = note->getProperty(Pid::VELO_TYPE).value<VeloType>() == VeloType::USER_VAL;
            const bool known = !play || DynamicsPlayback::enabled(score) || (note->userVelocity() != 0 && absolute);
            const int base = DynamicsPlayback::velocity(note, 80);
            const int velocity = !play ? 0 : note->userVelocity() ? note->customizeVelocity(base) : base;
            dynamics.append(QJsonObject {
                { "pitch", note->pitch() },
                { "category", roles.at(std::clamp(DynamicsPlayback::role(note), 0, 9)) },
                { "play", play },
                { "velocityKnown", known },
                { "velocity", known ? QJsonValue(velocity) : QJsonValue(QJsonValue::Null) },
                { "velocitySource", known ? "notation/custom mapping; not a measured sample output" : "sound engine automatic" },
                { "overrideValue", note->userVelocity() },
                { "overrideMode", absolute ? "absolute" : "offset" }
            });
        }
        rows.append(QJsonObject {
            { "staff", static_cast<int>(chord->staffIdx()) + 1 },
            { "voice", static_cast<int>(chord->voice()) + 1 },
            { "tick", chord->tick().ticks() },
            { "writtenPitches", pitches },
            { "dynamics", dynamics },
            { "rawSticking", QString::fromStdString(assignment.rawText) },
            { "recognition", kinds.at(static_cast<int>(assignment.kind)) },
            { "strokes", strokes },
            { "diagnostic", QString::fromStdString(assignment.diagnostic) },
            { "sampleApplication", "pending VDL playback integration" }
        });
    }
    QJsonObject report {
        { "scoreOpen", score != nullptr },
        { "scope", m_fullScore ? "full score" : "selected notes" },
        { "rowLimit", static_cast<int>(limit) },
        { "truncated", chords.size() > limit },
        { "sticking", rows },
        { "customDynamicsEnabled", score && DynamicsPlayback::enabled(score) },
        { "reportVersion", 2 },
        { "runtime", runtimeDiagnostics() },
        { "virtualDrumline", "Setup helper available; conversion and sample application not yet implemented" },
        { "malletVisualizer", "planned; will use shared sticking assignments" }
    };
    const QString next = QString::fromUtf8(QJsonDocument(report).toJson(QJsonDocument::Indented));
    if (next == m_report) return;
    m_report = next;
    emit reportChanged();
}

void FeatureDebugModel::copyReport()
{
    if (auto clipboard = QGuiApplication::clipboard()) clipboard->setText(m_report);
}
