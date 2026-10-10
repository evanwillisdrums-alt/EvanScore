/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <set>
#include "../malletplacement.h"
using namespace mu::notation::mallet;

TEST(MalletPlacement, FiveOctaveKeyboardHasCorrectBarsAndOverhang) {
    const auto k = keyboard(36, 96);
    ASSERT_EQ(k.bars.size(), 61);
    EXPECT_EQ(std::count_if(k.bars.begin(), k.bars.end(), [](const Bar& b) { return b.accidental; }), 25);
    EXPECT_EQ(k.bars.front().pitch, 36); EXPECT_EQ(k.bars.back().pitch, 96);
    EXPECT_GT(k.bar(36)->length, k.bar(96)->length);
    for (const auto& b : k.bars) {
        EXPECT_GE(b.x, 0); EXPECT_LE(b.x + b.width, k.width + 1e-8);
        EXPECT_TRUE(std::isfinite(b.length)); EXPECT_GT(b.length, 0);
        if (b.accidental) {
            EXPECT_LT(b.y, 62);
            EXPECT_GT(b.y + b.length, 62);
            EXPECT_NEAR((b.y + b.length - 62) / b.length, .12, 1e-9);
        }
    }
    // The two/three pattern follows pitch classes rather than evenly spaced black piano keys.
    EXPECT_FALSE(k.bar(40)->accidental); EXPECT_FALSE(k.bar(41)->accidental);
    EXPECT_FALSE(k.bar(47)->accidental); EXPECT_FALSE(k.bar(48)->accidental);
}
TEST(MalletPlacement, RangeBeginningAndEndingOnAccidentalsIsComplete) {
    for (int low = 0; low < 127; ++low) {
        const int high = std::min(127, low + 13); const auto k = keyboard(low, high);
        ASSERT_EQ(k.bars.size(), size_t(high-low+1)) << low;
        EXPECT_EQ(k.bars.front().pitch, low); EXPECT_EQ(k.bars.back().pitch, high);
    }
}
TEST(MalletPlacement, WrittenNumbersKeepPhysicalIdentityAcrossCrossovers) {
    const auto k = keyboard(36, 96); Player player;
    const auto result = solve(k, {60, 64, 67, 71}, player, {3, 4, 1, 2});
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result.front().mallets, (std::vector<int>{3,4,1,2}));
    EXPECT_GT(result.front().wrists[0].x, result.front().wrists[1].x);
    EXPECT_TRUE(result.front().valid); // Crossovers warn; they are not inherently impossible.
    EXPECT_EQ(result.front().severity, 1);
}
TEST(MalletPlacement, HandAssignmentsNeverGiveOneHandThreeSimultaneousTargets) {
    const auto k = keyboard(36, 96); Player player;
    auto result = solve(k, {60,64,67}, player, {-1,-1,-1});
    ASSERT_EQ(result.size(), 1); EXPECT_FALSE(result.front().valid);
    result = solve(k, {60,64,67,71}, player, {-1,-1,-2,-2});
    ASSERT_FALSE(result.empty()); EXPECT_TRUE(result.front().valid);
    EXPECT_LE(result.front().mallets[0], 2); EXPECT_LE(result.front().mallets[1], 2);
    EXPECT_GE(result.front().mallets[2], 3); EXPECT_GE(result.front().mallets[3], 3);
}
TEST(MalletPlacement, ExcessAttacksUnsupportedNumbersAndRangeConflictsAreExplicit) {
    const auto k = keyboard(36, 96); Player player;
    EXPECT_FALSE(solve(k, {48,52,55,59,62}, player).front().valid);
    EXPECT_FALSE(solve(k, {60}, player, {5}).front().valid);
    EXPECT_FALSE(solve(k, {35}, player).front().valid);
    EXPECT_TRUE(solve(k, {}, player).empty());
}
TEST(MalletPlacement, TwoMalletModeUsesOnlyOuterLeftAndRight) {
    Player player; player.malletCount = 2;
    for (const auto& pose : solve(keyboard(36,96), {60,67}, player)) {
        EXPECT_TRUE(pose.valid); EXPECT_EQ(std::set<int>(pose.mallets.begin(), pose.mallets.end()), (std::set<int>{1,4}));
    }
    EXPECT_FALSE(solve(keyboard(36,96), {60,64,67}, player).front().valid);
}
TEST(MalletPlacement, AlternativesPreserveAllVoicesAndChangeOnlyOctaves) {
    const std::vector<int> original {48,60,64,67};
    const auto candidates = alternatives(keyboard(36,96), original, Player{});
    ASSERT_FALSE(candidates.empty());
    for (const auto& pose : candidates) {
        ASSERT_EQ(pose.pitches.size(), original.size()); ASSERT_EQ(pose.mallets.size(), original.size());
        EXPECT_EQ(std::set<int>(pose.mallets.begin(), pose.mallets.end()).size(), original.size());
        for (size_t i = 0; i < original.size(); ++i) EXPECT_EQ((pose.pitches[i]-original[i]) % 12, 0);
        EXPECT_TRUE(pose.valid);
    }
}
TEST(MalletPlacement, MovingStrikePointChangesPoseWithoutChangingPitchesOrIdentity) {
    Player player; const auto k = keyboard(36,96); const std::vector<int> pitches {60,61};
    const auto before = solve(k, pitches, player, {1,2}).front(); player.strikeFractions[1] = .88;
    const auto after = solve(k, pitches, player, {1,2}).front();
    EXPECT_EQ(before.pitches, after.pitches); EXPECT_EQ(before.mallets, after.mallets);
    EXPECT_GT(after.targets[1].y, before.targets[1].y);
    EXPECT_LT(after.openings[0], before.openings[0]);
}
TEST(MalletPlacement, RepeatedAnalysisHasBoundedCandidatesAndDeterministicResults) {
    const auto k = keyboard(36,96); Player player;
    const auto expected = solve(k, {60,64,67,71}, player).front().mallets;
    for (int i = 0; i < 1000; ++i) {
        const auto poses = solve(k, {60,64,67,71}, player);
        EXPECT_LE(poses.size(), 24); EXPECT_EQ(poses.front().mallets, expected);
        EXPECT_LE(alternatives(k, {60,64,67,71}, player).size(), 8);
    }
}
