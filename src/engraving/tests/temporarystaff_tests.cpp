/* SPDX-License-Identifier: GPL-3.0-only */
#include <gtest/gtest.h>
#include "engraving/compat/scoreaccess.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/part.h"
#include "engraving/dom/staff.h"
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
    for (int i = 0; i < 8; ++i) {
        auto* part = new Part(score.get()); score->appendPart(part);
        score->appendStaff(Factory::createStaff(part));
    }
    score->appendMeasures(2);
    score->startCmd(muse::TranslatableString::untranslatable("Temporary solo staves"));
    for (int i = 0; i < 8; ++i)
        score->staff(i)->undoChangeProperty(Pid::HIDE_WHEN_EMPTY, i % 2 ? AutoOnOff::ON : AutoOnOff::OFF);
    score->endCmd();
    score->undoRedo(true, nullptr);
    for (auto* staff : score->staves()) EXPECT_EQ(staff->hideWhenEmpty(), AutoOnOff::AUTO);
    score->undoRedo(false, nullptr);
    auto buffer = muse::io::Buffer::opened(muse::io::IODevice::WriteOnly);
    ASSERT_TRUE(rw::RWRegister::writer()->writeScore(score.get(), &buffer)); buffer.close();
    std::unique_ptr<MasterScore> reopened(compat::ScoreAccess::createMasterScore(nullptr));
    reopened->setMscVersion(500);
    XmlReader reader(buffer.data());
    ASSERT_TRUE(rw::RWRegister::reader(500)->readScoreFile(reopened.get(), reader, nullptr));
    ASSERT_EQ(reopened->nstaves(), 8);
    for (int i = 0; i < 8; ++i)
        EXPECT_EQ(reopened->staff(i)->hideWhenEmpty(), i % 2 ? AutoOnOff::ON : AutoOnOff::OFF);
}
