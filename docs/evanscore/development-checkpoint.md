# Development checkpoint

Updated 2026-10-09 UTC. Branch: `codex/rounded-ui-windows-preview`.

## Immediate priority: a verified Windows download

The user reported that the latest fully extracted Windows ZIP never leaves the splash screen. Do not provide another download until the full native package passes the Windows desktop checks. An isolated startup experiment, a successful compilation, or a diagnostic artifact is insufficient.

The most recent full build before this checkpoint was source `efbfe3b7f845d0ff30e4771af7b2313f12618174`, [run 37850997632](https://github.com/evanwillisdrums-alt/EvanScore/actions/runs/37850997632), job `113563715269`. It failed compiling `playback_qml`: the generated UI compilation included `qmlaccessible.h`, but lacked Qt Quick's C++ headers (`QQuickItem`, C1083). No runnable release was produced.

The accompanying CMake change links every app-owned QML module under `src` privately to `Qt::Quick`. QML imports alone do not provide C++ dependencies for generated AOT code. A real Qt 6.8 compile using the repository's `qmlaccessible.h` reproduced the missing-header error without this helper and successfully built with it. The full Windows Qt 6.11 build remains the required validation.

Source `1e1a95f83443b316206e5bd6b0b6f0edafdd5edc`, [run 37882347384](https://github.com/evanwillisdrums-alt/EvanScore/actions/runs/37882347384), job `113664616114`, passed the playback Qt Quick compilation but failed in `PreferencesModel::resetFactorySettings`: QCursor was only forward-declared when converting `Qt::WaitCursor`. A direct native source check without PCH reproduced the error and passed after adding `<QCursor>` to preferencesmodel.cpp. The accompanying workflow change collects other independent compile failures with Ninja's keep-going option after an initial failure, then still fails the job. This avoids repeated builds discovering only one missing header at a time. Diagnostic annotations now retain concise error lines without huge compiler commands.

After a full build passes, run the desktop diagnostic against the actual uploaded `EvanScore-Windows-x64` artifact on fresh Windows runners. The updated desktop check must also confirm normal exit after requesting window closure; do not silently accept a forced shutdown. These checks can use the newer script without rebuilding the native binary.

## Startup findings and fixes to preserve

- `643cfebc6ed7ebb65f352177a3f2b384d3200046`: the notation toolbar must be resizable and left-aligned. A centered, fixed toolbar repeatedly changed minimum sizes/padding during Qt window polishing, trapping startup in layout. Windows experiment [37841470959](https://github.com/evanwillisdrums-alt/EvanScore/actions/runs/37841470959), left variant job `113531633141`, passed empty-workspace and score-open response checks. This was an isolated experiment; the new full native package must pass without its helper.
- Keep `WorkspaceDockWindow`'s retained page URI. The pinned docking code's deferred callback otherwise refers to a temporary string.
- Keep the main toolbar's stable width calculation for bold selected labels; avoid emitting layout changes repeatedly during polish.
- `dc24ee934c76a5a84b2044be97c49ee8a7e6cee1`: automatic sound-library marketing checks/dialogs were removed from startup and score opening. Required sound-engine compatibility checks remain. Deferred checks use a weak scenario reference instead of a potentially destroyed owner.
- Compiler-cache builds use MSVC without PCH. Explicit C++ dependencies and includes are required: QCursor for the pinned popup code, Qt Quick for app QML views, `muse::ui` for PaletteConfiguration, and QJsonArray in MuseSounds tools and score conversion. Do not restore accidental PCH dependencies to hide build errors.

## Release gate

`.github/workflows/build_evanscore_windows.yml` builds, deploys, and invokes `buildscripts/ci/windows/verify_evanscore_app.ps1` before uploading `EvanScore-Windows-x64`.

Required checks: x64 executable validation; score-to-PDF export; responsive empty workspace and current-format score with both default and software rendering; no splash or unexpected dialogs; sustained response checks; clean close. Review the resulting Windows score screenshot before sharing the artifact link. The release verifier rejects the diagnostic startup helper.

Use the `EvanScore-Windows-x64` artifact from the passing source build. Users must extract the entire ZIP and run `bin/MuseScoreStudio5.exe`. Never present `EvanScore-Windows-diagnostic-only`, symbols, logs, source archives, or the keypad plugin ZIP as the full app.

## Diagnosing CI

The workspace proxy cannot reliably follow Azure's signed GitHub artifact/log redirects. GitHub API metadata and check annotations work. Use `.github/workflows/diagnose_evanscore_windows_startup.yml` with `mode=inspect-logs`, `run_id=<native run>`, and `log_artifact=job:<native job>`; its runner downloads job logs via Octokit. Read the diagnostic check annotations for `checks/compiler-errors`. For runtime captures, inspect `EvanScore-Windows-build-checks`; the diagnostic workflow publishes a small screenshot in numbered base64 annotations. Reassemble without printing the encoded content and view the image.

Compiler caching is verified, but do not promise a build duration before a full build completes. Pushes on this branch can cancel an in-progress native build; avoid unrelated updates during compilation. Use `[skip ci]` for checkpoint-only commits when necessary.

## Completed feature scope and prior verification

The app includes the optional keypad plugin, persistent percussion mode/style defaults, native optional Dynamics sidebar, independent articulation velocities, visual hairpin playback curves, score XML persistence/undo, and dynamics presets/defaults. See `AGENTS.md`, [workspace.md](workspace.md), and [dynamics.md](dynamics.md) for exact behavior and design requirements.

Prior-session checks passed: 21 native dynamics tests; 13 close-project tests; 53 open-project tests and 5 startup tests after the promotion fix; actual keypad/Dynamics/toolbar QML checks with host API/model stubs; affected native source syntax checks without PCH. These establish component behavior, not successful execution of the new complete Windows app. Virtual Drumline playback has not been interactively verified.

The reconnected cloud retained `/workspace`, the local SDK/build, and `/workspace/evanscore-env.sh`. Earlier `/tmp` test binaries and preview captures were lost; do not assume they remain available. Current dependency-probe logs are in `/tmp/evanscore-qml-dependency-check` and may also disappear on a future reconnect.

## Work after the download is verified

The user authorized further UI polish, useful quality-of-life improvements, and optimization after finishing current build/startup work. Preserve the priority order. Audit the existing UI/keypad/dynamics against the supplied Logic-inspired references, retain native music fonts/glyphs, improve spacing/rounding/contrast and keyboard access, and verify controls/resizing/persistence on Windows. Avoid changes that needlessly restart a pending release build.

The mallet visualizer remains in planning. Do not claim it ships. The entire 36-page uploaded research synthesis and the original demonstration timeline were reviewed. [mallet-visualizer-plan.md](mallet-visualizer-plan.md) records the optional Mixer-like bottom dock, range-correct keyboard, mandatory overhead body/arms/mallets, front view, sticking, diagnostics, alternatives, audition/compare, and explicit undoable commit. [mallet-research-sources.md](mallet-research-sources.md) records citations, corrections, and primary sources still needing verification. Visual panning and expanded library-specific technique mapping are separate feature phases. Battery remains the user's main focus.
