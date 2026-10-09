/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include "stickingresolver.h"

#include "dom/chord.h"
#include "dom/segment.h"
#include "dom/sticking.h"

using namespace mu::engraving;

StickingAssignment StickingResolver::parse(std::string_view text)
{
    StickingAssignment result;
    result.rawText = std::string(text);
    bool hands = false;
    bool mallets = false;
    for (char character : text) {
        switch (character) {
        case 'R': case 'r':
            result.strokes.push_back({ StickingHand::Right, 0 });
            hands = true;
            break;
        case 'L': case 'l':
            result.strokes.push_back({ StickingHand::Left, 0 });
            hands = true;
            break;
        case '1': case '2': case '3': case '4': case '5': case '6':
            result.strokes.push_back({ StickingHand::Unspecified, character - '0' });
            mallets = true;
            break;
        case ' ': case '\t': case '\r': case '\n': case '.': case '-':
            break;
        default:
            result.kind = StickingKind::Unsupported;
            result.strokes.clear();
            result.diagnostic = "Unrecognized sticking syntax; no hand or mallet assignment applied";
            return result;
        }
    }
    if (hands && mallets) {
        result.kind = StickingKind::Ambiguous;
        result.strokes.clear();
        result.diagnostic = "Mixed hand and mallet numbering needs an explicit convention";
    } else if (hands) {
        result.kind = StickingKind::Hands;
    } else if (mallets) {
        result.kind = StickingKind::Mallets;
        result.diagnostic = "Mallet numbers retained; grip/numbering convention is not inferred";
    } else {
        result.diagnostic = "No explicit sticking at this note";
    }
    return result;
}

StickingAssignment StickingResolver::resolve(const Chord* chord)
{
    if (!chord || chord->isGrace() || !chord->segment()) {
        return parse({});
    }
    const Sticking* marking = nullptr;
    for (const EngravingItem* annotation : chord->segment()->annotations()) {
        if (!annotation || !annotation->isSticking() || annotation->track() != chord->track()) {
            continue;
        }
        if (marking) {
            StickingAssignment conflict;
            conflict.kind = StickingKind::Ambiguous;
            conflict.diagnostic = "Multiple sticking markings on the same staff, voice and onset";
            return conflict;
        }
        marking = toSticking(annotation);
    }
    // Do not carry a previous hand forward, read ordinary text as sticking, or
    // borrow an assignment from another staff/voice or from a main note's grace.
    return marking ? parse(marking->plainText().toStdString()) : parse({});
}
