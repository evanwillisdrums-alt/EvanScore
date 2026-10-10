# EvanScore working workspace

With no file specified, EvanScore starts on the Home landing page with New/Open and recent scores; the user explicitly restored this on 2026-10-09. Home and Score remain in the compact primary navigation. Opening a file or continuing a session goes directly to notation. Closing the last score returns to Home. Session recovery and unsaved-change prompts remain intact. Explicit startup preferences (new score, specific score, continue last) still work.

Automatic first-launch setup, welcome/version promotions, and instructional tours are removed. The Learn item is omitted from the file-browser sidebar; the Help menu remains available. Publishing and developer tools are removed from the primary navigation. Project titles identify EvanScore.

The neutral charcoal appearance is applied once on first launch of this update. Later appearance choices in Preferences are respected across restarts; high-contrast themes are preserved. Rounded document tabs have quieter borders, an unsaved-change dot, close buttons, and existing context-menu/middle-click behavior. No score music fonts or notation styles are changed by the UI theme.

The reference direction is the user's IMG_0529/0530 Logic-style workspace and IMG_0532 translucent keypad, as recorded in AGENTS.md. Attempts to retrieve the official Apple Logic Pro/Human Interface Guidelines and Microsoft Fluent design pages were blocked by the environment's network proxy; no claims rely on reviewing those inaccessible pages. The implementation emphasizes grouped native controls, consistent rounding/spacing, restrained blue selection, and clear musical icons.

## Dynamics responsiveness

The transport time readout switches between elapsed time and bar.beat when clicked; it remembers the choice across app restarts. The third musical-position field represents thousandths of a beat (1.2.500 is halfway through beat 2). The separate Bar · Beat fields retain native seeking. Time, bar/beat, tempo, and meter/key share a common value center line in the thin 36px toolbar. The format toggle remains usable when no audio device is available.

Mixer visibility changes coalesce panel resizing until the current QML layout completes. Removed aux channels and hidden aux-send controls release their owned UI objects; hidden meters disconnect their UI signal subscribers and resume when shown. These changes preserve audio routing and send values. Local isolated checks using the unchanged native method bodies and actual AuxSendItem/async channels reproduce 1,001 owned controls after 1,000 hide/show cycles before the fix and one control afterward. This is component ownership evidence, not a full-app or licensed-library memory benchmark.

The native Dynamics model caches the mapping table and only notifies it when score-wide mapping values change. Selection, curve, and local-note updates use a separate notification. The QML grid uses stable dynamic row identities, so editing a velocity does not destroy/recreate its editor or lose keyboard focus. Preset loading and undo/redo still refresh changed values.

Playback-only dynamic profile edits preserve native change notifications, audio rebuilding, save and Undo without rescanning/re-laying out printed notation. Duplicate edits are skipped. Numeric fields protect Return/Enter from score shortcuts while leaving Ctrl+S available.

Bottom panels open at 25% of available height, bounded to 140–220px, with manual resizing available afterward. View → Mallet visualizer opens the optional native illustrated instrument/player panel; closing unloads its model and stops timers/subscriptions. It supports score selection, bar picking, Top/Front, estimated diagnostics, alternative sticking/one-note octave previews, audition/compare, and explicit undoable Commit. Passage animation and calibrated physical limits remain future work.

## Validation

- Qt C++ syntax checks for startup, appearance, navigation, project closing, and dynamics model sources.
- Actual Qt Quick interaction checks for all five keypad tabs, modern undo/redo dispatch, tremolo type/track assignment, independent open noteheads, layout swap/save/reset, and per-tab resize persistence. Host services were mocked; symbols were rendered with the repository's actual fonts.
- Actual Dynamics QML checks for curve dragging, note selection, zero-velocity mute, presets/defaults, and repeated keyboard edits without recreating the mapping editor. Host model was mocked.
- Windows release gate requires score export and a real responsive desktop window beyond the splash; its title must identify EvanScore and no automatic onboarding dialog may be present. Native Windows build results are reported separately.

Plugin packaging waits for the matching native Windows build when both native and plugin files change, preventing an update archive from silently reverting newer native features.

## Optional feature diagnostics

Diagnostics → EvanScore feature diagnostics opens a floating read-only report for selected notes or an explicit full-score scan. It shares the host theme, stays out of the normal writing flow, supports resize/close/refresh/copy, and bounds/coalesces scans. The shared sticking reader and current note-dynamics state are exposed; VDL sample routing remains pending; the native mallet panel reports its current options, pose, diagnostics and estimated-geometry status. See [feature-diagnostics.md](feature-diagnostics.md). Native Window interactions are included in the Windows diagnostic release checks, separately from local host-substitute checks.

Runtime diagnostics include the actual process working set (and Windows private bytes), saved transport format, and loaded Mixer channel/aux-control ownership and metering state. Refresh gives a new snapshot; closing diagnostics stops its work. It does not measure total system RAM or separate licensed sample allocations from other process allocations.
