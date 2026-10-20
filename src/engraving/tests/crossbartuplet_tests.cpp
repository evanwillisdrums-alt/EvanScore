/* SPDX-License-Identifier: GPL-3.0-only */
// Experimental branch only: persistence and timing, not editor/layout acceptance.
#include <gtest/gtest.h>

#include "engraving/compat/scoreaccess.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/part.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/tie.h"
#include "engraving/dom/tuplet.h"
#include "engraving/rw/rwregister.h"
#include "engraving/rw/xmlreader.h"
#include "engraving/rw/read500/readcontext.h"
#include "global/io/buffer.h"

using namespace mu::engraving;
namespace {
struct Fixture {
    std::unique_ptr<MasterScore> score { compat::ScoreAccess::createMasterScore(nullptr) };
    Measure* bars[2] {};
    std::vector<std::unique_ptr<Tuplet>> groups;
    Fixture() {
        auto* part = new Part(score.get());
        score->appendPart(part);
        score->appendStaff(Factory::createStaff(part));
        score->appendStaff(Factory::createStaff(part));
        for (int i = 0; i < 2; ++i) {
            bars[i] = Factory::createMeasure(score.get());
            bars[i]->setTick(Fraction(i, 1));
            bars[i]->setTicks(Fraction(1, 1));
            score->measures()->add(bars[i]);
        }
    }
    Tuplet* group(const Fraction& start, const Fraction& base, const Fraction& ratio, int track, Tuplet* parent = nullptr) {
        auto* t = Factory::createTuplet(start < Fraction(1, 1) ? bars[0] : bars[1]);
        groups.emplace_back(t);
        t->setTrack(track); t->setTick(start); t->setRatio(ratio);
        t->setBaseLen(TDuration(base, true)); t->setTicks(base * ratio.denominator());
        if (parent) { parent->add(t); }
        return t;
    }
    Chord* chord(const Fraction& tick, const Fraction& written, int track, int pitch, Tuplet* group) {
        auto* segment = (tick < Fraction(1, 1) ? bars[0] : bars[1])->getSegment(SegmentType::ChordRest, tick);
        auto* c = Factory::createChord(segment);
        c->setTrack(track); c->setTicks(written); c->setDurationType(TDuration(written, true)); segment->add(c);
        auto* n = Factory::createNote(c); n->setTrack(track); n->setPitch(pitch); n->setTpcFromPitch(); c->add(n);
        if (group) { group->add(c); }
        return c;
    }
    void rest(const Fraction& tick, const Fraction& duration, int track) {
        auto* segment = (tick < Fraction(1, 1) ? bars[0] : bars[1])->getSegment(SegmentType::ChordRest, tick);
        auto* r = Factory::createRest(segment); r->setTrack(track); r->setTicks(duration);
        r->setDurationType(TDuration(duration, true)); segment->add(r);
    }
    // Split a real attack at the barline, retaining one logical group and tie.
    void attack(Tuplet* group, Fraction& cursor, const Fraction& base, const Fraction& effectiveRatio, int pitch) {
        Fraction remaining = base / effectiveRatio;
        Note* previous = nullptr;
        while (remaining > Fraction()) {
            auto* measure = cursor < Fraction(1, 1) ? bars[0] : bars[1];
            const auto length = std::min(remaining, measure->endTick() - cursor);
            const Fraction written = length * effectiveRatio;
            const TDuration duration(written, true);
            ASSERT_TRUE(duration.isValid()); ASSERT_EQ(duration.fraction(), written);
            auto* c = chord(cursor, written, group->track(), pitch, group);
            EXPECT_EQ(c->actualTicks(), length);
            auto* note = c->notes().front();
            if (previous) {
                auto* tie = Factory::createTie(score->dummy());
                tie->setTrack(group->track()); tie->setEndNote(note); tie->setTicks(cursor - previous->tick()); previous->add(tie);
            }
            previous = note; cursor += length; remaining -= length;
        }
    }
    Tuplet* common(int actual, int track = 0, Fraction start = Fraction(3, 4)) {
        const Fraction ratio(actual, actual == 3 ? 2 : 4), base = actual == 3 ? Fraction(1, 4) : Fraction(1, 8);
        auto* t = group(start, base, ratio, track);
        Fraction cursor = start;
        for (int i = 0; i < actual; ++i) { attack(t, cursor, base, ratio, 60 + i); }
        return t;
    }
    muse::ByteArray save() {
        auto buffer = muse::io::Buffer::opened(muse::io::IODevice::WriteOnly);
        EXPECT_TRUE(rw::RWRegister::writer()->writeScore(score.get(), &buffer));
        buffer.close(); return buffer.data();
    }
};
std::unique_ptr<MasterScore> load(const muse::ByteArray& data, bool expectedSuccess = true) {
    std::unique_ptr<MasterScore> result(compat::ScoreAccess::createMasterScore(nullptr)); result->setMscVersion(500);
    XmlReader reader(data);
    const auto status = rw::RWRegister::reader(500)->readScoreFile(result.get(), reader, nullptr);
    EXPECT_EQ(bool(status), expectedSuccess);
    if (!expectedSuccess) { EXPECT_EQ(reader.error(), muse::XmlStreamReader::CustomError); }
    return result;
}
Chord* chordAt(Score* score, const Fraction& tick, int track) {
    auto* measure = score->tick2measure(tick);
    auto* segment = measure ? measure->findSegment(SegmentType::ChordRest, tick) : nullptr;
    auto* item = segment ? segment->element(track) : nullptr;
    return item && item->isChord() ? toChord(item) : nullptr;
}
std::string stringData(const muse::ByteArray& data) { return { data.constChar(), data.size() }; }
void eraseAll(std::string& text, const std::string& needle) {
    for (auto pos = text.find(needle); pos != std::string::npos; pos = text.find(needle)) { text.erase(pos, needle.size()); }
}
}

