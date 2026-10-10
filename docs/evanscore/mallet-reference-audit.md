# Marimba reference audit and next implementation requirements

Reviewed 2026-10-10. This is an evidence-based comparison with the current first native visualizer, not a claim that the gaps below have been implemented.

## What was reviewed

Revisited the complete 55.7-second `IMG_0523.mov` through a frame sequence covering its full duration, then inspected original-resolution close-ups of Diagnostics and Alternatives. Reviewed `IMG_0529.jpeg`, `IMG_0530.jpeg` and `IMG_0531.jpeg`; the last picture contains the visualizer toolbar, strike-zone legend, body controls and empty-state instructions. Source copies are in `/workspace/attachments/7f299adc-758f-4afd-99e3-4c6741241e95/contents/`. These are the user's previously supplied references, not a new retrieval from Drive. There is no callable Drive connector in this session, so newly added Drive files have not been checked.

The source-code comparison uses `MalletPanel.qml`, `malletpanelmodel.cpp`, `malletplacement.cpp` and `malletsceneview.cpp`. Media documents the requested behavior; it does not independently validate the friend's physical model or establish universal performer limits.

## Information and interactions that need closer matching

| Evidence | What the reference communicates | Current implementation gap and required improvement |
| --- | --- | --- |
| Video, roughly 9–15 seconds | Each physical mallet has a numbered/color-coded row giving its pitch name, hand and strike quality: **preferred**, **acceptable**, or **edge**. | Current rows show a hand and MIDI number. Show readable pitch names, physical IDs and the actual strike classification. Do not use ID colors as the only explanation. |
| `IMG_0531.jpeg` toolbar/legend | Separate **preferred**, **edge** and **node** strike zones. | The current renderer has bar selection highlights but no zone model/legend or node avoidance. Add instrument-appropriate, explicitly estimated/calibrated zones and show the chosen point against them. |
| Video, 9–15 seconds | Each hand's opening and rotation is a separate diagnostic. The example reports 19 cm/4 cm opening and 47°/45° rotation toward the rear row, with a displayed 40° comfort setting. | Current opening/rotation summaries omit directional explanations and individual passing checks. Show measured estimate, configured limit, severity and adjustment advice for each hand. The reference's 40° is an example setting, not a universal rule to copy blindly. |
| Video, 14 seconds | **Strike points** explains that mallets 3/4 strike near the edges of C6/C♯6 and that the tone is thinner than at the centre. | Dragging currently changes geometry without diagnosing this sound/placement tradeoff. Explain which mallet/note is affected and what moving the strike point changes. Instrument-specific advice must be verified. |
| Video, 14 seconds | **Head and shaft clearance** identifies that the hands cannot fit side by side even when angled outward. Range, note count and reach can all pass while this fails. | Current checks test head-target proximity and generic within-hand shaft reversal. They do not model hand footprints, shaft-segment clearance or wrist crowding. A green range/reach check must not mask an unresolved hand collision. |
| Video, 14 seconds | Left/right arm reach is reported individually (41/40 cm in this example), outer spread between mallets 1–4 is 23 cm, and body status is “Square to the instrument.” | Current reach can generate a warning but is not exposed as a complete measurement row; outer spread and body orientation are missing. Use one shared pose geometry for rendering and analysis. Current shoulder offsets differ between solver and painter (14 versus 21 model units); reconcile this before presenting their measurements as consistent. |
| Video, roughly 17–25 and 45–49 seconds | Each alternative explains opening **before → after**, bass/top-note preservation, and register change. One candidate widens the right hand from 4 cm to 10 cm and is called more comfortable. | Current cards say “Same pitches” or “Octave change” and show final openings. Include the specific tradeoffs and why the candidate improves this placement. Minimizing total hand opening alone is insufficient; a wider opening can resolve hand/shaft crowding. |
| Video, roughly 21–25 seconds | The selected alternative changes the overhead pose and overall status; A/B identifies the preview. An already comfortable chord is explicitly described as already comfortable, with other ways to voice the same harmony. | Current preview/audition/compare/explicit Commit are present, but suggestions can include the original or warning-bearing permutations with generic explanations. Do not manufacture a problem or label a merely different placement “better.” Mark useful equivalent choices and filter duplicates. |
| `IMG_0531.jpeg` Body controls and help | **Automatic**, **Turn**, **Step**, **Left elbow**, **Right elbow**; help describes dragging the body and double-clicking a mallet to reset its strike point. | Current body follows register and permits a horizontal offset. It has no turn/elbow controls, body dragging or double-click strike reset. Preserve automatic following and add purposeful manual adjustments without breaking the analysis/drawing agreement. |
| `IMG_0531.jpeg` toolbar | Instrument/range selector, **Designate staff**, Score selection and Pick on bars. Separate view tabs say Keyboard, Player and Performance. | Current source selection/picking exists and range comes from the selected instrument; explicit designated-staff and instrument-preview controls are missing. The picture does not reveal the contents of Player/Performance views, so their labels alone cannot establish their functionality. |
| Video and photo diagnostic footer | Stevens grip; opening, reach and timing limits are approximations adjustable to the performer. | Keep estimates qualified. Current previous-event movement heuristic has no elapsed-time/tempo analysis; it cannot establish transition feasibility. The recording does not demonstrate the hidden timing controls or a complete animated performance mode. |

