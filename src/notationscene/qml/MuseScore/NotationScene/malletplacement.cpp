/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include "malletplacement.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <set>

namespace mu::notation::mallet {
static bool sharp(int pitch) { const int pc = pitch % 12; return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; }
static double distance(Point a, Point b) { return std::hypot(a.x - b.x, a.y - b.y); }
const Bar* Keyboard::bar(int pitch) const {
    for (const auto& b : bars) if (b.pitch == pitch) return &b;
    return nullptr;
}
std::string pitchName(int pitch) {
    static const char* names[] { "C", "C♯", "D", "E♭", "E", "F", "F♯", "G", "A♭", "A", "B♭", "B" };
    return names[std::clamp(pitch, 0, 127) % 12] + std::to_string(pitch / 12 - 1);
}
Keyboard keyboard(int low, int high, bool metal) {
    Keyboard k;
    k.low = std::clamp(low, 0, 127); k.high = std::clamp(high, k.low, 127);
    // Estimated graduated geometry. Shared upper-row front edge overlaps the
    // natural row by 12%, with the rest of the accidental extending behind it.
    // Generate neighbours even for a range beginning/ending on an accidental.
    std::vector<Bar> naturals;
    double x = 0;
    for (int p = k.low - 2; p <= std::min(129, k.high + 2); ++p) {
        if (p < 0 || sharp(p)) continue;
        const double t = std::clamp((p - k.low) / double(std::max(1, k.high - k.low)), 0.0, 1.0);
        const double width = metal ? 4.0 - t * 1.0 : 7.0 - t * 3.0;
        const double length = metal ? 38.0 - t * 18.0 : 62.0 * std::pow(0.38, t);
        naturals.push_back({ p, false, x, 62, width, length });
        x += width + 0.4;
    }
    for (int p = k.low; p <= k.high; ++p) {
        if (!sharp(p)) {
            for (const auto& b : naturals) if (b.pitch == p) k.bars.push_back(b);
        } else {
            const Bar* left = nullptr; const Bar* right = nullptr;
            for (const auto& b : naturals) { if (b.pitch == p - 1) left = &b; if (b.pitch == p + 1) right = &b; }
            if (!left || !right) continue;
            const double width = (left->width + right->width) * 0.46;
            const double length = (left->length + right->length) / 2;
            const double center = (left->strike().x + right->strike().x) / 2;
            k.bars.push_back({ p, true, center - width / 2, 62 + length * 0.12 - length, width, length });
        }
    }
    double begin = 1e9, end = 0;
    for (const auto& b : k.bars) { begin = std::min(begin, b.x); end = std::max(end, b.x + b.width); k.front = std::max(k.front, b.y + b.length); }
    for (auto& b : k.bars) b.x -= begin;
    k.width = end - begin;
    return k;
}
static void issue(Pose& pose, const char* key, const char* message, int severity) {
    pose.issues.push_back({ key, message, severity }); pose.severity = std::max(pose.severity, severity);
    if (severity == 2) pose.valid = false;
}
static Pose evaluate(const Keyboard& keyboard, const std::vector<int>& pitches, const std::vector<int>& ids,
                     const Player& player, const Pose* previous) {
    Pose pose; pose.pitches = pitches; pose.mallets = ids;
    std::vector<double> centers;
    for (size_t i = 0; i < pitches.size(); ++i) {
        const auto* bar = keyboard.bar(pitches[i]);
        if (!bar) { issue(pose, "range", "A new attack lies outside this instrument's range.", 2); continue; }
        Point target = bar->strike();
        target.y = bar->y + bar->length * std::clamp(player.strikeFractions[i], .12, .88);
        pose.targets[ids[i]-1] = target; pose.active[ids[i]-1] = true; centers.push_back(target.x);
    }
    pose.body = { keyboard.width / 2, keyboard.front + 43 };
    if (!centers.empty()) { std::sort(centers.begin(), centers.end()); pose.body.x = std::clamp(centers[centers.size()/2] + player.bodyOffset, 12.0, std::max(12.0, keyboard.width - 12)); }
    for (int hand = 0; hand < 2; ++hand) {
        std::vector<Point> targets;
        for (int m = hand * 2; m < hand * 2 + 2; ++m) if (pose.active[m]) targets.push_back(pose.targets[m]);
        Point midpoint { pose.body.x + (hand == 0 ? -14 : 14), keyboard.front + 10 };
        if (!targets.empty()) {
            midpoint = {}; for (const auto& target : targets) { midpoint.x += target.x / targets.size(); midpoint.y += target.y / targets.size(); }
            pose.openings[hand] = targets.size() == 2 ? distance(targets[0], targets[1]) : 0;
        }
        const double half = pose.openings[hand] / 2;
        pose.wrists[hand] = { midpoint.x, midpoint.y + std::sqrt(std::max(25.0, player.shaft * player.shaft - half * half)) };
        if (targets.size() == 2) pose.rotations[hand] = std::atan2(std::abs(targets[0].y - targets[1].y), std::abs(targets[0].x - targets[1].x)) * 180 / 3.141592653589793;
        const Point shoulder { pose.body.x + (hand == 0 ? -14 : 14), pose.body.y };
        const double reach = distance(shoulder, pose.wrists[hand]);
        pose.cost += pose.openings[hand] + reach * 0.1 + pose.rotations[hand] * 0.08;
        if (half >= player.shaft) issue(pose, "shaft", "This hand's targets exceed the configured mallet-shaft geometry.", 2);
        if (pose.openings[hand] > player.opening) {
            issue(pose, hand ? "right-opening" : "left-opening", hand ? "Right hand exceeds your configured comfortable opening." : "Left hand exceeds your configured comfortable opening.", 1);
            pose.cost += 3 * (pose.openings[hand] - player.opening);
        }
        if (!targets.empty() && reach > player.reach) { issue(pose, hand ? "right-reach" : "left-reach", "Move the body or adjust the player profile: estimated arm reach exceeds your limit.", 1); pose.cost += (reach - player.reach) * 2; }
        if (pose.rotations[hand] > 50) issue(pose, "rotation", "Targets span both rows; this hand needs substantial rotation. Check your grip and strike location.", 1);
    }
    if (pose.wrists[0].x > pose.wrists[1].x) { issue(pose, "arms", "Arms cross in this placement. A crossover is a technique choice, not automatically unplayable.", 1); pose.cost += 18; }
    if ((pose.active[0] && pose.active[1] && pose.targets[0].x > pose.targets[1].x)
        || (pose.active[2] && pose.active[3] && pose.targets[2].x > pose.targets[3].x)) {
        issue(pose, "shafts", "Mallet shafts cross within a hand; verify grip and shaft clearance.", 1); pose.cost += player.grip == 0 ? 12 : 5;
    }
    for (int a = 0; a < 4; ++a) for (int b = a + 1; b < 4; ++b)
        if (pose.active[a] && pose.active[b] && distance(pose.targets[a], pose.targets[b]) < player.head)
            issue(pose, "heads", "Mallet heads overlap at these strike points. Move a strike point or use a different attack.", 1);
    if (previous) {
        double movement = 0;
        for (int m = 0; m < 4; ++m) if (pose.active[m] && previous->active[m]) movement += distance(pose.targets[m], previous->targets[m]);
        pose.cost += movement * 0.18 + std::abs(pose.body.x - previous->body.x) * 0.15;
        if (movement > 80) issue(pose, "transition", "The preceding attack needs a large movement. Check the tempo and preparation time.", 1);
    }
    if (pose.issues.empty()) issue(pose, "comfortable", "No configured range or comfort limit exceeded. This is an estimated static placement, not a performance guarantee.", 0);
    return pose;
}
std::vector<Pose> solve(const Keyboard& keyboard, const std::vector<int>& pitches, const Player& player,
                        const std::vector<int>& required, const Pose* previous) {
    if (pitches.empty()) return {};
    if (pitches.size() > size_t(player.malletCount)) {
        Pose pose; pose.pitches = pitches; issue(pose, "count", "More simultaneous new attacks than available mallets. Sustained notes are counted separately.", 2); return { pose };
    }
    std::array<int, 4> ids { 1, 2, 3, 4 };
    std::vector<Pose> result;
    std::set<std::vector<int>> seen;
    do {
        std::vector<int> assignment(ids.begin(), ids.begin() + pitches.size());
        bool accepted = true;
        for (size_t i = 0; i < assignment.size(); ++i) {
            const int id = assignment[i]; const int r = i < required.size() ? required[i] : 0;
            if ((player.malletCount == 2 && id != 1 && id != 4) || (r > 0 && r != id) || (r == -1 && id > 2) || (r == -2 && id < 3)) accepted = false;
        }
        if (accepted && seen.insert(assignment).second) result.push_back(evaluate(keyboard, pitches, assignment, player, previous));
    } while (std::next_permutation(ids.begin(), ids.end()));
    std::stable_sort(result.begin(), result.end(), [](const Pose& a, const Pose& b) { return a.severity != b.severity ? a.severity < b.severity : a.cost < b.cost; });
    if (result.empty()) { Pose pose; pose.pitches = pitches; issue(pose, "sticking", "Written sticking cannot assign these simultaneous attacks with this mallet count.", 2); result.push_back(pose); }
    return result;
}
std::vector<Pose> alternatives(const Keyboard& keyboard, const std::vector<int>& pitches, const Player& player, const Pose* previous) {
    if (pitches.empty() || pitches.size() > size_t(player.malletCount)) return {};
    auto same = solve(keyboard, pitches, player, {}, previous);
    std::vector<Pose> result;
    for (size_t i = 0; i < std::min(size_t(3), same.size()); ++i) if (same[i].valid) result.push_back(same[i]);
    std::set<std::vector<int>> seen { pitches };
    std::vector<Pose> changed;
    for (size_t n = 0; n < pitches.size(); ++n) for (const int shift : {-12, 12}) {
        auto next = pitches; next[n] += shift;
        if (!keyboard.bar(next[n]) || !seen.insert(next).second) continue;
        // Do not omit or merge a voice to make a suggestion appear playable.
        if (std::count(next.begin(), next.end(), next[n]) > 1) continue;
        auto candidates = solve(keyboard, next, player, {}, previous);
        if (!candidates.empty() && candidates.front().valid) changed.push_back(candidates.front());
    }
    std::stable_sort(changed.begin(), changed.end(), [](const Pose& a, const Pose& b) { return a.severity != b.severity ? a.severity < b.severity : a.cost < b.cost; });
    for (size_t i = 0; i < std::min(size_t(5), changed.size()); ++i) result.push_back(changed[i]);
    return result;
}
}
