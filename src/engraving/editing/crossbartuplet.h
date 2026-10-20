/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <vector>
#include "../dom/durationtype.h"
#include "global/types/string.h"
namespace mu::engraving {
class Rest;
class Tuplet;
class Transaction;

// Isolated engine prototype; deliberately not exposed as a notation command.
class CrossBarTuplet
{
public:
    struct Options {
        int actual = 3;
        int normal = 2;
        TDuration base { DurationType::V_QUARTER };
    };
    struct Fragment {
        Fraction tick;
        TDuration written;
        int attack = -1; // -1 is the preserved ordinary rest after the group
    };
    struct Preview {
        muse::String error;
        Fraction start;
        Fraction end;
        std::vector<Fragment> fragments;
        bool valid() const { return error.empty(); }
    };
    static Preview preview(const Rest* seed, const Options& options);
    // Caller owns a native transaction. Preflight is repeated immediately before
    // mutation; all unsupported cases return an error without touching the score.
    static Tuplet* create(Transaction& transaction, Rest* seed, const Options& options, muse::String& error);
};
}
