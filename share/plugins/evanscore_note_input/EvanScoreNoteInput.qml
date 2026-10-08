// SPDX-License-Identifier: GPL-3.0-only
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import MuseScore 3.0
import MuseScore.NotationScene
import Muse.Ui
import "KeypadLayout.js" as Layouts

MuseScore {
    id: root
    title: "EvanScore Note Input"
    description: "A customizable translucent keypad using native notation symbols."
    version: "1.4"
    categoryCode: "composing-arranging-tools"
    thumbnailName: ""
    pluginType: ""
    requiresScore: true
    onRun: {
        loadPreferences();
        voicesFilter.load();
        syncSelection();
        noteWindow.show();
    }
    onScoreStateChanged: Qt.callLater(syncSelection)

    property url musicFontSource: "qrc:/fonts/leland/Leland.otf"
    property url fallbackFontSource: "qrc:/fonts/bravura/Bravura.otf"
    property url iconFontSource: "qrc:/ui/data/MusescoreIcon.ttf"
    property var elementTypes: typeof Element !== "undefined" ? Element : ({})
    property var noteHeadTypes: typeof NoteHeadType !== "undefined" ? NoteHeadType : ({})
    property var symbolTypes: typeof SymId !== "undefined" ? SymId : ({})
    property var tremoloTypes: typeof TremoloType !== "undefined" ? TremoloType : ({})
    property bool openNoteheads: false
    property int activePage: 0
    property int activeVoice: 0
    property string activeDuration: ""
    property string activeTremolo: ""
    property string notice: ""
    property bool customizeMode: false
    property bool preferencesLoaded: false
    property bool changingPage: false
    property var layoutOverrides: ({})
    property string editingSlot: ""
    readonly property color accentColor: typeof ui !== "undefined" ? ui.theme.accentColor : "#008edb"
    readonly property var pages: Layouts.defaults()
    readonly property var currentPage: pages[activePage]
    readonly property var catalogById: Layouts.index(toolCatalog)
    readonly property var currentKeys: Layouts.effective(currentPage, layoutOverrides, catalogById)
    readonly property var tabIcons: [IconCode.NOTE_QUARTER, IconCode.ACCIACCATURA, IconCode.BEAM_JOIN, IconCode.ACCENT, IconCode.SHARP]
    readonly property var toolCatalog: [
        { id: "whole", label: "Whole note", action: "pad-note-1", category: "Durations", icon: IconCode.NOTE_WHOLE, shortcut: "7" },
        { id: "half", label: "Half note", action: "pad-note-2", category: "Durations", icon: IconCode.NOTE_HALF, shortcut: "6" },
        { id: "quarter", label: "Quarter note", action: "pad-note-4", category: "Durations", icon: IconCode.NOTE_QUARTER, shortcut: "5" },
        { id: "eighth", label: "Eighth note", action: "pad-note-8", category: "Durations", icon: IconCode.NOTE_8TH, shortcut: "4" },
        { id: "sixteenth", label: "16th note", action: "pad-note-16", category: "Durations", icon: IconCode.NOTE_16TH, shortcut: "3" },
        { id: "thirtysecond", label: "32nd note", action: "pad-note-32", category: "Durations", icon: IconCode.NOTE_32ND, shortcut: "2" },
        { id: "sixtyfourth", label: "64th note", action: "pad-note-64", category: "Durations", icon: IconCode.NOTE_64TH, shortcut: "1" },
        { id: "breve", label: "Double whole note", action: "note-breve", category: "Durations", icon: IconCode.NOTE_WHOLE_DOUBLE },
        { id: "dot", label: "Dot", action: "pad-dot", category: "Durations", icon: IconCode.NOTE_DOTTED },
        { id: "dot2", label: "Double dot", action: "pad-dot2", category: "Durations", icon: IconCode.NOTE_DOTTED_2 },
        { id: "dot3", label: "Triple dot", action: "pad-dot3", category: "Durations", icon: IconCode.NOTE_DOTTED_3 },
        { id: "rest", label: "Rest", action: "pad-rest", category: "Durations", icon: IconCode.REST },
        { id: "tie", label: "Tie", action: "tie", category: "Durations", icon: IconCode.NOTE_TIE },
        { id: "slur", label: "Slur", action: "add-slur", category: "Durations", icon: IconCode.NOTE_SLUR },
        { id: "noteinput", label: "Note input", action: "note-input", category: "Tools", icon: IconCode.DURATION_CURSOR },
        { id: "openheads", label: "Open / automatic noteheads", action: "open-noteheads", category: "Tools", icon: IconCode.NOTE_HEAD_HALF },
        { id: "natural", label: "Natural", action: "nat", category: "Pitch", icon: IconCode.NATURAL },
        { id: "sharp", label: "Sharp", action: "sharp", category: "Pitch", icon: IconCode.SHARP },
        { id: "flat", label: "Flat", action: "flat", category: "Pitch", icon: IconCode.FLAT },
        { id: "flat2", label: "Double flat", action: "flat2", category: "Pitch", icon: IconCode.FLAT_DOUBLE },
        { id: "sharp2", label: "Double sharp", action: "sharp2", category: "Pitch", icon: IconCode.SHARP_DOUBLE },
        { id: "flip", label: "Flip stem / direction", action: "flip", category: "Pitch", icon: IconCode.NOTE_FLIP },
        { id: "pitchup", label: "Raise pitch", action: "pitch-up", category: "Pitch", icon: IconCode.ARROW_UP },
        { id: "pitchdown", label: "Lower pitch", action: "pitch-down", category: "Pitch", icon: IconCode.ARROW_DOWN },
        { id: "octaveup", label: "Raise octave", action: "pitch-up-octave", category: "Pitch", icon: IconCode.SMALL_ARROW_UP, badge: "8" },
        { id: "octavedown", label: "Lower octave", action: "pitch-down-octave", category: "Pitch", icon: IconCode.SMALL_ARROW_DOWN, badge: "8" },
        { id: "accent", label: "Accent", action: "add-sforzato", category: "Articulations", icon: IconCode.ACCENT },
        { id: "tenuto", label: "Tenuto", action: "add-tenuto", category: "Articulations", icon: IconCode.TENUTO },
        { id: "marcato", label: "Marcato", action: "add-marcato", category: "Articulations", icon: IconCode.MARCATO },
        { id: "staccato", label: "Staccato", action: "add-staccato", category: "Articulations", icon: IconCode.STACCATO },
        { id: "fermata", label: "Fermata", action: "add-fermata", category: "Articulations", icon: IconCode.FERMATA },
        { id: "staccatissimo", label: "Staccatissimo", action: "symbol:articStaccatissimoAbove", category: "Articulations", symbolName: "articStaccatissimoAbove", glyphs: [{"symbol": "\ue4a6", "x": 0, "y": 0, "factor": 1, "fallback": false}], bounds: [0, 0, 127, 248] },
        { id: "accenttenuto", label: "Accent + tenuto", action: "symbol:articTenutoAccentAbove", category: "Articulations", symbolName: "articTenutoAccentAbove", glyphs: [{"symbol": "\ue4b4", "x": 0, "y": 0, "factor": 1, "fallback": false}], bounds: [0, 0, 361, 358] },
        { id: "accentstaccato", label: "Accent + staccato", action: "symbol:articAccentStaccatoAbove", category: "Articulations", symbolName: "articAccentStaccatoAbove", glyphs: [{"symbol": "\ue4b0", "x": 0, "y": 0, "factor": 1, "fallback": false}], bounds: [0, 0, 361, 344] },
        { id: "softaccent", label: "Soft accent", action: "symbol:articSoftAccentAbove", category: "Articulations", symbolName: "articSoftAccentAbove", glyphs: [{"symbol": "\ued40", "x": 0, "y": 0, "factor": 1, "fallback": true}], bounds: [0, 1, 708, 245] },
        { id: "stress", label: "Stress", action: "symbol:articStressAbove", category: "Articulations", symbolName: "articStressAbove", glyphs: [{"symbol": "\ue4b6", "x": 0, "y": 0, "factor": 1, "fallback": false}], bounds: [0, 0, 212, 212] },
        { id: "unstress", label: "Unstress", action: "symbol:articUnstressAbove", category: "Articulations", symbolName: "articUnstressAbove", glyphs: [{"symbol": "\ue4b8", "x": 0, "y": 0, "factor": 1, "fallback": false}], bounds: [0, -162, 342, 0] },
        { id: "flam", label: "Flam / slashed grace note", action: "acciaccatura", category: "Grace & percussion", icon: IconCode.ACCIACCATURA, grace: true },
        { id: "appoggiatura", label: "Appoggiatura", action: "appoggiatura", category: "Grace & percussion", icon: IconCode.APPOGGIATURA, grace: true },
        { id: "grace4", label: "Quarter grace note", action: "grace4", category: "Grace & percussion", icon: IconCode.NOTE_QUARTER, grace: true },
        { id: "grace16", label: "16th grace note", action: "grace16", category: "Grace & percussion", icon: IconCode.NOTE_16TH, grace: true },
        { id: "grace32", label: "32nd grace note", action: "grace32", category: "Grace & percussion", icon: IconCode.NOTE_32ND, grace: true },
        { id: "grace8after", label: "Eighth grace note after", action: "grace8after", category: "Grace & percussion", icon: IconCode.NOTE_8TH, grace: true, after: true },
        { id: "grace16after", label: "16th grace note after", action: "grace16after", category: "Grace & percussion", icon: IconCode.NOTE_16TH, grace: true, after: true },
        { id: "grace32after", label: "32nd grace note after", action: "grace32after", category: "Grace & percussion", icon: IconCode.NOTE_32ND, grace: true, after: true },
        { id: "diddle", label: "Diddle / one-slash tremolo", action: "tremolo", category: "Grace & percussion", tremoloName: "R8", glyphs: [{"symbol": "\ue1d5", "x": 0, "y": 0, "factor": 1, "fallback": false}, {"symbol": "\ue220", "x": 309, "y": 390, "factor": 1, "fallback": false}], bounds: [0, -131, 454, 875] },
        { id: "tremolo2", label: "Two-slash tremolo", action: "tremolo", category: "Grace & percussion", tremoloName: "R16", glyphs: [{"symbol": "\ue1d5", "x": 0, "y": 0, "factor": 1, "fallback": false}, {"symbol": "\ue221", "x": 309, "y": 390, "factor": 1, "fallback": false}], bounds: [0, -131, 454, 875] },
        { id: "roll", label: "Three-slash tremolo", action: "tremolo", category: "Grace & percussion", tremoloName: "R32", glyphs: [{"symbol": "\ue1d5", "x": 0, "y": 0, "factor": 1, "fallback": false}, {"symbol": "\ue222", "x": 309, "y": 390, "factor": 1, "fallback": false}], bounds: [0, -131, 454, 875] },
        { id: "tremolo4", label: "Four-slash tremolo", action: "tremolo", category: "Grace & percussion", tremoloName: "R64", glyphs: [{"symbol": "\ue1d5", "x": 0, "y": 0, "factor": 1, "fallback": false}, {"symbol": "\ue223", "x": 309, "y": 390, "factor": 1, "fallback": false}], bounds: [0, -131, 454, 875] },
        { id: "buzz", label: "Buzz roll", action: "tremolo", category: "Grace & percussion", tremoloName: "BUZZ_ROLL", glyphs: [{"symbol": "\ue1d5", "x": 0, "y": 0, "factor": 1, "fallback": false}, {"symbol": "\ue22a", "x": 309, "y": 390, "factor": 1, "fallback": false}], bounds: [0, -131, 459, 875] },
        { id: "beamauto", label: "Automatic beaming", action: "beam-auto", category: "Beaming", icon: IconCode.AUTO_TEXT },
        { id: "beamstart", label: "Start beam", action: "beam-break-left", category: "Beaming", icon: IconCode.BEAM_BREAK_LEFT },
        { id: "beamjoin", label: "Continue beam", action: "beam-join", category: "Beaming", icon: IconCode.BEAM_JOIN },
        { id: "beamnone", label: "Remove beam", action: "beam-none", category: "Beaming", icon: IconCode.BEAM_NONE },
        { id: "beambreak8", label: "Break secondary beam (8th)", action: "beam-break-inner-8th", category: "Beaming", icon: IconCode.BEAM_BREAK_INNER_8TH },
        { id: "beambreak16", label: "Break secondary beam (16th)", action: "beam-break-inner-16th", category: "Beaming", icon: IconCode.BEAM_BREAK_INNER_16TH },
        { id: "delete", label: "Delete", action: "delete", category: "Tools", icon: IconCode.DELETE_TANK },
        { id: "undo", label: "Undo (Ctrl+Z)", action: "command://notation/undo", category: "Tools", icon: IconCode.UNDO },
        { id: "redo", label: "Redo (Ctrl+Y)", action: "command://notation/redo", category: "Tools", icon: IconCode.REDO },
        { id: "escape", label: "Cancel note input / selection tool", action: "escape", category: "Tools", icon: IconCode.CLOSE_X_ROUNDED },
        { id: "empty", label: "Empty key", action: "", category: "Tools", icon: IconCode.NONE }
    ]

    FontLoader { id: musicFont; source: root.musicFontSource }
    FontLoader { id: fallbackFont; source: root.fallbackFontSource }
    FontLoader { id: iconFont; source: root.iconFontSource }
    Settings { id: preferences; category: "EvanScoreNoteInput" }
    VoicesSelectionFilterModel { id: voicesFilter; objectName: "keypad-voices-filter" }

    function showNotice(message) { notice = message; noticeTimer.restart(); }
    function defaultHeight(page) { return 230 + page.rows * 58 + page.groups.length * 18; }
    function validSize(value, fallback, minimum, maximum) {
        return typeof value === "number" && isFinite(value) ? Math.max(minimum, Math.min(maximum, value)) : fallback;
    }
    function loadPreferences() {
        try { layoutOverrides = Layouts.validate(JSON.parse(preferences.value("layoutsV2", "{}")), pages, catalogById); }
        catch (error) { layoutOverrides = ({}); }
        changingPage = true;
        noteWindow.width = validSize(preferences.value("widthV2", 320), 320, noteWindow.minimumWidth, 900);
        noteWindow.height = validSize(preferences.value("heightV2-" + currentPage.id, defaultHeight(currentPage)), defaultHeight(currentPage), noteWindow.minimumHeight, 1200);
        changingPage = false;
        preferencesLoaded = true;
    }
    function saveSize() {
        if (!preferencesLoaded || changingPage) return;
        preferences.setValue("widthV2", noteWindow.width);
        preferences.setValue("heightV2-" + currentPage.id, noteWindow.height);
        preferences.sync();
    }
    function setPage(index) {
        if (index === activePage) return;
        saveSize();
        changingPage = true;
        picker.close();
        activePage = index;
        noteWindow.height = validSize(preferences.value("heightV2-" + currentPage.id, defaultHeight(currentPage)), defaultHeight(currentPage), noteWindow.minimumHeight, 1200);
        changingPage = false;
    }
    function resetSize() {
        noteWindow.width = 320;
        noteWindow.height = defaultHeight(currentPage);
        saveSize();
    }
    function saveLayouts() { preferences.setValue("layoutsV2", JSON.stringify(layoutOverrides)); preferences.sync(); }
    function replaceKey(toolId) {
        layoutOverrides = Layouts.replace(currentPage, layoutOverrides, catalogById, editingSlot, toolId);
        saveLayouts(); picker.close();
    }
    function resetPage() {
        let updated = JSON.parse(JSON.stringify(layoutOverrides));
        delete updated[currentPage.id];
        layoutOverrides = updated;
        saveLayouts();
    }
    function beginCustomize(slot) { editingSlot = slot; picker.open(); }
    function isChecked(spec) {
        return spec.action === activeDuration || (spec.action === "open-noteheads" && openNoteheads)
            || (!!spec.tremoloName && spec.tremoloName === activeTremolo);
    }
    function selectedChords() {
        let chords = [];
        for (const note of selectedNotes()) {
            const chord = note.parent;
            if (!chords.some(function(existing) { return typeof existing.is === "function" ? existing.is(chord) : existing === chord; })) chords.push(chord);
        }
        return chords;
    }
    function toggleTremolo(name) {
        const chords = selectedChords();
        const type = tremoloTypes[name];
        if (!chords.length || type === undefined || elementTypes.TREMOLO_SINGLECHORD === undefined) return false;
        if (chords.some(function(chord) { return chord.tremoloTwoChord; })) {
            showNotice("Remove the two-note tremolo before adding a single-note roll.");
            return true;
        }
        const remove = chords.every(function(chord) { return chord.tremoloSingleChord && chord.tremoloSingleChord.tremoloType === type; });
        curScore.startCmd();
        try {
            for (const chord of chords) {
                if (remove) removeElement(chord.tremoloSingleChord);
                else if (chord.tremoloSingleChord) chord.tremoloSingleChord.tremoloType = type;
                else {
                    const tremolo = newElement(elementTypes.TREMOLO_SINGLECHORD);
                    tremolo.track = chord.track;
                    tremolo.tremoloType = type;
                    chord.add(tremolo);
                }
            }
        } finally { curScore.endCmd(); }
        Qt.callLater(syncSelection);
        return true;
    }
    function toggleSymbol(name) {
        const chords = selectedChords();
        const symbol = symbolTypes[name];
        if (!chords.length || symbol === undefined) return false;
        function matching(chord) { return Array.from(chord.articulations).filter(function(a) { return a.symbol === symbol; }); }
        const remove = chords.every(function(chord) { return matching(chord).length > 0; });
        curScore.startCmd();
        try {
            for (const chord of chords) {
                const existing = matching(chord);
                if (remove) { for (const a of existing) removeElement(a); }
                else if (!existing.length) {
                    const articulation = newElement(elementTypes.ARTICULATION);
                    articulation.track = chord.track;
                    articulation.symbol = symbol;
                    chord.add(articulation);
                }
            }
        } finally { curScore.endCmd(); }
        return true;
    }
    function selectedNotes() {
        let notes = [];
        if (!curScore || !curScore.selection)
            return notes;
        function append(note) {
            if (!notes.some(function(existing) { return typeof existing.is === "function" ? existing.is(note) : existing === note; }))
                notes.push(note);
        }
        for (let item of curScore.selection.elements) {
            if (item.type === root.elementTypes.NOTE)
                append(item);
            else if (item.type === root.elementTypes.CHORD)
                for (let note of item.notes)
                    append(note);
        }
        return notes;
    }

    function toggleOpenNoteheads() {
        let notes = selectedNotes();
        if (!notes.length || root.noteHeadTypes.HEAD_HALF === undefined || root.noteHeadTypes.HEAD_AUTO === undefined)
            return false;
        let allOpen = notes.every(function(note) { return note.headType === root.noteHeadTypes.HEAD_HALF; });
        let type = allOpen ? root.noteHeadTypes.HEAD_AUTO : root.noteHeadTypes.HEAD_HALF;
        curScore.startCmd();
        try {
            for (let note of notes)
                note.headType = type;
        } finally {
            curScore.endCmd();
        }
        openNoteheads = !allOpen;
        return true;
    }

    function syncSelection() {
        activeDuration = "";
        activeTremolo = "";
        let notes = selectedNotes();
        openNoteheads = root.noteHeadTypes.HEAD_HALF !== undefined && notes.length > 0 && notes.every(function(note) { return note.headType === root.noteHeadTypes.HEAD_HALF; });
        if (notes.length && notes.every(function(note) { return note.parent.tremoloSingleChord && note.parent.tremoloSingleChord.tremoloType === notes[0].parent.tremoloSingleChord.tremoloType; })) {
            const type = notes[0].parent.tremoloSingleChord.tremoloType;
            for (const name of ["R8", "R16", "R32", "R64", "BUZZ_ROLL"]) if (root.tremoloTypes[name] === type) activeTremolo = name;
        }
        if (!curScore || !curScore.selection)
            return;
        let selected = curScore.selection.elements;
        if (!selected.length)
            return;
        let item = selected[0];
        if (item.type === root.elementTypes.NOTE)
            item = item.parent;
        if (!item)
            return;
        if (item.track >= 0)
            activeVoice = item.track % 4;
        if (item.duration) {
            let ratio = item.duration.numerator / item.duration.denominator;
            let lengths = [1, 2, 4, 8, 16, 32, 64];
            for (let n of lengths)
                for (let dots = 0; dots <= 3; ++dots)
                    if (Math.abs(ratio - (2 - Math.pow(0.5, dots)) / n) < 0.000001)
                        activeDuration = "pad-note-" + n;
        }
    }
    function addFermata() {
        if (!curScore || !curScore.selection)
            return false;
        let items = curScore.selection.elements;
        let targets = [];
        let seen = {};
        for (let item of items) {
            let cr = item.type === root.elementTypes.NOTE ? item.parent : item;
            if (!cr || (cr.type !== root.elementTypes.CHORD && cr.type !== root.elementTypes.REST))
                continue;
            let segment = cr.parent;
            let key = segment.tick + ":" + cr.track;
            if (seen[key])
                continue;
            seen[key] = true;
            let existing = [];
            for (let annotation of segment.annotations)
                if (annotation.type === root.elementTypes.FERMATA && annotation.track === cr.track)
                    existing.push(annotation);
            targets.push({ tick: segment.tick, track: cr.track, existing: existing });
        }
        if (!targets.length)
            return false;
        let remove = targets.every(function(target) { return target.existing.length > 0; });
        curScore.startCmd();
        try {
            let cursor = curScore.newCursor();
            for (let target of targets) {
                if (remove) {
                    for (let existing of target.existing)
                        removeElement(existing);
                    continue;
                }
                if (target.existing.length)
                    continue;
                cursor.track = target.track;
                cursor.rewindToTick(target.tick);
                let fermata = newElement(root.elementTypes.FERMATA);
                fermata.symbol = root.symbolTypes.fermataAbove;
                cursor.add(fermata);
            }
        } finally {
            curScore.endCmd();
        }
        return true;
    }
    function activate(spec) {
        if (!spec.action) return;
        if (spec.action === "open-noteheads") {
            if (!toggleOpenNoteheads()) showNotice("Select notes to change their noteheads.");
        } else if (spec.tremoloName) {
            if (!toggleTremolo(spec.tremoloName)) showNotice("Select notes before adding a roll or diddle.");
        } else if (spec.symbolName) {
            if (!toggleSymbol(spec.symbolName)) showNotice("Select notes before adding an articulation.");
        } else if (spec.action === "add-fermata") {
            if (!addFermata()) showNotice("Select a note or rest first.");
        } else {
            if (spec.action.indexOf("pad-note-") === 0) activeDuration = spec.action;
            cmd(spec.action);
        }
    }
    Timer { id: noticeTimer; interval: 3000; onTriggered: root.notice = "" }
    Timer { id: sizeTimer; interval: 350; onTriggered: root.saveSize() }

    Window {
        id: noteWindow
        objectName: "evanscore-keypad"
        width: 320
        height: root.defaultHeight(root.currentPage)
        minimumWidth: 300
        minimumHeight: 214 + root.currentPage.rows * 36 + root.currentPage.groups.length * 18 + (root.customizeMode ? 28 : 0)
        maximumWidth: 900
        maximumHeight: 1200
        color: "transparent"
        title: "EvanScore Keypad"
        flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
        onWidthChanged: if (root.preferencesLoaded && !root.changingPage) sizeTimer.restart()
        onHeightChanged: if (root.preferencesLoaded && !root.changingPage) sizeTimer.restart()
        onClosing: { root.saveSize(); quit(); }

        Rectangle {
            anchors.fill: parent
            radius: 16
            border.color: "#777e858e"
            border.width: 1
            gradient: Gradient {
                GradientStop { position: 0; color: "#c52b2e32" }
                GradientStop { position: 1; color: "#bb34373b" }
            }
        }
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 7
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 30; Layout.maximumHeight: 30
                MouseArea {
                    anchors.fill: parent
                    onPressed: noteWindow.startSystemMove()
                    onDoubleClicked: root.resetSize()
                }
                KeyButton {
                    width: 30; height: 28
                    spec: ({label: "Customize this keypad", icon: IconCode.CONFIGURE})
                    checked: root.customizeMode
                    onClicked: { root.customizeMode = !root.customizeMode; picker.close(); }
                    objectName: "keypad-customize"
                }
                Text { anchors.centerIn: parent; text: "Keypad"; color: "#f2f3f5"; font.pixelSize: 14; font.weight: Font.DemiBold }
                Row {
                    anchors.right: parent.right
                    spacing: 3
                    KeyButton {
                        width: 28; height: 28
                        spec: ({label: "Minimize", icon: IconCode.APP_MINIMIZE})
                        onClicked: noteWindow.showMinimized()
                    }
                    KeyButton {
                        width: 28; height: 28
                        spec: ({label: "Close keypad", icon: IconCode.CLOSE_X_ROUNDED})
                        onClicked: noteWindow.close()
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 38; Layout.maximumHeight: 38
                spacing: 6
                Repeater {
                    model: ["openheads", "delete", "undo", "redo"]
                    KeyButton {
                        required property string modelData
                        objectName: "keypad-" + modelData
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spec: root.catalogById[modelData]
                        checked: root.isChecked(spec)
                        onClicked: root.activate(spec)
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 32; Layout.maximumHeight: 32
                spacing: 3
                Repeater {
                    model: root.pages
                    KeyButton {
                        required property var modelData
                        required property int index
                        objectName: "keypad-tab-" + index
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spec: ({label: modelData.label, icon: root.tabIcons[index]})
                        checked: root.activePage === index
                        onClicked: root.setPage(index)
                    }
                }
            }
            RowLayout {
                visible: root.customizeMode
                Layout.fillWidth: true
                Layout.preferredHeight: 24; Layout.maximumHeight: 24
                Text { text: "Choose a key to replace"; color: "#e2e6ec"; font.pixelSize: 11; Layout.fillWidth: true }
                Button {
                    text: "Reset tab"
                    flat: true
                    font.pixelSize: 11
                    onClicked: root.resetPage()
                    contentItem: Text { text: parent.text; color: "#ffffff"; font: parent.font }
                    background: Rectangle { radius: 6; color: parent.hovered ? "#665c626c" : "transparent" }
                    ToolTip.visible: hovered
                    ToolTip.text: "Restore the factory buttons on this tab"
                }
            }
            Item {
                id: keysGrid
                objectName: "keypad-grid"
                Layout.fillWidth: true
                Layout.fillHeight: true
                readonly property real columnWidth: (width - 18) / 4
                readonly property real rowHeight: (height - root.currentPage.groups.length * 18) / root.currentPage.rows
                function groupOffset(row) { return root.currentPage.groups.filter(function(g) { return g.row <= row; }).length * 18; }
                Repeater {
                    model: root.currentPage.groups
                    Text {
                        required property var modelData
                        x: 2; y: modelData.row * keysGrid.rowHeight + keysGrid.groupOffset(modelData.row) - 18
                        text: modelData.label
                        color: "#bfc7d1"
                        font.pixelSize: 10
                        height: 16
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                Repeater {
                    model: root.currentKeys
                    KeyButton {
                        required property var modelData
                        objectName: "keypad-key-" + modelData.toolId
                        spec: modelData
                        x: modelData.col * (keysGrid.columnWidth + 6)
                        y: modelData.row * keysGrid.rowHeight + keysGrid.groupOffset(modelData.row)
                        width: keysGrid.columnWidth * modelData.colSpan + 6 * (modelData.colSpan - 1)
                        height: keysGrid.rowHeight * modelData.rowSpan - 6
                        visible: modelData.toolId !== "empty" || root.customizeMode
                        checked: root.isChecked(spec)
                        onClicked: root.customizeMode ? root.beginCustomize(modelData.id) : root.activate(spec)
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 32; Layout.maximumHeight: 32
                spacing: 5
                Repeater {
                    model: ["1", "2", "3", "4", "All"]
                    KeyButton {
                        required property string modelData
                        required property int index
                        Layout.fillWidth: true; Layout.fillHeight: true
                        spec: ({ label: index < 4 ? "Voice " + modelData : "Select all voices", text: modelData })
                        checked: index < 4 && root.activeVoice === index
                        onClicked: {
                            if (index < 4) { root.activeVoice = index; root.cmd("voice-" + modelData); }
                            else voicesFilter.selectAll()
                        }
                        Rectangle {
                            anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter
                            height: 2; width: parent.width - 18; radius: 1
                            color: ["#80aaff", "#78ca94", "#f2948c", "#d5a1ec", "#aab2bf"][index]
                        }
                    }
                }
            }
            Item {
                Layout.fillWidth: true; Layout.preferredHeight: 19; Layout.maximumHeight: 19
                Text {
                    anchors.left: parent.left; anchors.right: resizeHandle.left; anchors.verticalCenter: parent.verticalCenter
                    text: root.notice || (root.customizeMode ? "Layouts are saved automatically" : root.currentPage.label)
                    color: root.notice ? "#f2ddba" : "#abb4bf"
                    font.pixelSize: 10; elide: Text.ElideRight
                }
                KeyButton {
                    id: resizeHandle
                    objectName: "keypad-resize"
                    anchors.right: parent.right; width: 24; height: 19
                    spec: ({label: "Drag to resize; double-click to reset", icon: IconCode.SPLIT_OUT_ARROWS})
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.SizeFDiagCursor
                        property point start
                        property size original
                        property bool systemResize: false
                        onPressed: function(mouse) {
                            start = mapToGlobal(mouse.x, mouse.y);
                            original = Qt.size(noteWindow.width, noteWindow.height);
                            systemResize = noteWindow.startSystemResize(Qt.RightEdge | Qt.BottomEdge);
                        }
                        onPositionChanged: function(mouse) {
                            if (!pressed || systemResize) return;
                            const current = mapToGlobal(mouse.x, mouse.y);
                            noteWindow.width = root.validSize(original.width + current.x - start.x, original.width, noteWindow.minimumWidth, noteWindow.maximumWidth);
                            noteWindow.height = root.validSize(original.height + current.y - start.y, original.height, noteWindow.minimumHeight, noteWindow.maximumHeight);
                        }
                        onDoubleClicked: root.resetSize()
                    }
                }
            }
        }
        Shortcut { sequence: "Ctrl+Z"; enabled: noteWindow.active && !picker.visible; onActivated: root.cmd("command://notation/undo") }
        Shortcut { sequences: ["Ctrl+Y", "Ctrl+Shift+Z"]; enabled: noteWindow.active && !picker.visible; onActivated: root.cmd("command://notation/redo") }
        Shortcut {
            sequence: "Escape"; enabled: noteWindow.active
            onActivated: {
                if (picker.visible) picker.close();
                else if (root.customizeMode) root.customizeMode = false;
                else root.cmd("escape");
            }
        }
        Repeater {
            model: root.currentKeys
            Item {
                id: shortcutDelegate
                required property var modelData
                Shortcut {
                    sequence: shortcutDelegate.modelData.shortcut || ""
                    enabled: noteWindow.active && !root.customizeMode && !picker.visible && !!shortcutDelegate.modelData.shortcut
                    onActivated: root.activate(shortcutDelegate.modelData)
                }
            }
        }
        Popup {
            id: picker
            objectName: "keypad-picker"
            parent: noteWindow.contentItem
            x: 12; y: 46
            width: noteWindow.width - 24
            height: Math.min(500, noteWindow.height - 64)
            modal: true; focus: true
            padding: 12
            onOpened: { search.text = ""; search.forceActiveFocus(); }
            background: Rectangle { color: "#f22b2e33"; radius: 12; border.color: "#777d8591" }
            ColumnLayout {
                anchors.fill: parent; spacing: 8
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Replace key"; color: "#f2f4f7"; font.pixelSize: 14; font.weight: Font.DemiBold; Layout.fillWidth: true }
                    KeyButton { width: 26; height: 26; spec: ({label: "Close",icon:IconCode.CLOSE_X_ROUNDED}); onClicked: picker.close() }
                }
                TextField {
                    id: search
                    objectName: "keypad-search"
                    Layout.fillWidth: true
                    placeholderText: "Search symbols and actions"
                    color: "#ffffff"; placeholderTextColor: "#adb6c2"; font.pixelSize: 12
                    background: Rectangle { radius: 7; color: "#363b43"; border.color: search.activeFocus ? root.accentColor : "#646d79" }
                }
                ScrollView {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    contentWidth: availableWidth
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    Column {
                        width: parent.width
                        spacing: 5
                        Repeater {
                            model: root.toolCatalog.filter(function(t) { return (t.label + " " + t.category).toLowerCase().indexOf(search.text.toLowerCase()) >= 0; })
                            RowLayout {
                                required property var modelData
                                width: parent.width
                                height: 40
                                KeyButton { Layout.preferredWidth: 42; Layout.fillHeight: true; spec: modelData; onClicked: root.replaceKey(modelData.id) }
                                Button {
                                    Layout.fillWidth: true; Layout.fillHeight: true
                                    text: modelData.label; focusPolicy: Qt.NoFocus
                                    onClicked: root.replaceKey(modelData.id)
                                    background: Rectangle { radius: 6; color: parent.hovered ? "#536070" : "#30343b" }
                                    contentItem: Text { text: parent.text; color: "#f4f6fa"; font.pixelSize: 11; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter }
                                }
                            }
                        }
                    }
                }
                Text { Layout.fillWidth: true; text: "Existing buttons swap places. Each tab saves independently."; color: "#b7c0cd"; font.pixelSize: 10; wrapMode: Text.WordWrap }
            }
        }
    }
    component KeyButton: Button {
        id: key
        property var spec: ({})
        focusPolicy: Qt.NoFocus
        hoverEnabled: true
        padding: 0
        Accessible.name: spec.label || "Empty key"
        ToolTip.visible: hovered
        ToolTip.text: (spec.label || "Empty key") + (spec.shortcut ? " (" + spec.shortcut + ")" : "")
        ToolTip.delay: 550
        background: Rectangle {
            radius: Math.min(10, key.height / 4)
            border.color: key.checked ? "#809dd3f8" : (key.hovered ? "#88949fad" : "#606d737c")
            border.width: 1
            gradient: Gradient {
                GradientStop { position: 0; color: key.checked ? root.accentColor : (key.down ? "#aa5a6470" : key.hovered ? "#995b6470" : "#80535a65") }
                GradientStop { position: 1; color: key.checked ? Qt.darker(root.accentColor,1.18) : "#70464c55" }
            }
        }
        contentItem: Item {
            Text {
                anchors.centerIn: parent
                visible: !key.spec.glyphs
                text: key.spec.text || String.fromCharCode(key.spec.icon || IconCode.NONE)
                font.family: key.spec.text ? "Arial" : iconFont.name
                font.pixelSize: key.spec.text ? 18 : Math.min(key.height - 12, key.width - 14, 38) * (key.spec.grace ? 0.78 : 1)
                font.weight: key.spec.text ? Font.DemiBold : Font.Normal
                color: "#f8faff"
                renderType: Text.NativeRendering
            }
            Item {
                id: ink
                anchors.fill: parent
                visible: !!key.spec.glyphs
                readonly property var bounds: key.spec.bounds || [0,0,1,1]
                readonly property real scale: Math.min(0.045, Math.max(0,height - 14) / (bounds[3]-bounds[1]), Math.max(0,width-14) / (bounds[2]-bounds[0]))
                Repeater {
                    model: key.spec.glyphs || []
                    Text {
                        required property var modelData
                        text: modelData.symbol
                        font.family: modelData.fallback ? fallbackFont.name : musicFont.name
                        font.pixelSize: 1000 * ink.scale
                        x: (ink.width - (ink.bounds[0]+ink.bounds[2])*ink.scale)/2 + modelData.x * ink.scale
                        y: (ink.height + (ink.bounds[1]+ink.bounds[3])*ink.scale)/2 - modelData.y * ink.scale - baselineOffset
                        color: "#f8faff"
                        renderType: Text.NativeRendering
                    }
                }
            }
            Text { anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 5; text: key.spec.badge || (key.spec.after ? "→" : ""); color: "#d1d9e4"; font.pixelSize: 10 }
        }
    }
}
