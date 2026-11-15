# Writing and mixing tools

These additions are source-complete and locally tested. A matching full Windows
build and desktop interaction checks are required before calling them released.

## Find and command search

Use Edit → Find / commands (the normal Find shortcut, Ctrl+F on Windows).
Type `>` and an action name, such as `> transpose`, or click the mode button
and type the name without `>`. Ordinary Find / Go to remains available.

Up/Down selects among up to eight results. Enter or a pointer click invokes the
selected action; Escape closes the window and returns notation focus. Unavailable
actions are disabled. No-match Enter does nothing. Clearing the search does not
clear the score selection. Search itself does not dispatch commands or modify
notation. Each invocation rechecks current availability and registration.

This searches registered UI actions by translated title, description and action
code. It is not an AI, a Dorico-compatible shorthand language, or every command
requiring typed arguments. There is no automatic score editing as you type.

## Mixer views

The mixer header offers Mix, Sounds, Effects and All. Narrow headers use a view
picker rather than compressing button labels. Existing individual View-menu
choices remain; manual combinations are shown as a custom view.

- Mix: faders/meters, pan, numerical volume, mute/solo and channel names.
- Sounds: sound selectors plus numerical volume, mute/solo and names.
- Effects: inserts and sends plus numerical volume, mute/solo and names. Bypass
  and selector controls remain visible; supported insert names open their editors.
- All: every section and send.

These choices persist through the existing mixer-visibility settings. They do
not change sound sources, effect parameters, automation or instrument identity.
Changing views no longer requests resizing of the dock. Resize its divider as
needed; opening uses the compact panel default. Hidden faders still release meter
subscriptions. Navigation connections now have one managed lifetime per channel,
including model resets, instead of adding anonymous connections repeatedly.

This is the first mixer workflow overhaul. Channel FX reordering, reusable effect
chains and the larger spatial-placement roadmap remain separate work.

## Percussion and text

Customize Kit searches the current kit by name, MIDI pitch and shortcut. Audition
or double-click previews through the staff's current sound. Turn off Mapped sounds
only to inspect all128 keys; unmapped keys can be silent. This does not search
inside arbitrary Kontakt libraries, index local audio files or assign a different
plugin to an individual drum key.

Properties → Noteheads → Staff position moves selected percussion notes while
retaining MIDI pitch. Reset restores their drum-map positions. The role is printed
placement, separate from choosing a sample.

Styles → Text styles → Arial for score text applies ordinary Arial families to an
existing score and removes family overrides while retaining bold/italics/sizes and
embedded music symbols. Cancel/Undo remain available. For new scores, default text
families also apply after bundled template/percussion styles. A user-saved default
style takes priority. Opening an existing score does not restyle it.

## Validation and remaining limits

24 focused tests cover existing percussion behavior, split layout/name placement,
font defaults, written-position Undo, command search/dispatch/destruction and mixer
view policies. Palette/EditStyle/new-score/model syntax checks and the desktop
Inspector compile locally. Native Windows checks are extended to actual view
buttons, retained dock height, search typing/mode switching/result clicks/Escape,
fonts, split layout, keyboard editing and kit search/preview. Their matching
Windows run and visual review are still required. No licensed-library audio or
universal crash/memory claim follows from focused tests.
