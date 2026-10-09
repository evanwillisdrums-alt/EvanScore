# Virtual Drumline setup

Virtual Drumline has two separately licensed editions. The MuseHub edition is a MuseSounds library. Tapspace's VDL 2.5.5 library uses Kontakt/Kontakt Player. The existing Kontakt library files are not a MuseHub installation and cannot be loaded by MuseSampler.

Tapspace confirms the edition distinction and separate pricing in [its crossgrade guidance](https://support.tapspace.com/support/solutions/articles/26000058170-is-there-a-crossgrade-from-the-musesounds-library-versions-), reviewed 2026-10-09. [Native Instruments' Kontakt Player page](https://www.native-instruments.com/en/products/komplete/samplers/kontakt-8-player/) confirms that Player is free, has a Windows VST3 version, and needs internet access for download, installation, and activation. Actual VDL 2.5.5 compatibility must be checked before choosing a Kontakt version.

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

The user also requests automatic Muse Drumline-to-VDL conversion without remapping notes manually. Translate techniques, not raw pitch numbers, while preserving notation, rhythms, articulations, dynamics, and sticking. Conversion must be undoable and report unsupported or ambiguous sounds. Neither this converter nor the VDL-specific staff-text dictionary is implemented by the setup helper. Obtain the user's VDL 2.5.5 Documentation guide/key maps before coding the mappings; the online 2.5.6 support article currently exposes no downloadable guide content.

## Verification status

The source audit confirms the MuseHub integration path in `muse/framework/musesampler/internal/musesamplerresolver.cpp`, `src/playback/qml/MuseScore/Playback/inputresourceitem.cpp`, and `src/playback/internal/drumsetloader.cpp`. Existing Dynamics settings feed the host playback system; their numeric scale is not an independently verified VDL loudness calibration.

Neither licensed VDL edition is installed in the cloud workspace. VDL-specific sound loading, live audio, articulation behavior, dynamics, save/reopen, and instrument changes remain untested. The user's earlier Muse Drumline crash is not evidence that VDL fixes it. Complete playback verification needs the installed licensed library and a compatible runtime on Windows.
