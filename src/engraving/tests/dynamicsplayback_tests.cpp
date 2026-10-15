/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include <gtest/gtest.h>
#include "engraving/compat/scoreaccess.h"
#include "engraving/compat/midi/compatmidirender.h"
#include "engraving/dom/articulation.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/dynamic.h"
#include "engraving/dom/dynamicsplayback.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/hairpin.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/part.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/stafftype.h"
#include "engraving/dom/tempotimeline.h"
#include "engraving/dom/tremolosinglechord.h"
#include "engraving/editing/cmd.h"
#include "engraving/editing/transaction/undostack.h"
#include "engraving/rw/read400/tread.h"
#include "engraving/rw/read500/tread.h"
#include "engraving/rw/read500/readcontext.h"
#include "engraving/rw/write/twrite.h"
#include "engraving/rw/write/writecontext.h"
#include "engraving/rw/xmlwriter.h"
#include "engraving/playback/playbackcontext.h"
#include "io/buffer.h"
#include "async/asyncable.h"
#include "serialization/json.h"

using namespace mu::engraving;
using muse::String;

class Engraving_DynamicsPlaybackTests : public ::testing::Test {
protected:
    std::unique_ptr<MasterScore> score;
    std::vector<Note*> notes;
    void SetUp() override {
        score.reset(compat::ScoreAccess::createMasterScore(nullptr));
        score->style().set(Sid::evanDynamicsEnabled, true);
        Part* part = new Part(score.get());
        score->appendPart(part);
        part->instrument()->setUseDrumset(true);
        Staff* staff = Factory::createStaff(part);
        score->appendStaff(staff);
        StaffType type; type.setGroup(StaffGroup::PERCUSSION);
        staff->setStaffType(Fraction(), type);
        for (int measureIndex = 0; measureIndex < 2; ++measureIndex) {
            Measure* measure = Factory::createMeasure(score.get());
            measure->setTick(Fraction(measureIndex, 1));
            measure->setTicks(Fraction(1, 1));
            score->measures()->add(measure);
            for (int beat = 0; beat < 4; ++beat) {
                Segment* segment = measure->getSegment(SegmentType::ChordRest, Fraction(measureIndex * 4 + beat, 4));
                Chord* chord = Factory::createChord(segment);
                chord->setTrack(0); chord->setTicks(Fraction(1, 4));
                chord->setDurationType(DurationType::V_QUARTER);
                segment->add(chord);
                Note* note = Factory::createNote(chord);
                note->setTrack(0); note->setPitch(60); chord->add(note);
                notes.push_back(note);
            }
        }
    }
    void mapping(Sid sid, DynamicType type, int value) {
        std::string text;
        int role = DynamicsPlayback::Normal;
        for (int category = DynamicsPlayback::Normal; category <= DynamicsPlayback::Unstress; ++category) {
            if (DynamicsPlayback::mappingStyle(category) == sid) role = category;
        }
        for (const auto& def : Dynamic::definitions()) {
            text += std::to_string(def.type == type ? value : DynamicsPlayback::level(score.get(), def.type, role)) + " ";
        }
        score->style().set(sid, String::fromStdString(text));
    }
    Dynamic* mark(int index, DynamicType type) {
        Dynamic* d = Factory::createDynamic(notes[index]->chord()->segment());
        d->setTrack(0); d->setDynamicType(type); notes[index]->chord()->segment()->add(d);
        return d;
    }
    void accent(int index) {
        Articulation* a = Factory::createArticulation(notes[index]->chord());
        a->setSymId(SymId::articAccentAbove); notes[index]->chord()->add(a);
    }
    Hairpin* ramp() {
        Hairpin* h = Factory::createHairpin(score->dummy());
        h->setTrack(0); h->setTick(Fraction()); h->setTicks(Fraction(3, 4));
        h->setHairpinType(HairpinType::DIM_HAIRPIN);
        h->setProperty(Pid::DYNAMICS_START_DYNAMIC, static_cast<int>(DynamicType::FF));
        h->setProperty(Pid::DYNAMICS_START_ROLE, DynamicsPlayback::Accent);
        h->setProperty(Pid::DYNAMICS_END_DYNAMIC, static_cast<int>(DynamicType::MP));
        h->setProperty(Pid::DYNAMICS_END_ROLE, DynamicsPlayback::Tap);
        score->addElement(h);
        return h;
    }
    std::unique_ptr<EngravingItem> roundTrip(EngravingItem* item, bool current = false) {
        auto buffer = muse::io::Buffer::opened(muse::io::IODevice::WriteOnly);
        XmlWriter xml(&buffer); xml.startDocument();
        write::WriteContext wc(score.get()); write::TWrite::writeItem(item, xml, wc); xml.flush();
        buffer.close();
        XmlReader reader(buffer.data()); reader.readNextStartElement();
        std::unique_ptr<EngravingItem> result(Factory::createItemByName(reader.name(), score->dummy()));
        if (current) { read500::ReadContext rc(score.get(), true); read500::TRead::readItem(result.get(), reader, rc); }
        else { read400::ReadContext rc(score.get()); read400::TRead::readItem(result.get(), reader, rc); }
        return result;
    }
};

TEST_F(Engraving_DynamicsPlaybackTests, CurveEndpointsAndMonotonicity) {
    for (int shape = 0; shape < 3; ++shape) {
        for (double bend : {-2.0, -0.7, 0.0, 0.7, 2.0}) {
            EXPECT_DOUBLE_EQ(DynamicsPlayback::progress(0, shape, bend), 0);
            EXPECT_DOUBLE_EQ(DynamicsPlayback::progress(1, shape, bend), 1);
            double previous = 0;
            for (int i = 0; i <= 100; ++i) {
                double value = DynamicsPlayback::progress(i / 100.0, shape, bend);
                EXPECT_GE(value, previous); EXPECT_LE(value, 1); previous = value;
            }
        }
    }
    EXPECT_LT(DynamicsPlayback::progress(.5, 0, .7), .5);
    EXPECT_GT(DynamicsPlayback::progress(.5, 0, -.7), .5);
    EXPECT_EQ(DynamicsPlayback::progress(.2, 2, 0), 0);
}

TEST_F(Engraving_DynamicsPlaybackTests, TapsAndAccentsHaveSeparateExactMappings) {
    mapping(Sid::evanDynamicsAccent, DynamicType::FF, 116);
    mapping(Sid::evanDynamicsTap, DynamicType::FF, 40);
    mark(0, DynamicType::FF); accent(0);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[0], 80), 116);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[1], 80), 40);
    score->style().set(Sid::evanDynamicsEnabled, false);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[0], 80), 80);
}

TEST_F(Engraving_DynamicsPlaybackTests, LegacyQuietMarkingCannotOverrideCustomTaps) {
    mapping(Sid::evanDynamicsTap, DynamicType::P, 30);
    Dynamic* d = mark(0, DynamicType::P);
    d->setProperty(Pid::VELOCITY, 64); // Uploaded Pad Lick #1 first bar.
    EXPECT_EQ(DynamicsPlayback::velocity(notes[0], 80), 30);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[1], 80), 30);
    d->setVisible(false);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[1], 80), 30);
    // A marking override edits the accent lane, including zero, leaving taps alone.
    d->setProperty(Pid::DYNAMICS_MARK_VELOCITY, 0);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[1], 80), 30);
    d->setProperty(Pid::DYNAMICS_MARK_VELOCITY, -1);
    notes[1]->setProperty(Pid::VELO_TYPE, VeloType::USER_VAL);
    notes[1]->setProperty(Pid::USER_VELOCITY, 57);
    EXPECT_EQ(notes[1]->customizeVelocity(DynamicsPlayback::velocity(notes[1], 80)), 57);
}

