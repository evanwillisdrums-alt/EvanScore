/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#pragma once
#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace mu::notation::mallet {
struct Point { double x = 0, y = 0, z = 0; };
struct Bar {
    int pitch = 0;
    bool accidental = false;
    double x = 0, y = 0, width = 0, length = 0;
    Point strike() const { return { x + width / 2, y + length / 2 }; }
};
struct Keyboard {
    int low = 36, high = 96;
    double width = 0, front = 0;
    std::vector<Bar> bars;
    const Bar* bar(int pitch) const;
};
struct Player {
    int malletCount = 4;
    int grip = 0; // Stevens, Burton, traditional cross grip.
    double opening = 24, reach = 65, shaft = 40, head = 3;
    double rotation = 40, handWidth = 7, bodyDistance = 28;
    double accidentalHeight = 4; // Estimated cm above naturals; performer/instrument editable.
    double bodyOffset = 0, travelSpeed = 200; // cm/s: editable heuristic, not a human limit.
    bool respectSticking = true, optimizeStrikes = true, calibrated = false;
    bool reverseNumbering = false;
    std::array<double, 4> strikeFractions { .5, .5, .5, .5 };
    std::array<bool, 4> manualStrikes {};
};
struct Issue { std::string key, message; int severity = 0; };
struct Pose {
    // These vectors remain in SOURCE VOICE order after revoicing. Never sort
    // pitches without carrying their voice identity and assigned mallet.
    std::vector<int> pitches, mallets;
    std::array<Point, 4> targets {}, anchors {};
    std::array<bool, 4> active {};
    std::array<double, 4> fractions { .5, .5, .5, .5 };
    std::array<Point, 2> wrists {}, shoulders {};
    std::array<double, 2> openings {}, rotations {}, reaches {};
    Point body;
    double cost = 0, outerSpread = 0, handClearance = 0, shaftClearance = 0;
    double movement = 0, preparation = 0;
    int severity = 0;
    bool valid = true, uncertain = false;
    std::vector<Issue> issues;
};
struct SearchOptions {
    bool allowOctaves = true, keepBass = true, keepMelody = true, allowInversion = false;
};
struct WrittenSticking { std::vector<int> required; bool unknown = false; std::string reason; };
WrittenSticking parseWrittenSticking(const std::string& text, std::size_t notes, bool reverseNumbering = false);
Keyboard keyboard(int low, int high, bool metal = false);
std::string pitchName(int pitch);
std::string number(double value, int decimals = 1);
// 0 is unknown; 1..4 fixes a PHYSICAL mallet; -1/-2 fixes left/right hand.
std::vector<Pose> solve(const Keyboard&, const std::vector<int>& pitches, const Player&,
                        const std::vector<int>& required = {}, const Pose* previous = nullptr,
                        double previousSeconds = 0, const Pose* next = nullptr, double nextSeconds = 0);
std::vector<Pose> alternatives(const Keyboard&, const std::vector<int>& pitches, const Player&,
                              const Pose* previous = nullptr, const std::vector<int>& required = {},
                              const SearchOptions& options = {}, double previousSeconds = 0,
                              const Pose* next = nullptr, double nextSeconds = 0);
}
