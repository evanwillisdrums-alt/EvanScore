"""Verify the exact native preview, including voice identity, in the GUI-saved score."""
from pathlib import Path
import json
import xml.etree.ElementTree as ET
from zipfile import ZipFile

for folder in ('score', 'mallet-unwritten'):
    base = Path('checks', folder)
    paths = list(base.glob('*.mscz'))
    assert len(paths) == 1, f'Missing native saved score: {folder}'
    expected = json.loads((base / 'expected-mallet.json').read_text())
    source = []
    for entry in expected['sourceKey'].split(';'):
        values = entry.split(':')
        # Source identity also includes both TPC spellings. Otherwise the
        # spelling-preservation change silently leaves this list empty.
        if len(values) == 6 and all(v.lstrip('-').isdigit() for v in values):
            source.append([int(v) for v in values])
    assert len(source) == len(expected['pitches']) == len(expected['mallets']) == 4, f'Incomplete native preview identity: {folder}'
    per_staff = {}
    changes = []
    for (track, tick, written, sounding, tpc1, tpc2), pitch, mallet in zip(source, expected['pitches'], expected['mallets']):
        delta = pitch - sounding
        assert delta % 12 == 0, 'Preview changed pitch class'
        changes.append(delta)
        number = 5 - mallet if expected['reverseNumbering'] else mallet
        per_staff.setdefault(track // 4 + 1, []).append((written + delta, tpc1, number))
    assert any(changes), 'Expected a deliberately selected octave revoicing'
    with ZipFile(paths[0]) as archive:
        name = next(name for name in archive.namelist() if name.endswith('.mscx'))
        root = ET.fromstring(archive.read(name))
    score = root.find('Score')
    assert score is not None, 'Saved score XML is missing'
    assert not score.findall('.//LayoutBreak'), 'Player Enter leaked into score layout shortcuts'
    staves = score.findall('Staff')
    assert len(staves) == 2, 'Commit changed the grand staff'
    ids = []
    for staff in staves:
        chords = staff.findall('.//Chord')
        assert len(chords) == 1 and chords[0].findtext('durationType') == 'quarter', 'Commit changed rhythm'
        actual = sorted((int(note.findtext('pitch')), int(note.findtext('tpc'))) for note in chords[0].findall('Note'))
        wanted = sorted(per_staff[int(staff.attrib['id'])])
        assert actual == [(pitch, tpc) for pitch, tpc, _ in wanted], f'Committed notes or spelling differ from preview: {folder}'
        text = staff.findall('.//Sticking/text')
        assert len(text) == 1, 'Commit duplicated or lost sticking'
        numbers = [int(value) for value in ''.join(text[0].itertext()).split()]
        assert numbers == [mallet for _, _, mallet in wanted], 'Written sticking lost source-note identity'
        ids.extend(numbers)
    assert sorted(ids) == [1, 2, 3, 4], 'Commit lost physical mallet identities'
    print(f'{folder}: native saved notes and sticking exactly match preview; staves, rhythm and voice identity retained')
