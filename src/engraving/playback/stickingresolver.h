/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace mu::engraving {
class Chord;

// Shared notation data for sample selection and the future mallet visualizer.
// Lowercase is preserved in rawText, but does not itself change dynamics.
enum class StickingKind { Missing, Hands, Mallets, Unsupported, Ambiguous };
enum class StickingHand { Unspecified, Left, Right };
struct StickingStroke {
    StickingHand hand = StickingHand::Unspecified;
    int mallet = 0;
};
struct StickingAssignment {
    std::string rawText;
    StickingKind kind = StickingKind::Missing;
    std::vector<StickingStroke> strokes;
    std::string diagnostic;
};

class StickingResolver {
public:
    static StickingAssignment parse(std::string_view text);
    static StickingAssignment resolve(const Chord* chord);
};
}