TEST_F(Engraving_DynamicsPlaybackTests, SmoothRampUsesOneEnvelopeAcrossIntermediateArticulations) {
    mapping(Sid::evanDynamicsAccent, DynamicType::FF, 110);
    mapping(Sid::evanDynamicsTenuto, DynamicType::FF, 80);
    mapping(Sid::evanDynamicsTap, DynamicType::MP, 30);
    accent(0);
    Articulation* tenuto = Factory::createArticulation(notes[1]->chord());
    tenuto->setSymId(SymId::articTenutoAbove); notes[1]->chord()->add(tenuto);
    Hairpin* h = ramp();
    EXPECT_FALSE(DynamicsPlayback::smoothArticulations(h));
    // Existing articulation lanes remain available with the switch off.
    EXPECT_EQ(DynamicsPlayback::velocity(notes[1], 80), 63);
    score->style().set(Sid::evanDynamicsSmoothArticulations, true);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[0], 80), 110);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[1], 80), 83);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[2], 80), 57);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[3], 80), 30);
    EXPECT_EQ(DynamicsPlayback::velocityAt(notes[1], Fraction(3, 8), 80), 70);
    EXPECT_EQ(DynamicsPlayback::hairpinValue(h, .5, DynamicsPlayback::Ghost), 70);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[4], 80), 30);
    h->setProperty(Pid::DYNAMICS_SMOOTH_ARTICULATIONS, 0);
    EXPECT_FALSE(DynamicsPlayback::smoothArticulations(h));
    EXPECT_EQ(DynamicsPlayback::velocity(notes[1], 80), 63);
    h->setProperty(Pid::DYNAMICS_SMOOTH_ARTICULATIONS, -1);
    EXPECT_TRUE(DynamicsPlayback::smoothArticulations(h));
}

TEST_F(Engraving_DynamicsPlaybackTests, SmoothRampInfersPrintedEndpointRolesAndWorksBothDirections) {
    mapping(Sid::evanDynamicsAccent, DynamicType::MF, 100);
    mapping(Sid::evanDynamicsTap, DynamicType::P, 30);
    mark(0, DynamicType::MF); mark(3, DynamicType::P);
    accent(0);
    Hairpin* h = ramp();
    for (Pid id : {Pid::DYNAMICS_START_DYNAMIC, Pid::DYNAMICS_END_DYNAMIC}) h->setProperty(id, -1);
    for (Pid id : {Pid::DYNAMICS_START_ROLE, Pid::DYNAMICS_END_ROLE}) h->setProperty(id, 0);
    h->setProperty(Pid::DYNAMICS_SMOOTH_ARTICULATIONS, 1);
    EXPECT_EQ(DynamicsPlayback::hairpinValue(h, 0, DynamicsPlayback::Tenuto), 100);
    EXPECT_EQ(DynamicsPlayback::hairpinValue(h, .5, DynamicsPlayback::Tap), 65);
    EXPECT_EQ(DynamicsPlayback::hairpinValue(h, 1, DynamicsPlayback::Accent), 30);
    h->setHairpinType(HairpinType::CRESC_HAIRPIN);
    h->setProperty(Pid::DYNAMICS_START_VELOCITY, 30);
    h->setProperty(Pid::DYNAMICS_END_VELOCITY, 100);
    EXPECT_EQ(DynamicsPlayback::hairpinValue(h, .5, DynamicsPlayback::Tenuto), 65);
}

TEST_F(Engraving_DynamicsPlaybackTests, AccentToTapDecrescendoResolvesCorrectEndpointAndHolds) {
    mapping(Sid::evanDynamicsAccent, DynamicType::FF, 116);
    mapping(Sid::evanDynamicsAccent, DynamicType::MP, 75);
    mapping(Sid::evanDynamicsTap, DynamicType::MP, 52);
    mark(0, DynamicType::FF); accent(0); Hairpin* h = ramp();
    EXPECT_EQ(DynamicsPlayback::endpoint(h, false), 116);
    EXPECT_EQ(DynamicsPlayback::endpoint(h, true), 52);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[0], 80), 116);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[3], 80), 52);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[4], 80), 52);
    EXPECT_EQ(DynamicsPlayback::hairpinValue(h, 0, DynamicsPlayback::Tap), 49);
    EXPECT_EQ(DynamicsPlayback::hairpinValue(h, 1, DynamicsPlayback::Tap), 52);
    mark(5, DynamicType::P);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[5], 80), 49);
}

TEST_F(Engraving_DynamicsPlaybackTests, LocalCurvesAndVoiceAssignment) {
    Hairpin* h = ramp();
    h->setProperty(Pid::DYNAMICS_CURVE_SHAPE, 0);
    h->setProperty(Pid::DYNAMICS_CURVE_BEND, .7);
    EXPECT_GT(DynamicsPlayback::hairpinValue(h, .5), (DynamicsPlayback::endpoint(h, false) + DynamicsPlayback::endpoint(h, true)) / 2.0);
    h->setProperty(Pid::VOICE_ASSIGNMENT, static_cast<int>(VoiceAssignment::CURRENT_VOICE_ONLY));
    EXPECT_TRUE(DynamicsPlayback::applies(h, 0)); EXPECT_FALSE(DynamicsPlayback::applies(h, 1));
    h->setProperty(Pid::VOICE_ASSIGNMENT, static_cast<int>(VoiceAssignment::ALL_VOICE_IN_STAFF));
    EXPECT_TRUE(DynamicsPlayback::applies(h, 1));
}

TEST_F(Engraving_DynamicsPlaybackTests, ZeroIsSilentWithoutOverwritingIntrinsicPlayFlag) {
    mapping(Sid::evanDynamicsTap, DynamicType::MF, 0);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[0], 80), 0);
    EXPECT_FALSE(notes[0]->play()); EXPECT_TRUE(notes[0]->getProperty(Pid::PLAY).toBool());
    notes[0]->setProperty(Pid::VELO_TYPE, VeloType::USER_VAL); notes[0]->setProperty(Pid::USER_VELOCITY, 92);
    EXPECT_TRUE(notes[0]->play());
    notes[0]->setProperty(Pid::PLAY, false); EXPECT_FALSE(notes[0]->play());
}

TEST_F(Engraving_DynamicsPlaybackTests, CompoundAttackDoesNotOverwriteSettledLevel) {
    Dynamic* d = mark(0, DynamicType::FP);
    d->setProperty(Pid::DYNAMICS_MARK_VELOCITY, 118);
    accent(0); accent(1);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[0], 80), 118);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[1], 80), DynamicsPlayback::level(score.get(), DynamicType::P, DynamicsPlayback::Accent));
}

TEST_F(Engraving_DynamicsPlaybackTests, HairpinAndMarkingPropertiesSurviveNativeXml) {
    Hairpin* h = ramp();
    h->setProperty(Pid::DYNAMICS_CURVE_SHAPE, 1); h->setProperty(Pid::DYNAMICS_CURVE_BEND, .65);
    h->setProperty(Pid::DYNAMICS_SMOOTH_ARTICULATIONS, 1);
    h->setProperty(Pid::DYNAMICS_START_VELOCITY, 127); h->setProperty(Pid::DYNAMICS_END_VELOCITY, 0);
    auto copy = roundTrip(h);
    for (Pid id : {Pid::DYNAMICS_CURVE_SHAPE, Pid::DYNAMICS_CURVE_BEND, Pid::DYNAMICS_START_DYNAMIC, Pid::DYNAMICS_END_DYNAMIC,
                   Pid::DYNAMICS_START_ROLE, Pid::DYNAMICS_END_ROLE, Pid::DYNAMICS_START_VELOCITY, Pid::DYNAMICS_END_VELOCITY, Pid::DYNAMICS_SMOOTH_ARTICULATIONS}) {
        EXPECT_EQ(copy->getProperty(id), h->getProperty(id));
    }
    Dynamic* d = mark(0, DynamicType::FF); d->setProperty(Pid::DYNAMICS_MARK_VELOCITY, 0);
    auto dc = roundTrip(d); EXPECT_EQ(dc->getProperty(Pid::DYNAMICS_MARK_VELOCITY).toInt(), 0);
}

TEST_F(Engraving_DynamicsPlaybackTests, ScoreMappingsSurviveStyleXml) {
    mapping(Sid::evanDynamicsAccent, DynamicType::FF, 116);
    auto buffer = muse::io::Buffer::opened(muse::io::IODevice::WriteOnly);
    ASSERT_TRUE(score->style().write(&buffer)); buffer.close();
    auto bytes = buffer.data();
    muse::io::Buffer input(&bytes); ASSERT_TRUE(input.open(muse::io::IODevice::ReadOnly));
    MStyle restored; ASSERT_TRUE(restored.read(&input));
    EXPECT_EQ(restored.styleSt(Sid::evanDynamicsAccent), score->style().styleSt(Sid::evanDynamicsAccent));
    EXPECT_TRUE(restored.styleB(Sid::evanDynamicsEnabled));
}

