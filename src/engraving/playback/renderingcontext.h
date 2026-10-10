/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include "engraving/dom/dynamicsplayback.h"
#include "mpe/events.h"

#include "../dom/chord.h"
#include "../dom/note.h"
#include "../dom/sig.h"
#include "../dom/part.h"
#include "../dom/instrument.h"
#include "../dom/drumset.h"
#include "stickingresolver.h"

#include "utils/arrangementutils.h"
#include "utils/pitchutils.h"
#include "playbackcontext.h"

namespace mu::engraving {
struct RenderingContext {
    muse::mpe::timestamp_t nominalTimestamp = 0;
    muse::mpe::duration_t nominalDuration = 0;
    muse::mpe::dynamic_level_t nominalDynamicLevel = 0;
    int nominalPositionStartTick = 0;
    int nominalPositionEndTick = 0;
    int nominalDurationTicks = 0;
    int positionTickOffset = 0;

    BeatsPerSecond beatsPerSecond = 0;
    TimeSigFrac timeSignatureFraction;

    muse::mpe::ArticulationMap commonArticulations;

    const Score* score = nullptr;
    const muse::mpe::ArticulationsProfilePtr profile;
    const PlaybackContextPtr playbackCtx;

    bool isValid() const
    {
        return score
               && profile
               && playbackCtx
               && beatsPerSecond > 0
               && nominalDuration > 0
               && nominalDurationTicks > 0;
    }
};

inline RenderingContext buildRenderingCtx(const Chord* chord, const int tickPositionOffset,
                                          const muse::mpe::ArticulationsProfilePtr profile, const PlaybackContextPtr playbackCtx,
                                          const muse::mpe::ArticulationMap& articulations = {})
{
    int chordPosTick = chord->tick().ticks();
    int chordDurationTicks = chord->actualTicks().ticks();
    int chordPosTickWithOffset = chordPosTick + tickPositionOffset;

    const Score* score = chord->score();

    auto chordTnD = timestampAndDurationFromStartAndDurationTicks(score, chordPosTick, chordDurationTicks, tickPositionOffset);

    BeatsPerSecond bps = score->multipliedTempoAtUtick(chordPosTickWithOffset);
    TimeSigFrac timeSignatureFraction = score->sigmap()->timesig(chordPosTick).timesig();

    RenderingContext ctx{ chordTnD.timestamp,
                          chordTnD.duration,
                          playbackCtx->appliableDynamicLevel(chord->track(), chordPosTickWithOffset),
                          chordPosTick,
                          chordPosTick + chordDurationTicks,
                          chordDurationTicks,
                          tickPositionOffset,
                          bps,
                          timeSignatureFraction,
                          articulations,
                          score,
                          profile,
                          playbackCtx };

    return ctx;
}

struct NominalNoteCtx {
    voice_idx_t voiceIdx = 0;
    staff_idx_t staffIdx = 0;
    muse::mpe::timestamp_t timestamp = 0;
    muse::mpe::duration_t duration = 0;
    BeatsPerSecond tempo = 0;
    muse::mpe::dynamic_level_t dynamicLevel = 0;
    float userVelocityFraction = 0.f;

    muse::mpe::pitch_level_t pitchLevel = 0;
    muse::mpe::PercussionSource percussion;

    RenderingContext chordCtx;
    muse::mpe::ArticulationMap articulations;

    explicit NominalNoteCtx(const Note* note, const RenderingContext& ctx)
        : voiceIdx(note->voice()),
        staffIdx(note->staffIdx()),
        timestamp(ctx.nominalTimestamp),
        duration(ctx.nominalDuration),
        tempo(ctx.beatsPerSecond),
        dynamicLevel(ctx.nominalDynamicLevel),
        userVelocityFraction(note->userVelocityFraction()),
        pitchLevel(notePitchLevel(note->playingTpc(),
                                  note->playingOctave(),
                                  note->playingTuning())),
        chordCtx(ctx),
        articulations(ctx.commonArticulations)
    {
        const auto* instrument = note->part() ? note->part()->instrument(note->tick()) : nullptr;
        if (instrument && instrument->useDrumset() && instrument->drumset()) {
            percussion.present = true;
            percussion.instrument = instrument->id().toStdString();
            const int pitch = note->pitch();
            if (pitch >= 0 && pitch < 128) percussion.sound = instrument->drumset()->name(pitch).toStdString();
            const auto assignment = StickingResolver::resolve(note->chord());
            if (assignment.kind == StickingKind::Hands && assignment.strokes.size() == 1)
                percussion.hand = assignment.strokes.front().hand == StickingHand::Left ? 2 : 1;
            else if (assignment.kind != StickingKind::Missing) percussion.hand = -1;
            if (ctx.playbackCtx) {
                const auto flags = ctx.playbackCtx->percussionFlags(note->track(), ctx.nominalPositionStartTick + ctx.positionTickOffset);
                percussion.zone = flags[0]; percussion.snaresOff = flags[1]; percussion.solo = flags[2];
            }
        }
        if (DynamicsPlayback::enabled(note->score())) {
            const int fallback = static_cast<int>(ctx.nominalDynamicLevel * 127 / muse::mpe::MAX_DYNAMIC_LEVEL);
            int velocity = DynamicsPlayback::velocity(note, fallback);
            if (note->userVelocity() != 0) velocity = note->customizeVelocity(velocity);
            userVelocityFraction = std::clamp(velocity, 1, 127) / 127.f;
            dynamicLevel = static_cast<muse::mpe::dynamic_level_t>(userVelocityFraction * muse::mpe::MAX_DYNAMIC_LEVEL);
        }
    }
};

inline muse::mpe::NoteEvent buildNoteEvent(const NominalNoteCtx& ctx, const muse::mpe::PitchCurve& pitchCurve = {})
{
    auto event = muse::mpe::NoteEvent(ctx.timestamp,
                                ctx.duration,
                                static_cast<muse::mpe::voice_layer_idx_t>(ctx.voiceIdx),
                                static_cast<muse::mpe::staff_layer_idx_t>(ctx.staffIdx),
                                ctx.pitchLevel,
                                ctx.dynamicLevel,
                                ctx.articulations,
                                ctx.tempo.val,
                                ctx.userVelocityFraction,
                                pitchCurve);
    event.setPercussionSource(ctx.percussion);
    return event;
}
}
