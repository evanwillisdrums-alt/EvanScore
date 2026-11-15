# Workflow upgrade cycle — November 14, 2026 (America/Chicago)

User authorizes an extended implementation/research cycle. New messages queue
additional work. Preserve save fixes, dirty muse, and the isolated cross-bar work.
This document separates intended work from shipped behavior.

## First batch in development

- Reliable editing shortcuts: floating keypad forwards platform Undo/Redo,
  Cut/Copy/Paste and Delete to native notation commands; numeric editors retain
  local editing shortcuts. Actual Windows keyboard regression added; not yet run.
- Compact percussion/mixer opening: both use the same default, raised16px;
  dock opening owns the deferred clamp so automatic percussion opening is covered.
- Snare-split staves: a saved Solo/split role, independent of generic hiding.
  Auxiliary staves above the main staff show only when needed, disappear before
  and after the passage including the first system, consume no empty staff slot,
  carry no instrument/staff label and do not move the main instrument name's
  vertical center. Multiple auxiliaries within one instrument are the initial
  target. Association across separate instrument parts needs explicit design.
  Test actual bounding boxes and name placement; a visibility checkbox alone is
  not acceptance. Manual system breaks and pagination must remain correct.

## Additional implementation awaiting native verification

- Same-sound written percussion position: Properties → Noteheads offers Up,
  Down and Reset, changing fixed staff position without changing MIDI pitch.
- Arial:70 ordinary text-style factory defaults; Styles → Text styles →
  Arial for score text explicitly resets family overrides in an existing score.
  Music fonts and embedded notation symbols remain, with Undo/Cancel support.
  Existing files are not silently restyled on open; saved user styles still win.
- Customize Kit: search current-kit name/pitch/shortcut, mapped-only toggle,
  audition button and double-click preview through the staff's current sound.
  Existing notehead mapping remains available. This is the first browser step;
  cross-library indexing, external samples and timbre similarity remain pending.
- Appearance: latest user direction is strong Logic inspiration with original
  EvanScore identity, including useful rearrangement. Graphite/steel surfaces,
  restrained teal-blue accent and EvanScore branding drafted; actual captures
  still required. This is not a completed full application redesign.
- Validation:15 focused engraving/percussion tests pass; expanded native Windows
  interactions are compiled locally but have not run on the latest full binary.

## Additional queued requests

- Move individual percussion notes among staff lines/spaces while preserving
  pitch/sample, sticking, velocity and playback. Expose a distinct written-position
  operation with Undo/Redo and persistence, separate from drum-sound transposition.
- Percussion sound browser/mapper: choose a printed notehead (e.g. W), search and
  audition available sounds, bind a playback mapping and save reusable kit presets.
  Distinguish host-exposed plugin/library sounds from local audio samples requiring
  a sampler. Do not claim arbitrary Kontakt patches expose a searchable catalogue.
  Similarity search by timbre is separate from initial name/tag search.
- Faster notation entry inspired by popovers; selection transformations and
  paste-special; practice loops; mixer/effects workflow; reusable writing setups.
  Audit existing commands first. Do not call existing MuseScore features missing
  merely because their workflow is awkward. These remain candidates, not shipped.

## Research evidence and access limits

Official Logic and Dorico feature pages were attempted but the environment proxy
returned403, also outside the command sandbox. No official-page review or full
current cross-product audit is claimed. Runtime request_changes is unavailable;
access to support.apple.com/steinberg.net and relevant community sites requires an
owner's running-environment network setting change. No draft configuration changed.

Accessible direct user feedback (examples, not a popularity survey):

- https://github.com/musescore/MuseScore/issues/24469 — requests faster keyboard
  palette insertion; relevant to popover/command entry.
- https://github.com/musescore/MuseScore/issues/22583 — divided-section notation
  and label-placement problems; relevant to split-staff workflow.
- https://github.com/musescore/MuseScore/issues/24937 — Logic synchronization
  request; interoperability is a larger project, not a five-hour commitment.
- https://github.com/musescore/MuseScore/issues/27934 — imported MIDI velocity
  versus dynamics conflicts; compare existing EvanScore reset/lock behavior first.
- https://github.com/musescore/MuseScore/issues/30662 — request for harmonic
  playback scrubbing, referencing Finale; distinguish from existing looping.

Raw retrieved issue responses and local validation logs are under
/workspace/evanscore-validation/20261115. No commercial apps were run locally.

## Release requirements

Full Windows build, fresh-machine startup/export and native save gate remain
mandatory. Add relevant keypad, Dynamics, percussion/split-layout and panel-size
checks. Verify real score edits, Undo/Redo, saved XML, reopening, missing staff
space and label positions. No new download until affected native checks pass.
