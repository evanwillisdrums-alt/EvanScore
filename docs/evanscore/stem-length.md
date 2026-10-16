# Score-wide stem length

In **Format → Style → Notes**, set **Stem length (all notes)** below 100% to shorten stems across the score. Try 80% first. This control stays available when **Shorten stems** is off; that older option only adjusts stems in particular staff positions.

The scale adjusts free stems beyond the chord's outer note. Ordinary notes, ledger-line notes, grace notes, and connected beam groups use it. Beams move together while keeping their slope and attached stems. Space needed for flags, rolls and beams limits shortening; chord-spanning shafts remain intact. Manually positioned beams and beams between opposing cross-staff stems retain their positions.

100% restores ordinary engraving. The reset arrow restores the current style default. Cancel rolls back edits in the dialog; accepting supports score Undo/Redo. The setting saves with the score and exported style files. **Make Default Style** saves it for future scores; existing scores keep their saved style until **Apply Default Style** is used.

Five native engraving tests cover staff positions with conditional shortening off, both stem directions, grace/chord spans, connected-beam geometry, malformed scale values, Undo/Redo and native style XML. Complete Windows checks additionally type/click the real Styles control, measure every fixture stem, exercise reset/Cancel/Undo/Redo/Make Default, and inspect saved score/default XML. Component tests alone do not establish that the Windows package passed.
