# Dynamics sidebar

Open **View > Dynamics**. The panel starts with **Full score** settings. Choose **Use custom playback dynamics** to use the profile; closing the panel leaves playback settings active. Turning the checkbox off restores the original playback system without deleting the profile.

Each dynamic marking keeps **Tap, Tenuto, Accent, and Marcato** visible. **More articulations** expands **Ghost, Normal, Soft accent, Stress, and Unstress** in that card. Collapsing preserves every value. Values range from **0 to 127**; zero is silent. Search by marking to find ppp, mp, ff, sfz, fp, or another supported dynamic. Battery taps default to piano levels, while explicitly quiet markings use their own level.

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

## One velocity for an articulation across all dynamics

In Full score → Dynamic mappings, choose an articulation (Tap, Accent, Tenuto, etc.), enter a MIDI velocity, and click **Set all**. For example, Tap → 10 sets all tap mappings to 10 while preserving the accent and other articulation columns. Editing an individual mapping prefills this control with that category and value. The batch operation enables custom dynamics, is one undoable change, and persists through normal score saves, preset export, and Make Default Dynamics. It does not replace local note overrides or link future individual edits.

## Smooth through articulations

Enable **Smooth through articulations** for one curve between the chosen hairpin endpoints, even as printed notes change from accents to tenutos to taps. It is off initially. Full score sets the preference; a selected hairpin can override it, and **Use score defaults** restores inheritance. Explicit note overrides remain in effect. A ghost mapped to zero remains silent. Presets include the preference; older presets retain the original independent articulation lanes.

Legacy imported marking velocities, such as p=64, no longer supersede a custom battery Tap mapping. Explicit note edits and the panel's precise marking override still take priority. Hairpins can still vary the actual note velocity while the Tap mapping is constant.

## Marching snare starting profile

New battery scores use this profile if you have not saved your own default. Use **Marching Snare Defaults** to apply it explicitly to an existing score, with one Undo. Opening an existing score keeps its settings. **Make Default Dynamics** saves your preferred adjustments for future scores.

| Marking | Ordinary stroke | Tap | Tenuto | Accent / Marcato | Ghost |
| --- | ---: | ---: | ---: | ---: | ---: |
| pp | 45 | 45 | 60 | 60 | 0 |
| p | 60 | 60 | 64 | 64 | 0 |
| mp | 72 | 60 | 72 | 72 | 0 |
| mf | 84 | 60 | 72 | 84 | 0 |
| f | 100 | 60 | 84 | 100 | 0 |
| ff | 114 | 60 | 100 | 114 | 0 |
| fff | 126 | 60 | 114 | 126 | 0 |

The user's stroke reference is pp=1 inch, p=3, mp=6, mf=9, f=12, ff=15, fff=18; pp accents/tenutos=3, p accents/tenutos=4, and p+ taps=3. Ghosts in battery writing mean no stroke. These velocities are an editable starting calibration with a raised quiet end, not a universal inches-to-velocity formula or verified Virtual Drumline preset. Sound patches have different velocity layers and responses; audition and adjust with the actual library. Additional quieter/louder markings remain available. Soft accent follows Tenuto, Stress follows Accent, and Unstress follows Tap in this starting profile.

Research checked on 2026-10-09: [Vic Firth Marching Percussion 101](https://ae.vicfirth.com/education/marching-percussion-101/) explicitly teaches two-height control as a fundamental battery skill. [Tapspace Virtual Drumline 2.5](https://www.tapspace.com/virtual-drumline/) documents multi-sampled velocity layers, attack/release/EQ controls, automatic hand alternation, and loading through Kontakt Player. These support separate stroke roles and library-specific calibration, but do not specify a universal MIDI velocity for each physical height. The linked VDL user guide remains inaccessible from this environment until its support hostname is enabled.
