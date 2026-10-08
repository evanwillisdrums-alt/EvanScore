// SPDX-License-Identifier: GPL-3.0-only
.pragma library

function row(keys, index) {
    return keys.map(function(tool, col) {
        return { id: "r" + index + "c" + col, tool: tool, row: index, col: col, colSpan: 1, rowSpan: 1 };
    });
}
function page(id, label, rows, groups) {
    return { id: id, label: label, rows: rows.length, groups: groups,
             slots: rows.reduce(function(slots, keys, index) { return slots.concat(row(keys, index)); }, []) };
}
function defaults() {
    var notes = page("notes", "Notes", [
        ["whole", "half", "quarter", "eighth"],
        ["sixteenth", "thirtysecond", "dot", "tie"],
        ["rest", "empty", "dot2", "empty"],
        ["natural", "sharp", "flat", "flip"],
        ["accent", "tenuto", "marcato", "staccato"],
        ["flam", "diddle", "buzz", "noteinput"]
    ], [{row:0,label:"Durations"},{row:3,label:"Pitch"},{row:4,label:"Articulations & percussion"}]);
    notes.slots.find(function(s) { return s.tool === "rest"; }).colSpan = 2;
    notes.slots.find(function(s) { return s.tool === "tie"; }).rowSpan = 2;
    notes.slots = notes.slots.filter(function(s) { return s.tool !== "empty"; });
    return [notes,
        page("percussion", "Grace & percussion", [
            ["flam", "appoggiatura", "grace4", "grace16"],
            ["grace32", "grace8after", "grace16after", "grace32after"],
            ["diddle", "tremolo2", "roll", "buzz"],
            ["accent", "tenuto", "marcato", "fermata"]
        ], [{row:0,label:"Grace notes"},{row:2,label:"Rolls & strokes"},{row:3,label:"Articulations"}]),
        page("beaming", "Beaming", [
            ["beamauto", "beamstart", "beamjoin", "beamnone"],
            ["beambreak8", "beambreak16", "dot", "rest"],
            ["tie", "slur", "eighth", "sixteenth"]
        ], [{row:0,label:"Beam grouping"},{row:1,label:"Secondary beams"},{row:2,label:"Phrasing & duration"}]),
        page("symbols", "Articulations", [
            ["accent", "tenuto", "marcato", "staccato"],
            ["fermata", "staccatissimo", "accenttenuto", "accentstaccato"],
            ["softaccent", "stress", "unstress", "slur"],
            ["flam", "diddle", "buzz", "flip"]
        ], [{row:0,label:"Articulations"},{row:2,label:"Emphasis & phrasing"},{row:3,label:"Percussion & direction"}]),
        page("pitch", "Pitch & noteheads", [
            ["natural", "sharp", "flat", "flip"],
            ["flat2", "sharp2", "openheads", "noteinput"],
            ["pitchdown", "pitchup", "octavedown", "octaveup"]
        ], [{row:0,label:"Accidentals"},{row:1,label:"Noteheads & entry"},{row:2,label:"Pitch & octave"}])
    ];
}
function index(catalog) {
    var result = {};
    catalog.forEach(function(tool) { result[tool.id] = tool; });
    return result;
}
function effective(page, overrides, catalog) {
    var saved = overrides[page.id] || {};
    return page.slots.map(function(slot) {
        var tool = catalog[saved[slot.id]] || catalog[slot.tool];
        return Object.assign({}, tool, slot, {toolId: tool.id});
    });
}
function validate(raw, pages, catalog) {
    var result = {};
    if (!raw || typeof raw !== "object" || Array.isArray(raw)) return result;
    pages.forEach(function(page) {
        var saved = raw[page.id];
        if (!saved || typeof saved !== "object" || Array.isArray(saved)) return;
        var values = {};
        page.slots.forEach(function(slot) {
            if (typeof saved[slot.id] === "string" && catalog[saved[slot.id]]) values[slot.id] = saved[slot.id];
        });
        result[page.id] = values;
    });
    return result;
}
function replace(page, overrides, catalog, slotId, toolId) {
    if (!catalog[toolId] || !page.slots.some(function(slot) { return slot.id === slotId; })) return overrides;
    var result = JSON.parse(JSON.stringify(overrides));
    var keys = effective(page, overrides, catalog);
    var current = keys.find(function(key) { return key.id === slotId; });
    // effective() keeps slot IDs; tool identity is available through toolId.
    var duplicate = keys.find(function(key) { return key.id !== slotId && key.toolId === toolId; });
    if (!result[page.id]) result[page.id] = {};
    if (duplicate && toolId !== "empty") result[page.id][duplicate.id] = current.toolId;
    result[page.id][slotId] = toolId;
    return result;
}
