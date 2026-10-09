/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#pragma once
#include "engraving/types/types.h"
#include "engraving/style/styledef.h"
#include "types/bytearray.h"
#include <array>
namespace mu::engraving {
class MStyle; class Score; class Note; class Hairpin; class Chord;
class DynamicsPlayback {
public:
    enum Role { Auto = 0, Normal = 1, Tap = 2, Accent = 3, Tenuto = 4, Marcato = 5, Ghost = 6, SoftAccent = 7, Stress = 8, Unstress = 9 };
    static Sid mappingStyle(int role);
    static std::array<Sid, 14> profileStyles();
    static muse::ByteArray preset(const Score* score);
    // Validates the complete preset before changing any style value.
    static bool readPreset(const muse::ByteArray& data, MStyle& target);
    static MStyle marchingSnareDefaults(const MStyle& base);
    static const MStyle& profile(const Score* score);
    static bool enabled(const Score* score);
    static double progress(double t, int shape, double bend);
    static int level(const Score* score, DynamicType type, int role);
    static int velocity(const Note* note, int fallback);
    static int velocityAt(const Note* note, const Fraction& position, int fallback);
    static bool accented(const Chord* chord);
    static int role(const Note* note);
    static bool applies(const Hairpin* hairpin, track_idx_t track);
    static const Hairpin* activeHairpin(const Score* score, const Fraction& tick, track_idx_t track);
    static double hairpinValue(const Hairpin* hairpin, double position, int role = Auto);
    static bool smoothArticulations(const Hairpin* hairpin);
    static int endpoint(const Hairpin* hairpin, bool end, int lane = Auto);
};
}