TEST_F(Engraving_DynamicsPlaybackTests, CurveAndNoteEditsUndoTogether) {
    Hairpin* h = ramp();
    UndoStack* undo = score->undoStack();
    score->lockUpdates(true);
    score->startCmd(muse::TranslatableString("undoableAction", "Edit dynamics"));
    h->undoChangeProperty(Pid::DYNAMICS_CURVE_SHAPE, 1);
    h->undoChangeProperty(Pid::DYNAMICS_CURVE_BEND, .7);
    notes[0]->undoChangeProperty(Pid::VELO_TYPE, VeloType::USER_VAL);
    notes[0]->undoChangeProperty(Pid::USER_VELOCITY, 92);
    score->endCmd();
    EXPECT_EQ(h->dynamicsCurveShape(), 1); EXPECT_EQ(notes[0]->userVelocity(), 92);
    undo->undo(nullptr);
    EXPECT_EQ(h->dynamicsCurveShape(), -1); EXPECT_EQ(notes[0]->userVelocity(), 0);
    undo->redo();
    EXPECT_EQ(h->dynamicsCurveShape(), 1); EXPECT_DOUBLE_EQ(h->dynamicsCurveBend(), .7);
    EXPECT_EQ(notes[0]->userVelocity(), 92);
}

TEST_F(Engraving_DynamicsPlaybackTests, PlaybackProfileEditAvoidsPrintedLayoutAndStillUndoes) {
    score->lockUpdates(true);
    muse::async::Asyncable receiver;
    int playbackNotifications = 0;
    score->changesChannel().onReceive(&receiver, [&](const ScoreChanges& changes) {
        if (changes.changedStyleIdSet.count(Sid::evanDynamicsTap)) ++playbackNotifications;
    });
    const auto original = score->style().styleSt(Sid::evanDynamicsTap);
    score->cmdState().reset();
    score->startCmd(muse::TranslatableString::untranslatable("Tap profile"));
    score->undoChangeStyleVal(Sid::evanDynamicsTap, String(u"49 49 49 49 49 49 49 49 49 49 49 49"));
    EXPECT_FALSE(score->cmdState().layoutRange());
    score->endCmd();
    EXPECT_EQ(playbackNotifications, 1);
    EXPECT_EQ(DynamicsPlayback::level(score.get(), DynamicType::FF, DynamicsPlayback::Tap), 49);
    score->undoRedo(true, nullptr);
    EXPECT_EQ(playbackNotifications, 2);
    EXPECT_EQ(score->style().styleSt(Sid::evanDynamicsTap), original);
    score->undoRedo(false, nullptr);
    EXPECT_EQ(playbackNotifications, 3);
    EXPECT_EQ(DynamicsPlayback::level(score.get(), DynamicType::FF, DynamicsPlayback::Tap), 49);
    // Ordinary engraving settings must continue to request a layout.
    score->cmdState().reset();
    score->startCmd(muse::TranslatableString::untranslatable("Staff spacing"));
    score->undoChangeStyleVal(Sid::staffDistance, score->style().styleS(Sid::staffDistance) + Spatium(1));
    EXPECT_TRUE(score->cmdState().layoutRange());
    score->endCmd();
}

TEST_F(Engraving_DynamicsPlaybackTests, AudioAutomationUsesTheSameExactNoteAndCurveValues) {
    mapping(Sid::evanDynamicsAccent, DynamicType::FF, 116);
    mapping(Sid::evanDynamicsTap, DynamicType::MP, 52);
    mark(0, DynamicType::FF); accent(0);
    Hairpin* h = ramp(); h->setProperty(Pid::DYNAMICS_CURVE_BEND, .7);
    score->updateRepeatList();
    PlaybackContext context(score.get());
    auto layers = context.dynamicLevelLayers(0, 4);
    ASSERT_EQ(layers.size(), 1);
    const auto& layer = layers.begin()->second;
    ASSERT_GT(layer.size(), 8);
    EXPECT_NEAR(layer.begin()->second.outValue, 116.0 / 127, .00001);
    const auto& timeline = score->tempoTimeline(true);
    const auto end = timeline.utick2utime(Fraction(3, 4).ticks()) * 1000000;
    ASSERT_TRUE(layer.contains(end));
    EXPECT_NEAR(layer.at(end).outValue, 52.0 / 127, .00001);
    notes[1]->setProperty(Pid::VELO_TYPE, VeloType::USER_VAL);
    notes[1]->setProperty(Pid::USER_VELOCITY, 91);
    layers = context.dynamicLevelLayers(0, 4);
    const auto noteTime = timeline.utick2utime(Fraction(1, 4).ticks()) * 1000000;
    EXPECT_NEAR(layers.begin()->second.at(noteTime).outValue, 91.0 / 127, .00001);
}

TEST_F(Engraving_DynamicsPlaybackTests, MidiEventsUseExactTapAndAccentVelocities) {
    mapping(Sid::evanDynamicsAccent, DynamicType::FF, 116);
    mapping(Sid::evanDynamicsTap, DynamicType::FF, 40);
    mark(0, DynamicType::FF); accent(0);
    score->rebuildMidiMapping();
    EventsHolder events;
    CompatMidiRender::renderScore(score.get(), events, {}, true);
    std::map<int, int> velocities;
    for (size_t channel = 0; channel < events.size(); ++channel) {
        for (const auto& [tick, event] : events[channel]) {
            if (event.type() == ME_NOTEON && event.velo() > 0) velocities[tick] = event.velo();
        }
    }
    ASSERT_GE(velocities.size(), 2);
    EXPECT_EQ(velocities.at(0), 116);
    EXPECT_EQ(velocities.at(Fraction(1, 4).ticks()), 40);
}

TEST_F(Engraving_DynamicsPlaybackTests, RollStrokesFollowTheCurveAtTheirOwnTime) {
    mapping(Sid::evanDynamicsAccent, DynamicType::FF, 116);
    mapping(Sid::evanDynamicsTap, DynamicType::MP, 52);
    mark(0, DynamicType::FF); accent(0);
    Hairpin* h = ramp(); h->setProperty(Pid::DYNAMICS_CURVE_BEND, -.7);
    TremoloSingleChord* tremolo = Factory::createTremoloSingleChord(notes[0]->chord());
    tremolo->setTremoloType(TremoloType::R32); notes[0]->chord()->add(tremolo);
    score->rebuildMidiMapping();
    EventsHolder events; CompatMidiRender::renderScore(score.get(), events, {}, true);
    int strokes = 0;
    for (size_t channel = 0; channel < events.size(); ++channel) {
        for (const auto& [tick, event] : events[channel]) {
            if (event.type() != ME_NOTEON || event.velo() <= 0 || tick >= Fraction(1, 4).ticks()) continue;
            EXPECT_EQ(event.velo(), DynamicsPlayback::velocityAt(notes[0], Fraction::fromTicks(tick), 80));
            ++strokes;
        }
    }
    EXPECT_GT(strokes, 2);
}

TEST_F(Engraving_DynamicsPlaybackTests, TenutoMarcatoAndGhostHaveIndependentMappings) {
    mapping(Sid::evanDynamicsTenuto, DynamicType::MF, 71);
    mapping(Sid::evanDynamicsAccent, DynamicType::MF, 99);
    mapping(Sid::evanDynamicsMarcato, DynamicType::MF, 123);
    mapping(Sid::evanDynamicsGhost, DynamicType::MF, 19);
    auto articulation = [&](int index, SymId symbol) {
        Articulation* a = Factory::createArticulation(notes[index]->chord());
        a->setSymId(symbol); notes[index]->chord()->add(a);
    };
    articulation(0, SymId::articTenutoAbove);
    accent(1); articulation(1, SymId::articTenutoAbove);
    accent(2); articulation(2, SymId::articMarcatoAbove);
    notes[3]->setGhost(true);
    EXPECT_EQ(DynamicsPlayback::role(notes[0]), DynamicsPlayback::Tenuto);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[0], 80), 71);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[1], 80), 99);
    EXPECT_EQ(DynamicsPlayback::role(notes[2]), DynamicsPlayback::Marcato);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[2], 80), 123);
    EXPECT_EQ(DynamicsPlayback::role(notes[3]), DynamicsPlayback::Ghost);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[3], 80), 19);
    score->style().set(Sid::evanDynamicsBattery, false);
    EXPECT_EQ(DynamicsPlayback::role(notes[4]), DynamicsPlayback::Normal);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[0], 80), 71);
}