TEST(CrossBarTupletPrototype, RatiosVoicesAndStavesRetainExactTimingMembershipAndTie) {
    for (int actual : { 3, 5, 7 }) {
        for (int track : { 0, 1, 3, 4 }) {
            SCOPED_TRACE("ratio=" + std::to_string(actual) + " track=" + std::to_string(track));
            Fixture fixture;
            auto* original = fixture.common(actual, track);
            fixture.rest(Fraction(), Fraction(3, 4), track);
            fixture.rest(Fraction(5, 4), Fraction(3, 4), track);
            const int other = track == 0 ? 1 : 0;
            fixture.chord(Fraction(1, 1), Fraction(1, 4), other, 48, nullptr);
            ASSERT_EQ(original->elements().front()->tick(), Fraction(3, 4));
            ASSERT_EQ(original->elements().back()->endTick(), Fraction(5, 4));
            auto reopened = load(fixture.save());
            auto* middle = chordAt(reopened.get(), Fraction(1, 1), track); ASSERT_NE(middle, nullptr);
            const auto* logical = middle->tuplet(); ASSERT_NE(logical, nullptr);
            ASSERT_EQ(logical->elements().size(), original->elements().size());
            Fraction sum; int tieCount = 0;
            for (size_t i = 0; i < logical->elements().size(); ++i) {
                auto* value = logical->elements()[i]; auto* source = original->elements()[i];
                EXPECT_EQ(value->tuplet(), logical); EXPECT_EQ(value->track(), track);
                EXPECT_EQ(value->tick(), source->tick()); EXPECT_EQ(value->ticks(), source->ticks());
                EXPECT_EQ(value->actualTicks(), source->actualTicks()); sum += value->actualTicks();
                auto* chord = toChord(value); EXPECT_EQ(chord->notes().front()->pitch(), toChord(source)->notes().front()->pitch());
                if (chord->notes().front()->tieBack()) { ++tieCount; }
            }
            EXPECT_EQ(sum, Fraction(1, 2)); EXPECT_EQ(tieCount, 1);
            auto* plain = chordAt(reopened.get(), Fraction(1, 1), other); ASSERT_NE(plain, nullptr);
            EXPECT_EQ(plain->tuplet(), nullptr); EXPECT_EQ(plain->actualTicks(), Fraction(1, 4));
        }
    }
}
TEST(CrossBarTupletPrototype, OrdinaryTupletRetainsItsExistingMeasureLocalFormatAndTiming) {
    Fixture fixture; fixture.common(3, 0, Fraction(1, 4)); fixture.rest(Fraction(1, 1), Fraction(1, 1), 0);
    const auto bytes = fixture.save(); EXPECT_EQ(stringData(bytes).find("<crossBar>"), std::string::npos);
    auto score = load(bytes);
    const Tuplet* logical = nullptr;
    for (int i = 0; i < 3; ++i) {
        auto* c = chordAt(score.get(), Fraction(1, 4) + Fraction(i, 6), 0); ASSERT_NE(c, nullptr); ASSERT_NE(c->tuplet(), nullptr);
        if (!logical) { logical = c->tuplet(); }
        EXPECT_EQ(c->tuplet(), logical); EXPECT_EQ(c->actualTicks(), Fraction(1, 6));
    }
}
TEST(CrossBarTupletPrototype, SimultaneousStaffGroupsHaveIndependentRatios) {
    Fixture fixture; fixture.common(3, 0); fixture.common(5, 4);
    auto score = load(fixture.save());
    auto* a = chordAt(score.get(), Fraction(1, 1), 0); auto* b = chordAt(score.get(), Fraction(1, 1), 4);
    ASSERT_NE(a, nullptr); ASSERT_NE(b, nullptr); ASSERT_NE(a->tuplet(), nullptr); ASSERT_NE(b->tuplet(), nullptr);
    EXPECT_NE(a->tuplet(), b->tuplet()); EXPECT_EQ(a->tuplet()->ratio(), Fraction(3, 2)); EXPECT_EQ(b->tuplet()->ratio(), Fraction(5, 4));
}
TEST(CrossBarTupletPrototype, NestedGroupRetainsBothRatiosAndOneBoundaryTie) {
    Fixture fixture;
    auto* outer = fixture.group(Fraction(3, 4), Fraction(1, 4), Fraction(3, 2), 1);
    fixture.chord(Fraction(3, 4), Fraction(1, 4), 1, 60, outer);
    auto* inner = fixture.group(Fraction(11, 12), Fraction(1, 8), Fraction(3, 2), 1, outer);
    Fraction cursor(11, 12);
    for (int i = 0; i < 3; ++i) { fixture.attack(inner, cursor, Fraction(1, 8), Fraction(9, 4), 62 + i); }
    ASSERT_EQ(cursor, Fraction(13, 12));
    fixture.chord(cursor, Fraction(1, 4), 1, 67, outer);
    auto score = load(fixture.save()); auto* boundary = chordAt(score.get(), Fraction(1, 1), 1); ASSERT_NE(boundary, nullptr);
    auto* restoredInner = boundary->tuplet(); ASSERT_NE(restoredInner, nullptr);
    auto* restoredOuter = restoredInner->tuplet(); ASSERT_NE(restoredOuter, nullptr);
    EXPECT_EQ(restoredInner->elements().size(), 4); EXPECT_EQ(restoredOuter->elements().size(), 3);
    EXPECT_EQ(restoredInner->actualTicks(), Fraction(1, 6)); EXPECT_EQ(restoredOuter->actualTicks(), Fraction(1, 2));
    ASSERT_NE(boundary->notes().front()->tieBack(), nullptr); EXPECT_EQ(boundary->actualTicks(), Fraction(1, 36));
}
TEST(CrossBarTupletPrototype, MissingTerminatorIsRejectedBeforeLaterMusicIsAbsorbed) {
    Fixture fixture; fixture.common(3); fixture.chord(Fraction(5, 4), Fraction(1, 4), 0, 72, nullptr);
    auto text = stringData(fixture.save()); ASSERT_NE(text.find("<endTuplet/>"), std::string::npos);
    eraseAll(text, "<endTuplet/>"); load(muse::ByteArray(text.c_str(), text.size()), false);
    // A failed read must not leave continuation state in the next score.
    auto valid = load(fixture.save()); EXPECT_EQ(chordAt(valid.get(), Fraction(5, 4), 0)->tuplet(), nullptr);
}
TEST(CrossBarTupletPrototype, MissingFinalMeasureIsRejected) {
    Fixture fixture; fixture.common(3); auto text = stringData(fixture.save());
    auto first = text.find("<Measure"); ASSERT_NE(first, std::string::npos);
    auto second = text.find("<Measure", first + 1); ASSERT_NE(second, std::string::npos);
    auto end = text.find("</Measure>", second); ASSERT_NE(end, std::string::npos);
    text.erase(second, end + std::string("</Measure>").size() - second);
    load(muse::ByteArray(text.c_str(), text.size()), false);
}
TEST(CrossBarTupletPrototype, OrdinaryUnclosedLegacyGroupDoesNotContinueIntoNextMeasure) {
    Fixture fixture; fixture.common(3, 0, Fraction(1, 4)); fixture.chord(Fraction(1, 1), Fraction(1, 4), 0, 72, nullptr);
    auto text = stringData(fixture.save()); eraseAll(text, "<endTuplet/>");
    auto score = load(muse::ByteArray(text.c_str(), text.size())); auto* c = chordAt(score.get(), Fraction(1, 1), 0);
    ASSERT_NE(c, nullptr); EXPECT_EQ(c->tuplet(), nullptr); EXPECT_EQ(c->actualTicks(), Fraction(1, 4));
}
TEST(CrossBarTupletPrototype, ReaderContextScopesContinuationToItsScoreAndTrack) {
    Fixture first, second; auto* a = first.common(3, 1); auto* b = second.common(5, 1);
    read500::ReadContext context(first.score.get()); context.setContinuingTuplet(1, a);
    EXPECT_EQ(context.continuingTuplet(0), nullptr); EXPECT_EQ(context.continuingTuplet(1), a);
    context.setScore(second.score.get()); EXPECT_EQ(context.continuingTuplet(1), nullptr); context.setContinuingTuplet(1, b);
    context.setScore(first.score.get()); EXPECT_EQ(context.continuingTuplet(1), a);
    context.setContinuingTuplet(1, nullptr); EXPECT_FALSE(context.hasUnfinishedCrossBarTuplets());
    context.setScore(second.score.get()); EXPECT_EQ(context.continuingTuplet(1), b);
}

