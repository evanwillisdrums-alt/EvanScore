# Virtual Drumline setup

Virtual Drumline has two separately licensed editions. The MuseHub edition is a MuseSounds library. Tapspace's VDL 2.5.5 library uses Kontakt/Kontakt Player. The existing Kontakt library files are not a MuseHub installation and cannot be loaded by MuseSampler.

Tapspace confirms the edition distinction and separate pricing in [its crossgrade guidance](https://support.tapspace.com/support/solutions/articles/26000058170-is-there-a-crossgrade-from-the-musesounds-library-versions-), reviewed 2026-10-09. [Native Instruments' Kontakt Player page](https://www.native-instruments.com/en/products/komplete/samplers/kontakt-8-player/) confirms that Player is free, has a Windows VST3 version, and needs internet access for download, installation, and activation. Actual VDL 2.5.5 compatibility must be checked before choosing a Kontakt version.

## Prepared Kontakt sounds in this build

The Mixer now saves and restores Kontakt component/controller state as reusable **Virtual Drumline · prepared sounds**. This changes the playback source while retaining the score instrument, original drumset and compatible sound flags. Prepared sounds do not automatically open Kontakt's editor. This is state restoration, not automatic NKI discovery or bundled VDL licensing.

1. On your Windows computer, use `Set up Virtual Drumline.cmd` from the ZIP's VDL helper, install Native Access, sign in and activate your VDL serial. Install a compatible Kontakt Player with its VST3 component and locate your VDL library. Account activation must happen on that computer.
2. Restart EvanScore. In Mixer → Sound, choose Kontakt and open its editor. Load **SnareLine Manual** or **SnareLine Manual LITE**; set its MIDI input to Omni (or the host's channel). Close the editor so Kontakt supplies its state.
3. In Sound, choose **Save current Kontakt patch…**. Enter a name and enable **SnareLine Manual / LITE translation** only for that exact patch. Save, then select the named prepared sound under **Virtual Drumline · prepared sounds**.
4. Save the score. Future sound selections restore the prepared state without reopening Kontakt; its editor remains available on demand.

The first reviewed playback map covers marching-snare hits, shots, rims and backsticks, using the independent SnareLine Manual diagram on page 29 of the supplied VDL 2.5 guide. Explicit single R/L assignments are retained, including repeated hands. Missing sticking uses a documented right-hand fallback; conflicting or unsupported multi-stroke assignments are skipped with a log warning. Staff-text center/halfway/edge sends explicit CC1 resets on every attack; snares/guts on/off and line/solo use the documented hit/shot registers. Measure-repeat regions copy technique changes. Unsupported sounds and variants are skipped rather than mapped to unrelated samples.

Other Kontakt patches can be prepared, but their original MIDI mapping is retained. Tenors, basses, keyboards, auxiliary patches, rolls/buzzes and full automatic library conversion still require reviewed maps. Snare rim/backstick off/solo variants are explicitly unsupported. Do not describe this first map as complete library support.

Validation: seven modified native source units passed syntax compilation; six guide-based routing cases passed ASan/UBSan, and 48 core dynamics/sticking/score-edit regressions passed. Binary profile round-trip and corruption checks are recorded in the development checkpoint after completion. Actual Kontakt patch restoration and licensed audio require Windows validation; the library is not installed in this cloud workspace. Feature diagnostics expose the selected plugin, prepared sound and playback-map status, without claiming measured sample output.

## MuseHub edition

1. Install MuseHub on the Windows computer running EvanScore and sign in to the account that owns the MuseHub edition of Virtual Drumline.
2. Install that library through MuseHub and finish any installation or activation steps there.
3. Restart EvanScore, open a battery score, and open the Mixer.
4. In the instrument channel's **Sound** selector, look under **MuseSounds** for the installed Tapspace/Virtual Drumline sounds. Vendor, pack, and category submenu names come from the installed library; do not assume an exact submenu hierarchy.
5. Choose the matching snare, tenor, or bass sound. Save the score to retain the sound selection. Check playback and the percussion input strip before continuing to write.

EvanScore already contains the native MuseSampler resource discovery, Mixer selection, and library-supplied drum-map loading used by this route. No separate EvanScore QML plugin is required for basic sound loading. The sampler runtime and paid samples are installed by MuseHub, rather than bundled with the EvanScore ZIP.

## Existing VDL 2.5.5 edition

Use a compatible Kontakt/Kontakt Player installation with VDL activated in the user's account. The Windows Kontakt VST3 plugin is the host integration route. Select it in the Mixer, open its native editor, and load the appropriate VDL instrument. Library-specific percussion maps and notation techniques need verification against that patch; selecting Kontakt alone does not configure them. Do not promise automatic notehead, technique, sticking, or keyswitch mappings without testing.

The optional Windows helper in `share/evanscore/vdl` opens the included setup page and an existing Native Access installation, or downloads the official installer and verifies its Native Instruments Authenticode signature before launching it. It does not install paid samples or automate account sign-in. CheckOnly discovers local file candidates without launching anything and explicitly leaves activation/playback unverified. DownloadOnly verifies retrieval/signature without launching the installer. The helper is installed in the `vdl` folder beside the app's `bin` folder in future Windows packages.

Tapspace [documents VDL 2.5 support in Kontakt 7](https://support.tapspace.com/support/solutions/articles/26000050384-using-virtual-drumline-2-5-in-kontakt-7). Its general installation guide recommends Kontakt 6 for widest compatibility across hosts, but that older VST2 plugin is not usable by EvanScore's VST3 host. A chosen Player must supply VST3. Newer Player compatibility still needs validation with the user's actual 2.5.5 files.

## Requested technique integration

The target workflow keeps Kontakt in the background after setup. Prepared VDL sounds should be selected in the existing Mixer sound menu with the same effort as selecting MS Basic or Muse Drumline. Keep Kontakt's native editor accessible for advanced editing, but do not open it automatically on each prepared sound change. Preserve the score's instrument identity and notation, translating library differences only in playback. Existing generic VST selection currently opens the editor, and switching away from MuseSampler can replace the drumset and remove sound flags; these paths require selective handling for VDL rather than a global change to existing library behavior. Restoring a prepared patch needs verified Kontakt component/controller state; this is not yet an automatic NKI-loading implementation.

The user confirmed they own only the Kontakt VDL 2.5.5 edition. Target the entire library, including battery, keyboard, concert, and auxiliary percussion, rather than assuming every patch is a marching snare. Native staff-text control should expose the selected patch's actual supported techniques and user-friendly aliases, including edge, guts on/off, line/solo, and other VDL-specific techniques where available. These require verified note/controller/keyswitch mappings and clear resets; a generic text label does not make Kontakt interpret a technique.

The user also requests automatic Muse Drumline-to-VDL conversion without remapping notes manually. Translate techniques, not raw pitch numbers, while preserving notation, rhythms, articulations, dynamics, and sticking. Conversion must be undoable and report unsupported or ambiguous sounds. The setup helper does not implement conversion; the first native SnareLine Manual map described above handles only its reviewed core techniques. The user's uploaded 116-page VDL 2.5 guide is now available. Verify each patch map against its diagrams before coding mappings; the online 2.5.6 support article itself exposes no downloadable guide content.

## Verification status

The source audit confirms the MuseHub integration path in `muse/framework/musesampler/internal/musesamplerresolver.cpp`, `src/playback/qml/MuseScore/Playback/inputresourceitem.cpp`, and `src/playback/internal/drumsetloader.cpp`. Existing Dynamics settings feed the host playback system; their numeric scale is not an independently verified VDL loudness calibration.

Neither licensed VDL edition is installed in the cloud workspace. VDL-specific sound loading, live audio, articulation behavior, dynamics, save/reopen, and instrument changes remain untested. The user's earlier Muse Drumline crash is not evidence that VDL fixes it. Complete playback verification needs the installed licensed library and a compatible runtime on Windows.

[Conversion validation](vdl-conversion-validation.md) records the requested Muse Drumline switching tests, independent guide expectations, notation preservation checks, and licensed Windows playback gate. The converter is not considered ready until those checks actually run and pass.

## Sticking and feature diagnostics

The new shared native sticking reader recognizes explicit R/r/L/l markings, preserves repeated hands, and retains mallet numbers 1–6 without guessing a grip convention. It isolates staff/voice/onset and reports missing, conflicting, or unsupported assignments. The optional Diagnostics → EvanScore feature diagnostics window exposes these assignments alongside notation/custom-dynamics velocities and overrides. It reports the current prepared profile and map; nonmapped sources retain the pending status. This reader does not by itself change Kontakt samples; patch-specific routing, sub-stroke sequencing, and licensed playback verification remain necessary. Every subsequently added mapping feature must expose its selected patch, input technique/sticking, output notes/controllers, and unsupported fallbacks here.
