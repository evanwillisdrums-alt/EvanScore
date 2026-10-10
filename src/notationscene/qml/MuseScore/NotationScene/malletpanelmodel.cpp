/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include "malletpanelmodel.h"
#include "malletscoreedit.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <QRandomGenerator>
#include <QSettings>
#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationselection.h"
#include "notation/inotationundostack.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/note.h"
#include "engraving/dom/noteval.h"
#include "engraving/dom/score.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/part.h"
#include "engraving/dom/instrument.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/sticking.h"
#include "engraving/dom/factory.h"
#include "engraving/editing/editnote.h"
#include "engraving/playback/stickingresolver.h"
using namespace mu::notation;
using namespace mu::engraving;
using namespace mu::notation::mallet;
static QVariantMap point(mallet::Point p) { return {{"x", p.x}, {"y", p.y}}; }
static QVariantList integers(const std::vector<int>& values) { QVariantList out; for (int v : values) out << v; return out; }
static QString pitchesText(const std::vector<int>& pitches) { QStringList out; for (int p : pitches) out << QString::fromStdString(pitchName(p)); return out.join("  "); }
static QVariantMap poseData(const Pose& pose) {
    QVariantList heads, wrists, issues;
    for (int m = 0; m < 4; ++m) heads << QVariantMap {{"id", m + 1}, {"active", pose.active[m]}, {"x", pose.targets[m].x}, {"y", pose.targets[m].y}};
    for (int h = 0; h < 2; ++h) wrists << point(pose.wrists[h]);
    for (const auto& issue : pose.issues) issues << QVariantMap {{"key", QString::fromStdString(issue.key)}, {"message", QString::fromStdString(issue.message)}, {"severity", issue.severity}};
    return {{"heads", heads}, {"wrists", wrists}, {"body", point(pose.body)}, {"pitches", integers(pose.pitches)},
        {"mallets", integers(pose.mallets)}, {"pitchText", pitchesText(pose.pitches)}, {"severity", pose.severity},
        {"issues", issues}, {"leftOpening", pose.openings[0]}, {"rightOpening", pose.openings[1]},
        {"leftRotation", pose.rotations[0]}, {"rightRotation", pose.rotations[1]}, {"valid", pose.valid}};
}
MalletPanelModel::MalletPanelModel(QObject* parent) : QObject(parent), Contextable(muse::iocCtxForQmlObject(this)) {
    setObjectName("mallet-panel-model");
    m_refreshTimer.setSingleShot(true); m_refreshTimer.setInterval(35);
    connect(&m_refreshTimer, &QTimer::timeout, this, &MalletPanelModel::refresh);
    m_compareTimer.setSingleShot(true); m_compareTimer.setInterval(650);
    connect(&m_compareTimer, &QTimer::timeout, this, [this]() { if (m_active) audition(); });
    QSettings prefs;
    m_player.malletCount = prefs.value("evanscore/mallet/malletCount", 4).toInt() == 2 ? 2 : 4;
    m_player.grip = std::clamp(prefs.value("evanscore/mallet/grip", 0).toInt(), 0, 2);
    m_player.opening = std::clamp(prefs.value("evanscore/mallet/opening", 24).toDouble(), 5.0, 50.0);
    m_player.reach = std::clamp(prefs.value("evanscore/mallet/reach", 65).toDouble(), 30.0, 100.0);
    m_player.shaft = std::clamp(prefs.value("evanscore/mallet/shaft", 40).toDouble(), 20.0, 60.0);
    m_player.head = std::clamp(prefs.value("evanscore/mallet/head", 3).toDouble(), 1.0, 8.0);
    m_player.respectSticking = prefs.value("evanscore/mallet/respectSticking", true).toBool();
    m_keyboard = keyboard(36, 96); publish();
}
void MalletPanelModel::setActive(bool value) {
    if (value == m_active) return;
    m_active = value;
    if (value) {
        const QStringList skin {"#f0c5a2", "#d99d78", "#b47752", "#885737", "#603e2d"};
        const QStringList hair {"#231e1c", "#473229", "#8a5933", "#c3a46a", "#aeb0ad"};
        m_skin = skin.at(QRandomGenerator::global()->bounded(skin.size()));
        m_hair = hair.at(QRandomGenerator::global()->bounded(hair.size()));
        context()->currentNotationChanged().onNotify(this, [this]() { onNotationChanged(); }); onNotationChanged();
    } else {
        async_disconnectAll(); m_receiver.async_disconnectAll(); m_refreshTimer.stop(); m_compareTimer.stop();
        m_notation.reset(); m_candidates.clear(); m_alternativeRows.clear(); m_pitches.clear(); m_sourceKey.clear();
        publish();
    }
    emit activeChanged();
}
void MalletPanelModel::onNotationChanged() {
    m_receiver.async_disconnectAll(); m_compareTimer.stop(); m_notation = context()->currentNotation();
    m_pick = false; m_selected = -1; m_sourceKey.clear(); m_notice.clear();
    if (m_notation) {
        m_notation->interaction()->selectionChanged().onNotify(&m_receiver, [this]() { if (!m_editing) { m_compareTimer.stop(); m_refreshTimer.start(); } });
        m_notation->notationChanged().onReceive(&m_receiver, [this](const muse::RectF&) { if (!m_editing) { m_compareTimer.stop(); m_refreshTimer.start(); } });
    }
    refresh();
}
std::vector<Chord*> MalletPanelModel::selectedOnset() const {
    if (!m_notation) return {};
    Chord* focus = nullptr;
    for (Note* note : m_notation->interaction()->selection()->notes())
        if (note && note->chord() && !note->chord()->isGrace() && (!focus || note->tick() < focus->tick())) focus = note->chord();
    if (!focus || !focus->segment() || !focus->staff()) return {};
    std::vector<Chord*> result;
    const auto range = focus->staff()->part()->trackRange();
    for (auto track = range.startTrack; track < range.endTrack; ++track) {
        auto* element = focus->segment()->element(track);
        if (element && element->isChord()) result.push_back(toChord(element));
    }
    return result;
}
std::vector<Note*> MalletPanelModel::sourceNotes() const {
    std::vector<Note*> result;
    for (auto* chord : selectedOnset()) for (auto* note : chord->notes()) if (!note->tieBack()) result.push_back(note);
    std::stable_sort(result.begin(), result.end(), [](const Note* a, const Note* b) { return a->ppitch() < b->ppitch(); });
    return result;
}
QString MalletPanelModel::sourceKey() const {
    QStringList out;
    for (const auto* note : sourceNotes()) out << QString("%1:%2:%3:%4").arg(note->track()).arg(note->tick().ticks()).arg(note->pitch()).arg(note->ppitch());
    return out.join(";");
}
void MalletPanelModel::refresh() {
    if (!m_active || m_editing) return;
    if (m_pick) { analyze(); return; }
    m_pitches.clear(); m_required.clear(); m_held.clear(); m_sticking.clear(); m_supported = false; m_notice.clear();
    auto chords = selectedOnset(); auto notes = sourceNotes();
    m_keyboard = keyboard(36, 96); m_metal = false; m_instrument = tr("Marimba");
    if (!chords.empty()) {
        const auto* instrument = chords.front()->staff()->part()->instrument(chords.front()->tick());
        const auto id = instrument->id().toQString();
        m_supported = id.contains("marimba") || id.contains("vibraphone") || id.contains("xylophone") || id.contains("glockenspiel");
        if (m_supported) {
            m_metal = id.contains("vibraphone") || id.contains("glockenspiel");
            m_instrument = chords.front()->staff()->part()->instrumentName(chords.front()->tick()).toQString();
            int low = instrument->minPitchP(), high = instrument->maxPitchP();
            if (low < 0 || high > 127 || low >= high) { low = 36; high = 96; }
            m_keyboard = keyboard(low, high, m_metal);
        }
    }
    QStringList raw;
    for (auto* chord : chords) {
        const auto sticking = StickingResolver::resolve(chord);
        if (!sticking.rawText.empty()) raw << QString::fromStdString(sticking.rawText);
        if (m_player.respectSticking && sticking.kind != StickingKind::Missing && sticking.strokes.size() != chord->notes().size())
            m_notice = tr("Sticking needs one assignment per written note at this onset. Unsupported or ambiguous text is not guessed.");
        for (auto* note : chord->notes()) if (note->tieBack()) m_held.push_back(note->ppitch());
    }
    m_sticking = raw.join(" / ");
    if (m_supported) for (auto* note : notes) {
        m_pitches.push_back(note->ppitch()); int required = 0;
        const auto assignment = StickingResolver::resolve(note->chord());
        if (m_player.respectSticking && assignment.strokes.size() == note->chord()->notes().size()) {
            const auto& chordNotes = note->chord()->notes();
            const size_t index = std::distance(chordNotes.begin(), std::find(chordNotes.begin(), chordNotes.end(), note));
            const auto& stroke = assignment.strokes[index];
            required = stroke.mallet ? stroke.mallet : stroke.hand == StickingHand::Left ? -1 : stroke.hand == StickingHand::Right ? -2 : 0;
        }
        m_required.push_back(required);
    }
    const auto key = sourceKey();
    if (key != m_sourceKey) { m_selected = -1; m_player.strikeFractions = {.5, .5, .5, .5}; }
    m_sourceKey = key; analyze();
}
void MalletPanelModel::analyze() {
    m_compareTimer.stop();
    m_original = {}; m_original.body = { m_keyboard.width / 2, m_keyboard.front + 43 };
    m_previous = {};
    auto chords = selectedOnset();
    if (!m_pick && m_supported && !chords.empty()) {
        auto* part = chords.front()->staff()->part(); const auto range = part->trackRange();
        auto* segment = chords.front()->segment()->prev1(SegmentType::ChordRest);
        for (int steps = 0; segment && steps < 256; segment = segment->prev1(SegmentType::ChordRest), ++steps) {
            std::vector<int> preceding;
            for (auto track = range.startTrack; track < range.endTrack; ++track) {
                const auto* item = segment->element(track);
                if (item && item->isChord()) for (const auto* n : toChord(item)->notes()) if (!n->tieBack()) preceding.push_back(n->ppitch());
            }
            if (preceding.empty()) continue;
            auto candidates = solve(m_keyboard, preceding, m_player);
            if (!candidates.empty() && candidates.front().valid) m_previous = candidates.front();
            break;
        }
    }
    const Pose* previous = m_previous.pitches.empty() ? nullptr : &m_previous;
    const auto solved = solve(m_keyboard, m_pitches, m_player, m_pick ? std::vector<int>{} : m_required, previous);
    if (!solved.empty()) m_original = solved.front();
    m_candidates = mallet::alternatives(m_keyboard, m_pitches, m_player, previous);
    if (m_selected >= int(m_candidates.size())) m_selected = -1;
    publish();
}
void MalletPanelModel::publish() {
    const auto& pose = m_selected >= 0 && m_selected < int(m_candidates.size()) ? m_candidates[m_selected] : m_original;
    QVariantList bars;
    for (const auto& bar : m_keyboard.bars) bars << QVariantMap {{"pitch", bar.pitch}, {"label", QString::fromStdString(pitchName(bar.pitch))},
        {"accidental", bar.accidental}, {"x", bar.x}, {"y", bar.y}, {"width", bar.width}, {"length", bar.length}};
    const bool editable = m_active && m_supported && !m_pick && mallet::canApply(sourceNotes(), pose);
    m_alternativeRows.clear();
    for (size_t i = 0; i < m_candidates.size(); ++i) {
        const auto& candidate = m_candidates[i];
        const bool same = candidate.pitches == m_pitches;
        m_alternativeRows << QVariantMap {{"index", int(i)}, {"pitches", pitchesText(candidate.pitches)}, {"selected", int(i) == m_selected},
            {"description", same ? tr("Same pitches • different sticking / body position") : tr("Octave change • every voice retained; review the new register")},
            {"severity", candidate.severity}, {"leftOpening", candidate.openings[0]}, {"rightOpening", candidate.openings[1]}, {"valid", candidate.valid}};
    }
    QString status = m_pitches.empty() ? tr("Select a mallet chord, or pick notes on the bars") : pose.severity == 2 ? tr("Constraint conflict") : pose.severity == 1 ? tr("Check placement") : tr("Within configured limits");
    m_state = {{"active", m_active}, {"available", m_notation != nullptr}, {"supported", m_supported}, {"pickMode", m_pick}, {"sideView", m_side},
        {"instrument", m_instrument}, {"bars", bars}, {"keyboardWidth", m_keyboard.width}, {"keyboardFront", m_keyboard.front},
        {"low", m_keyboard.low}, {"high", m_keyboard.high}, {"rangeLabel", pitchesText({m_keyboard.low, m_keyboard.high})}, {"metal", m_metal},
        {"pose", poseData(pose)}, {"heldPitches", integers(m_held)}, {"status", status}, {"notice", m_notice}, {"sticking", m_sticking},
        {"previousPitches", pitchesText(m_previous.pitches)},
        {"skin", m_skin}, {"hair", m_hair}, {"malletCount", m_player.malletCount}, {"grip", m_player.grip},
        {"opening", m_player.opening}, {"reach", m_player.reach}, {"shaft", m_player.shaft}, {"head", m_player.head},
        {"bodyOffset", m_player.bodyOffset}, {"respectSticking", m_player.respectSticking}, {"selectedAlternative", m_selected},
        {"canCommit", editable && m_selected >= 0 && pose.valid && pose.pitches.size() == sourceNotes().size()},
        {"canAudition", m_supported && !m_pitches.empty()}, {"sourceKey", m_sourceKey},
        {"geometryNote", tr("Estimated bar dimensions and comfort limits; not manufacturer measurements.")}};
    emit stateChanged();
}
void MalletPanelModel::setPickMode(bool value) { m_pick = value; m_selected = -1; m_notice.clear(); if (!value) refresh(); else analyze(); }
void MalletPanelModel::togglePitch(int pitch) {
    if (!m_pick || !m_keyboard.bar(pitch)) return;
    const auto found = std::find(m_pitches.begin(), m_pitches.end(), pitch);
    if (found != m_pitches.end()) m_pitches.erase(found);
    else if (m_pitches.size() < 8) m_pitches.push_back(pitch);
    std::sort(m_pitches.begin(), m_pitches.end()); m_selected = -1; m_held.clear(); analyze();
}
void MalletPanelModel::clearPicked() { if (!m_pick) return; m_pitches.clear(); m_selected = -1; analyze(); }
void MalletPanelModel::selectAlternative(int index) { if (index < 0 || index >= int(m_candidates.size())) return; m_compareTimer.stop(); m_selected = index; publish(); }
void MalletPanelModel::showOriginal() { m_compareTimer.stop(); m_selected = -1; publish(); }
void MalletPanelModel::setOption(const QString& key, const QVariant& value) {
    QSettings prefs; QVariant saved;
    if (key == "malletCount") { m_player.malletCount = value.toInt() == 2 ? 2 : 4; saved = m_player.malletCount; }
    else if (key == "grip") { m_player.grip = std::clamp(value.toInt(), 0, 2); saved = m_player.grip; }
    else if (key == "respectSticking") { m_player.respectSticking = value.toBool(); saved = m_player.respectSticking; }
    else if (key == "sideView") { m_side = value.toBool(); publish(); return; }
    else {
        bool ok = false; const double number = value.toDouble(&ok); if (!ok || !std::isfinite(number)) return;
        if (key == "opening") { m_player.opening = std::clamp(number, 5.0, 50.0); saved = m_player.opening; }
        else if (key == "reach") { m_player.reach = std::clamp(number, 30.0, 100.0); saved = m_player.reach; }
        else if (key == "shaft") { m_player.shaft = std::clamp(number, 20.0, 60.0); saved = m_player.shaft; }
        else if (key == "head") { m_player.head = std::clamp(number, 1.0, 8.0); saved = m_player.head; }
        else if (key == "bodyOffset") { m_player.bodyOffset = std::clamp(number, -50.0, 50.0); analyze(); return; }
        else return;
    }
    prefs.setValue("evanscore/mallet/" + key, saved); m_selected = -1; refresh();
}
void MalletPanelModel::setStrikePoint(int id, double fraction) {
    if (!std::isfinite(fraction)) return;
    const auto pose = m_selected >= 0 && m_selected < int(m_candidates.size()) ? m_candidates[m_selected] : m_original;
    const auto found = std::find(pose.mallets.begin(), pose.mallets.end(), id);
    if (found == pose.mallets.end()) return;
    m_player.strikeFractions[std::distance(pose.mallets.begin(), found)] = std::clamp(fraction, .12, .88);
    const auto updated = solve(m_keyboard, pose.pitches, m_player, pose.mallets, m_previous.pitches.empty() ? nullptr : &m_previous);
    if (updated.empty()) return;
    if (m_selected >= 0) m_candidates[m_selected] = updated.front(); else m_original = updated.front();
    publish();
}
void MalletPanelModel::playPitches(const std::vector<int>& pitches) {
    auto chords = selectedOnset(); if (!m_active || !m_supported || chords.empty()) return;
    // The diagram uses sounding pitches; native audition applies the selected
    // staff's ottava/capo offsets to its temporary notes again.
    const auto* reference = chords.front()->notes().empty() ? nullptr : chords.front()->notes().front();
    const int offset = reference ? reference->ppitch() - reference->pitch() : 0;
    NoteValList values; for (int pitch : pitches) values.emplace_back(pitch - offset);
    playback::IPlaybackController::PlayParams params; params.duration = 450000;
    playbackController()->playNotes(values, chords.front()->staffIdx(), chords.front()->segment(), params);
}
void MalletPanelModel::audition() { if (!m_active) return; const auto& pose = m_selected >= 0 && m_selected < int(m_candidates.size()) ? m_candidates[m_selected] : m_original; playPitches(pose.pitches); }
void MalletPanelModel::compare() { if (!m_active || m_selected < 0 || m_selected >= int(m_candidates.size())) return; playPitches(m_original.pitches); m_compareTimer.start(); }
void MalletPanelModel::commit() {
    m_compareTimer.stop();
    if (!m_state.value("canCommit").toBool() || m_sourceKey != sourceKey() || m_selected < 0 || m_selected >= int(m_candidates.size())) {
        m_notice = tr("Select an unchanged, untied score chord and a valid alternative before committing."); publish(); return;
    }
    auto notes = sourceNotes(); const auto candidate = m_candidates[m_selected];
    if (notes.size() != candidate.pitches.size() || candidate.mallets.size() != notes.size()) return;
    m_editing = true; auto undo = m_notation->undoStack();
    undo->prepareChanges(muse::TranslatableString("undoableAction", "Apply mallet placement"));
    if (!mallet::apply(notes, candidate)) { undo->rollbackChanges(); m_editing = false; refresh(); return; }
    undo->commitChanges(); m_editing = false; m_selected = -1; refresh();
    m_notice = tr("Placement committed in one Undo step. Durations, voices, articulations and playback mappings are preserved."); publish();
}
QVariantMap MalletPanelModel::diagnostics() const { auto out = m_state; out.remove("bars"); out.insert("alternativesCount", int(m_candidates.size())); out.insert("analysis", "Estimated static geometry; fixed physical mallet IDs; simultaneous new attacks across the selected part"); return out; }
