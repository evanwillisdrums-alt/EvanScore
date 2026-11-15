/* SPDX-License-Identifier: GPL-3.0-only */
#include <gtest/gtest.h>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include "engraving/compat/scoreaccess.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/part.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/system.h"
#include "engraving/dom/instrumentname.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/note.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/segment.h"
#include "engraving/style/styledef.h"
#include "engraving/rw/rwregister.h"
#include "engraving/rw/xmlreader.h"
#include "global/io/buffer.h"
#include "notation/types/staffparttypes.h"
using namespace mu::engraving;

TEST(Engraving_TemporaryStaff, VisibilityChoiceParticipatesInStaffConfigComparison)
{
    mu::notation::StaffConfig normal, temporary;
    temporary.hideWhenEmpty = AutoOnOff::ON;
    EXPECT_NE(normal, temporary);
    normal.hideWhenEmpty = AutoOnOff::ON;
    EXPECT_EQ(normal, temporary);
}

TEST(Engraving_TemporaryStaff, IndependentChoicesUndoAndPersist)
{
    std::unique_ptr<MasterScore> score(compat::ScoreAccess::createMasterScore(nullptr));
    score->lockUpdates(true);
    score->startCmd(muse::TranslatableString::untranslatable("Create test staves"));
    for (int i = 0; i < 8; ++i) {
        auto* part = new Part(score.get()); score->appendPart(part);
        score->appendStaff(Factory::createStaff(part));
    }
    score->appendMeasures(2);
    score->endCmd();
    score->startCmd(muse::TranslatableString::untranslatable("Temporary solo staves"));
    for (int i = 0; i < 8; ++i) {
        score->staff(i)->undoChangeProperty(Pid::HIDE_WHEN_EMPTY, i % 2 ? AutoOnOff::ON : AutoOnOff::OFF);
        score->staff(i)->undoChangeProperty(Pid::STAFF_AUXILIARY, bool(i % 2));
    }
    score->endCmd();
    score->undoRedo(true, nullptr);
    for (auto* staff : score->staves()) { EXPECT_EQ(staff->hideWhenEmpty(), AutoOnOff::AUTO); EXPECT_FALSE(staff->auxiliary()); }
    score->undoRedo(false, nullptr);
    auto buffer = muse::io::Buffer::opened(muse::io::IODevice::WriteOnly);
    ASSERT_TRUE(rw::RWRegister::writer()->writeScore(score.get(), &buffer)); buffer.close();
    std::unique_ptr<MasterScore> reopened(compat::ScoreAccess::createMasterScore(nullptr));
    reopened->setMscVersion(500);
    XmlReader reader(buffer.data());
    ASSERT_TRUE(rw::RWRegister::reader(500)->readScoreFile(reopened.get(), reader, nullptr));
    ASSERT_EQ(reopened->nstaves(), 8);
    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(reopened->staff(i)->hideWhenEmpty(), i % 2 ? AutoOnOff::ON : AutoOnOff::OFF);
        EXPECT_EQ(reopened->staff(i)->auxiliary(), bool(i % 2));
    }
}

TEST(Engraving_TemporaryStaff, OrdinaryTextUsesArialAndMusicFamiliesRemainDistinct)
{
    MStyle style;
    for (Sid sid : { Sid::defaultFontFace, Sid::longInstrumentFontFace, Sid::shortInstrumentFontFace,
                     Sid::tempoFontFace, Sid::lyricsOddFontFace, Sid::titleFontFace })
        EXPECT_EQ(style.styleSt(sid), muse::String(u"Arial"));
    EXPECT_NE(style.styleSt(Sid::dynamicsFontFace), muse::String(u"Arial"));
    EXPECT_NE(style.styleSt(Sid::pedalFontFace), muse::String(u"Arial"));
}

TEST(Engraving_TemporaryStaff, WrittenLineOverrideRetainsPitchVelocityAndNoteheadThroughUndo)
{
    std::unique_ptr<MasterScore> score(compat::ScoreAccess::createMasterScore(nullptr));
    score->lockUpdates(true);
    auto* part = new Part(score.get()); score->appendPart(part);
    score->appendStaff(Factory::createStaff(part)); score->appendMeasures(1);
    auto* segment = score->firstMeasure()->getSegment(SegmentType::ChordRest, Fraction());
    auto* chord = Factory::createChord(segment); chord->setTrack(0);
    chord->setTicks(Fraction(1, 4)); chord->setDurationType(DurationType::V_QUARTER); segment->add(chord);
    auto* note = Factory::createNote(chord); note->setTrack(0); note->setPitch(60); note->setTpcFromPitch(); note->setLine(1); chord->add(note);
    note->setProperty(Pid::USER_VELOCITY, 31);
    const auto head = note->headGroup();
    score->startCmd(muse::TranslatableString::untranslatable("Move written percussion position"));
    note->undoChangeProperty(Pid::FIXED_LINE, -2); note->undoChangeProperty(Pid::FIXED, true);
    score->endCmd();
    EXPECT_EQ(note->line(), -2); EXPECT_EQ(note->pitch(), 60); EXPECT_EQ(note->userVelocity(), 31); EXPECT_EQ(note->headGroup(), head);
    score->undoRedo(true, nullptr); EXPECT_FALSE(note->fixed()); EXPECT_EQ(note->line(), 1);
    score->undoRedo(false, nullptr); EXPECT_EQ(note->line(), -2); EXPECT_EQ(note->pitch(), 60);
}

TEST(Engraving_TemporaryStaff, SnareSplitReclaimsSpaceAndKeepsNameOnMainStaff)
{
    const QString fixture = QFileInfo(QString::fromUtf8(__FILE__)).absoluteDir().filePath("../../../buildscripts/ci/windows/fixtures/snare-split.mscx");
    QFile file(fixture); ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    std::unique_ptr<MasterScore> score(compat::ScoreAccess::createMasterScore(nullptr));
    score->setMscVersion(460);
    XmlReader reader(file.readAll());
    ASSERT_TRUE(rw::RWRegister::reader(460)->readScoreFile(score.get(), reader, nullptr));
    ASSERT_EQ(score->nstaves(), 2); ASSERT_TRUE(score->staff(0)->auxiliary());
    // This vertical-layout regression does not depend on horizontal font
    // metrics; the focused harness uses a font stub. Real-font justification
    // is exercised separately in the full Windows split-layout gate.
    const bool previousStretch = MScore::noHorizontalStretch;
    MScore::noHorizontalStretch = true;
    score->doLayout();
    MScore::noHorizontalStretch = previousStretch;
    ASSERT_EQ(score->systems().size(), 3);
    for (size_t i = 0; i < 3; ++i) {
        auto* system = score->systems()[i];
        auto* aux = system->staff(0); auto* main = system->staff(1);
        EXPECT_EQ(aux->show(), i == 1); EXPECT_TRUE(main->show());
        EXPECT_EQ(aux->name(InstrumentNameRole::PART), nullptr);
        auto* label = main->name(InstrumentNameRole::PART); ASSERT_NE(label, nullptr);
        EXPECT_NEAR(label->pos().y() + label->ldata()->bbox().center().y(), main->bbox().center().y(), .1);
        if (i != 1) { EXPECT_EQ(aux->bbox().height(), 0); EXPECT_NEAR(main->bbox().y(), 0, .1); }
    }
}
