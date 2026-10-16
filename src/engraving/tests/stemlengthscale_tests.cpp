/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include <gtest/gtest.h>
#include <limits>
#include "engraving/compat/scoreaccess.h"
#include "engraving/dom/beam.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/part.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/stem.h"
#include "engraving/editing/transaction/undostack.h"
#include "engraving/rendering/score/stemlayout.h"
#include "engraving/rendering/score/beamtremololayout.h"
#include "io/buffer.h"

using namespace mu::engraving;
using namespace mu::engraving::rendering::score;

class Engraving_StemLengthScale : public ::testing::Test {
protected:
    std::unique_ptr<MasterScore> score;
    Measure* measure = nullptr;
    void SetUp() override {
        score.reset(compat::ScoreAccess::createMasterScore(nullptr));
        score->lockUpdates(true);
        auto* part = new Part(score.get()); score->appendPart(part);
        score->appendStaff(Factory::createStaff(part));
        measure = Factory::createMeasure(score.get());
        measure->setTick(Fraction()); measure->setTicks(Fraction(1,1)); score->measures()->add(measure);
    }
    Chord* chord(int line, bool up, Fraction tick=Fraction()) {
        auto* segment=measure->getSegment(SegmentType::ChordRest,tick);
        auto* c=Factory::createChord(segment); c->setTrack(0); c->setTicks(Fraction(1,4));
        c->setDurationType(DurationType::V_QUARTER); segment->add(c); c->setUp(up);
        auto* n=Factory::createNote(c); n->setTrack(0); n->setPitch(60); n->setTpcFromPitch(); n->setLine(line);
        n->mutldata()->setPos(0,line*score->style().spatium()/2); c->add(n);
        c->add(Factory::createStem(c));
        return c;
    }
};

TEST_F(Engraving_StemLengthScale, ShortensEveryStaffPositionWithConditionalShorteningOff) {
    score->style().set(Sid::shortenStem,false);
    LayoutContext ctx(score.get());
    for(bool up:{false,true}) for(int line:{-8,-2,0,2,4,6,8,12,16}) {
        SCOPED_TRACE(::testing::Message()<<"line="<<line<<" up="<<up);
        auto* c=chord(line,up);
        score->style().set(Sid::evanStemLengthScale,1.0);
        double normal=StemLayout::calcDefaultStemLength(c,ctx);
        score->style().set(Sid::evanStemLengthScale,0.8);
        double shorter=StemLayout::calcDefaultStemLength(c,ctx);
        EXPECT_NEAR(shorter,normal*0.8,1e-8);
        EXPECT_LT(shorter,normal);
        score->style().set(Sid::evanStemLengthScale,1.0);
        EXPECT_DOUBLE_EQ(StemLayout::calcDefaultStemLength(c,ctx),normal);
        c->segment()->remove(c); delete c;
    }
}

TEST_F(Engraving_StemLengthScale, GraceAndChordShaftKeepTheirSpanAndRespectScale) {
    auto* c=chord(8,true);
    auto* n=Factory::createNote(c); n->setTrack(0); n->setPitch(67); n->setTpcFromPitch();
    n->setLine(2); n->mutldata()->setPos(0,score->style().spatium()); c->add(n);
    const double span=3*score->style().spatium();
    LayoutContext ctx(score.get()); score->style().set(Sid::evanStemLengthScale,0.75);
    const double full=span+4*score->style().spatium();
    EXPECT_NEAR(StemLayout::scaledStemLength(c,full,ctx),span+3*score->style().spatium(),1e-8);
    c->setNoteType(NoteType::ACCIACCATURA);
    const double graceLength=span+3*score->style().spatium()*c->intrinsicMag();
    EXPECT_LT(StemLayout::scaledStemLength(c,graceLength,ctx),graceLength);
    EXPECT_GT(StemLayout::scaledStemLength(c,graceLength,ctx),span);
}

TEST_F(Engraving_StemLengthScale, BeamMovesAsAConnectedGroupWithoutChangingSlope) {
    for(bool up:{true,false}) {
        auto* a=chord(4,up); auto* b=chord(4,up,Fraction(1,4));
        b->mutldata()->setPos(10*score->style().spatium(),0);
        auto* beam=Factory::createBeam(score.get()); beam->add(a); beam->add(b);
        a->setBeam(beam); b->setBeam(beam); a->setDurationType(DurationType::V_EIGHTH); b->setDurationType(DurationType::V_EIGHTH);
        auto* data=beam->mutldata(); data->elements={a,b}; data->up=up; data->spatium=score->style().spatium(); data->beamWidth=0.5*score->style().spatium();
        const double y=(2+(up?-4:4))*score->style().spatium();
        data->startAnchor=PointF(0,y); data->endAnchor=PointF(10*score->style().spatium(),y+0.2*score->style().spatium());
        const auto start=data->startAnchor, end=data->endAnchor;
        score->style().set(Sid::evanStemLengthScale,0.8); LayoutContext ctx(score.get());
        BeamTremoloLayout::applyStemLengthScale(data,ctx);
        EXPECT_GT((data->startAnchor.y()-start.y())*(up?1:-1),0);
        EXPECT_NEAR(data->startAnchor.y()-start.y(),data->endAnchor.y()-end.y(),1e-8);
        EXPECT_EQ(data->startAnchor.x(),start.x()); EXPECT_EQ(data->endAnchor.x(),end.x());
        a->segment()->remove(a); b->segment()->remove(b); delete a; delete b;
    }
}

TEST_F(Engraving_StemLengthScale, InvalidScaleIsBoundedAndHundredPercentPreservesLayout) {
    auto* c=chord(4,true); LayoutContext ctx(score.get()); const double length=4*score->style().spatium();
    score->style().set(Sid::evanStemLengthScale,std::numeric_limits<double>::quiet_NaN());
    EXPECT_DOUBLE_EQ(StemLayout::scaledStemLength(c,length,ctx),length);
    score->style().set(Sid::evanStemLengthScale,-100.0);
    EXPECT_GE(StemLayout::scaledStemLength(c,length,ctx),length*0.5);
    score->style().set(Sid::evanStemLengthScale,1.0);
    EXPECT_DOUBLE_EQ(StemLayout::scaledStemLength(c,length,ctx),length);
}

TEST_F(Engraving_StemLengthScale, StyleIsUndoableAndSurvivesNativeStyleRoundTrip) {
    score->startCmd(muse::TranslatableString::untranslatable("Stem length"));
    score->undoChangeStyleVal(Sid::evanStemLengthScale,0.8); score->endCmd();
    EXPECT_DOUBLE_EQ(score->style().styleD(Sid::evanStemLengthScale),0.8);
    score->undoRedo(true,nullptr); EXPECT_DOUBLE_EQ(score->style().styleD(Sid::evanStemLengthScale),1.0);
    score->undoRedo(false,nullptr); EXPECT_DOUBLE_EQ(score->style().styleD(Sid::evanStemLengthScale),0.8);
    muse::ByteArray bytes; auto buffer=muse::io::Buffer::opened(muse::io::IODevice::WriteOnly,&bytes);
    ASSERT_TRUE(score->style().write(&buffer)); buffer.close();
    auto input=muse::io::Buffer::opened(muse::io::IODevice::ReadOnly,&bytes);
    MStyle restored; ASSERT_TRUE(restored.read(&input));
    EXPECT_DOUBLE_EQ(restored.styleD(Sid::evanStemLengthScale),0.8);
    EXPECT_EQ(restored.styleB(Sid::shortenStem),score->style().styleB(Sid::shortenStem));
}
