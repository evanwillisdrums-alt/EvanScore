# Mallet workflow audit — 2026-10-16

The user reports inconsistent chord selection and sparse explanations, primarily when choosing a suggested chord. The audit follows a first-time writer through selecting one score note, understanding the full chord, choosing a card, comparing it, applying it, Undo/Redo, picking temporary notes and changing settings. The existing IMG_0531 reference was reviewed again; the native host theme, illustrated overhead player and optional bottom dock remain the baseline.

## Findings and changes

| Workflow | Finding | Change and regression coverage |
| --- | --- | --- |
| Choose a suggestion | Only the small pitch-name button handled clicks; most of the card looked selectable but did nothing. | The entire card previews its chord. Native release checks click each card's descriptive body and check the actual displayed pitches and unchanged score. |
| Keep a suggestion selected | Previewing republished/recreated the whole list along with selection state. | A separate notification updates the list only when its contents change. Preview/Original update selection without destroying cards. The model test checks all choices, source identity and zero list rebuilds. The earlier isolated scroll test did not reproduce a scroll jump; no claim that every reported failure had this cause. |
| Keep the original | A comfortable suggested original could not write its sticking: Apply required choosing an alternative. | Apply accepts the displayed original when it adds missing sticking. Real native Undo/Redo tests retain pitches and restore/remove exactly one marking. An additional Windows fixture has no written sticking. |
| Change score chord quickly | A pending selection refresh could leave an old preview visible; a rejected stale Apply did not refresh it. | Preview, Apply and audition verify the source immediately. A stale action refreshes the chord and explains that a new suggestion must be chosen. |
| Remove a picked note | The head's drag hit area swallowed clicks on occupied bars and sometimes neighboring bars in a compact view. | Click-to-toggle and movement-to-drag are distinct. Nine real painter/window tests cover occupied heads, nearby bars, dragging, repeated clicks, score mode and double-click reset. |
| Pick another note | Automatic focusing could move the next target as the chord changed. | Picking retains camera bounds until an explicit Fit/Focus. Every pitch in the 61-bar overview is clickable. |
| Edit a preview on the bars | Edits used the underlying original chord even when a different suggestion was displayed. | The displayed preview becomes the temporary chord before the clicked pitch is toggled. No score insertion is implied. |
| Change picked notes or suggestions | Written-sticking text leaked from the score into picking; adding a lower note shifted manually placed strike points to the wrong voice index. | Picking clears written constraints; points follow pitch across insert/remove/reorder and remain protected in existing alternatives. Fixed alternatives are reevaluated without a new search/reordering during a drag. |
| Understand a warning | Numerical summaries lacked their configured limits and next action. Empty/unsupported contexts could show generic messages and zero measurements. | Inline instructions, chord/preview headings, explicit Apply reasons and optional per-hand measurements/limits with adjustment advice. Inner/outer hand names accompany mallet numbers. |
| Audition/compare | A stale displayed chord could be sent to a newly selected staff. A comparison could outlive a change in context. | Tests inspect the actual playback-controller pitch/staff request, reject stale requests, explain unavailable audio and cancel the second audition when closed. No licensed-audio output is claimed. |
| Use a narrow dock | A long single toolbar could crowd its actions. | Source and action groups wrap into two rows. The actual panel QML check covers a 700×220 dock, visible nonoverlapping actions, card-body clicks, numeric Enter and Ctrl+S. Host theme/model substitutes are used locally. |

## Validation boundaries

The isolated workflow runner uses the actual Qt engraving library, native model, solver and notation Undo stack, with mocks for score selection, global context, font/config providers and the playback controller. It includes 40 mixed cycles of suggestions, original, picking and reselection. Its address-instrumented dependencies run with leak reporting disabled because the process retains global mock services; this is not a clean leak-sanitizer result.

The painter tests use the actual QQuickWindow and MalletSceneView with real pointer events, not a simulated solver-only API. The panel QML runner substitutes host UI components and cannot establish complete Windows behavior. See the latest checkpoint for executed counts and native run IDs.

The complete repository Linux test target configures but fails in unrelated pinned-framework `FuzzyFilter::QHash::try_emplace`: the retained local SDK is Qt 6.8.2, while the project/Windows build uses 6.11.2. Do not patch upstream behavior to hide this mismatch or call that target passed. The focused model/painter/QML tests run independently; the matching Windows app remains the release gate.

Native release checks now require real suggestion-card clicks on both written and unwritten chords, original assignment Apply/Undo/Redo, repeated panel reopening, compact geometry, and saving the exact selected preview. Persistence validation compares the saved notes and per-staff sticking with that captured preview; it does not assume every valid revoicing changes exactly one note. Source voice identity, staves, rhythm and pitch classes remain asserted.

This audit improves interaction and explanation. It does not calibrate a performer's body, prove continuous passage motion, or turn the estimated Stevens model into a universal playability rule.
