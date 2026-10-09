# Feature diagnostics

Open **Diagnostics → EvanScore feature diagnostics…**. Depending on the app's diagnostics menu placement, Diagnostics may be inside Help. It is an optional floating window, usable without a score, and follows the current score and selection. It does not change the score or start playback.

Select notes or a sticking label to inspect its native source marking, staff/voice/onset, recognized hand or mallet assignments, written pitches, and notation/custom-dynamics values. Choose **Full score** for a bounded scan; the report says when more than 500 chords are truncated. **Refresh** rescans; **Copy report** puts the displayed JSON on the clipboard only when requested. Resize or close the window normally. Rapid selection/notation updates are coalesced to avoid repeated scans.

Explicit R/r/L/l sequences retain repeated hands; lowercase is not a dynamics instruction. Numbers 1–6 are retained without inventing a grip or numbering convention. Missing, unsupported, mixed, or duplicate markings are reported rather than treated as a known assignment. Assignments are local to a staff, voice, and onset. Grace/diddle/roll sub-stroke routing needs separate playback integration.

Known velocities come from notation/custom mappings and explicit note overrides. A disabled automatic sound-engine value is reported as null, not as a measured velocity of 80. Actual sound-library gain/sample output cannot be inferred from this report. The current VDL sample application/conversion and mallet-visualizer status are explicitly pending/planned.

Future added features must extend this report with useful runtime state, selected inputs, resolved outputs, and unsupported/ambiguous reasons. In particular, VDL integration must report its actual patch/profile, technique, hand-specific sample pitches, controllers/keyswitches, and fallback behavior when implemented. Never label merely recognized sticking as a sample that was actually applied.
