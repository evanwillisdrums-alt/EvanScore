/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#pragma once
#include <vector>
#include "malletplacement.h"
namespace mu::engraving { class Note; }
namespace mu::notation::mallet {
bool canApply(const std::vector<engraving::Note*>& notes, const Pose& candidate);
// Caller owns the native undo transaction. Validate everything before mutation.
bool apply(const std::vector<engraving::Note*>& notes, const Pose& candidate, bool reverseNumbering = false);
}
