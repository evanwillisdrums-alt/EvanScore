# Native cross-bar tuplets

Requested and confirmed 2026-10-16. Queued after the mallet/keypad/global-stem release. This is an investigation and acceptance plan; the feature is not implemented.

The requested result is one continuous tuplet group across a barline, retaining its ratio and correct note timing. Selecting its bracket should select that group. Moving the number above a bar or disguising separate tuplets does not satisfy this request.

## Findings from the current engine

- `Tuplet` is a `DurationElement` with one `Measure` ownership parent (`src/engraving/dom/tuplet.h`). Its elements retain real rational timing through `DurationElement::globalTicks()` and `actualTicks()`. That timing model can represent a continuous ratio; the surrounding measure assumptions must change.
- `Score::cmdCreateTuplet()` inserts every new rest into the starting measure (`src/engraving/editing/edit.cpp`). Cross-bar creation needs to resolve the correct measure for every event and preserve existing music through one native transaction.
- The current-format reader starts with a local null tuplet for each measure/voice and expects its end within that measure (`src/engraving/rw/read500/measureread.cpp`). The writer emits the start at the first element and end at the last (`src/engraving/rw/write/twrite.cpp`). A cross-measure group must survive reading, not merely display correctly in memory.
- Missing-element repair calls the owning measure for each rest (`Tuplet::addMissingElement`). Repair, deletion and incomplete imported groups need explicit cross-measure handling.
- Paste explicitly excludes tuplets from splitting over a barline (`src/engraving/editing/paste.cpp`). Removing a UI restriction alone would leave this operation incorrect.
- Tuplet layout keeps one bracket/number and measure-relative geometry (`src/engraving/rendering/score/tupletlayout.cpp`). A group continuing onto another system needs real layout segments and hit testing, without changing its musical identity.

- MusicXML import explicitly calls `resetTuplets()` at every measure boundary (`src/importexport/musicxml/internal/import/importmusicxmlpass2.cpp`, comment “prevent tuplets from crossing measure boundaries”). An open group may be filled/stopped there. MusicXML import is therefore also part of the implementation, rather than an existing way to obtain native continuous groups.

## Concrete timing reference

[Reference MusicXML](research/cross-bar-tuplet-reference.musicxml) describes a quarter-note 3:2 C–D–E group beginning on beat 4 of 4/4 and ending after beat 1 of the next measure. D splits into tied triplet eighths over the barline; there are three attacks and four printed note fragments. Both measures contain exactly four beats, and the one group lasts two beats. XML structure/rational durations have been checked; import/rendering in the current app is **not** claimed to preserve the group. Use this as an implementation acceptance input, not a released workaround.

## Implementation direction to prove

Retain one logical tuplet and its existing ratio/nesting semantics. Give it system-specific bracket geometry, resolve event ownership by actual tick, and carry explicitly identified open groups between measure/voice reader contexts. Keep ordinary one-measure tuplets and their saved files unchanged.

A note can itself intersect the barline inside the group. That note must split into tied written durations whose actual durations sum exactly to the original attack, with the tuplet ratio retained on both portions. The barline must not create a second attack or introduce a rounding gap. Preserve existing music outside the affected interval and reject an unsafe edit before mutating anything.

Choose the native creation interaction after the engraving prototype passes: specify the group ratio/base duration from a starting note/rest, preview its full span, and apply in one Undo. Existing shortcuts for ordinary tuplets must retain their behavior. Selection, Properties and diagnostics should identify the complete group, including its continuation.

## Acceptance cases

1. A 3:2 group spanning two measures has exact rational onsets and the correct total duration in every affected measure.
2. A note crossing the barline splits/ties correctly; playback attacks only once, and MIDI/export timing agrees with notation.
3. 5:4 and 7:4 groups, rests, chords, secondary voices and an adjacent ordinary tuplet retain their own membership and timing.
4. One Undo/Redo restores both measures, including overwritten rests and links, without changing nearby music.
5. Native save/reopen retains one group and identical timing; incomplete/malformed data is rejected or repaired deterministically.
6. Page/continuous layouts and a forced system/page break show correct continuation brackets, number placement and selection/hit testing.
7. Copy/paste, delete, duration edits, bar insertion/deletion and time-signature changes preserve the group or give a clear refusal before mutation.
8. Linked parts and full score agree; ordinary/nested single-measure tuplet regressions continue to pass. MusicXML export/import requires explicit verification and must not silently change timing.

No release claim is justified until the native object, editing, persistence, layout and playback paths pass these checks. Do not fold an unverified notation-model change into the currently compiling caption/stem release.
