/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include <gtest/gtest.h>
#include "playback/stickingresolver.h"
#include "compat/scoreaccess.h"
#include "dom/chord.h"
#include "dom/factory.h"
#include "dom/masterscore.h"
#include "dom/measure.h"
#include "dom/part.h"
#include "dom/segment.h"
#include "dom/staff.h"
#include "dom/sticking.h"

using namespace mu::engraving;

TEST(StickingResolver, PreservesExplicitRepeatedHandsAndCase)
{
    const auto value = StickingResolver::parse("R r L l");
    ASSERT_EQ(value.kind, StickingKind::Hands);
    ASSERT_EQ(value.strokes.size(), 4);
    EXPECT_EQ(value.rawText, "R r L l");
    EXPECT_EQ(value.strokes[0].hand, StickingHand::Right);
    EXPECT_EQ(value.strokes[1].hand, StickingHand::Right);
    EXPECT_EQ(value.strokes[2].hand, StickingHand::Left);
    EXPECT_EQ(value.strokes[3].hand, StickingHand::Left);
}

TEST(StickingResolver, DoesNotInferAHandFromMissingOrContinuationText)
{
    for (const auto text : { "", " ", "...", "-" }) {
        const auto value = StickingResolver::parse(text);
        EXPECT_EQ(value.kind, StickingKind::Missing);
        EXPECT_TRUE(value.strokes.empty());
    }
}

TEST(StickingResolver, RetainsMalletNumbersWithoutGuessingGripOrHand)
{
    const auto value = StickingResolver::parse("1 4 2 3 6 5");
    ASSERT_EQ(value.kind, StickingKind::Mallets);
    ASSERT_EQ(value.strokes.size(), 6);
    EXPECT_EQ(value.strokes[0].mallet, 1);
    EXPECT_EQ(value.strokes[1].mallet, 4);
    EXPECT_EQ(value.strokes[4].mallet, 6);
    for (const auto& stroke : value.strokes) EXPECT_EQ(stroke.hand, StickingHand::Unspecified);
}

TEST(StickingResolver, RejectsMixedAndUnrecognizedSyntaxWithoutPartialAssignment)
{
    EXPECT_EQ(StickingResolver::parse("R1").kind, StickingKind::Ambiguous);
    for (const auto text : { "roll", "R?", "R/L", "R+L", "7", "0" }) {
        const auto value = StickingResolver::parse(text);
        EXPECT_EQ(value.kind, StickingKind::Unsupported);
        EXPECT_TRUE(value.strokes.empty());
        EXPECT_FALSE(value.diagnostic.empty());
    }
}

class StickingResolverScore : public ::testing::Test {
protected:
    std::unique_ptr<MasterScore> score;
    Segment* segment = nullptr;
    Chord* chord = nullptr;
    void SetUp() override {
        score.reset(compat::ScoreAccess::createMasterScore(nullptr));
        auto part = new Part(score.get());
        score->appendPart(part);
        for (int i = 0; i < 2; ++i) score->appendStaff(Factory::createStaff(part));
        auto measure = Factory::createMeasure(score.get());
        measure->setTick(Fraction());
        measure->setTicks(Fraction(1, 1));
        score->measures()->add(measure);
        segment = measure->getSegment(SegmentType::ChordRest, Fraction());
        chord = Factory::createChord(segment);
        chord->setTrack(0);
        segment->add(chord);
    }
    void marking(int track, const char* xml) {
        auto text = Factory::createSticking(segment);
        text->setTrack(track);
        text->setXmlText(xml);
        segment->add(text);
    }
};

TEST_F(StickingResolverScore, ReadsNativeFormattedStickingWithoutChangingNotation)
{
    marking(0, "<b>r</b>");
    const auto before = chord->tick();
    const auto value = StickingResolver::resolve(chord);
    ASSERT_EQ(value.kind, StickingKind::Hands);
    ASSERT_EQ(value.strokes.size(), 1);
    EXPECT_EQ(value.strokes.front().hand, StickingHand::Right);
    EXPECT_EQ(value.rawText, "r");
    EXPECT_EQ(chord->tick(), before);
    EXPECT_EQ(toSticking(segment->annotations().front())->xmlText(), muse::String(u"<b>r</b>"));
}

TEST_F(StickingResolverScore, DoesNotBorrowFromAnotherVoiceOrStaff)
{
    marking(1, "L");
    marking(4, "R");
    EXPECT_EQ(StickingResolver::resolve(chord).kind, StickingKind::Missing);
    marking(0, "l");
    const auto value = StickingResolver::resolve(chord);
    ASSERT_EQ(value.kind, StickingKind::Hands);
    EXPECT_EQ(value.strokes.front().hand, StickingHand::Left);
}

TEST_F(StickingResolverScore, ReportsConflictingNativeMarkings)
{
    marking(0, "R");
    marking(0, "L");
    const auto value = StickingResolver::resolve(chord);
    EXPECT_EQ(value.kind, StickingKind::Ambiguous);
    EXPECT_TRUE(value.strokes.empty());
}

TEST_F(StickingResolverScore, DoesNotCarryStickingIntoLaterOnsets)
{
    marking(0, "R");
    auto next = segment->measure()->getSegment(SegmentType::ChordRest, Fraction(1, 4));
    auto later = Factory::createChord(next);
    later->setTrack(0);
    next->add(later);
    EXPECT_EQ(StickingResolver::resolve(later).kind, StickingKind::Missing);
}

TEST_F(StickingResolverScore, GraceStrokesRequireTheirOwnAssignment)
{
    marking(0, "R");
    chord->setNoteType(NoteType::ACCIACCATURA);
    EXPECT_EQ(StickingResolver::resolve(chord).kind, StickingKind::Missing);
}
