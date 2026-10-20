# Percussion accents and staff defaults

Select the printed accent or marcato, then open **Properties**.

- **Placement → Auto / Above / Below** controls which side of the staff holds the marking. The percussion workspace defaults Auto accents/marcatos above unpitched percussion; a manual placement takes priority.
- **Symbol direction** controls the glyph's orientation. The percussion above default applies while placement and direction are both Auto; choose Above or Below to pin the staff side when using a manual glyph direction.
- **Attach accent to voice → 1 / 2 / 3 / 4** moves this marking to an existing chord at the same onset on the same staff. Notes, rests, durations, velocities and playback locks remain. One Undo restores the previous attachment and its linked counterparts.

A destination is disabled if it has no chord, already has that articulation, or cannot preserve a linked destination. The control applies to one selected accent or marcato on an ordinary chord. It does not invent a chord or move a grace articulation into the main attack. It is not a staff-wide playback voice assignment.

The automatic above default follows the instrument at the note's onset, including instrument changes. Pitched staves retain native articulation placement. An explicit score-wide articulation style remains a user choice; opening an existing score does not rewrite it. The bundled percussion style now uses the native Auto anchor so its percussion-only accent default can coexist with pitched staves in new mixed scores.

New **Marching Snare Drum** staves use three lines; new **Marching Cymbals** staves use two. Existing scores and an explicitly chosen staff type retain their lines and drum mappings.

**Diagnostics → EvanScore feature diagnostics** follows a selected articulation as well as notes/sticking. Chord rows include its written/effective anchor, symbol direction, rendered side, owning voice and available voice destinations, plus whether the instrument is unpitched percussion. Anchor codes are Above=0, Below=1, Auto=2; symbol directions are Auto=0, Up=1, Down=2. This is read-only notation state, not a measurement of a sound library's output.

Native regression cases cover percussion/pitched scope, manual overrides, instrument changes, one-step Undo/Redo, unchanged notes/velocities/locks, unavailable and duplicate destinations, linked staves, save/reopen, template defaults and explicit staff choices. Complete Windows pointer, persistence and startup checks remain release gates.
