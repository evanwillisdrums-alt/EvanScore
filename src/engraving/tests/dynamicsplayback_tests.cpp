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
#include "engraving/editing/transaction/undostack.h"
#include "engraving/rw/read400/tread.h"
#include "engraving/rw/write/twrite.h"
#include "engraving/rw/write/writecontext.h"
#include "engraving/rw/xmlwriter.h"
#include "engraving/playback/playbackcontext.h"
#include "io/buffer.h"
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
    std::unique_ptr<EngravingItem> roundTrip(EngravingItem* item) {
        auto buffer = muse::io::Buffer::opened(muse::io::IODevice::WriteOnly);
        XmlWriter xml(&buffer); xml.startDocument();
        write::WriteContext wc(score.get()); write::TWrite::writeItem(item, xml, wc); xml.flush();
        buffer.close();
        XmlReader reader(buffer.data()); reader.readNextStartElement();
        std::unique_ptr<EngravingItem> result(Factory::createItemByName(reader.name(), score->dummy()));
        read400::ReadContext rc(score.get()); read400::TRead::readItem(result.get(), reader, rc);
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
    h->setProperty(Pid::DYNAMICS_START_VELOCITY, 127); h->setProperty(Pid::DYNAMICS_END_VELOCITY, 0);
    auto copy = roundTrip(h);
    for (Pid id : {Pid::DYNAMICS_CURVE_SHAPE, Pid::DYNAMICS_CURVE_BEND, Pid::DYNAMICS_START_DYNAMIC, Pid::DYNAMICS_END_DYNAMIC,
                   Pid::DYNAMICS_START_ROLE, Pid::DYNAMICS_END_ROLE, Pid::DYNAMICS_START_VELOCITY, Pid::DYNAMICS_END_VELOCITY}) {
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
    const auto data = DynamicsPlayback::preset(score.get());
    MStyle restored;
    restored.set(Sid::musicalSymbolFont, String(u"Bravura"));
    const auto musicalFont = restored.value(Sid::musicalSymbolFont);
    ASSERT_TRUE(DynamicsPlayback::readPreset(data, restored));
    EXPECT_EQ(restored.value(Sid::musicalSymbolFont), musicalFont);
    for (Sid sid : { Sid::evanDynamicsEnabled, Sid::evanDynamicsBattery, Sid::evanDynamicsCurveShape, Sid::evanDynamicsCurveBend }) {
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
