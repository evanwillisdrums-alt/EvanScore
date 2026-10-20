/* SPDX-License-Identifier: GPL-3.0-only */
#include <gtest/gtest.h>
#include "engraving/compat/scoreaccess.h"
#include "engraving/dom/articulation.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/instrument.h"
#include "engraving/dom/instrtemplate.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/part.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/stafftype.h"
#include "engraving/editing/editvoice.h"
#include "engraving/editing/transaction/transaction.h"
#include "engraving/rw/rwregister.h"
#include "engraving/rw/xmlreader.h"
#include "global/io/buffer.h"

using namespace mu::engraving;
class Engraving_PercussionAccent : public ::testing::Test {
protected:
    std::unique_ptr<MasterScore> score;
    Part* drums = nullptr;
    Measure* measure = nullptr;
    void SetUp() override {
        score.reset(compat::ScoreAccess::createMasterScore(nullptr)); score->lockUpdates(true);
        score->style().set(Sid::percussionAccentsAbove, true);
        score->style().set(Sid::articulationAnchorDefault, int(ArticulationAnchor::AUTO));
        auto* pitched = new Part(score.get()); score->appendPart(pitched); score->appendStaff(Factory::createStaff(pitched));
        drums = new Part(score.get()); score->appendPart(drums); drums->instrument()->setUseDrumset(true);
        auto* staff = Factory::createStaff(drums); score->appendStaff(staff);
        staff->setStaffType(Fraction(), *StaffType::getDefaultPreset(StaffGroup::PERCUSSION));
        measure = Factory::createMeasure(score.get()); measure->setTick(Fraction()); measure->setTicks(Fraction(1, 1)); score->measures()->add(measure);
    }
    Chord* chord(int track, Fraction tick = Fraction(), int pitch = 60) {
        auto* s = measure->getSegment(SegmentType::ChordRest, tick); auto* c = Factory::createChord(s);
        c->setTrack(track); c->setTicks(Fraction(1, 4)); c->setDurationType(DurationType::V_QUARTER); s->add(c);
        auto* n = Factory::createNote(c); n->setTrack(track); n->setPitch(pitch); n->setTpcFromPitch(); c->add(n); return c;
    }
    Articulation* mark(Chord* chord, SymId sym = SymId::articAccentAbove) {
        auto* a = Factory::createArticulation(chord); a->setTrack(chord->track()); a->setSymId(sym); chord->add(a); return a;
    }
    void move(Articulation* a, int voice) {
        score->select(a, SelectType::SINGLE, a->staffIdx());
        score->startCmd(muse::TranslatableString::untranslatable("Move accent"));
        EditVoice::changeSelectedElementsVoice(score->transactionManager()->currentOrDummyTransaction(), score.get(), voice);
        score->endCmd();
    }
};
TEST_F(Engraving_PercussionAccent, AboveDefaultIsLimitedToUnpitchedPercussionAndAccentFamily) {
    for (int track : { 0, 4 }) for (SymId sym : { SymId::articAccentAbove, SymId::articMarcatoAbove, SymId::articTenutoAbove }) {
        auto* c = chord(track); auto* a = mark(c, sym); a->setUp(false);
        const bool expected = track == 4 && sym != SymId::articTenutoAbove;
        EXPECT_EQ(a->up(), expected); EXPECT_EQ(a->anchor(), expected ? ArticulationAnchor::TOP : ArticulationAnchor::AUTO);
        c->segment()->remove(c); delete c;
    }
}
TEST_F(Engraving_PercussionAccent, ExplicitPlacementAndSymbolDirectionOverrideTheDefault) {
    auto* a = mark(chord(4), SymId::articMarcatoAbove);
    a->setAnchor(ArticulationAnchor::BOTTOM); a->setUp(false);
    EXPECT_EQ(a->anchor(), ArticulationAnchor::BOTTOM); EXPECT_FALSE(a->up()); EXPECT_EQ(a->symId(), SymId::articMarcatoBelow);
    a->setAnchor(ArticulationAnchor::AUTO); a->setDirection(DirectionV::DOWN); a->setUp(false);
    EXPECT_EQ(a->anchor(), ArticulationAnchor::AUTO); EXPECT_FALSE(a->up()); EXPECT_EQ(a->symId(), SymId::articMarcatoBelow);
    a->setDirection(DirectionV::AUTO); a->setUp(false); EXPECT_TRUE(a->up());
    score->style().set(Sid::percussionAccentsAbove, false); a->setUp(false); EXPECT_FALSE(a->up());
}
TEST_F(Engraving_PercussionAccent, InstrumentChangesUseTheInstrumentAtTheActualOnset) {
    auto* pitched = new Instrument(*drums->instrument()); pitched->setUseDrumset(false); drums->setInstrument(pitched, Fraction(1, 2));
    auto* before = mark(chord(4)); auto* after = mark(chord(4, Fraction(1, 2)));
    before->setUp(false); after->setUp(false); EXPECT_TRUE(before->up()); EXPECT_FALSE(after->up());
}
TEST_F(Engraving_PercussionAccent, PlacementIsUndoableWithoutChangingTheScoreDefault) {
    auto* a = mark(chord(4));
    score->startCmd(muse::TranslatableString::untranslatable("Place accent"));
    a->undoChangeProperty(Pid::ARTICULATION_ANCHOR, int(ArticulationAnchor::BOTTOM), PropertyFlags::UNSTYLED); score->endCmd();
    EXPECT_EQ(a->anchor(), ArticulationAnchor::BOTTOM);
    score->undoRedo(true, nullptr); EXPECT_EQ(a->anchor(), ArticulationAnchor::TOP);
    score->undoRedo(false, nullptr); EXPECT_EQ(a->anchor(), ArticulationAnchor::BOTTOM);
    EXPECT_TRUE(score->style().styleB(Sid::percussionAccentsAbove));
}
TEST_F(Engraving_PercussionAccent, MovingOnlyTheAccentRetainsNotesDurationsVelocityAndLocksWithOneUndo) {
    auto* source = chord(4); auto* target = chord(5, Fraction(), 64); auto* a = mark(source);
    auto* n = source->notes().front(); n->setProperty(Pid::USER_VELOCITY, 31); n->setProperty(Pid::DYNAMICS_LOCK, muse::String(u"31"));
    ASSERT_TRUE(EditVoice::canMoveArticulationToVoice(a, 1)); move(a, 1);
    EXPECT_EQ(a->chordRest(), target); EXPECT_EQ(a->track(), 5); EXPECT_TRUE(source->articulations().empty());
    EXPECT_EQ(source->notes().size(), 1); EXPECT_EQ(target->notes().size(), 1); EXPECT_EQ(source->notes().front()->pitch(), 60);
    EXPECT_EQ(source->ticks(), Fraction(1, 4)); EXPECT_EQ(n->getProperty(Pid::USER_VELOCITY).toInt(), 31); EXPECT_TRUE(n->dynamicsLocked());
    score->undoRedo(true, nullptr); EXPECT_EQ(a->chordRest(), source); EXPECT_EQ(a->track(), 4); EXPECT_TRUE(target->articulations().empty());
    score->undoRedo(false, nullptr); EXPECT_EQ(a->chordRest(), target); EXPECT_EQ(a->track(), 5);
}
TEST_F(Engraving_PercussionAccent, MissingRestDuplicateAndGraceDestinationsAreNotInventedOrOverwritten) {
    auto* source = chord(4); auto* a = mark(source);
    EXPECT_FALSE(EditVoice::canMoveArticulationToVoice(a, 1));
    auto* rest = Factory::createRest(source->segment()); rest->setTrack(5); rest->setTicks(Fraction(1, 4)); rest->setDurationType(DurationType::V_QUARTER); source->segment()->add(rest);
    EXPECT_FALSE(EditVoice::canMoveArticulationToVoice(a, 1)); source->segment()->remove(rest); delete rest;
    auto* target = chord(5); mark(target); EXPECT_FALSE(EditVoice::canMoveArticulationToVoice(a, 1));
    move(a, 1); EXPECT_EQ(a->chordRest(), source); EXPECT_EQ(target->articulations().size(), 1);
    source->setNoteType(NoteType::ACCIACCATURA); EXPECT_FALSE(EditVoice::canMoveArticulationToVoice(a, 0));
}
TEST_F(Engraving_PercussionAccent, LinkedStaffMovesArePreflightedAndUndoTogether) {
    auto* linkedStaff = Factory::createStaff(drums); score->appendStaff(linkedStaff); linkedStaff->linkTo(score->staff(1));
    auto* source = chord(4); auto* linkedSource = chord(8); linkedSource->linkTo(source);
    auto* target = chord(5); auto* a = mark(source); auto* linkedA = mark(linkedSource); linkedA->linkTo(a);
    EXPECT_FALSE(EditVoice::canMoveArticulationToVoice(a, 1));
    auto* linkedTarget = chord(9); linkedTarget->linkTo(target); ASSERT_TRUE(EditVoice::canMoveArticulationToVoice(a, 1));
    move(a, 1); EXPECT_EQ(a->chordRest(), target); EXPECT_EQ(a->track(), 5); EXPECT_EQ(linkedA->chordRest(), linkedTarget); EXPECT_EQ(linkedA->track(), 9);
    score->undoRedo(true, nullptr); EXPECT_EQ(a->chordRest(), source); EXPECT_EQ(a->track(), 4); EXPECT_EQ(linkedA->chordRest(), linkedSource); EXPECT_EQ(linkedA->track(), 8);
}
TEST_F(Engraving_PercussionAccent, SavedMoveAndManualPlacementReopenOnTheCorrectVoice) {
    auto* source = chord(4); auto* target = chord(5); auto* a = mark(source); a->setAnchor(ArticulationAnchor::BOTTOM); a->setDirection(DirectionV::DOWN);
    a->setPropertyFlags(Pid::ARTICULATION_ANCHOR, PropertyFlags::UNSTYLED);
    move(a, 1); ASSERT_EQ(a->chordRest(), target);
    auto buffer = muse::io::Buffer::opened(muse::io::IODevice::WriteOnly); ASSERT_TRUE(rw::RWRegister::writer()->writeScore(score.get(), &buffer)); buffer.close();
    std::unique_ptr<MasterScore> reopened(compat::ScoreAccess::createMasterScore(nullptr)); reopened->setMscVersion(500);
    XmlReader reader(buffer.data()); ASSERT_TRUE(rw::RWRegister::reader(500)->readScoreFile(reopened.get(), reader, nullptr));
    auto* s = reopened->firstMeasure()->findSegment(SegmentType::ChordRest, Fraction()); ASSERT_NE(s, nullptr);
    EXPECT_TRUE(toChord(s->element(4))->articulations().empty());
    const auto& markings = toChord(s->element(5))->articulations(); ASSERT_EQ(markings.size(), 1);
    EXPECT_EQ(markings.front()->track(), 5); EXPECT_EQ(markings.front()->anchor(), ArticulationAnchor::BOTTOM); EXPECT_EQ(markings.front()->direction(), DirectionV::DOWN);
}
TEST_F(Engraving_PercussionAccent, NewInstrumentStaffDefaultsDoNotOverrideExplicitStaffTypes) {
    if (!searchTemplate(u"marching-snare")) { ASSERT_TRUE(loadInstrumentTemplates(":/engraving/instruments/instruments.xml")); }
    for (auto [name, lines] : { std::pair { u"marching-snare", 3 }, std::pair { u"marching-cymbals", 2 } }) {
        const auto* instrument = searchTemplate(name); ASSERT_NE(instrument, nullptr);
        auto* staff = Factory::createStaff(drums); score->appendStaff(staff); staff->init(instrument, nullptr, 0);
        EXPECT_EQ(staff->staffType(Fraction())->lines(), lines);
        const auto* explicitType = StaffType::presetFromXmlName(u"perc5Line"); ASSERT_NE(explicitType, nullptr);
        staff->init(instrument, explicitType, 0); EXPECT_EQ(staff->staffType(Fraction())->lines(), 5);
    }
}
TEST_F(Engraving_PercussionAccent, AppendedPartsUseTheirInstrumentTemplateStaffDefaults) {
    if (!searchTemplate(u"marching-snare")) { ASSERT_TRUE(loadInstrumentTemplates(":/engraving/instruments/instruments.xml")); }
    auto* original = score->staff(1);
    const int existingLines = original->staffType(Fraction())->lines();
    score->startCmd(muse::TranslatableString::untranslatable("Append marching instruments"));
    for (auto [name, lines] : { std::pair { u"marching-snare", 3 }, std::pair { u"marching-cymbals", 2 } }) {
        const auto* instrument = searchTemplate(name); ASSERT_NE(instrument, nullptr);
        score->appendPart(instrument);
        EXPECT_EQ(score->staves().back()->staffType(Fraction())->lines(), lines);
        EXPECT_EQ(score->staves().back()->staffType(Fraction())->group(), StaffGroup::PERCUSSION);
    }
    score->endCmd();
    EXPECT_EQ(original->staffType(Fraction())->lines(), existingLines);
    score->undoRedo(true, nullptr); EXPECT_EQ(score->nstaves(), 2);
    score->undoRedo(false, nullptr); EXPECT_EQ(score->nstaves(), 4);
    EXPECT_EQ(score->staff(2)->staffType(Fraction())->lines(), 3);
    EXPECT_EQ(score->staff(3)->staffType(Fraction())->lines(), 2);
}
