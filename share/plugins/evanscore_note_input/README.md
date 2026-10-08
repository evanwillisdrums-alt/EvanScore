# EvanScore floating note input

Enable **EvanScore Note Input** through Extensions > Manage plugins, then run it from the Extensions menu. It opens only on request as a draggable, nonmodal QML plugin window. The score remains available while it is open.

The dark translucent keypad follows the supplied IMG_0532.jpeg reference: a separate open-notehead button, delete/undo/redo, five category tabs, a four-column notation grid with a wide rest key and tall tie key, and the voice row. Windows minimize and close buttons are in the top right. Restore a minimized keypad from the Windows taskbar. Closing it stops this plugin, not the application.

The top-left open notehead button changes selected notes to open heads without changing duration, pitch, velocity, or playback. Clicking again restores duration-based automatic heads. This is separate from the half-note duration key in the main grid.

The category buttons expose note values, grace notes and percussion commands (flam/diddle/roll), beam controls, articulations, and accidentals. Hover for names; the keys show musical symbols. Buttons dispatch the app's native commands. Undo/redo use the modern notation command URIs and Ctrl+Z / Ctrl+Y / Ctrl+Shift+Z work while the keypad has focus. Fermatas use the score plugin API in one undoable operation, and clicking again removes them when all selected targets already have a fermata. Diddle, two-, three-, and four-slash tremolos and buzz rolls use native chord objects in one undoable operation, with correct track assignment. Clicking again removes the matching tremolo; choosing another changes it. Two-note tremolos are protected from accidental replacement. Select notes before adding flam/diddle/roll or fermatas.

Voice 1–4 selects the native input voice. **All** enables all four voices in MuseScore's selection filter for range selections; it does not enter the same note in four voices.

The existing MusescoreIcon font supplies interface, duration, tie, and beaming icons. Leland supplies composed tremolo symbols, with Bravura as the native fallback for uncommon articulation glyphs. Score music fonts are unchanged. The panel and inactive keys have translucent backgrounds; notation glyphs stay opaque and legible. Selected keys reuse the app's blue accent.

The keypad's number shortcuts work while the keypad has focus. Global keyboard shortcuts and mouse side-button assignments are configured in Preferences > Shortcuts.

For manual installation, put this whole folder in the user plugins folder configured in Preferences. To update an extracted EvanScore app, replace its `plugins/evanscore_note_input` folder and restart. Updating this plugin does not rebuild or update native percussion-strip and toolbar changes; those require the new full app download.

Version 1.4 groups durations, pitch, and articulations on the main tab, with flam, diddle, and buzz immediately available. The percussion tab includes one-, two-, and three-slash tremolos and buzz. Beam start/continue/break buttons use the native visual symbols. The Notes tab no longer changes noteheads.

Click the gear, then a key, to search and choose a replacement. Choosing a tool already on the current tab swaps the two buttons. Tabs save independently and persist across restarts. “Reset tab” restores that tab only; choosing “Empty key” hides a slot outside customization. All supported tools, including 64th/breve/triple dots and four-slash rolls, remain available in the picker.

Drag the lower-right resize handle; double-click it or the title bar to restore the default size. Width is shared, while each tab remembers its own height. Resizing keeps every key visible without scrolling. Keypad opacity is slightly reduced while icons remain opaque.
