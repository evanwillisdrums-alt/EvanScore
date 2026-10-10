/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include "malletpanelmodel.h"
#include "malletscoreedit.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <map>
#include <QRandomGenerator>
#include <QSettings>
#include <QElapsedTimer>
#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationselection.h"
#include "notation/inotationundostack.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/note.h"
#include "engraving/dom/noteval.h"
#include "engraving/dom/score.h"
#include "engraving/dom/tempotimeline.h"
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
static QVariantMap point(mallet::Point p) { return {{"x", p.x}, {"y", p.y}, {"z",p.z}}; }
static QVariantList integers(const std::vector<int>& values) { QVariantList out; for (int v : values) out << v; return out; }
static QString pitchesText(const std::vector<int>& pitches) { QStringList out; for (int p : pitches) out << QString::fromStdString(pitchName(p)); return out.join("  "); }
static QString statusText(const Pose& pose) {
    return pose.uncertain ? QObject::tr("Unknown — needs clarification") : !pose.valid ? QObject::tr("Conflict in this setup") : pose.severity ? QObject::tr("Difficult — review placement") : QObject::tr("Comfortable in this model");
}
static QVariantMap poseData(const Pose& pose) {
    QVariantList heads, wrists, shoulders, anchors, issues, voices;
    QStringList pros,cons;
    for (int m = 0; m < 4; ++m) {
        heads << QVariantMap {{"id", m + 1}, {"active", pose.active[m]}, {"x", pose.targets[m].x}, {"y", pose.targets[m].y}, {"z",pose.targets[m].z}};
        anchors << point(pose.anchors[m]);
    }
    for (int h = 0; h < 2; ++h) { wrists << point(pose.wrists[h]); shoulders << point(pose.shoulders[h]); }
    for (size_t i=0;i<pose.pitches.size() && i<pose.mallets.size();++i) voices << QVariantMap {
        {"voice", int(i+1)}, {"mallet", pose.mallets[i]}, {"pitch", pose.pitches[i]}, {"name", QString::fromStdString(pitchName(pose.pitches[i]))},
        {"fraction", pose.fractions[i]}, {"zone", pose.fractions[i]>.70 || pose.fractions[i]<.30 ? QObject::tr("End access") : QObject::tr("Central")}};
    for (const auto& issue : pose.issues) {
        (issue.severity>0 || issue.key=="edge"?cons:pros) << QString::fromStdString(issue.message);
        issues << QVariantMap {{"key", QString::fromStdString(issue.key)}, {"message", QString::fromStdString(issue.message)}, {"severity", issue.severity}};
    }
    if(pose.valid) pros << QObject::tr("Every new attack has a distinct physical mallet; pitch classes and rhythm are retained.");
    if(cons.empty()) cons << QObject::tr("Estimated static geometry; articulation, tone and continuous motion require a performer check.");
    return {{"heads", heads}, {"wrists", wrists}, {"shoulders", shoulders}, {"anchors", anchors}, {"voices", voices}, {"body", point(pose.body)}, {"pitches", integers(pose.pitches)},
        {"mallets", integers(pose.mallets)}, {"pitchText", pitchesText(pose.pitches)}, {"severity", pose.severity}, {"uncertain",pose.uncertain}, {"status", statusText(pose)},
        {"issues", issues}, {"pros",pros}, {"cons",cons}, {"leftOpening", pose.openings[0]}, {"rightOpening", pose.openings[1]},
        {"leftRotation", pose.rotations[0]}, {"rightRotation", pose.rotations[1]}, {"leftReach",pose.reaches[0]}, {"rightReach",pose.reaches[1]},
        {"handClearance",pose.handClearance}, {"shaftClearance",pose.shaftClearance}, {"outerSpread",pose.outerSpread},
        {"movement",pose.movement}, {"preparation",pose.preparation}, {"valid", pose.valid}};
}
// Mallet-specific partial syntax does not change the shared battery R/L reader.
// Explicit ?/_ slots retain their voice positions; punctuation is never guessed.
static std::vector<int> requiredForChord(const Chord* chord, bool reverse, bool& ambiguous, QString& raw) {
    const Sticking* marking=nullptr;
    for (const auto* item:chord->segment()->annotations()) if(item->isSticking() && item->track()==chord->track()) {
        if(marking) {ambiguous=true;raw=QObject::tr("Conflicting sticking markings");return {};}
        marking=toSticking(item);
    }
    if(!marking) return std::vector<int>(chord->notes().size(),0);
    raw=marking->plainText().toQString();
    const auto parsed=parseWrittenSticking(raw.toStdString(),chord->notes().size(),reverse);
    ambiguous=parsed.unknown;return parsed.required;
}
MalletPanelModel::MalletPanelModel(QObject* parent) : QObject(parent), Contextable(muse::iocCtxForQmlObject(this)) {
    setObjectName("mallet-panel-model");
    m_refreshTimer.setSingleShot(true); m_refreshTimer.setInterval(35);
    connect(&m_refreshTimer, &QTimer::timeout, this, &MalletPanelModel::refresh);
    m_compareTimer.setSingleShot(true); m_compareTimer.setInterval(650);
    connect(&m_compareTimer, &QTimer::timeout, this, [this]() { if (m_active) audition(); });
    QSettings prefs;
    m_player.malletCount = std::clamp(prefs.value("evanscore/mallet/malletCount", 4).toInt(),2,4);
    m_player.grip = std::clamp(prefs.value("evanscore/mallet/grip", 0).toInt(), 0, 2);
    m_player.opening = std::clamp(prefs.value("evanscore/mallet/opening", 24).toDouble(), 5.0, 50.0);
    m_player.reach = std::clamp(prefs.value("evanscore/mallet/reach", 65).toDouble(), 30.0, 100.0);
    m_player.shaft = std::clamp(prefs.value("evanscore/mallet/shaft", 40).toDouble(), 20.0, 60.0);
    m_player.head = std::clamp(prefs.value("evanscore/mallet/head", 3).toDouble(), 1.0, 8.0);
    m_player.respectSticking = prefs.value("evanscore/mallet/respectSticking", true).toBool();
    m_player.accidentalHeight=std::clamp(prefs.value("evanscore/mallet/accidentalHeight",4).toDouble(),0.0,15.0);
    m_player.rotation=std::clamp(prefs.value("evanscore/mallet/rotation",40).toDouble(),5.0,90.0);
    m_player.bodyDistance=std::clamp(prefs.value("evanscore/mallet/bodyDistance",28).toDouble(),15.0,60.0);
    m_player.handWidth=std::clamp(prefs.value("evanscore/mallet/handWidth",7).toDouble(),4.0,15.0);
    m_player.travelSpeed=std::clamp(prefs.value("evanscore/mallet/travelSpeed",200).toDouble(),30.0,600.0);
    m_player.reverseNumbering=prefs.value("evanscore/mallet/reverseNumbering",false).toBool();
    m_player.optimizeStrikes=prefs.value("evanscore/mallet/optimizeStrikes",true).toBool();
    m_search.allowOctaves=prefs.value("evanscore/mallet/allowOctaves",true).toBool();
    m_search.keepBass=prefs.value("evanscore/mallet/keepBass",true).toBool();
    m_search.keepMelody=prefs.value("evanscore/mallet/keepMelody",true).toBool();
    m_search.allowInversion=prefs.value("evanscore/mallet/allowInversion",false).toBool();
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
    for (const auto* chord:selectedOnset()) {
        bool ambiguous=false;QString raw;requiredForChord(chord,m_player.reverseNumbering,ambiguous,raw);
        out << raw;
    }
    return out.join(";");
}
void MalletPanelModel::refresh() {
    if (!m_active || m_editing) return;
    if (m_pick) { analyze(); return; }
    m_pitches.clear(); m_required.clear(); m_held.clear(); m_sticking.clear(); m_ambiguous=false; m_supported = false; m_notice.clear();
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
    std::map<const Chord*,std::vector<int>> assignments;
    for (auto* chord : chords) {
        bool ambiguous=false;QString text;
        assignments[chord]=requiredForChord(chord,m_player.reverseNumbering,ambiguous,text);
        if(!text.isEmpty()) raw << text;
        if(m_player.respectSticking && ambiguous) m_ambiguous=true;
        if(chord->arpeggio() || chord->tremoloTwoChord()) m_ambiguous=true;
        for (auto* note : chord->notes()) if (note->tieBack()) m_held.push_back(note->ppitch());
    }
    m_sticking = raw.join(" / ");
    if(m_ambiguous) m_notice=tr("Cannot infer this attack reliably. Use one numbered or R/L assignment per note (lowest to highest), with ? for unknown slots. Rolls/arpeggiation need a separate motion model. Written text has not been replaced.");
    if (m_supported) for (auto* note : notes) {
        m_pitches.push_back(note->ppitch()); int required = 0;
        const auto& assignment=assignments[note->chord()];
        if(m_player.respectSticking && assignment.size()==note->chord()->notes().size()) {
            auto written=note->chord()->notes();
            std::stable_sort(written.begin(),written.end(),[](const Note* a,const Note* b){return a->ppitch()<b->ppitch();});
            required=assignment[std::distance(written.begin(),std::find(written.begin(),written.end(),note))];
        }
        m_required.push_back(required);
    }
    const auto key = sourceKey();
    if (key != m_sourceKey) { m_selected = -1; m_player.strikeFractions = {.5, .5, .5, .5}; m_player.manualStrikes={}; }
    m_sourceKey = key; analyze();
}
void MalletPanelModel::analyze() {
    m_compareTimer.stop(); QElapsedTimer timer;timer.start();
    m_original = {}; m_original.body = { m_keyboard.width / 2, m_keyboard.front + m_player.bodyDistance };
    m_previous = {}; m_next={};m_previousSeconds=m_nextSeconds=0;m_contextNotes.clear();
    auto chords = selectedOnset();
    if (!m_pick && m_supported && !chords.empty()) {
        auto* part=chords.front()->staff()->part();const auto range=part->trackRange();
        auto neighbor=[&](bool ahead,Pose& pose,double& seconds) {
            auto* segment=ahead ? chords.front()->segment()->next1(SegmentType::ChordRest) : chords.front()->segment()->prev1(SegmentType::ChordRest);
            for(int steps=0;segment && steps<256;++steps,segment=ahead?segment->next1(SegmentType::ChordRest):segment->prev1(SegmentType::ChordRest)) {
                std::vector<std::pair<int,int>> events;bool unknown=false;
                for(auto track=range.startTrack;track<range.endTrack;++track) {
                    const auto* item=segment->element(track);if(!item || !item->isChord()) continue;
                    const auto* chord=toChord(item);QString raw;bool ambiguous=false;
                    const auto required=requiredForChord(chord,m_player.reverseNumbering,ambiguous,raw);
                    unknown |= ambiguous || chord->arpeggio() || chord->tremoloTwoChord();
                    auto notes=chord->notes();std::stable_sort(notes.begin(),notes.end(),[](const Note*a,const Note*b){return a->ppitch()<b->ppitch();});
                    for(size_t i=0;i<notes.size();++i) if(!notes[i]->tieBack()) events.push_back({notes[i]->ppitch(),m_player.respectSticking && required.size()==notes.size()?required[i]:0});
                }
                if(events.empty()) continue;
                if(unknown) {m_contextNotes << (ahead?tr("Next attack has unsupported sticking/attack syntax; no motion estimate was inferred."):tr("Previous attack has unsupported sticking/attack syntax; no motion estimate was inferred."));break;}
                std::stable_sort(events.begin(),events.end());std::vector<int> pitches,required;
                for(auto event:events){pitches.push_back(event.first);required.push_back(event.second);}
                auto solutions=solve(m_keyboard,pitches,m_player,required);
                if(!solutions.empty() && solutions.front().valid) pose=solutions.front();
                const auto& timeline=chords.front()->score()->tempoTimeline(false);
                seconds=std::abs(timeline.utick2utime(segment->tick().ticks())-timeline.utick2utime(chords.front()->tick().ticks()));
                if(pose.pitches.empty()) m_contextNotes << tr("Nearby attack has no valid modeled pose; travel comparison is unavailable.");
                break;
            }
        };
        neighbor(false,m_previous,m_previousSeconds);neighbor(true,m_next,m_nextSeconds);
    }
    const Pose* previous=m_previous.pitches.empty()?nullptr:&m_previous;
    const Pose* next=m_next.pitches.empty()?nullptr:&m_next;
    auto baseline=m_player;baseline.optimizeStrikes=false;
    const auto required=m_pick?std::vector<int>{}:m_required;
    const auto solved=solve(m_keyboard,m_pitches,baseline,required,previous,m_previousSeconds,next,m_nextSeconds);
    if(!solved.empty()) m_original=solved.front();
    if(m_ambiguous && !m_pick) {
        m_original.uncertain=true;m_original.valid=false;m_original.mallets.clear();m_original.active={};
        m_original.issues.push_back({"unknown",m_notice.toStdString(),1});m_candidates.clear();
    } else m_candidates=mallet::alternatives(m_keyboard,m_pitches,m_player,previous,required,m_search,m_previousSeconds,next,m_nextSeconds);
    if(m_selected>=int(m_candidates.size())) m_selected=-1;
    m_analysisMs=timer.elapsed();publish();
}
void MalletPanelModel::publish() {
    const auto& pose = m_selected >= 0 && m_selected < int(m_candidates.size()) ? m_candidates[m_selected] : m_original;
    QVariantList bars;
    for (const auto& bar : m_keyboard.bars) bars << QVariantMap {{"pitch", bar.pitch}, {"label", QString::fromStdString(pitchName(bar.pitch))},
        {"accidental", bar.accidental}, {"x", bar.x}, {"y", bar.y}, {"width", bar.width}, {"length", bar.length}, {"height",bar.accidental?m_player.accidentalHeight:0}};
    const bool editable = m_active && m_supported && !m_pick && mallet::canApply(sourceNotes(), pose);
    m_alternativeRows.clear();
    for (size_t i = 0; i < m_candidates.size(); ++i) {
        const auto& candidate = m_candidates[i];
        const bool same = candidate.pitches == m_pitches;
        bool links=true;for(size_t v=0;v<m_required.size() && v<candidate.mallets.size();++v) {
            int r=m_required[v],id=candidate.mallets[v];if((r>0 && id!=r)||(r==-1 && id>2)||(r==-2 && id<3)) links=false;
        }
        QStringList pros,cons,changes;
        const bool written=std::any_of(m_required.begin(),m_required.end(),[](int v){return v!=0;});
        pros << (same?tr("Exact sounding notes retained."):tr("All voices and pitch classes retained; octave displacement only."));
        if(written) (links?pros:cons) << (links?tr("Known written voice-to-mallet links retained."):tr("Changes written mallet/hand assignments."));
        if(candidate.openings[0]+candidate.openings[1]<m_original.openings[0]+m_original.openings[1]-1)
            pros << tr("Combined hand opening decreases by %1 cm.").arg(m_original.openings[0]+m_original.openings[1]-candidate.openings[0]-candidate.openings[1],0,'f',1);
        if(std::abs(candidate.rotations[0])+std::abs(candidate.rotations[1])<std::abs(m_original.rotations[0])+std::abs(m_original.rotations[1])-5)
            pros << tr("Less mixed-row tilt: L %1° / R %2°.").arg(std::abs(candidate.rotations[0]),0,'f',0).arg(std::abs(candidate.rotations[1]),0,'f',0);
        if(candidate.handClearance>m_original.handClearance+1) pros << tr("Hand clearance increases by %1 cm.").arg(candidate.handClearance-m_original.handClearance,0,'f',1);
        if(candidate.severity==0) pros << tr("Within the configured comfort settings.");
        for(size_t v=0;v<candidate.pitches.size();++v) {
            const int from=m_pitches[v],to=candidate.pitches[v];
            if(from!=to) changes << tr("Voice %1: %2 → %3 (mallet %4)").arg(v+1).arg(QString::fromStdString(pitchName(from)),QString::fromStdString(pitchName(to))).arg(m_player.reverseNumbering?5-candidate.mallets[v]:candidate.mallets[v]);
            if(v<m_original.mallets.size() && candidate.mallets[v]!=m_original.mallets[v]) changes << tr("Voice %1: mallet %2 → %3").arg(v+1).arg(m_player.reverseNumbering?5-m_original.mallets[v]:m_original.mallets[v]).arg(m_player.reverseNumbering?5-candidate.mallets[v]:candidate.mallets[v]);
            if(std::abs(candidate.fractions[v]-m_original.fractions[v])>.1) changes << tr("%1: strike %2% → %3% along the bar").arg(QString::fromStdString(pitchName(to))).arg(m_original.fractions[v]*100,0,'f',0).arg(candidate.fractions[v]*100,0,'f',0);
        }
        if(!same) {
            cons << tr("Register/spacing changes: listen for voice-leading and balance.");
            const int low=*std::min_element(candidate.pitches.begin(),candidate.pitches.end()),high=*std::max_element(candidate.pitches.begin(),candidate.pitches.end());
            (low==m_pitches.front()?pros:cons) << (low==m_pitches.front()?tr("Bass pitch retained."):tr("Bass pitch changes."));
            (high==m_pitches.back()?pros:cons) << (high==m_pitches.back()?tr("Top melody pitch retained."):tr("Top melody pitch changes."));
        }
        for(const auto& issue:candidate.issues) if(issue.severity>0 || issue.key=="edge") cons << QString::fromStdString(issue.message);
        if(cons.empty()) cons << tr("Static geometry is an estimate; motion and tone still need a player check.");
        const auto description=written && links ? (same?tr("Keep sticking • adjust strike placement"):tr("Keep sticking • revoice")) : same?tr("Keep notes • change sticking"):tr("Change voicing and sticking");
        m_alternativeRows << QVariantMap {{"index", int(i)}, {"pitches", pitchesText(candidate.pitches)}, {"selected", int(i) == m_selected},
            {"description", description}, {"pros",pros}, {"cons",cons}, {"changes",changes}, {"mallets",integers(candidate.mallets)}, {"status",statusText(candidate)}, {"keepsSticking",links},
            {"severity", candidate.severity}, {"leftOpening", candidate.openings[0]}, {"rightOpening", candidate.openings[1]}, {"valid", candidate.valid}};
    }
    QString status = m_pitches.empty() ? tr("Select a mallet chord, or pick notes on the bars") : statusText(pose);
    m_state = {{"active", m_active}, {"available", m_notation != nullptr}, {"supported", m_supported}, {"pickMode", m_pick}, {"sideView", m_side},
        {"instrument", m_instrument}, {"bars", bars}, {"keyboardWidth", m_keyboard.width}, {"keyboardFront", m_keyboard.front},
        {"low", m_keyboard.low}, {"high", m_keyboard.high}, {"rangeLabel", pitchesText({m_keyboard.low, m_keyboard.high})}, {"metal", m_metal},
        {"pose", poseData(pose)}, {"originalPose",poseData(m_original)}, {"recommendKeep",!m_pitches.empty() && m_original.valid && !m_original.uncertain && m_original.severity==0}, {"heldPitches", integers(m_held)}, {"status", status}, {"notice", m_notice}, {"sticking", m_sticking},
        {"previousPitches", pitchesText(m_previous.pitches)}, {"nextPitches",pitchesText(m_next.pitches)}, {"contextNotes",m_contextNotes}, {"timingSource",tr("Native tempo timeline, without repeat expansion")}, {"previousSeconds",m_previousSeconds}, {"nextSeconds",m_nextSeconds}, {"analysisMs",m_analysisMs}, {"searchOrder",!m_player.respectSticking || m_sticking.isEmpty()?tr("Exact notes and sticking first; revoicing if needed"):tr("Written links first; revoicing before reassignment")},
        {"skin", m_skin}, {"hair", m_hair}, {"malletCount", m_player.malletCount}, {"grip", m_player.grip},
        {"opening", m_player.opening}, {"reach", m_player.reach}, {"shaft", m_player.shaft}, {"head", m_player.head},
        {"accidentalHeight",m_player.accidentalHeight},{"rotation",m_player.rotation},{"bodyDistance",m_player.bodyDistance},{"handWidth",m_player.handWidth},{"travelSpeed",m_player.travelSpeed},
        {"reverseNumbering",m_player.reverseNumbering},{"optimizeStrikes",m_player.optimizeStrikes},{"allowOctaves",m_search.allowOctaves},{"keepBass",m_search.keepBass},{"keepMelody",m_search.keepMelody},{"allowInversion",m_search.allowInversion},
        {"bodyOffset", m_player.bodyOffset}, {"respectSticking", m_player.respectSticking}, {"selectedAlternative", m_selected},
        {"canCommit", editable && m_selected >= 0 && pose.valid && !pose.uncertain && pose.pitches.size() == sourceNotes().size() && mallet::changesScore(sourceNotes(),pose,m_player.reverseNumbering)},
        {"canAudition", m_supported && !m_pitches.empty()}, {"sourceKey", m_sourceKey},
        {"geometryNote", tr("Estimated dimensions; no universal playability score. Timing follows the native tempo timeline; rolls and full passage motion are not simulated.")}};
    emit stateChanged();
}
void MalletPanelModel::setPickMode(bool value) { m_pick = value; m_selected = -1; m_notice.clear(); m_ambiguous=false; m_player.manualStrikes={}; if (!value) refresh(); else analyze(); }
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
    if (key == "malletCount") { m_player.malletCount = std::clamp(value.toInt(),2,4); saved = m_player.malletCount; }
    else if (key == "grip") { m_player.grip = std::clamp(value.toInt(), 0, 2); saved = m_player.grip; }
    else if (key == "respectSticking") { m_player.respectSticking = value.toBool(); saved = m_player.respectSticking; }
    else if (key == "reverseNumbering") {m_player.reverseNumbering=value.toBool();saved=m_player.reverseNumbering;}
    else if (key == "optimizeStrikes") {m_player.optimizeStrikes=value.toBool();saved=m_player.optimizeStrikes;}
    else if (key == "allowOctaves") {m_search.allowOctaves=value.toBool();saved=m_search.allowOctaves;}
    else if (key == "keepBass") {m_search.keepBass=value.toBool();saved=m_search.keepBass;}
    else if (key == "keepMelody") {m_search.keepMelody=value.toBool();saved=m_search.keepMelody;}
    else if (key == "allowInversion") {m_search.allowInversion=value.toBool();saved=m_search.allowInversion;}
    else if (key == "sideView") { m_side = value.toBool(); publish(); return; }
    else {
        bool ok = false; const double number = value.toDouble(&ok); if (!ok || !std::isfinite(number)) return;
        if (key == "opening") { m_player.opening = std::clamp(number, 5.0, 50.0); saved = m_player.opening; }
        else if (key == "reach") { m_player.reach = std::clamp(number, 30.0, 100.0); saved = m_player.reach; }
        else if (key == "shaft") { m_player.shaft = std::clamp(number, 20.0, 60.0); saved = m_player.shaft; }
        else if (key == "head") { m_player.head = std::clamp(number, 1.0, 8.0); saved = m_player.head; }
        else if (key == "accidentalHeight") {m_player.accidentalHeight=std::clamp(number,0.0,15.0);saved=m_player.accidentalHeight;}
        else if (key == "rotation") {m_player.rotation=std::clamp(number,5.0,90.0);saved=m_player.rotation;}
        else if (key == "bodyDistance") {m_player.bodyDistance=std::clamp(number,15.0,60.0);saved=m_player.bodyDistance;}
        else if (key == "handWidth") {m_player.handWidth=std::clamp(number,4.0,15.0);saved=m_player.handWidth;}
        else if (key == "travelSpeed") {m_player.travelSpeed=std::clamp(number,30.0,600.0);saved=m_player.travelSpeed;}
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
    const size_t index=std::distance(pose.mallets.begin(),found); m_player.manualStrikes[index]=true;
    m_player.strikeFractions[index] = std::clamp(fraction, .02, .98);
    auto edited=m_player;edited.strikeFractions=pose.fractions;edited.strikeFractions[index]=m_player.strikeFractions[index];edited.manualStrikes={true,true,true,true};
    const auto updated = solve(m_keyboard, pose.pitches, edited, pose.mallets, m_previous.pitches.empty() ? nullptr : &m_previous,m_previousSeconds,m_next.pitches.empty()?nullptr:&m_next,m_nextSeconds);
    if (updated.empty()) return;
    if (m_selected >= 0) m_candidates[m_selected] = updated.front(); else m_original = updated.front();
    publish();
}
void MalletPanelModel::resetStrikePoints() {m_player.strikeFractions={.5,.5,.5,.5};m_player.manualStrikes={};m_selected=-1;analyze();}
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
    if (!mallet::apply(notes, candidate, m_player.reverseNumbering)) { undo->rollbackChanges(); m_editing = false; refresh(); return; }
    undo->commitChanges(); m_editing = false; m_selected = -1; refresh();
    m_notice = tr("Placement committed in one Undo step. Durations, voices, articulations and playback mappings are preserved."); publish();
}
QVariantMap MalletPanelModel::diagnostics() const { auto out = m_state; out.remove("bars"); out.insert("alternativesCount", int(m_candidates.size())); out.insert("analysis", "Estimated shared static geometry; source-voice identity retained; written-sticking-first search; bounded previous/next attacks; no calibrated biomechanics or full passage simulation"); return out; }
