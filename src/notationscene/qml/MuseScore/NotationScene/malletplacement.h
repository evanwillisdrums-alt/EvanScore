/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#pragma once
#include <array>
#include <string>
#include <vector>

namespace mu::notation::mallet {
struct Point { double x = 0; double y = 0; };
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
    int grip = 0; // Stevens, Burton, traditional cross grip; no universal limits implied.
    double opening = 24;
    double reach = 65;
    double shaft = 40;
    double head = 3;
    double bodyOffset = 0;
    bool respectSticking = true;
    std::array<double, 4> strikeFractions { .5, .5, .5, .5 };
};
struct Issue { std::string key, message; int severity = 0; };
struct Pose {
    std::vector<int> pitches;
    std::vector<int> mallets; // physical identities: L outer=1, L inner=2, R inner=3, R outer=4
    std::array<Point, 4> targets {};
    std::array<bool, 4> active {};
    std::array<Point, 2> wrists {};
    std::array<double, 2> openings {};
    std::array<double, 2> rotations {};
    Point body;
    double cost = 0;
    int severity = 0;
    bool valid = true;
    std::vector<Issue> issues;
};
Keyboard keyboard(int low, int high, bool metal = false);
std::string pitchName(int pitch);
// Required physical IDs: 0 is free, 1..4 fixes a mallet, -1/-2 fixes left/right hand.
std::vector<Pose> solve(const Keyboard&, const std::vector<int>& pitches, const Player&,
                        const std::vector<int>& required = {}, const Pose* previous = nullptr);
std::vector<Pose> alternatives(const Keyboard&, const std::vector<int>& pitches, const Player&, const Pose* previous = nullptr);
}
