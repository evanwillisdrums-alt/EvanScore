/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include <gtest/gtest.h>
#include "musesampler/internal/musesamplersequencer.h"
#include "audio/common/rpc/rpcpacker.h"

using namespace muse;
using namespace muse::musesampler;
namespace {
struct Capture { std::map<long long, double> dynamics; };
struct Tracks : IMuseSamplerTracks {
    int token = 0;
    TrackList tracks { &token };
    const TrackList& allTracks() const override { return tracks; }
    ms_Track addTrack() override { return nullptr; }
};
mpe::NoteEvent note(long long time, int velocity, bool custom)
{
    mpe::ArrangementContext arrangement;
    arrangement.nominalTimestamp = arrangement.actualTimestamp = time;
    arrangement.nominalDuration = arrangement.actualDuration = 100000;
    arrangement.bps = 2;
    mpe::ExpressionContext expression;
    expression.nominalDynamicLevel = mpe::dynamicLevelFromType(mpe::DynamicType::mf);
    expression.velocityOverride = velocity / 127.f;
    expression.customDynamics = custom;
    return mpe::NoteEvent(std::move(arrangement), mpe::PitchContext {}, std::move(expression));
}
struct Fixture {
    Capture captured;
    Tracks tracks;
    MuseSamplerSequencer sequencer;
    Fixture() {
        auto lib = std::make_shared<MuseSamplerLibHandler>();
        lib->clearTrack = [](ms_MuseSampler sampler, ms_Track) { static_cast<Capture*>(sampler)->dynamics.clear(); return ms_Result_OK; };
        lib->finalizeTrack = [](ms_MuseSampler, ms_Track) { return ms_Result_OK; };
        lib->addNoteEvent = [](ms_MuseSampler, ms_Track, const NoteEvent&, long long& id) { id = 1; return true; };
        lib->addDynamicsEvent = [](ms_MuseSampler sampler, ms_Track, DynamicEvent event) {
            static_cast<Capture*>(sampler)->dynamics[event._location_us] = event._value; return ms_Result_OK;
        };
        sequencer.init(lib, &captured, &tracks, {});
    }
};
}
TEST(MuseSamplerDynamics, CustomPlaybackMatchesAuditionAndIncludesGraceAttack)
{
    Fixture f;
    mpe::PlaybackData data;
    data.originEvents[0].emplace_back(note(0, 110, true));
    data.originEvents[450000].emplace_back(note(450000, 30, true)); // Flam grace, absent from main-chord automation.
    data.originEvents[500000].emplace_back(note(500000, 110, true));
    for (long long t : {0LL, 500000LL}) {
        mpe::AutomationPoint point; point.outValue = 110.0 / 127;
        data.dynamics[0][t] = point;
    }
    f.sequencer.load(data);
    ASSERT_TRUE(f.captured.dynamics.contains(450000));
    EXPECT_NEAR(f.captured.dynamics.at(0), 110.0 / 127, 1e-7);
    EXPECT_NEAR(f.captured.dynamics.at(450000), 30.0 / 127, 1e-7);
    EXPECT_NEAR(f.captured.dynamics.at(500000), 110.0 / 127, 1e-7);
}
TEST(MuseSamplerDynamics, SwitchingCustomDynamicsOffRestoresStandardConversion)
{
    Fixture f;
    mpe::PlaybackData data;
    data.originEvents[0].emplace_back(note(0, 20, true));
    mpe::AutomationPoint point; point.outValue = 0.5; data.dynamics[0][0] = point;
    f.sequencer.load(data);
    EXPECT_NEAR(f.captured.dynamics.at(0), 20.0 / 127, 1e-7);
    data = mpe::PlaybackData {}; data.dynamics[0][0] = point;
    data.originEvents[0].emplace_back(note(0, 20, false));
    f.sequencer.load(data);
    Fixture standard; standard.sequencer.load(data);
    EXPECT_DOUBLE_EQ(f.captured.dynamics.at(0), standard.captured.dynamics.at(0));
    EXPECT_GT(std::abs(f.captured.dynamics.at(0) - 20.0 / 127), .01);
}
TEST(MuseSamplerDynamics, VelocityOnlyChangesAreNotEqualAndCustomFlagParticipatesInEquality)
{
    auto first = note(0, 30, true), second = note(0, 100, true);
    EXPECT_FALSE(first == second);
    EXPECT_FALSE(first == note(0, 30, false));
}

TEST(MuseSamplerDynamics, ExactDynamicsFlagAndVelocitySurviveAudioRpcRoundTrip)
{
    const auto original = note(450000, 30, true);
    const auto bytes = audio::rpc::RpcPacker::pack(original);
    mpe::NoteEvent restored;
    ASSERT_TRUE(audio::rpc::RpcPacker::unpack(bytes, restored));
    EXPECT_TRUE(restored.expressionCtx().customDynamics);
    ASSERT_TRUE(restored.expressionCtx().velocityOverride.has_value());
    EXPECT_NEAR(*restored.expressionCtx().velocityOverride, 30.0 / 127, 1e-7);
    EXPECT_TRUE(original == restored);
}
