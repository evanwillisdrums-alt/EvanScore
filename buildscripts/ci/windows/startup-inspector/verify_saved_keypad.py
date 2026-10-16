"""Verify real keypad roll saves on pitched and marching-snare notation."""
from pathlib import Path
import xml.etree.ElementTree as ET
from zipfile import ZipFile

for folder, name, pitch in [('score', 'keypad-notes', 60), ('keypad-snare', 'keypad-snare', 50)]:
    with ZipFile(Path('checks') / folder / (name + '.mscz')) as archive:
        xml = next(n for n in archive.namelist() if n.endswith('.mscx'))
        root = ET.fromstring(archive.read(xml))
    notes = root.findall('.//Note')
    chords = root.findall('.//Chord')
    assert len(notes) == len(chords) == 25
    assert all(int(n.findtext('pitch')) == pitch for n in notes)
    assert all(c.findtext('TremoloSingleChord/subtype') == 'buzzroll' for c in chords)
    assert not root.findall('.//LayoutBreak')
    if folder == 'score':
        assert len(root.findall('.//Fermata')) == 1
    else:
        assert root.findtext('.//Instrument/useDrumset') == '1'
print('Native keypad pitches, complete buzz rolls and score persistence passed')
