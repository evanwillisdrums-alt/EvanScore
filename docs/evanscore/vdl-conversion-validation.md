# Virtual Drumline conversion validation

The user requests easy Muse Drumline-to-VDL sound switching, with Kontakt running in the background and the same written instruments retained. These are release requirements for the converter. They are not passing-test claims: the converter and prepared Kontakt patch selection are still unimplemented.

## Fixtures and independent expectations

Use synthetic, shareable scores for automated regression tests. Keep the user's private `Pad Lick #1.mscz` out of the repository; use it separately as a realistic regression once mapping exists. Include the existing `buildscripts/ci/windows/fixtures/ff-snare.mscx` (four quarters, accents on beats 1 and 4) and add technique/sticking passages covering these cases:

| Source passage | Expected result |
| --- | --- |
| Marching snare, normal center strokes with R/L/R/R sticking | Preserve written pitch 50 and instrument identity; the Manual/LITE patch's playback-only notes distinguish right and left hands. Do not silently use AutoRL for an explicit repeated hand. |
| Center → halfway → edge → center staff text | Send the documented CC1 zone before the relevant attacks, then reset correctly. SnareLine Manual uses 0–43 / 44–89 / 90–127; other patches need their own rules. |
| Rim shot, rim click, stick click, backstick, stick shot, shell, buzz | Translate the source drumset's sound semantics and articulation, rather than treating its MIDI pitch as the VDL MIDI pitch. Unsupported equivalents produce a visible diagnostic. |
| Guts/snares on → off → on; line → solo → line | Resolve the selected patch's documented sounds. Do not assume all patches have identical switches or that every change uses CC1. |
| Flams, diddles, buzzes, tuplets, grace notes, ties and rolls | Retain written durations, onset order, tremolo type, grace-note relationships, and hand assignments. Check generated playback events separately. |
| Accents → tenutos → taps under a decrescendo | Preserve the user's custom velocities, Smooth through articulations preference, curve and exact tap endpoint. Ghosts remain silent. |
| Tenors, bass drums, cymbals and pitched/auxiliary instruments | Select the matching instrument and variant. Preserve tenor/bass voice identity and keyboard register; reject absent matches rather than substituting a snare. |
| Unknown/ambiguous source drumset name or unsupported technique | Keep the original score intact and identify the unresolved sound; never silently choose an unrelated playable pitch. |

The uploaded guide's note convention is **C3 = MIDI 60**. Independent expected values must use numeric MIDI notes to avoid an octave error. For SnareLine Manual/LITE, the diagram on page 29 places the ordinary right-hand hit at G♯4 (80) and left-hand hit at F♯4 (78); D♯4 (75) and C♯4 (73) are right/left rim sounds. These expectations were checked against the diagram's C4 anchor, white-key sequence and red strike-point positions, not inferred from label order. They are diagram-derived expectations, not an auditioned patch result. AutoRL has a different map; Manual and Manual LITE share mapping but differ in sample-layer coverage.

## Native regression checks

1. Capture instrument IDs, staff types/lines/clefs, custom drum definitions, written pitches and noteheads, durations/tuplets/ties/grace notes, articulations, dynamics/overrides/curves, sticking, staff text and sound flags before selecting VDL. Compare those fields afterward, excluding the intended playback-source/profile state.
2. Inspect generated playback notes, CC events and keyswitches against independently reviewed guide expectations. Check resets and event ordering before the first audible attack. An unchanged notation snapshot alone does not prove correct playback.
3. Test seeking into each technique, starting mid-score, repeats, alternate endings, stopping/restarting, and returning to the beginning. Previous playback must not leave stale Kontakt controller or keyswitch state.
4. Save, close the app, reopen and check both the unchanged notation and restored Kontakt component/controller state. Missing plugins, relocated libraries, activation failures and invalid profile data must yield useful recoverable errors.
5. Switch VDL → Muse Drumline → MS Basic → VDL repeatedly. Keep notation and compatible technique semantics intact; reapply the correct playback map each time. Verify multi-staff scores and instrument changes without cross-channel/controller leakage.
6. Verify the conversion operation's documented Undo/Redo behavior; failed or cancelled preparation must not leave partial instrument mappings or delete sound flags.
7. In the full Windows app, click the existing Mixer sound selector, select a prepared VDL sound and confirm no Kontakt window opens automatically. Explicit editor access must still work. Restart with that score and repeat.
8. Measure repeated switching/editing memory and responsiveness with an actual loaded library. Include full/LITE variants and verify silence/velocity limits; do not equate a basic soundfont render with a VDL playback test.
9. Cover every supported library patch/variant with a catalog-to-map completeness check and representative functional passages. Do not present an instrument as supported merely because its name appears in a menu.

## Licensed playback gate

Run real Kontakt Player VST3 on Windows with the user's activated VDL 2.5.5 files. Render and audition each instrument family and each technique category, verify save/reopen and background selection, and compare the resulting behavior with the event expectations above. The guide supplies mapping evidence; it does not supply executable Kontakt state or sample audio. A setup-helper test, mock plugin, source inspection, or synthetic MIDI render cannot satisfy this gate.

## Current evidence

EvanScore's generic VST3 host exists. The setup helper passed Windows PowerShell checks and retrieved the official Native Access installer with a verified Native Instruments Authenticode signature. No activated VDL library is installed here. Source inspection found generic VST selection automatically opens the editor, and MuseSampler-to-VST changes can replace the drumset and remove sound flags. Those are integration issues to fix selectively for prepared VDL profiles before running the full conversion suite.
