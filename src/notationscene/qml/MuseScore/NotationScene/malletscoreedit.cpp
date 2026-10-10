/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include "malletscoreedit.h"
#include <algorithm>
#include <set>
#include "engraving/dom/chord.h"
#include "engraving/dom/note.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/sticking.h"
#include "engraving/dom/factory.h"
#include "engraving/editing/editnote.h"
using namespace mu::engraving;
namespace mu::notation::mallet {
bool canApply(const std::vector<Note*>& notes, const Pose& candidate) {
    if (!candidate.valid || candidate.uncertain || notes.empty() || notes.size() > 4 || notes.size() != candidate.pitches.size() || notes.size() != candidate.mallets.size()) return false;
    std::set<int> ids;
    std::set<const Note*> unique;
    for (size_t i = 0; i < notes.size(); ++i) {
        const auto* note = notes[i];
        if (!note || !note->chord() || !note->staff() || !unique.insert(note).second || note->tieBack() || note->tieFor()) return false;
        const auto* chord = note->chord();
        if (chord->isGrace() || chord->arpeggio() || chord->tremoloTwoChord()) return false;
        for (const auto* chordNote : chord->notes())
            if (chordNote->tieBack() || chordNote->tieFor() || std::find(notes.begin(), notes.end(), chordNote) == notes.end()) return false;
        if (note->score() != notes.front()->score() || note->tick() != notes.front()->tick() || note->staff()->part() != notes.front()->staff()->part()) return false;
        const int change = candidate.pitches[i] - note->ppitch();
        const int pitch = note->pitch() + change;
        if (pitch < 0 || pitch > 127 || change % 12 != 0 || candidate.mallets[i] < 1 || candidate.mallets[i] > 4 || !ids.insert(candidate.mallets[i]).second) return false;
    }
    return true;
}
bool changesScore(const std::vector<Note*>& notes, const Pose& candidate, bool reverseNumbering) {
    if(!canApply(notes,candidate)) return false;
    for(size_t i=0;i<notes.size();++i) if(notes[i]->ppitch()!=candidate.pitches[i]) return true;
    std::set<const Chord*> seen;
    for(const auto* note:notes) if(seen.insert(note->chord()).second) {
        const Sticking* existing=nullptr;
        for(const auto* item:note->chord()->segment()->annotations()) if(item->isSticking() && item->track()==note->track()) {existing=toSticking(item);break;}
        if(!existing) return true;
        const auto parsed=parseWrittenSticking(existing->plainText().toStdString(),note->chord()->notes().size(),reverseNumbering);
        if(parsed.unknown) return true;
        auto written=note->chord()->notes();std::stable_sort(written.begin(),written.end(),[](const Note*a,const Note*b){return a->ppitch()<b->ppitch();});
        for(size_t i=0;i<written.size();++i) {
            const auto found=std::find(notes.begin(),notes.end(),written[i]);
            if(found==notes.end() || parsed.required[i]!=candidate.mallets[std::distance(notes.begin(),found)]) return true;
        }
    }
    return false;
}
bool apply(const std::vector<Note*>& notes, const Pose& candidate, bool reverseNumbering) {
    if (!canApply(notes, candidate)) return false;
    std::set<Chord*> chords;
    for (size_t i = 0; i < notes.size(); ++i) {
        chords.insert(notes[i]->chord());
        const int pitch = notes[i]->pitch() + candidate.pitches[i] - notes[i]->ppitch();
        if (pitch != notes[i]->pitch()) EditNote::undoChangePitch(notes[i]->score(), notes[i], pitch, notes[i]->tpc1(), notes[i]->tpc2());
    }
    for (auto* chord : chords) {
        auto written = chord->notes();
        std::stable_sort(written.begin(), written.end(), [](const Note* a, const Note* b) { return a->ppitch() < b->ppitch(); });
        std::string text;
        for (auto* note : written) {
            const auto found = std::find(notes.begin(), notes.end(), note);
            if (found == notes.end()) continue;
            if (!text.empty()) text += ' ';
            const int id=candidate.mallets[std::distance(notes.begin(), found)];
            text += std::to_string(reverseNumbering ? 5-id : id);
        }
        std::vector<EngravingItem*> old;
        for (auto* item : chord->segment()->annotations()) if (item->isSticking() && item->track() == chord->track()) old.push_back(item);
        if(old.size()==1) {
            // Keep the existing marking's placement/font/style properties.
            auto* marking=toSticking(old.front());
            if(marking->plainText().toStdString()!=text) marking->undoChangeProperty(Pid::TEXT,muse::String::fromStdString(text));
        } else {
            for (auto* item : old) chord->score()->undoRemoveElement(item);
            auto* marking = Factory::createSticking(chord->segment());
            marking->setTrack(chord->track()); marking->setPlainText(muse::String::fromStdString(text));
            chord->score()->undoAddElement(marking);
        }
    }
    return true;
}
}