The close-ups prove that this panel is intended to teach the user **what is wrong, why, and how a proposed change helps**. Matching its tab names and illustrated keyboard does not satisfy that requirement.

## Latest user corrections

- **Stevens**, not the dictated “speedings.” Draw fingers and distinct inner/outer shaft holding positions; do not start both shafts at the palm centre. Verified grip references and useful hand close-ups should guide the drawing and the model together.
- Remove the Front view from the visible mallet panel for now. Retain overhead body, arms, hands and mallets.
- Make the compact panel useful without shrinking the instrument/player into an unreadable thumbnail. A proposed focus/detail mode must keep the full-range context available and keep the hands/body understandable. Verify actual screenshots at minimum dock height, not only item dimensions.
- Place the body closer to the instrument where the modeled reach permits it. Investigate the reported blue mark on the left arm from actual captures rather than guessing its cause.
- Prevent toolbar/tab controls from overlapping or eliding to single letters. Spatial bounds and neighboring-control checks are required in addition to font-width checks.
- Add explanations for every suggested sticking. The ordinary, comfortably spaced C–E–G–upper-C Stevens case should prefer conventional physical IDs **1–2–3–4**. Do not recommend **1–2–4–3** by arbitrarily swapping the right-hand mallets. This is a concrete regression case, not a universal prohibition on all crossover techniques.

## Making the analysis more reliable

1. Parse a simultaneous onset and written sticking with stable physical IDs. Keep sustained notes distinct from new attacks and preserve the voices, durations and articulations.
2. Derive grip anchors, wrists, fingers, shaft segments, mallet heads, shoulders and body from a shared geometric representation. Rendering must show the same assumptions used by diagnostics.
3. Separate an out-of-range/impossible assignment from player-dependent comfort warnings and intentional advanced techniques. Preserve a problematic written assignment for diagnosis rather than silently changing it.
4. Rank candidates by physical validity, fidelity to musical intent, useful clearance/comfort and transition cost. Default advice should not prefer an unsupported within-hand swap. Evaluate possible crossovers explicitly instead of either allowing every permutation or banning every crossover.
5. Explain each alternative against the original: pitch-by-pitch mallet assignment, remaining warnings, opening/rotation changes, clearance changes, body movement, bass/top retention and any register changes. A comparison must acknowledge tradeoffs as well as improvements.
6. Use passage time/tempo before calling a transition feasible. If there is no verified timing model, say the transition has not been assessed.
7. Incorporate the user's forthcoming research with its sources, grip/numbering convention and known-good chord or passage examples. Do not treat a generated research summary or an attractive pose as a validated physical rule.

Acceptance cases include the ordinary C-major four-note example; a truly useful wider-hand alternative; an otherwise in-range/reachable chord with crowded hands; edge/node strike feedback; a legitimate documented crossover; explicit written sticking; sustained-note counting; and selection/preview/Commit/Undo/save/reopen. Release checks must cover native Windows controls, complete captions and compact rendering, not just solver output.

## Current use, before these improvements ship

Open **View → Mallet visualizer**, then select a keyboard-percussion chord. **Diagnostics** follows its onset and written sticking; **Player** changes the current estimated grip and comfort assumptions. **Pick on bars** allows a temporary keyboard experiment. In **Alternatives**, click a candidate to preview it, use **A** to restore the original preview, and use **Audition/Compare** with the selected score instrument. Only **Commit** writes the selected pitches and numbered sticking; **Undo** restores that edit. Picking bars does not commit new music. Current safety restrictions reject some tied/grace/arpeggiated/two-chord-tremolo edits.

The richer explanations, zone model, shared grip/clearance geometry, improved candidate ranking and newly requested compact rendering above are implementation work still to do. They must not be presented as features of the existing download.
