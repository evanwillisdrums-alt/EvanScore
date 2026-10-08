# Dynamics sidebar

Open **View > Dynamics**. The panel starts with **Full score** settings. Choose **Use custom playback dynamics** to use the profile; closing the panel leaves playback settings active. Turning the checkbox off restores the original playback system without deleting the profile.

Each dynamic marking has a compact card with independent **Tap, Tenuto, Accent, Marcato, Ghost, Normal, Soft accent, Stress, and Unstress** velocities. Values range from **0 to 127**; zero is silent. Search by marking to find ppp, mp, ff, sfz, fp, or another supported dynamic. Battery taps default to piano levels, while explicitly quiet markings use their own level.

Playback recognizes the score's articulations, including combined accent/staccato and marcato/tenuto symbols. A combined accent/tenuto uses the accent mapping; marcato takes precedence over accent, and playback-disabled articulations are ignored. Ghost dynamics follow the note's native ghost flag. Changing an open/filled notehead alone leaves duration and velocity unchanged. Staccato and staccatissimo retain native duration behavior.

## Edit selected music

Select notes, a dynamic marking, or a crescendo/decrescendo to reveal its local controls. **Full score** returns to the profile. An unsupported selection does not edit the whole score accidentally.

- Notes: exact velocity, silence/play, add/subtract, scale, or a percentage offset that follows score dynamics. Category filters restrict changes within the selection.
- Dynamic marking: override its playback level while keeping its printed text.
- Hairpin: drag the curve, fine-tune with the slider or arrow keys, or choose Linear, Early, Late, S-curve, or Delayed. Drag either endpoint or choose its marking, category, and exact velocity. For example, an ff-accent-to-mp-tap decrescendo resolves to the configured **mp tap** value. Dashed tap and blue endpoint previews show the two paths on battery staves. A roll's strokes follow their own positions along the curve.

**Use score defaults** removes local dynamics overrides. Edits are undoable, and mappings, curves, endpoint settings, and note changes save with the score. Curve edits affect playback, leaving the printed hairpin unchanged.

## Save and reuse a profile

**Save Preset** writes a readable JSON-based `.evands` file. **Load Preset** validates the whole file before applying one undoable change. Profiles include all marking/category velocities, the custom-playback switch, battery separation, and the default curve shape and bend. They preserve score engraving styles and local note/hairpin overrides.

**Make Default Dynamics** saves the current profile in the application's user data and remembers it across restarts. It applies only when creating a new score, including one from a template. **Apply Default Dynamics** explicitly applies the saved profile to an existing score. Opening existing scores does not replace their dynamics.

These values are playback instructions; different sound libraries have different loudness responses. The core settings do not configure patch-specific Virtual Drumline techniques or sound mappings. Continuous audio automation is per voice, so simultaneous notes within one voice share its gain curve; their note velocities remain individually editable.
