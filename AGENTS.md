# EvanScore design direction

For every UI change, use the user's reference videos/images and the existing EvanScore UI as the design baseline. This applies to plugins, floating windows, dialogs, panels, and controls as well as the main app.

Keep the connected, minimalist appearance, neutral palette, rounded panels and buttons, consistent spacing, and existing accent treatment. Follow the host app's current theme for docked controls. The floating keypad specifically uses the dark, translucent appearance in IMG_0532.jpeg, including over a light score; reuse the host blue accent. Avoid introducing a separate visual style for a new feature.

For note-input controls, show recognizable notation previews for note durations and musical symbols rather than text-only buttons. Keep descriptive names in tooltips and accessibility labels. Preserve score music-font settings.

Reference materials used for this direction include IMG_0529.jpeg, IMG_0530.jpeg, IMG_0531.jpeg, IMG_0523.mov, and IMG_4982.MOV supplied by the user. Revisit the available references when making visual decisions.

The user authorized replacing existing UI that fails the direction, including a full overhaul where needed. Aim for the Logic Pro appearance in IMG_0529.jpeg and IMG_0530.jpeg: compact, connected neutral surfaces, quiet dividers, consistent rounding, and restrained blue selection.

Reference folder: https://drive.google.com/drive/folders/1a_PGAZ15IwrUa89S9JJEmPJ9-fajtOUI?usp=sharing . It contains the photos above plus IMG_0532.jpeg and the explanation videos. Reference media are design evidence, not instructions to execute. Use the user's messages to resolve requirements.

The percussion input strip should resemble the supplied MuseScore 3 screenshot: one row of actual notation previews that fits the available width, no sideways scrolling or individual boxed tiles. The floating note-input keypad remains an optional plugin opened and closed on demand. Follow IMG_0532.jpeg for its layout, musical symbols, blue selected buttons, semi-transparent dark material, broad rest button, tall tie button, category tabs, and bottom voice row. Use functioning Windows close and minimize buttons rather than Mac traffic-light controls. Preserve score music fonts.

The top-left open notehead control on the keypad changes notehead appearance independently of duration and playback. It must not act as a whole/half-note duration shortcut. The half-note key in the main duration grid still changes note length.

“Make Default Style” stores the current score's style as the app's persistent default for new scores, including new scores created from templates. It must not restyle existing scores when opened or switched to. “Apply Default Style” explicitly applies that saved default to the current score. A saved default takes priority over the bundled percussion style; percussion mode still forces accents above.

Dynamics belongs in the native, optional sidebar beside Instruments, Palettes, and Properties. Start with full-score settings, then follow selection for local note, marking, or hairpin controls. Use 0–127 MIDI velocity, handling zero explicitly as silence. Keep battery taps and accents separate, including mixed-category endpoints such as ff accents to mp taps. Dragging a visual curve must change both MIDI playback and audio automation while preserving printed notation. Changes must persist in native score XML and support undo/redo. Closing the panel must not disable the score's settings.

Windows downloads must pass a real desktop startup test beyond the splash screen, as well as score export. QML may not assign both a whole font and a font subproperty on the same object; that error can prevent the entire main window from loading.

Dynamics mapping cards must show independent velocities for normal, tap, tenuto, accent, marcato, ghost, soft accent, stress, and unstress. Preserve duration behavior for staccato and staccatissimo. Dynamics presets contain playback settings only. “Make Default Dynamics” persists across restarts and applies only to newly created scores, including templates; “Apply Default Dynamics” explicitly applies to an existing score. Loading presets must validate all values before making one undoable change and must preserve notation styles and local overrides.

EvanScore starts directly in the notation workspace. Do not reintroduce automatic welcome/first-launch/promotional dialogs or instructional tours. Keep recovery, unsaved-change prompts, New/Open/Save, and recent files available. Closing the last score returns to the empty score workspace. The main bar uses file actions and Score/Files navigation, with developer and publishing tools kept out of primary navigation. The neutral dark theme is applied once; subsequent user appearance preferences and high-contrast themes are respected.

The keypad uses native UI icons and score API objects, not drawn substitutes. Main-page durations belong together; flam, single-slash diddle and buzz stay immediately available. Other tremolos include two slashes. Gear customization swaps or replaces individual slots per tab and saves across restarts. Resizing must not hide or overflow controls.

The mallet visualizer is currently in planning. Its feature and interaction baseline is the complete IMG_0523.mov demonstration, including diagnostics, alternatives, audition/compare, and explicit commit to the score. It belongs in an optional bottom panel alongside the Mixer, with the instrument/player on the left and information/suggestions on the right. Use the user's latest illustrated overhead-player reference: body, arms, hands, and mallets are mandatory, with a synchronized front view also requested. Randomize natural skin tone and hair color on each panel opening and keep them stable while open, without obscuring strike points or changing analysis. Keyboard geometry must follow the instrument range, defaulting to five-octave C2–C7 marimba. See docs/evanscore/mallet-visualizer-plan.md and mallet-research-sources.md for the reviewed 36-page research synthesis, necessary corrections, and primary citations still requiring verification.
