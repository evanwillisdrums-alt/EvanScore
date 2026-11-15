"""Verify saved native accent attachment and create-only instrument defaults."""
from pathlib import Path
from zipfile import ZipFile
import xml.etree.ElementTree as ET

paths = list(Path('checks/score').glob('*.mscz'))
assert len(paths) == 1, 'Expected one GUI-saved percussion score'
with ZipFile(paths[0]) as archive:
    name = next(n for n in archive.namelist() if n.endswith('.mscx'))
    score = ET.fromstring(archive.read(name)).find('Score')
assert score is not None
staff_defs = score.findall('./Part/Staff')
lines = [int(s.findtext('StaffType/lines', '5')) for s in staff_defs]
assert lines == [5, 1, 3, 2], f'Existing staff or new template defaults changed: {lines}'
groups = [s.find('StaffType').get('group') for s in staff_defs]
assert groups == ['pitched', 'percussion', 'percussion', 'percussion'], f'Instrument staff groups changed: {groups}'
staff = next(s for s in score.findall('Staff') if s.attrib['id'] == '2')
voices = staff.findall('Measure/voice')
assert len(voices) == 2
for voice in voices:
    chords = voice.findall('Chord')
    assert len(chords) == 1 and chords[0].findtext('durationType') == 'quarter'
    assert [n.findtext('pitch') for n in chords[0].findall('Note')] == ['50'], 'Moving accent changed notes'
assert not voices[0].findall('.//Articulation'), 'Accent remained in its old voice'
markings = voices[1].findall('.//Articulation')
assert len(markings) == 1 and markings[0].findtext('anchor') == '1', 'Manual Below placement was lost'
assert markings[0].findtext('direction') == 'down', 'Manual symbol direction was lost'
assert not score.findall('.//LayoutBreak')
print('Native accent attachment, notes, rhythm, manual placement/direction and create-only staff defaults persisted')

# The written placement must survive independently of its original MIDI pitch.
import json
position = json.loads(Path("checks/score/feature-debug.png.position.json").read_text())
with ZipFile(next(Path("checks/score").glob("*.mscz"))) as archive:
    xml_name = next(n for n in archive.namelist() if n.endswith(".mscx"))
    saved_root = ET.fromstring(archive.read(xml_name))
saved_note = saved_root.find("Score/Staff[@id='2']/Measure/voice/Chord/Note")
assert saved_note is not None
assert saved_note.findtext("fixed") == "1"
assert int(saved_note.findtext("fixedLine")) == position["writtenLine"]
assert int(saved_note.findtext("pitch")) == position["pitch"]
print("Saved percussion staff position retains its MIDI pitch")