#include "engraving/editing/crossbartuplet.h"
#include "engraving/editing/transaction/transaction.h"
namespace {
Rest* emptyCrossBarSpan(Fixture& fixture) {
    fixture.score->lockUpdates(true);
    fixture.rest(Fraction(), Fraction(3, 4), 0);
    fixture.rest(Fraction(3, 4), Fraction(1, 4), 0);
    fixture.rest(Fraction(1, 1), Fraction(1, 1), 0);
    return toRest(fixture.bars[0]->findSegment(SegmentType::ChordRest, Fraction(3, 4))->element(0));
}
}
TEST(CrossBarTupletPrototype, EmptySpanCreationAndItsOrdinaryRemainderUndoTogether) {
    for (int actual : { 3, 5, 7 }) {
        Fixture fixture; auto* seed = emptyCrossBarSpan(fixture);
        auto* originalSecondRest = fixture.bars[1]->first(SegmentType::ChordRest)->element(0);
        CrossBarTuplet::Options options; options.actual = actual; options.normal = actual == 3 ? 2 : 4;
        options.base = TDuration(actual == 3 ? DurationType::V_QUARTER : DurationType::V_EIGHTH);
        const auto preview = CrossBarTuplet::preview(seed, options); ASSERT_TRUE(preview.valid());
        EXPECT_EQ(preview.start, Fraction(3, 4)); EXPECT_EQ(preview.end, Fraction(5, 4));
        ASSERT_EQ(preview.fragments.size(), actual + 2); EXPECT_EQ(preview.fragments.back().attack, -1);
        fixture.score->startCmd(muse::TranslatableString::untranslatable("Create cross-bar tuplet"));
        muse::String error;
        auto* group = CrossBarTuplet::create(fixture.score->transactionManager()->currentOrDummyTransaction(), seed, options, error);
        fixture.score->endCmd(); ASSERT_NE(group, nullptr); EXPECT_TRUE(error.empty());
        ASSERT_EQ(group->elements().size(), actual + 1);
        Fraction total;
        for (auto* element : group->elements()) { EXPECT_TRUE(element->isRest()); total += element->actualTicks(); }
        EXPECT_EQ(total, Fraction(1, 2));
        auto* tail = fixture.bars[1]->findSegment(SegmentType::ChordRest, Fraction(5, 4))->element(0);
        ASSERT_TRUE(tail->isRest()); EXPECT_EQ(toRest(tail)->tuplet(), nullptr); EXPECT_EQ(toRest(tail)->actualTicks(), Fraction(3, 4));
        fixture.score->undoRedo(true, nullptr);
        EXPECT_EQ(fixture.bars[0]->findSegment(SegmentType::ChordRest, Fraction(3, 4))->element(0), seed);
        EXPECT_EQ(fixture.bars[1]->first(SegmentType::ChordRest)->element(0), originalSecondRest);
        EXPECT_TRUE(group->elements().empty());
        fixture.score->undoRedo(false, nullptr); EXPECT_EQ(group->elements().size(), actual + 1);
        auto restored = load(fixture.save());
        auto* segment = restored->firstMeasure()->nextMeasure()->findSegment(SegmentType::ChordRest, Fraction(1, 1)); ASSERT_NE(segment, nullptr);
        ASSERT_TRUE(segment->element(0)->isRest()); ASSERT_NE(toRest(segment->element(0))->tuplet(), nullptr);
        EXPECT_EQ(toRest(segment->element(0))->tuplet()->elements().size(), actual + 1);
    }
}
TEST(CrossBarTupletPrototype, NonDyadicBarlineSplitIsRefusedBeforeEditing) {
    Fixture fixture; auto* seed = emptyCrossBarSpan(fixture);
    CrossBarTuplet::Options options; options.actual = 5; options.normal = 3; options.base = TDuration(DurationType::V_EIGHTH);
    const auto preview = CrossBarTuplet::preview(seed, options); EXPECT_FALSE(preview.valid()); EXPECT_TRUE(preview.error.contains(u"nonrepresentable")); EXPECT_TRUE(preview.fragments.empty());
    fixture.score->startCmd(muse::TranslatableString::untranslatable("Rejected group")); muse::String error;
    EXPECT_EQ(CrossBarTuplet::create(fixture.score->transactionManager()->currentOrDummyTransaction(), seed, options, error), nullptr);
    fixture.score->endCmd(); EXPECT_FALSE(error.empty());
    EXPECT_EQ(fixture.bars[0]->findSegment(SegmentType::ChordRest, Fraction(3, 4))->element(0), seed); EXPECT_EQ(seed->tuplet(), nullptr);
    EXPECT_EQ(toRest(fixture.bars[1]->first(SegmentType::ChordRest)->element(0))->ticks(), Fraction(1, 1));
}
TEST(CrossBarTupletPrototype, CreationRevalidatesAfterPreviewAndPreservesExistingNotes) {
    Fixture fixture; auto* seed = emptyCrossBarSpan(fixture); CrossBarTuplet::Options options;
    ASSERT_TRUE(CrossBarTuplet::preview(seed, options).valid());
    auto* segment = fixture.bars[1]->first(SegmentType::ChordRest); auto* rest = toRest(segment->element(0)); segment->remove(rest); delete rest;
    auto* chord = fixture.chord(Fraction(1, 1), Fraction(1, 4), 0, 67, nullptr);
    fixture.score->startCmd(muse::TranslatableString::untranslatable("Rejected stale preview")); muse::String error;
    EXPECT_EQ(CrossBarTuplet::create(fixture.score->transactionManager()->currentOrDummyTransaction(), seed, options, error), nullptr);
    fixture.score->endCmd(); EXPECT_TRUE(error.contains(u"contains notes"));
    EXPECT_EQ(chord->notes().front()->pitch(), 67); EXPECT_EQ(chord->ticks(), Fraction(1, 4)); EXPECT_EQ(seed->tuplet(), nullptr);
}
TEST(CrossBarTupletPrototype, CreationRequiresAnUndoTransactionAndRefusesUnverifiedLinks) {
    Fixture fixture; auto* seed = emptyCrossBarSpan(fixture); CrossBarTuplet::Options options; muse::String error;
    EXPECT_EQ(CrossBarTuplet::create(fixture.score->transactionManager()->currentOrDummyTransaction(), seed, options, error), nullptr);
    EXPECT_TRUE(error.contains(u"transaction")); EXPECT_EQ(seed->tuplet(), nullptr);
    fixture.score->staff(1)->linkTo(fixture.score->staff(0));
    EXPECT_FALSE(CrossBarTuplet::preview(seed, options).valid());
}
TEST(CrossBarTupletPrototype, CreationCannotUseAnotherScoresTransaction) {
    Fixture first, second; auto* seed = emptyCrossBarSpan(first); emptyCrossBarSpan(second);
    first.score->startCmd(muse::TranslatableString::untranslatable("First score"));
    second.score->startCmd(muse::TranslatableString::untranslatable("Other score"));
    muse::String error;
    EXPECT_EQ(CrossBarTuplet::create(second.score->transactionManager()->currentOrDummyTransaction(), seed, CrossBarTuplet::Options(), error), nullptr);
    EXPECT_TRUE(error.contains(u"transaction")); EXPECT_EQ(seed->tuplet(), nullptr);
    first.score->endCmd(); second.score->endCmd();
}
TEST(CrossBarTupletPrototype, CompatibleNonBinaryRatioUsesExactDyadicSplits) {
    Fixture fixture; fixture.score->lockUpdates(true);
    fixture.rest(Fraction(), Fraction(3, 4), 0); fixture.rest(Fraction(3, 4), Fraction(1, 16), 0);
    fixture.rest(Fraction(13, 16), Fraction(3, 16), 0); fixture.rest(Fraction(1, 1), Fraction(1, 1), 0);
    auto* seed = toRest(fixture.bars[0]->findSegment(SegmentType::ChordRest, Fraction(13, 16))->element(0));
    CrossBarTuplet::Options options; options.actual = 5; options.normal = 3; options.base = TDuration(DurationType::V_EIGHTH);
    auto preview = CrossBarTuplet::preview(seed, options); ASSERT_TRUE(preview.valid()); EXPECT_EQ(preview.end, Fraction(19, 16));
    fixture.score->startCmd(muse::TranslatableString::untranslatable("Compatible 5:3 group")); muse::String error;
    auto* group = CrossBarTuplet::create(fixture.score->transactionManager()->currentOrDummyTransaction(), seed, options, error);
    fixture.score->endCmd(); ASSERT_NE(group, nullptr); ASSERT_EQ(group->elements().size(), 6);
    Fraction total; for (auto* item : group->elements()) { total += item->actualTicks(); } EXPECT_EQ(total, Fraction(3, 8));
    EXPECT_EQ(group->elements()[2]->ticks(), Fraction(1, 16)); EXPECT_EQ(group->elements()[3]->ticks(), Fraction(1, 16));
    fixture.score->undoRedo(true, nullptr); EXPECT_EQ(seed->tuplet(), nullptr); EXPECT_EQ(seed->ticks(), Fraction(3, 16));
}