TEST_F(Engraving_DynamicsPlaybackTests, NoteheadAppearanceDoesNotChangePlaybackVelocity) {
    const int original = DynamicsPlayback::velocity(notes[0], 80);
    notes[0]->setProperty(Pid::HEAD_TYPE, NoteHeadType::HEAD_HALF);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[0], 80), original);
    EXPECT_EQ(notes[0]->chord()->durationType().type(), DurationType::V_QUARTER);
}

TEST_F(Engraving_DynamicsPlaybackTests, PresetRoundTripIncludesEveryRoleAndPreservesEngraving) {
    for (int role = DynamicsPlayback::Normal; role <= DynamicsPlayback::Unstress; ++role) {
        mapping(DynamicsPlayback::mappingStyle(role), DynamicType::PPP, role * 11);
        mapping(DynamicsPlayback::mappingStyle(role), DynamicType::F, role == DynamicsPlayback::Ghost ? 0 : 127);
    }
    score->style().set(Sid::evanDynamicsCurveShape, 1);
    score->style().set(Sid::evanDynamicsCurveBend, .73);
    score->style().set(Sid::evanDynamicsSmoothArticulations, true);
    const auto data = DynamicsPlayback::preset(score.get());
    MStyle restored;
    restored.set(Sid::musicalSymbolFont, String(u"Bravura"));
    const auto musicalFont = restored.value(Sid::musicalSymbolFont);
    ASSERT_TRUE(DynamicsPlayback::readPreset(data, restored));
    EXPECT_EQ(restored.value(Sid::musicalSymbolFont), musicalFont);
    for (Sid sid : { Sid::evanDynamicsEnabled, Sid::evanDynamicsBattery, Sid::evanDynamicsCurveShape, Sid::evanDynamicsCurveBend, Sid::evanDynamicsSmoothArticulations }) {
        EXPECT_EQ(restored.value(sid), score->style().value(sid));
    }
    auto second = std::unique_ptr<MasterScore>(compat::ScoreAccess::createMasterScore(nullptr));
    second->setStyle(restored);
    // Presets normalize the unused OTHER slot; compare every playable marking's actual value.
    for (int role = DynamicsPlayback::Normal; role <= DynamicsPlayback::Unstress; ++role) {
        for (const auto& def : Dynamic::definitions()) {
            if (def.type == DynamicType::OTHER) continue;
            EXPECT_EQ(DynamicsPlayback::level(second.get(), def.type, role), DynamicsPlayback::level(score.get(), def.type, role));
        }
        EXPECT_EQ(DynamicsPlayback::level(second.get(), DynamicType::PPP, role), role * 11);
        EXPECT_EQ(DynamicsPlayback::level(second.get(), DynamicType::F, role), role == DynamicsPlayback::Ghost ? 0 : 127);
    }
}

TEST_F(Engraving_DynamicsPlaybackTests, InvalidPresetNeverPartiallyChangesSettings) {
    const auto valid = muse::JsonDocument::fromJson(DynamicsPlayback::preset(score.get())).rootObject();
    MStyle target;
    target.set(Sid::evanDynamicsCurveBend, -1.1);
    auto check = [&](const muse::JsonObject& root) {
        const MStyle before = target;
        EXPECT_FALSE(DynamicsPlayback::readPreset(muse::JsonDocument(root).toJson(), target));
        for (Sid sid : DynamicsPlayback::profileStyles()) EXPECT_EQ(target.value(sid), before.value(sid));
    };
    auto version = valid; version["version"] = 2; check(version);
    auto smoothing = valid; smoothing["smoothArticulations"] = "yes"; check(smoothing);
    auto shape = valid; shape["curveShape"] = 9e30; check(shape);
    auto missing = valid; missing["mappings"] = muse::JsonObject(); check(missing);
    for (double invalid : {-1.0, 128.0, 25.5, 9e30}) {
        auto root = valid;
        auto mappings = root.value("mappings").toObject();
        auto ghost = mappings.value("ghost").toObject();
        ghost["ppp"] = invalid; mappings["ghost"] = ghost; root["mappings"] = mappings;
        check(root);
    }
    EXPECT_FALSE(DynamicsPlayback::readPreset(muse::ByteArray("garbage"), target));
}

TEST_F(Engraving_DynamicsPlaybackTests, EveryRoleMappingSurvivesStyleXml) {
    score->style().set(Sid::evanDynamicsSmoothArticulations, true);
    for (int role = DynamicsPlayback::Normal; role <= DynamicsPlayback::Unstress; ++role) mapping(DynamicsPlayback::mappingStyle(role), DynamicType::PP, role * 13);
    auto buffer = muse::io::Buffer::opened(muse::io::IODevice::WriteOnly);
    ASSERT_TRUE(score->style().write(&buffer)); buffer.close();
    auto bytes = buffer.data(); muse::io::Buffer input(&bytes); ASSERT_TRUE(input.open(muse::io::IODevice::ReadOnly));
    MStyle restored; ASSERT_TRUE(restored.read(&input));
    for (Sid sid : DynamicsPlayback::profileStyles()) EXPECT_EQ(restored.value(sid), score->style().value(sid));
}

TEST_F(Engraving_DynamicsPlaybackTests, MixedTapDestinationUsesItsExplicitVelocityForMarcato) {
    Hairpin* h = ramp();
    h->setProperty(Pid::DYNAMICS_END_VELOCITY, 47);
    EXPECT_EQ(DynamicsPlayback::hairpinValue(h, 1.0, DynamicsPlayback::Marcato), 47);
    EXPECT_EQ(DynamicsPlayback::hairpinValue(h, 1.0, DynamicsPlayback::Tap), 47);
}

TEST_F(Engraving_DynamicsPlaybackTests, RollCanCrescendoFromSilenceWithoutMutingLaterStrokes) {
    Hairpin* h = ramp();
    h->setHairpinType(HairpinType::CRESC_HAIRPIN);
    h->setProperty(Pid::DYNAMICS_START_ROLE, 0); h->setProperty(Pid::DYNAMICS_END_ROLE, 0);
    h->setProperty(Pid::DYNAMICS_START_VELOCITY, 0); h->setProperty(Pid::DYNAMICS_END_VELOCITY, 127);
    TremoloSingleChord* tremolo = Factory::createTremoloSingleChord(notes[0]->chord());
    tremolo->setTremoloType(TremoloType::R32); notes[0]->chord()->add(tremolo);
    EXPECT_TRUE(notes[0]->play());
    EXPECT_EQ(DynamicsPlayback::velocityAt(notes[0], Fraction(), 80), 0);
    score->rebuildMidiMapping();
    EventsHolder events; CompatMidiRender::renderScore(score.get(), events, {}, true);
    int strokes = 0;
    for (size_t channel = 0; channel < events.size(); ++channel) {
        for (const auto& [tick, event] : events[channel]) {
            if (event.type() != ME_NOTEON || event.velo() <= 0 || tick >= Fraction(1, 4).ticks()) continue;
            EXPECT_GT(tick, 0);
            EXPECT_EQ(event.velo(), DynamicsPlayback::velocityAt(notes[0], Fraction::fromTicks(tick), 80));
            ++strokes;
        }
    }
    EXPECT_GT(strokes, 1);
}

TEST_F(Engraving_DynamicsPlaybackTests, SoftAccentStressAndUnstressAreDetectedIndependently) {
    const std::array<int, 3> roles { DynamicsPlayback::SoftAccent, DynamicsPlayback::Stress, DynamicsPlayback::Unstress };
    const std::array<SymId, 3> symbols { SymId::articSoftAccentTenutoStaccatoAbove, SymId::articStressAbove, SymId::articUnstressAbove };
    for (size_t i = 0; i < roles.size(); ++i) {
        mapping(DynamicsPlayback::mappingStyle(roles[i]), DynamicType::MF, 23 + i * 30);
        Articulation* a = Factory::createArticulation(notes[i]->chord());
        a->setSymId(symbols[i]); notes[i]->chord()->add(a);
        EXPECT_EQ(DynamicsPlayback::role(notes[i]), roles[i]);
        EXPECT_EQ(DynamicsPlayback::velocity(notes[i], 80), 23 + i * 30);
    }
}

