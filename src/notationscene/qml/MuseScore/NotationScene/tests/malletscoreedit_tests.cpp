/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include <gtest/gtest.h>
#include "../malletscoreedit.h"
#include "engraving/compat/scoreaccess.h"
#include "engraving/dom/articulation.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/part.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/sticking.h"
#include "engraving/dom/tie.h"
#include "engraving/editing/transaction/undostack.h"
#include "engraving/playback/stickingresolver.h"

using namespace mu::engraving;
namespace placement = mu::notation::mallet;

class MalletScoreEdit : public ::testing::Test {
protected:
    std::unique_ptr<MasterScore> score;
    std::vector<Note*> notes;
    Segment* segment = nullptr;
    void SetUp() override {
        score.reset(compat::ScoreAccess::createMasterScore(nullptr));
        auto* part = new Part(score.get()); score->appendPart(part);
        for (int i = 0; i < 2; ++i) score->appendStaff(Factory::createStaff(part));
        auto* measure = Factory::createMeasure(score.get());
        measure->setTick(Fraction()); measure->setTicks(Fraction(1, 1)); score->measures()->add(measure);
        segment = measure->getSegment(SegmentType::ChordRest, Fraction());
        for (int i = 0; i < 4; ++i) {
            // Two voices on each staff, all attacking at the same onset.
            const int track = i < 2 ? i : i + 2;
            auto* chord = Factory::createChord(segment); chord->setTrack(track);
            chord->setTicks(Fraction(1, 4)); chord->setDurationType(DurationType::V_QUARTER); segment->add(chord);
            auto* note = Factory::createNote(chord); note->setTrack(track); note->setPitch(60 + i * 3);
            note->setTpcFromPitch(); note->setProperty(Pid::VELO_TYPE, VeloType::USER_VAL);
            note->setProperty(Pid::USER_VELOCITY, 50 + i); chord->add(note); notes.push_back(note);
            auto* articulation = Factory::createArticulation(chord); articulation->setSymId(SymId::articAccentAbove); chord->add(articulation);
            auto* sticking = Factory::createSticking(segment); sticking->setTrack(track); sticking->setPlainText(u"R"); segment->add(sticking);
        }
    }
    placement::Pose candidate() const {
        placement::Pose pose; pose.valid = true; pose.mallets = {4, 3, 2, 1};
        for (auto* note : notes) pose.pitches.push_back(note->ppitch());
        pose.pitches[0] += 12; return pose;
    }
    void checkPreserved() const {
        for (size_t i = 0; i < notes.size(); ++i) {
            EXPECT_EQ(notes[i]->track(), i < 2 ? i : i + 2);
            EXPECT_EQ(notes[i]->chord()->ticks(), Fraction(1, 4));
            EXPECT_EQ(notes[i]->chord()->durationType().type(), DurationType::V_QUARTER);
            ASSERT_EQ(notes[i]->chord()->articulations().size(), 1);
            EXPECT_EQ(notes[i]->chord()->articulations().front()->symId(), SymId::articAccentAbove);
            EXPECT_EQ(notes[i]->userVelocity(), 50 + i);
        }
    }
};

TEST_F(MalletScoreEdit, PreviewDoesNotEditAndCommitPreservesVoicesRhythmAndPlayback) {
    const auto pose = candidate(); ASSERT_TRUE(placement::canApply(notes, pose));
    for (size_t i = 0; i < notes.size(); ++i) EXPECT_EQ(notes[i]->pitch(), 60 + i * 3);
    score->lockUpdates(true); score->startCmd(muse::TranslatableString::untranslatable("Mallet placement"));
    ASSERT_TRUE(placement::apply(notes, pose)); score->endCmd();
    EXPECT_EQ(notes[0]->pitch(), 72); checkPreserved();
    for (size_t i = 0; i < notes.size(); ++i) {
        const auto value = StickingResolver::resolve(notes[i]->chord());
        ASSERT_EQ(value.kind, StickingKind::Mallets); ASSERT_EQ(value.strokes.size(), 1);
        EXPECT_EQ(value.strokes.front().mallet, 4 - i);
    }
    score->undoStack()->undo(nullptr);
    for (size_t i = 0; i < notes.size(); ++i) {
        EXPECT_EQ(notes[i]->pitch(), 60 + i * 3);
        EXPECT_EQ(StickingResolver::resolve(notes[i]->chord()).rawText, "R");
    }
    checkPreserved(); score->undoStack()->redo(); EXPECT_EQ(notes[0]->pitch(), 72); checkPreserved();
    EXPECT_EQ(segment->annotations().size(), 4);
}

TEST_F(MalletScoreEdit, InvalidCandidateIsRejectedBeforeAnyPartialEdit) {
    auto pose = candidate(); pose.pitches.back() += 1;
    EXPECT_FALSE(placement::apply(notes, pose));
    for (size_t i = 0; i < notes.size(); ++i) EXPECT_EQ(notes[i]->pitch(), 60 + i * 3);
    EXPECT_EQ(segment->annotations().size(), 4); checkPreserved();
    pose = candidate(); pose.mallets.back() = pose.mallets.front(); EXPECT_FALSE(placement::canApply(notes, pose));
}

TEST_F(MalletScoreEdit, TiedAttackAndPartialChordCannotBeCommitted) {
    auto* tie = Factory::createTie(score->dummy()); notes[0]->setTieFor(tie);
    EXPECT_FALSE(placement::canApply(notes, candidate())); notes[0]->setTieFor(nullptr); delete tie;
    auto* extra = Factory::createNote(notes[0]->chord()); extra->setTrack(0); extra->setPitch(84); notes[0]->chord()->add(extra);
    EXPECT_FALSE(placement::canApply(notes, candidate()));
}
