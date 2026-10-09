"""Check the actual native GUI-saved score after the isolated Dynamics exercise."""
from pathlib import Path
from zipfile import ZipFile
import xml.etree.ElementTree as ET

for folder in ('score', 'drum-score'):
    scores = list(Path('checks', folder).glob('*.mscz'))
    assert len(scores) == 1, f'Expected one native score in {folder}'
    with ZipFile(scores[0]) as archive:
        xml_scores = [n for n in archive.namelist() if n.endswith('.mscx')]
        assert xml_scores, 'Missing score XML'
        score = ET.fromstring(archive.read(xml_scores[0]))
    style = score.find('./Score/Style')
    assert style is not None, 'Missing saved style'
    assert style.findtext('evanDynamicsEnabled') == '1', 'Custom dynamics did not persist'
    values = style.findtext('evanDynamicsTap', '').split()
    assert len(values) > 10 and all(int(value) == 10 for value in values[1:]), 'Bulk taps did not persist'
    if folder == 'drum-score':
        voice = score.find('./Score/Staff/Measure/voice')
        assert voice is not None
        chords = voice.findall('Chord')
        assert len(chords) == 4, 'The four-note pattern changed'
        assert all(c.findtext('durationType') == 'quarter' for c in chords)
        assert [bool(c.findall('Articulation')) for c in chords] == [True, False, False, True], 'Accents changed'
        assert voice.findtext('Dynamic/subtype') == 'ff', 'Written ff changed'
    print(f'Native saved Dynamics validated: {folder}, taps=10, notation preserved')