TEST_F(Engraving_DynamicsPlaybackTests, CombinedAccentTenutoAndPlaybackDisabledArticulations) {
    Articulation* a = Factory::createArticulation(notes[0]->chord());
    a->setSymId(SymId::articTenutoAccentAbove); notes[0]->chord()->add(a);
    EXPECT_EQ(DynamicsPlayback::role(notes[0]), DynamicsPlayback::Accent);
    a->setPlayArticulation(false);
    EXPECT_EQ(DynamicsPlayback::role(notes[0]), DynamicsPlayback::Tap);
}

TEST_F(Engraving_DynamicsPlaybackTests, OlderPresetRetainsIndependentArticulationLanes) {
    const auto current = muse::JsonDocument::fromJson(DynamicsPlayback::preset(score.get())).rootObject();
    muse::JsonObject legacy;
    for (const auto& key : current.keys()) if (key != "smoothArticulations") legacy[key] = current.value(key);
    MStyle target;
    target.set(Sid::evanDynamicsSmoothArticulations, true);
    ASSERT_TRUE(DynamicsPlayback::readPreset(muse::JsonDocument(legacy).toJson(), target));
    EXPECT_FALSE(target.styleB(Sid::evanDynamicsSmoothArticulations));
}

TEST_F(Engraving_DynamicsPlaybackTests, MarchingStartingProfileFollowsStrokeRolesAndPreservesNotation) {
    score->style().set(Sid::musicalSymbolFont, String(u"Bravura"));
    score->setStyle(DynamicsPlayback::marchingSnareDefaults(score->style()));
    EXPECT_EQ(score->style().styleSt(Sid::musicalSymbolFont), String(u"Bravura"));
    EXPECT_TRUE(DynamicsPlayback::enabled(score.get()));
    EXPECT_TRUE(score->style().styleB(Sid::evanDynamicsSmoothArticulations));
    const std::array<std::pair<DynamicType, int>, 7> levels {{
        {DynamicType::PP, 48}, {DynamicType::P, 58}, {DynamicType::MP, 69},
        {DynamicType::MF, 77}, {DynamicType::F, 88}, {DynamicType::FF, 97}, {DynamicType::FFF, 113}
    }};
    for (const auto& [type, normal] : levels) {
        EXPECT_EQ(DynamicsPlayback::level(score.get(), type, DynamicsPlayback::Normal), normal);
        EXPECT_EQ(DynamicsPlayback::level(score.get(), type, DynamicsPlayback::Tap), type == DynamicType::PP ? 48 : 58);
        EXPECT_EQ(DynamicsPlayback::level(score.get(), type, DynamicsPlayback::Ghost), 0);
    }
    for (auto type : {DynamicType::PP, DynamicType::P, DynamicType::MP}) {
        EXPECT_EQ(DynamicsPlayback::level(score.get(), type, DynamicsPlayback::Accent),
                  DynamicsPlayback::level(score.get(), type, DynamicsPlayback::Tenuto));
    }
    EXPECT_EQ(DynamicsPlayback::level(score.get(), DynamicType::PP, DynamicsPlayback::Accent), 58);
    EXPECT_EQ(DynamicsPlayback::level(score.get(), DynamicType::P, DynamicsPlayback::Accent), 62);
    EXPECT_EQ(DynamicsPlayback::level(score.get(), DynamicType::MP, DynamicsPlayback::Accent), 69);
    for (const auto& def : Dynamic::definitions()) {
        for (int role = DynamicsPlayback::Normal; role <= DynamicsPlayback::Unstress; ++role) {
            const int value = DynamicsPlayback::level(score.get(), def.type, role);
            EXPECT_GE(value, 0); EXPECT_LE(value, 127);
        }
    }
}

TEST_F(Engraving_DynamicsPlaybackTests, SilentGhostRemainsSilentInsideAndAfterSharedRamp) {
    score->setStyle(DynamicsPlayback::marchingSnareDefaults(score->style()));
    score->style().set(Sid::evanDynamicsSmoothArticulations, true);
    mark(0, DynamicType::FF); accent(0); ramp();
    notes[1]->setGhost(true); notes[4]->setGhost(true);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[1], 80), 0);
    EXPECT_EQ(DynamicsPlayback::velocityAt(notes[1], Fraction(3, 8), 80), 0);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[4], 80), 0);
    EXPECT_GT(DynamicsPlayback::velocity(notes[2], 80), 0);
}

TEST_F(Engraving_DynamicsPlaybackTests, SharedRampMatchesMidiAndAudioAtTenutoAndTapNotes) {
    mapping(Sid::evanDynamicsAccent, DynamicType::FF, 110);
    mapping(Sid::evanDynamicsTenuto, DynamicType::FF, 80);
    mapping(Sid::evanDynamicsTap, DynamicType::MP, 30);
    mark(0, DynamicType::FF); accent(0); ramp();
    Articulation* tenuto = Factory::createArticulation(notes[1]->chord());
    tenuto->setSymId(SymId::articTenutoAbove); notes[1]->chord()->add(tenuto);
    score->style().set(Sid::evanDynamicsSmoothArticulations, true);
    score->rebuildMidiMapping(); score->updateRepeatList();
    EventsHolder events; CompatMidiRender::renderScore(score.get(), events, {}, true);
    std::map<int, int> velocities;
    for (size_t channel = 0; channel < events.size(); ++channel) {
        for (const auto& [tick, event] : events[channel]) {
            if (event.type() == ME_NOTEON && event.velo() > 0) velocities[tick] = event.velo();
        }
    }
    PlaybackContext context(score.get());
    const auto layers = context.dynamicLevelLayers(0, 4);
    ASSERT_EQ(layers.size(), 1);
    const auto& layer = layers.begin()->second;
    const auto& timeline = score->tempoTimeline(true);
    for (int index = 0; index < 4; ++index) {
        const int tick = notes[index]->tick().ticks();
        const int expected = DynamicsPlayback::velocity(notes[index], 80);
        ASSERT_TRUE(velocities.contains(tick));
        EXPECT_EQ(velocities.at(tick), expected);
        const auto time = timeline.utick2utime(tick) * 1000000;
        ASSERT_TRUE(layer.contains(time));
        EXPECT_NEAR(layer.at(time).outValue, expected / 127.0, .00001);
    }
}

TEST_F(Engraving_DynamicsPlaybackTests, LocalSmoothingUndoReturnsToInheritedScoreSetting) {
    Hairpin* h = ramp();
    score->lockUpdates(true);
    score->style().set(Sid::evanDynamicsSmoothArticulations, true);
    score->startCmd(muse::TranslatableString::untranslatable("Local smoothing"));
    h->undoChangeProperty(Pid::DYNAMICS_SMOOTH_ARTICULATIONS, 0);
    score->endCmd();
    EXPECT_FALSE(DynamicsPlayback::smoothArticulations(h));
    score->undoStack()->undo(nullptr);
    EXPECT_EQ(h->dynamicsSmoothArticulations(), -1);
    EXPECT_TRUE(DynamicsPlayback::smoothArticulations(h));
    score->undoStack()->redo();
    EXPECT_FALSE(DynamicsPlayback::smoothArticulations(h));
}

