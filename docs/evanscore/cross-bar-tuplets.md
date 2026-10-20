# Native cross-bar tuplets

Requested and confirmed 2026-10-16. Queued after the mallet/keypad/global-stem release. This is an investigation and acceptance plan. A separate internal timing/persistence/empty-span creation prototype exists; the feature is not enabled in the app.

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

Native probe on 2026-10-19 confirms the rational duration model can represent the
four fragments below as exactly 1/2 whole note, with one logical Tuplet pointer
and 1/6 + 1/12 + 1/12 + 1/6 actual durations. It also reproduces an additional
editing defect: `Tuplet::add()` sorts by each member's measure-relative tick, so
the resulting absolute order is 1, 13/12, 3/4, 11/12 rather than chronological.
This is not a released cross-bar implementation. The isolated ASan-linked probe
and output are under `/workspace/evanscore-validation/20261019/crossbar`; it uses
real engraving objects and does not change production source. Sorting, missing-
element repair, ownership/Undo, persistence and system segments still need work.

After manually sorting that investigation object, the real native writer/current
reader still loses second-measure membership: the note fragment's intended actual
duration is 1/12, but it reopens as an ordinary 1/8. The same harness retains all
three 1/6 attacks of an ordinary one-measure triplet. This is differential evidence
for the boundary limitation, not successful cross-bar persistence. The probe does
not exercise full layout/audio or tie playback; retained mock services also report
missing chord resources/EID warnings. Do not offer its investigative MSCX as a
workaround. Its missing application-service harness error was fixed before the
read/write results were recorded.

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

## Isolated persistence prototype — 2026-10-19

Branch `codex/crossbar-tuplet-prototype` is separate from verified Windows release
041f9fe067b40675642e45578fbd80c96bd9a97c. It is not enabled or distributed.
Tuplet membership uses absolute onset order. The writer marks explicitly spanning
groups; the reader carries a per-score/per-track stack only for those groups,
retaining nested parent links. Missing end markers, expired intervals and EOF
before completion produce an import error. Outer reader loops stop on that error
so child note IDs cannot be mistaken for measure IDs during rejection.

Eight native test cases pass in the isolated ASan harness. Their loops cover
3:2/5:4/7:4 in tracks0/1/3/4, exact onsets/written/actual durations, one barline tie,
an unaffected ordinary voice, simultaneous groups with different ratios, nested
3:2 inside3:2, ordinary compatibility, missing terminator/final measure and
reader-context score isolation. The same real reader/writer preserves ordinary
single-measure timing. Build replacements include complete original unity units,
so the harness does not suppress duplicate definitions or change public Tuplet
layout. Missing chord-resource/EID/transaction mock warnings remain: this is not
full-app rendering, audio, editing or importer robustness certification.

The marker is an experimental persistence mechanism, not a finished format.
Current MscLoader rejects numeric format versions above Constants::MSC_VERSION;
a future feature-specific newer version can therefore make old readers refuse
these scores rather than silently losing ratios. Deliberately settle the format,
loader dispatch and exchange behavior before enabling creation or releasing files.
Keep ordinary files in their current format; do not globally upgrade every score
just to carry this one feature. No generated prototype MSCX is a working released
workaround. Remaining acceptance requirements above still apply.

## Empty-span creator prototype — 2026-10-20

`CrossBarTuplet` now previews exact fragments and creates one logical group over
ordinary rests in one caller-owned native transaction. It validates the complete
span before edits, revalidates after preview, retains an ordinary remainder after
the group, and supports Undo/Redo and native persistence. It checks transaction
identity against the target score rather than accepting another score's active
transaction. Invalid previews contain no half-built fragment list.

Fourteen native tests pass. Creation covers3:2/5:4/7:4 and an exact5:3 split from
13/16 to19/16. The same5:3 ratio at3/4 is refused because it would need a written
1/24 at the boundary; supported ratios are not rejected as a class. All arithmetic
is rational and decomposition must sum exactly. Notes, voice gaps, existing
nested tuplets, linked staves/parts, local time stretching, spans beyond the score
or eight-measure bound, and nonrepresentable durations produce an error before
mutation. These are explicit prototype limits, not completed support for those
operations. No UI command exposes this creator yet.

The empty creator splits rests, which have no attacks to tie. Existing chord
fixtures independently retain a native tie across the barline. Writing notes
through a split group, linked creation, selected-note preservation, deletion,
paste, system/page fragments and hit testing remain required. Do not present the
empty creator's tests as full notation-command or playback acceptance.

Leak-enabled differential observations were also retained. Both the ordinary
baseline (5,504 bytes/82 allocations) and three-ratio creator run (15,664/224)
report retained instrument/Part/read, repeat-list, IOC and profiler allocations;
the direct origin categories match, with per-score categories scaling threefold.
This is not a clean leak result or a diagnosis of the user's system memory load.
Instrument-list replacement/copy and instrument-change lifetime contracts require
further inspection before any cleanup; do not blindly delete shared/Undo-owned
instrument pointers. See validation `crossbar/leak-observations.json`.
