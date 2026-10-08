# Mallet visualizer — reference and planning notes

Status: planning only. This document records the user's requirements; it does not mean the visualizer has been implemented or included in the current Windows build.

## References and layout

Use the complete IMG_0523.mov demonstration, not only its keyboard picture, as the baseline for functionality, interactions, and layout. A local copy is available under `/workspace/attachments/7f299adc-758f-4afd-99e3-4c6741241e95/contents/`. IMG_4982.MOV documents the app's existing lower-panel workflow. The user's Drive reference folder is https://drive.google.com/drive/folders/1a_PGAZ15IwrUa89S9JJEmPJ9-fajtOUI . Previously saved references are available locally; a live Drive connector is not available in this session.

The visualizer belongs in the optional bottom dock group alongside Mixer, Piano Keyboard, Timeline, and Percussion. It opens and closes on demand. Use the user's sketch: a large instrument/player area on the left, with changes, diagnostics, and suggestions on the right. Preserve score space above it and allow resizing the panel and its internal split. Match the host's connected, rounded, restrained Logic-inspired theme.

Native integration is the current recommendation. The checked-out legacy extension loader converts both `pluginType: "dialog"` and `pluginType: "dock"` to a Form, and the extension provider opens forms through the extension viewer dialog. The legacy docking properties therefore do not deliver the requested native Mixer-style panel by themselves.

## Player and instrument appearance

The latest supplied illustration establishes the rendering direction: a clean illustrated overhead player with soft shading, body, shoulders, arms, hands, hair, and four mallets. The body and arms are mandatory and must show reach and crossovers. Also retain the previously requested front view. Both views must depict the same instrument, mallet assignments, and pose.

Randomize natural skin tone and hair color each time the panel opens, then keep the appearance stable during that session. This is cosmetic and must not alter reach calculations, player dimensions, note assignments, or saved score content. Keep diagnostic and mallet highlights readable against every player appearance; the avatar must not cover controls, note labels, or strike-point feedback.

Instrument geometry follows the selected instrument's actual range. Default marimba is five octaves, C2–C7 inclusive: 61 bars, with 36 naturals and 25 accidentals. Use graduated bar lengths and widths, correct groups of two and three accidentals, and correct relative upper-row placement and overhang. The supplied McCormick's diagram is a layout reference; no particular manufacturer is required. The portable metal-bar instrument photo is another instrument-layout reference. Do not use piano-key geometry for mallet bars. Physical measurements in centimeters need a calibrated geometry profile or an explicit estimate label.

## Functions visible in the original demonstration

The sampled video shows:

- Synchronization with a score selection; a separate mode to pick notes on the bars; Clear.
- Highlighted strike bars, four individually identified mallet heads, shafts, hands, and an articulated player pose, including crossovers.
- A compatibility/constraint status and per-mallet assignment feedback.
- Diagnostics, Alternatives, and Player tabs in the information area.
- Instrument-range and note-count checks.
- Left and right hand openings, hand rotation, strike points, mallet head/shaft clearance, left and right arm reach, outer spread, and body position checks. Some lower descriptions are less legible in the recording; do not invent their exact thresholds.
- Alternative chord placements with pitch lists and explanatory comparisons, including changed hand opening, register changes, and whether the bass or top note is retained.
- Audition, Compare, and Commit controls, including committing a chosen alternative back to the score.

These visible functions are part of the requested baseline. A tab being visible does not establish every control hidden inside it; unknown details should be verified rather than presented as already understood.

## Additional confirmed requirements

Read written sticking and use it when assigning mallets. Show why a placement is awkward under the configured grip, player dimensions, and technique assumptions; distinguish difficulty from impossibility. Suggest improvements both within the same voicing and through alternative positions/voicings, making any pitch change explicit. Provide subtle linked score diagnostics and contextual explanations grounded in mallet literature.

Previewing or comparing an alternative must not silently rewrite notation. Commit must be an explicit, undoable score edit. Keep durations, voices, articulation, and playback mappings intact unless the user deliberately changes them. Closing the panel must stop its ongoing visualization work without affecting score playback or stored settings.

Battery remains the user's main overall focus. The special emphasis on keyboard percussion applies to this particular visualizer, not to the application's entire feature roadmap.

## Supplied research bibliography

The user supplied a cross-chat reference titled “Research mallet placement patterns,” conversation ID `6ac69b45-3ae8-83ea-9f10-d634eba01aba`. Its cached preview is null, and this session has no `read_thread` tool. In response to a request for its contents, the user pasted 38 citation entries and an extensive scanned-source list. This provides a bibliography and snippets, not the report's conclusions or complete source texts. The curated source leads and verification status are recorded in [mallet-research-sources.md](mallet-research-sources.md).

The user subsequently uploaded **Keyboard Percussion and Multi-Mallet Placement: A Deep Research Synthesis.pdf**, a 36-page ChatGPT Deep Research report. Its complete extracted text was reviewed. The report provides context for the planned grip-aware, register-aware, passage-aware analysis, stroke families, musical voice preservation, body repositioning, and instrument-specific articulation. The report itself distinguishes its proposed chord defaults and comparative rankings from experimentally established universal rules.

Direct retrieval of PAS, Yamaha, PMC, and Nancy Zeltsman pages returned the environment proxy's HTTP 403 tunnel rejection. No primary-source full text was reviewed through those requests. The supplied synthesis can inform the design; exact rules still need verified support and calibration. See the research-source record for necessary corrections before translating report diagrams into software.