TEST_F(Engraving_DynamicsPlaybackTests, SharedRampWithoutEndMarkArrivesAtChangedArticulationMapping) {
    mapping(Sid::evanDynamicsAccent, DynamicType::MF, 100);
    mapping(Sid::evanDynamicsTap, DynamicType::MF, 30);
    mark(0, DynamicType::MF); accent(0);
    Hairpin* h = ramp();
    for (Pid id : {Pid::DYNAMICS_START_DYNAMIC, Pid::DYNAMICS_END_DYNAMIC}) h->setProperty(id, -1);
    for (Pid id : {Pid::DYNAMICS_START_ROLE, Pid::DYNAMICS_END_ROLE}) h->setProperty(id, 0);
    h->setProperty(Pid::DYNAMICS_SMOOTH_ARTICULATIONS, 1);
    EXPECT_EQ(DynamicsPlayback::endpoint(h, true), 30);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[3], 80), 30);
    h->setVeloChange(12);
    EXPECT_EQ(DynamicsPlayback::endpoint(h, true), 18);
    h->setVeloChange(0);
    h->setHairpinType(HairpinType::CRESC_HAIRPIN);
    auto* articulation = notes[0]->chord()->articulations().front();
    articulation->setPlayArticulation(false); accent(3);
    EXPECT_EQ(DynamicsPlayback::hairpinValue(h, 0), 30);
    EXPECT_EQ(DynamicsPlayback::hairpinValue(h, 1), 100);
}

TEST_F(Engraving_DynamicsPlaybackTests, ImportedMarkingVelocityCannotReplaceEditedAccentMapping) {
    mapping(Sid::evanDynamicsAccent, DynamicType::FF, 87);
    mapping(Sid::evanDynamicsTap, DynamicType::FF, 30);
    Dynamic* d = mark(0, DynamicType::FF); accent(0);
    d->setProperty(Pid::VELOCITY, 112); // Imported stock value differs in some libraries.
    EXPECT_EQ(DynamicsPlayback::velocity(notes[0], 80), 87);
    d->setProperty(Pid::VELOCITY, 101);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[0], 80), 87);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[1], 80), 30);
}

TEST_F(Engraving_DynamicsPlaybackTests, EditingBatteryMarkingDoesNotReplaceOtherStrokeCategories) {
    for (auto type : {DynamicType::P, DynamicType::FF}) {
        mapping(Sid::evanDynamicsTap, type, 30);
        mapping(Sid::evanDynamicsTenuto, type, 65);
        mapping(Sid::evanDynamicsMarcato, type, 117);
        mapping(Sid::evanDynamicsGhost, type, 0);
    }
    Dynamic* d = mark(0, DynamicType::FF); accent(0);
    auto* tenuto = Factory::createArticulation(notes[2]->chord());
    tenuto->setSymId(SymId::articTenutoAbove); notes[2]->chord()->add(tenuto);
    auto* marcato = Factory::createArticulation(notes[3]->chord());
    marcato->setSymId(SymId::articMarcatoAbove); notes[3]->chord()->add(marcato);
    notes[4]->setGhost(true);
    for (int velocity : {0, 1, 49, 100, 127}) {
        d->setProperty(Pid::DYNAMICS_MARK_VELOCITY, velocity);
        EXPECT_EQ(DynamicsPlayback::velocity(notes[0], 80), velocity);
        EXPECT_EQ(DynamicsPlayback::velocity(notes[1], 80), 30);
        EXPECT_EQ(DynamicsPlayback::velocity(notes[2], 80), 65);
        EXPECT_EQ(DynamicsPlayback::velocity(notes[3], 80), 117);
        EXPECT_EQ(DynamicsPlayback::velocity(notes[4], 80), 0);
    }
    d->setProperty(Pid::DYNAMIC_TYPE, DynamicType::P);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[1], 80), 30);
}

TEST_F(Engraving_DynamicsPlaybackTests, CompletedSmoothHairpinRestoresCategorySeparation) {
    mapping(Sid::evanDynamicsAccent, DynamicType::FF, 110);
    mapping(Sid::evanDynamicsAccent, DynamicType::MP, 85);
    mapping(Sid::evanDynamicsTap, DynamicType::MP, 30);
    mapping(Sid::evanDynamicsTenuto, DynamicType::MP, 60);
    mark(0, DynamicType::FF); accent(0); accent(3); accent(5);
    auto* tenuto = Factory::createArticulation(notes[6]->chord());
    tenuto->setSymId(SymId::articTenutoAbove); notes[6]->chord()->add(tenuto);
    Hairpin* h = ramp();
    h->setProperty(Pid::DYNAMICS_END_ROLE, DynamicsPlayback::Accent);
    h->setProperty(Pid::DYNAMICS_SMOOTH_ARTICULATIONS, 1);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[3], 80), 85); // Actual ramp arrival.
    EXPECT_EQ(DynamicsPlayback::velocity(notes[4], 80), 30); // Following tap.
    EXPECT_EQ(DynamicsPlayback::velocity(notes[5], 80), 85);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[6], 80), 60);
    h->setProperty(Pid::DYNAMICS_END_ROLE, DynamicsPlayback::Tap);
    h->setProperty(Pid::DYNAMICS_END_VELOCITY, 25);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[3], 80), 25);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[4], 80), 25);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[5], 80), 85);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[6], 80), 60);
}

TEST_F(Engraving_DynamicsPlaybackTests, ExplicitHairpinMarkingDoesNotReusePreviousMarkingOverride) {
    mapping(Sid::evanDynamicsAccent, DynamicType::FF, 110);
    mapping(Sid::evanDynamicsAccent, DynamicType::MP, 75);
    Dynamic* d = mark(0, DynamicType::FF); accent(0); accent(3);
    d->setProperty(Pid::DYNAMICS_MARK_VELOCITY, 120);
    Hairpin* h = ramp(); h->setProperty(Pid::DYNAMICS_END_ROLE, DynamicsPlayback::Accent);
    h->setProperty(Pid::DYNAMICS_END_DYNAMIC, -1); h->setProperty(Pid::END_TEXT, String(u"<sym>dynamicMezzo</sym><sym>dynamicPiano</sym>"));
    EXPECT_EQ(DynamicsPlayback::endpoint(h, true), 75);
}

TEST_F(Engraving_DynamicsPlaybackTests, FlamGraceIsAlwaysATapIncludingInsideSmoothedHairpins) {
    mapping(Sid::evanDynamicsAccent, DynamicType::FF, 110);
    mapping(Sid::evanDynamicsTap, DynamicType::FF, 30);
    mapping(Sid::evanDynamicsTap, DynamicType::MP, 30);
    Dynamic* d = mark(0, DynamicType::FF); accent(0);
    auto* grace = Factory::createChord(score->dummy());
    grace->setTrack(0); grace->setNoteType(NoteType::ACCIACCATURA);
    grace->setDurationType(DurationType::V_EIGHTH); grace->setTicks(Fraction(1, 8));
    auto* n = Factory::createNote(grace); n->setTrack(0); n->setPitch(60); grace->add(n);
    notes[0]->chord()->add(grace);
    EXPECT_EQ(DynamicsPlayback::role(n), DynamicsPlayback::Tap);
    EXPECT_EQ(DynamicsPlayback::velocity(n, 80), 30);
    d->setProperty(Pid::DYNAMICS_MARK_VELOCITY, 125);
    EXPECT_EQ(DynamicsPlayback::velocity(n, 80), 30);
    auto* h = ramp(); h->setProperty(Pid::DYNAMICS_SMOOTH_ARTICULATIONS, 1);
    h->setProperty(Pid::DYNAMICS_START_ROLE, DynamicsPlayback::Accent);
    h->setProperty(Pid::DYNAMICS_START_VELOCITY, 120);
    EXPECT_EQ(DynamicsPlayback::velocity(n, 80), 30);
    EXPECT_EQ(DynamicsPlayback::velocityAt(n, Fraction(1, 8), 80), 30);
    n->setProperty(Pid::VELO_TYPE, VeloType::USER_VAL); n->setProperty(Pid::USER_VELOCITY, 42);
    EXPECT_EQ(n->customizeVelocity(DynamicsPlayback::velocity(n, 80)), 42);
}

TEST_F(Engraving_DynamicsPlaybackTests, LoadingDefaultsDoesNotSilentlyDisableBatterySeparation) {
    mapping(Sid::evanDynamicsTap, DynamicType::FF, 30);
    mapping(Sid::evanDynamicsAccent, DynamicType::FF, 110);
    const auto preset = muse::JsonDocument::fromJson(DynamicsPlayback::preset(score.get())).rootObject();
    auto saved = preset; saved["battery"] = false; saved["enabled"] = false;
    MStyle target = score->style();
    ASSERT_TRUE(DynamicsPlayback::readPreset(muse::JsonDocument(saved).toJson(), target, true));
    score->setStyle(target);
    EXPECT_TRUE(DynamicsPlayback::enabled(score.get()));
    EXPECT_TRUE(target.styleB(Sid::evanDynamicsBattery));
    mark(0, DynamicType::FF); accent(0);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[0], 80), 110);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[1], 80), 30);
    // The parser still supports faithfully importing the full data when asked.
    ASSERT_TRUE(DynamicsPlayback::readPreset(muse::JsonDocument(saved).toJson(), target));
    EXPECT_FALSE(target.styleB(Sid::evanDynamicsBattery));
    EXPECT_FALSE(target.styleB(Sid::evanDynamicsEnabled));
}

