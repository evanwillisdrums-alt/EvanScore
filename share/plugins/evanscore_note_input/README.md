# EvanScore floating note input

Enable **EvanScore Note Input** through Extensions > Manage plugins, then run it from the Extensions menu. It opens only on request as a draggable, nonmodal QML plugin window. The score remains available while it is open.

The dark translucent keypad follows the supplied IMG_0532.jpeg reference: delete/undo/redo, five category tabs, a four-column notation grid with a wide rest key and tall tie key, and the voice row. Windows minimize and close buttons are in the top right. Restore a minimized keypad from the Windows taskbar. Closing it stops this plugin, not the application.

The top-left open notehead button changes selected notes to open heads without changing duration, pitch, velocity, or playback. Clicking again restores duration-based automatic heads. This is separate from the half-note duration key in the main grid.

The category buttons expose note values, grace notes and percussion commands (flam/diddle/roll), beam controls, articulations, and accidentals. Hover for names; the keys show musical symbols. Buttons dispatch MuseScore's native commands. Fermatas use the score plugin API in one undoable operation, and clicking again removes them when all selected targets already have a fermata. Diddle and roll require the updated EvanScore app. Select notes before adding flam/diddle/roll or fermatas.

Voice 1–4 selects the native input voice. **All** enables all four voices in MuseScore's selection filter for range selections; it does not enter the same note in four voices.

Leland supplies the keypad's UI symbols only. Score music fonts are unchanged. The panel and inactive keys have translucent backgrounds; notation glyphs stay opaque and legible. Selected keys reuse the app's blue accent.

The keypad's number shortcuts work while the keypad has focus. Global keyboard shortcuts and mouse side-button assignments are configured in Preferences > Shortcuts.

For manual installation, put this whole folder in the user plugins folder configured in Preferences. To update an extracted EvanScore app, replace its `plugins/evanscore_note_input` folder and restart. Updating this plugin does not rebuild or update native percussion-strip and toolbar changes; those require the new full app download.
