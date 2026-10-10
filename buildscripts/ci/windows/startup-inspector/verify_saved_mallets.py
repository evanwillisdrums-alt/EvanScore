"""Verify the actual native panel's committed score, not a synthetic export."""
from pathlib import Path
import xml.etree.ElementTree as ET
from zipfile import ZipFile

path = Path("checks/score/mallet-chords.mscz")
with ZipFile(path) as archive:
    name = next(name for name in archive.namelist() if name.endswith(".mscx"))
    root = ET.fromstring(archive.read(name))
score = root.find("Score")
assert score is not None, "Saved score XML is missing"
staves = score.findall("Staff")
assert len(staves) == 2, "Commit changed the grand staff"
notes = [note for staff in staves for note in staff.findall(".//Note")]
assert len(notes) == 4, "Commit omitted or duplicated a voice"
original = {0: 60, 4: 64, 7: 67, 11: 71}
pitches = [int(note.findtext("pitch")) for note in notes]
assert sorted(p % 12 for p in pitches) == sorted(original), "Commit changed pitch classes"
changes = [pitch - original[pitch % 12] for pitch in pitches]
assert sum(change != 0 for change in changes) == 1, "Expected one explicitly previewed octave change"
assert all(change in (-12, 0, 12) for change in changes), "Commit changed pitches beyond its preview"
ids = []
for staff in staves:
    chords = staff.findall(".//Chord")
    assert len(chords) == 1 and chords[0].findtext("durationType") == "quarter", "Commit changed rhythm"
    assert len(chords[0].findall("Note")) == 2, "Commit moved notes between staves"
    text = staff.findall(".//Sticking/text")
    assert len(text) == 1, "Commit duplicated or lost sticking"
    ids.extend(int(value) for value in "".join(text[0].itertext()).split())
assert sorted(ids) == [1, 2, 3, 4], "Commit lost physical mallet identities"
print("Native saved mallet placement: two staves, four notes, quarter durations, one octave change and all four mallet identities passed.")