TEST_F(Engraving_DynamicsPlaybackTests, RepeatedMixedEditsPresetsUndoMidiAndAutomationStayConsistent) {
    score->lockUpdates(true);
    const std::array<DynamicType, 7> types {DynamicType::PP, DynamicType::P, DynamicType::MP, DynamicType::MF,
                                         DynamicType::F, DynamicType::FF, DynamicType::FFF};
    const std::array<int, 8> roles {DynamicsPlayback::Accent, DynamicsPlayback::Tap, DynamicsPlayback::Tenuto,
        DynamicsPlayback::Marcato, DynamicsPlayback::Ghost, DynamicsPlayback::SoftAccent, DynamicsPlayback::Stress, DynamicsPlayback::Unstress};
    const std::array<SymId, 8> symbols {SymId::articAccentAbove, SymId::noSym, SymId::articTenutoAbove,
        SymId::articMarcatoAbove, SymId::noSym, SymId::articSoftAccentAbove, SymId::articStressAbove, SymId::articUnstressAbove};
    for (int i = 0; i < 8; ++i) {
        if (symbols[i] == SymId::noSym) continue;
        auto* a = Factory::createArticulation(notes[i]->chord()); a->setSymId(symbols[i]); notes[i]->chord()->add(a);
    }
    notes[4]->setGhost(true);
    Dynamic* d = mark(0, DynamicType::FF);
    score->rebuildMidiMapping();
    for (int iteration = 0; iteration < 84; ++iteration) {
        SCOPED_TRACE(iteration);
        const auto type = types[iteration % types.size()];
        const std::array<int, 8> expected {65 + iteration % 40, 30, 55, 118, 0, 46, 101, 22};
        d->setProperty(Pid::DYNAMIC_TYPE, type);
        d->setProperty(Pid::VELOCITY, 120 - iteration % 11); // Imported value must never leak back in.
        for (size_t i = 0; i < roles.size(); ++i) mapping(DynamicsPlayback::mappingStyle(roles[i]), type, expected[i]);
        const auto data = DynamicsPlayback::preset(score.get());
        MStyle restored = score->style();
        ASSERT_TRUE(DynamicsPlayback::readPreset(data, restored, true));
        const String before = score->style().styleSt(Sid::evanDynamicsAccent);
        score->startCmd(muse::TranslatableString::untranslatable("Temporary accent edit"));
        score->undoChangeStyleVal(Sid::evanDynamicsAccent, String());
        score->endCmd(); score->undoRedo(true, nullptr);
        ASSERT_EQ(score->style().styleSt(Sid::evanDynamicsAccent), before);
        if (iteration % 3 == 0) { score->undoRedo(false, nullptr); score->undoRedo(true, nullptr); }
        for (Sid sid : DynamicsPlayback::profileStyles()) score->style().set(sid, restored.value(sid));
        score->updateRepeatList();
        PlaybackContext context(score.get());
        const auto layers = context.dynamicLevelLayers(0, 4);
        ASSERT_EQ(layers.size(), 1);
        EventsHolder events; CompatMidiRender::renderScore(score.get(), events, {}, true);
        std::map<int, int> midi;
        for (size_t ch = 0; ch < events.size(); ++ch) for (const auto& [tick, event] : events[ch])
            if (event.type() == ME_NOTEON && event.velo() > 0) midi[tick] = event.velo();
        for (size_t i = 0; i < roles.size(); ++i) {
            SCOPED_TRACE(i);
            EXPECT_EQ(DynamicsPlayback::role(notes[i]), roles[i]);
            EXPECT_EQ(DynamicsPlayback::velocity(notes[i], 80), expected[i]);
            const int tick = notes[i]->tick().ticks();
            EXPECT_EQ(midi.contains(tick) ? midi.at(tick) : 0, expected[i]);
            const auto time = score->tempoTimeline(true).utick2utime(tick) * 1000000;
            ASSERT_TRUE(layers.begin()->second.contains(time));
            EXPECT_NEAR(layers.begin()->second.at(time).outValue, expected[i] / 127.0, 1e-7);
        }
    }
}

TEST_F(Engraving_DynamicsPlaybackTests, MappingCachesDoNotLeakBetweenScoresOrAfterPresetLoading) {
    mapping(Sid::evanDynamicsTap, DynamicType::FF, 30);
    mapping(Sid::evanDynamicsAccent, DynamicType::FF, 110);
    auto second = std::unique_ptr<MasterScore>(compat::ScoreAccess::createMasterScore(nullptr));
    MStyle restored = second->style();
    ASSERT_TRUE(DynamicsPlayback::readPreset(DynamicsPlayback::preset(score.get()), restored)); second->setStyle(restored);
    mapping(Sid::evanDynamicsAccent, DynamicType::FF, 83);
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(DynamicsPlayback::level(score.get(), DynamicType::FF, DynamicsPlayback::Accent), 83);
        EXPECT_EQ(DynamicsPlayback::level(second.get(), DynamicType::FF, DynamicsPlayback::Accent), 110);
        EXPECT_EQ(DynamicsPlayback::level(score.get(), DynamicType::FF, DynamicsPlayback::Tap), 30);
    }
}

TEST_F(Engraving_DynamicsPlaybackTests, AllItemLocksAreDefaultOffAndSurviveBothReaders) {
    auto* d = mark(0, DynamicType::FP);
    auto* h = ramp();
    d->setProperty(Pid::DYNAMICS_MARK_VELOCITY, 0);
    h->setProperty(Pid::DYNAMICS_CURVE_BEND, .65);
    h->setProperty(Pid::DYNAMICS_SMOOTH_ARTICULATIONS, 1);
    for (EngravingItem* item : std::array<EngravingItem*, 3> {d, h, notes[1]}) {
        EXPECT_TRUE(item->getProperty(Pid::DYNAMICS_LOCK).value<String>().empty());
        const auto snapshot = DynamicsPlayback::lockSnapshot(item);
        ASSERT_TRUE(item->setProperty(Pid::DYNAMICS_LOCK, snapshot));
        for (bool current : {false, true}) {
            auto copy = roundTrip(item, current);
            ASSERT_EQ(copy->getProperty(Pid::DYNAMICS_LOCK).value<String>(), snapshot);
            if (item == d) EXPECT_EQ(copy->getProperty(Pid::DYNAMICS_MARK_VELOCITY).toInt(), 0);
            if (item == h) {
                EXPECT_DOUBLE_EQ(copy->getProperty(Pid::DYNAMICS_CURVE_BEND).toDouble(), .65);
                EXPECT_EQ(copy->getProperty(Pid::DYNAMICS_SMOOTH_ARTICULATIONS).toInt(), 1);
            }
        }
        EXPECT_TRUE(item->setProperty(Pid::DYNAMICS_LOCK, String()));
    }
}

TEST_F(Engraving_DynamicsPlaybackTests, LockedMarkingFreezesEveryCategoryAndCompoundSettledValues) {
    score->setStyle(DynamicsPlayback::marchingSnareDefaults(score->style()));
    auto* d = mark(0, DynamicType::FP);
    d->setProperty(Pid::DYNAMICS_MARK_VELOCITY, 121);
    const auto snapshot = DynamicsPlayback::lockSnapshot(d);
    ASSERT_TRUE(d->setProperty(Pid::DYNAMICS_LOCK, snapshot));
    std::array<int, 9> attack, settled;
    for (int role = 1; role <= 9; ++role) {
        attack[role - 1] = DynamicsPlayback::markingVelocity(d, role);
        settled[role - 1] = DynamicsPlayback::markingVelocity(d, role, DynamicType::P);
        mapping(DynamicsPlayback::mappingStyle(role), DynamicType::FP, 17);
        mapping(DynamicsPlayback::mappingStyle(role), DynamicType::P, 23);
    }
    for (int role = 1; role <= 9; ++role) {
        EXPECT_EQ(DynamicsPlayback::markingVelocity(d, role), attack[role - 1]);
        EXPECT_EQ(DynamicsPlayback::markingVelocity(d, role, DynamicType::P), settled[role - 1]);
    }
}

