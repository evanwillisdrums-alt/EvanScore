# Mallet visualizer

Open **View → Mallet visualizer**. This optional bottom panel follows the selected mallet part. Closing it stops its score subscriptions and analysis timers. It does not affect playback settings.

## Read a chord

Choose **Score selection**, then select any note in an onset. The panel collects new attacks across that part's staves and voices; tied continuations are shown separately. The instrument's actual range determines the bars. Marimba defaults to C2–C7 when no reliable range is available.

Without sticking, the panel evaluates physical assignments for the exact notes first. A comfortable original says **Keep the original**, with alternatives behind **Explore optional alternatives**. With written sticking, it evaluates those assignments first, tries octave revoicings that preserve the known note-to-mallet links, and offers reassignment afterward. Notes keep their original identity even when an octave change moves them past another note.

Written numbered sticking is read from lowest to highest **within each chord**. The default convention is 1 outer left, 2 inner left, 3 inner right, 4 outer right. Choose the reversed 4–3 | 2–1 convention in **Player** if needed. Explicit R/L assignments constrain the hand; `?` or `_` fills an unknown slot, for example `1 ? 3 4`. Unrecognized, conflicting, incomplete or mixed R/L-and-number syntax is gray and cannot be committed. Ordinary staff text is not assumed to be sticking.

## Placement and tone

Click an occupied bar or a **Select** button in Diagnostics to select that mallet. Drag its head along the bar, or choose **Center** / **Near edge**. The latter moves toward the end nearest the player, keeping the configured head inside the bar. The reset icon restores central strike points.

Center remains the ordinary chord default. Suggestions may use front access on accidentals when it lowers a warning level or provides a substantial modeled improvement. The explanation identifies the moved strike point and the access/tone tradeoff. Merely having accidentals does not cause an edge suggestion. Near-node warnings use approximate fundamental-mode positions, not measured nodes for a particular carved bar.

Manual points stay attached to their notes and are not overridden by the automatic strike search. Strike edits are geometry previews; this version does not store them as printed score notation or change sample tone.

**Focus** shows the active register and complete player at a larger scale, with a full-range overview above. **Full** fits the entire instrument; +/− or the mouse wheel magnifies either view. No Front view is exposed. The overhead illustration includes body, arms, hands, fingers, independent Stevens holding regions and shaft ends. Natural skin/hair colors change on opening and remain stable during that session.

## Evaluate a suggestion

Diagnostics shows pitch names and each mallet's central/end-access zone, left/right openings, pair tilt, arm reach, estimated hand/shaft clearance, outer 1–4 spread, and nearby attack travel/preparation. Original and preview statuses stay separate.

- Green: within this model's configured comfort limits.
- Orange: difficult or requiring a technique/clearance check.
- Red: a modeled hard conflict in the configured setup, such as out-of-range pitches, too many simultaneous attacks or overlapping heads.
- Gray: incomplete information or a special technique outside the supported model. It is not a declaration that an advanced player cannot perform it.

Each alternative shows exact note/sticking/strike changes, benefits, drawbacks, remaining warnings, bass/top preservation and register changes. It retains every note and pitch class; it does not omit a chord tone. Chord naming is not used as a substitute for geometry.

**Preview** changes the diagram only. **Audition** plays a temporary chord using the selected native instrument; **Compare** auditions original then preview. This is not a full rhythmic passage simulation. **Commit** applies pitches and numbered sticking in one native Undo step. Undo restores original pitches and written sticking. Commit refuses stale selections, incomplete chords, ties, grace attacks, arpeggiation, two-chord tremolos and uncertain placements.

## Configure and inspect

**Player** stores the grip, mallet count/convention, comfortable opening/tilt/reach, shaft length, head diameter, hand width, body distance, accidental-row elevation and estimated travel speed. Two-mallet mode uses physical outer left/right; three-mallet mode explicitly deploys two left and one right. The body follows the selected register and tapered front automatically; its horizontal slider adds an offset. Suggestions can protect bass and top melody, disable octave changes, or permit inversion. Preference changes persist across restarts.

**Diagnostics → EvanScore feature diagnostics** exposes the selected original/preview geometry, written sticking, alternatives count, search order, neighboring pitches/timing and analysis duration. It is read-only unless you explicitly copy its report.

## Limits and research

This is bounded chord analysis with nearby-attack estimates. Bar dimensions, hand anchors, shaft height and movement speed are estimated. Accidental elevation defaults to an editable 4 cm model assumption; set it for the actual instrument (including flatter metal keyboards). Head/shaft clearance uses height as well as the overhead coordinates. Pair tilt is not a measured anatomical wrist angle. Timing follows the native tempo timeline (including tempo changes and pauses) without expanding repeat passes. It does not prove continuous motion feasibility, simulate roll families, train an AI, recognize grips from audio, or calibrate a real performer. Changes to register or striking position still need musical listening and player feedback.

The remake uses the supplied *A Repeatable System for Choosing Mallet Stickings and Revoicing Chords* synthesis, the complete app specification and its later written-sticking preservation update, the two supplied video transcripts, Stevens grip photographs, and IMG_0523's reviewed demonstration. The linked “Research Mallet Chord Voicings” preview confirms revoicing-before-reassignment when sticking is written. Its full assistant responses are unavailable in this environment.

For accidental front access, the synthesis explains that a near/front strike can reduce reach and mixed-row tilt at a tonal cost (pages 2–3; front-access worked exercise). Yamaha's bar-vibration explanation is cited there; the external page could not be retrieved here, so the synthesis is the reviewed evidence. See [research sources](mallet-research-sources.md) and [reference audit](mallet-reference-audit.md). Numeric comfort thresholds are editable engineering estimates, not universally established performance limits.
