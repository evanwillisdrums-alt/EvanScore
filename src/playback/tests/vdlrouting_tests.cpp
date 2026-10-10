/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include <gtest/gtest.h>
#include "mpe/percussionsource.h"
using namespace muse::mpe;
static PercussionSource hit(int hand = 1) {
    PercussionSource source; source.present = true; source.instrument = "marching-snare";
    source.sound = "Battery Snare"; source.hand = hand; return source;
}
TEST(VdlRouting, GuidePage29HandHitsAreNumericMidiAndPreserveRepeatedHands) {
    const int hands[] = {1, 2, 1, 1}; const int expected[] = {80, 78, 80, 80};
    for(int i=0;i<4;++i) EXPECT_EQ(snareLineManualRoute(hit(hands[i])).pitch, expected[i]);
}
TEST(VdlRouting, RimShotsRimsAndBacksticksHaveDistinctSamples) {
    auto source=hit();
    source.sound="Rim Shot";EXPECT_EQ(snareLineManualRoute(source).pitch,79);
    source.hand=2;EXPECT_EQ(snareLineManualRoute(source).pitch,77);
    source.sound="Rim Click";EXPECT_EQ(snareLineManualRoute(source).pitch,73);
    source.hand=1;EXPECT_EQ(snareLineManualRoute(source).pitch,75);
    source.sound="Backstick";EXPECT_EQ(snareLineManualRoute(source).pitch,83);
    source.hand=2;EXPECT_EQ(snareLineManualRoute(source).pitch,82);
}
TEST(VdlRouting, CenterHalfwayAndEdgeControllersResetExplicitlyOnEachAttack) {
    auto source=hit();
    for (const auto [zone,cc] : {std::pair{0,0}, {1,65}, {2,110}, {0,0}}) {
        source.zone=zone;const auto route=snareLineManualRoute(source);
        EXPECT_EQ(route.modulation,cc);EXPECT_EQ(route.pitch,80);
    }
}
TEST(VdlRouting, SnaresOffAndSoloUseDocumentedSeparateRegisters) {
    auto source=hit(2);source.snaresOff=true;EXPECT_EQ(snareLineManualRoute(source).pitch,102);
    source.hand=1;EXPECT_EQ(snareLineManualRoute(source).pitch,104);
    source.snaresOff=false;source.solo=true;EXPECT_EQ(snareLineManualRoute(source).pitch,63);
    source.hand=2;EXPECT_EQ(snareLineManualRoute(source).pitch,61);
    source.solo=false;EXPECT_EQ(snareLineManualRoute(source).pitch,78);
}
TEST(VdlRouting, UnreviewedTechniquesAndConflictingStickingDoNotPlayAnUnrelatedSample) {
    auto source=hit();source.sound="Unknown future technique";EXPECT_FALSE(snareLineManualRoute(source).supported());
    source=hit(-1);EXPECT_FALSE(snareLineManualRoute(source).supported());
    source=hit();source.instrument="marching-tenors";EXPECT_FALSE(snareLineManualRoute(source).supported());
    source=hit();source.present=false;EXPECT_FALSE(snareLineManualRoute(source).supported());
}
TEST(VdlRouting, MissingStickingIsExplicitRightHandFallbackWithoutAutoAlternation) {
    const auto route=snareLineManualRoute(hit(0));
    EXPECT_EQ(route.pitch,80);EXPECT_NE(route.diagnostic.find("without invented alternation"),std::string::npos);
}