TEST_F(Engraving_DynamicsPlaybackTests, ApplyDefaultsResetsEveryUnlockedOverrideAndOneUndoRestoresThem) {
    score->lockUpdates(true);
    mapping(Sid::evanDynamicsTap, DynamicType::FF, 30);
    auto* d = mark(0, DynamicType::FF); d->setProperty(Pid::DYNAMICS_MARK_VELOCITY, 120);
    auto* lockedMark = mark(5, DynamicType::MP); lockedMark->setProperty(Pid::DYNAMICS_MARK_VELOCITY, 98);
    lockedMark->setProperty(Pid::DYNAMICS_LOCK, DynamicsPlayback::lockSnapshot(lockedMark));
    notes[1]->setProperty(Pid::USER_VELOCITY, 81); notes[1]->setProperty(Pid::VELO_TYPE, VeloType::USER_VAL);
    notes[2]->setProperty(Pid::USER_VELOCITY, 73); notes[2]->setProperty(Pid::VELO_TYPE, VeloType::USER_VAL);
    notes[2]->setProperty(Pid::DYNAMICS_LOCK, DynamicsPlayback::lockSnapshot(notes[2]));
    auto* h = ramp(); h->setProperty(Pid::DYNAMICS_CURVE_BEND, .7);
    const auto oldCurve = h->getProperty(Pid::DYNAMICS_CURVE_BEND);
    const auto newStyle = DynamicsPlayback::marchingSnareDefaults(score->style());
    score->startCmd(muse::TranslatableString::untranslatable("Apply default dynamics"));
    for (Sid sid : DynamicsPlayback::profileStyles()) score->undoChangeStyleVal(sid, newStyle.value(sid));
    DynamicsPlayback::resetUnlockedOverrides(score.get());
    score->endCmd();
    EXPECT_EQ(d->preciseVelocity(), -1);
    EXPECT_EQ(lockedMark->preciseVelocity(), 98);
    EXPECT_EQ(notes[1]->userVelocity(), 0);
    EXPECT_EQ(notes[2]->userVelocity(), 73);
    EXPECT_EQ(h->getProperty(Pid::DYNAMICS_CURVE_BEND), h->propertyDefault(Pid::DYNAMICS_CURVE_BEND));
    score->undoRedo(true, nullptr);
    EXPECT_EQ(d->preciseVelocity(), 120);
    EXPECT_EQ(notes[1]->userVelocity(), 81);
    EXPECT_EQ(notes[2]->userVelocity(), 73);
    EXPECT_EQ(h->getProperty(Pid::DYNAMICS_CURVE_BEND), oldCurve);
    score->undoRedo(false, nullptr);
    EXPECT_EQ(d->preciseVelocity(), -1);
    EXPECT_EQ(notes[1]->userVelocity(), 0);
}

TEST_F(Engraving_DynamicsPlaybackTests, LockedHairpinAndNoteStayFixedWhenTheProfileChanges) {
    score->lockUpdates(true);
    mark(0, DynamicType::FF); accent(0);
    auto* h = ramp(); h->setProperty(Pid::DYNAMICS_CURVE_BEND, .67);
    std::array<int, 8> before;
    for (size_t i = 0; i < before.size(); ++i) before[i] = DynamicsPlayback::velocity(notes[i], 80);
    const auto locked = DynamicsPlayback::lockSnapshot(h);
    ASSERT_TRUE(h->setProperty(Pid::DYNAMICS_LOCK, locked));
    ASSERT_TRUE(notes[6]->setProperty(Pid::DYNAMICS_LOCK, DynamicsPlayback::lockSnapshot(notes[6])));
    const int lockedNote = DynamicsPlayback::velocity(notes[6], 80);
    score->setStyle(DynamicsPlayback::marchingSnareDefaults(score->style()));
    score->startCmd(muse::TranslatableString::untranslatable("Default"));
    DynamicsPlayback::resetUnlockedOverrides(score.get()); score->endCmd();
    for (size_t i = 0; i < before.size(); ++i) EXPECT_EQ(DynamicsPlayback::velocity(notes[i], 80), before[i]);
    EXPECT_EQ(DynamicsPlayback::velocityAt(notes[6], Fraction(1, 8), 80), lockedNote);
    EXPECT_EQ(h->getProperty(Pid::DYNAMICS_LOCK).value<String>(), locked);
}

TEST_F(Engraving_DynamicsPlaybackTests, MalformedLocksAreRejectedWithoutChangingExistingSnapshot) {
    auto* d = mark(0, DynamicType::FF); auto* h = ramp();
    for (EngravingItem* item : std::array<EngravingItem*, 3> {d, h, notes[0]}) {
        const auto valid = DynamicsPlayback::lockSnapshot(item);
        ASSERT_TRUE(item->setProperty(Pid::DYNAMICS_LOCK, valid));
        for (const auto& bad : {String(u"not a lock"), String(u"9999"), String(u"nan"), String(u"1 2")}) {
            EXPECT_FALSE(item->setProperty(Pid::DYNAMICS_LOCK, bad));
            EXPECT_EQ(item->getProperty(Pid::DYNAMICS_LOCK).value<String>(), valid);
        }
    }
}

TEST_F(Engraving_DynamicsPlaybackTests, VoiceScopedMarkingsDoNotChangeOtherVoices) {
    mapping(Sid::evanDynamicsTap, DynamicType::MF, 22);
    mapping(Sid::evanDynamicsTap, DynamicType::FF, 30);
    auto* segment = notes[0]->chord()->segment();
    auto* chord = Factory::createChord(segment); chord->setTrack(1); chord->setTicks(Fraction(1,4));
    chord->setDurationType(DurationType::V_QUARTER); segment->add(chord);
    auto* note = Factory::createNote(chord); note->setTrack(1); note->setPitch(60); chord->add(note);
    auto* d = mark(0, DynamicType::FF);
    d->setProperty(Pid::VOICE_ASSIGNMENT, VoiceAssignment::CURRENT_VOICE_ONLY);
    EXPECT_EQ(DynamicsPlayback::velocity(notes[0], 80), 30);
    EXPECT_EQ(DynamicsPlayback::velocity(note, 80), 22);
    d->setProperty(Pid::DYNAMICS_MARK_VELOCITY, 120);
    EXPECT_EQ(DynamicsPlayback::velocity(note, 80), 22);
    d->setProperty(Pid::VOICE_ASSIGNMENT, VoiceAssignment::ALL_VOICE_IN_STAFF);
    EXPECT_EQ(DynamicsPlayback::velocity(note, 80), 30);
}

TEST_F(Engraving_DynamicsPlaybackTests, LockUndoRedoNotifiesPlaybackWithoutPrintedLayout) {
    score->lockUpdates(true);
    muse::async::Asyncable receiver;
    int notifications = 0;
    score->changesChannel().onReceive(&receiver, [&](const ScoreChanges& changes) {
        if (changes.changedPropertyIdSet.count(Pid::DYNAMICS_LOCK)) ++notifications;
    });
    score->cmdState().reset();
    score->startCmd(muse::TranslatableString::untranslatable("Lock note dynamics"));
    notes[0]->undoChangeProperty(Pid::DYNAMICS_LOCK, DynamicsPlayback::lockSnapshot(notes[0]));
    EXPECT_FALSE(score->cmdState().layoutRange());
    score->endCmd();
    EXPECT_TRUE(notes[0]->dynamicsLocked());
    EXPECT_EQ(notifications, 1);
    score->undoRedo(true, nullptr);
    EXPECT_FALSE(notes[0]->dynamicsLocked()); EXPECT_EQ(notifications, 2);
    score->undoRedo(false, nullptr);
    EXPECT_TRUE(notes[0]->dynamicsLocked()); EXPECT_EQ(notifications, 3);
}
