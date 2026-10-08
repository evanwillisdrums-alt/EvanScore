# EvanScore floating note input

This is a QML plugin, not a native application window. It opens on demand as a draggable, nonmodal keypad, so the score remains available. Close it with its title-bar close button.

Enable **EvanScore Note Input** through Extensions > Manage plugins, then run it from the Extensions menu. The updated EvanScore build bundles it; for manual installation, put this folder in the user plugins folder configured in Preferences.

Buttons use MuseScore's note-input commands. Quarter/eighth/16th, half, rest, dot, accidentals, flam, accent, tie and marcato use existing commands. Diddle and roll use the new EvanScore commands and require the updated app. Select notes before applying flam, diddle or roll.

The keypad follows the app's light/dark theme and rounded control styling. Buttons show notation previews, including grace notes for flams and one/three tremolo slashes for diddles/rolls. Hover for descriptive labels and shortcuts. Leland supplies the keypad's UI symbols; score music fonts are unchanged.

To update an already downloaded app, replace `plugins/evanscore_note_input/EvanScoreNoteInput.qml` inside its extracted resources folder with this version, then restart the app. Updating this QML plugin does not require rebuilding the executable.

The keypad's number shortcuts work while the keypad has focus. Global keyboard shortcuts and mouse side-button assignments are configured in Preferences > Shortcuts.
