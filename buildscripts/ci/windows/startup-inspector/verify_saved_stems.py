"""Check the score and actual Make Default Style output from native UI checks."""
from pathlib import Path
import json
import xml.etree.ElementTree as ET
from zipfile import ZipFile

path = Path('checks/score/keypad-notes.mscz')
with ZipFile(path) as archive:
    xml = next(name for name in archive.namelist() if name.endswith('.mscx'))
    root = ET.fromstring(archive.read(xml))
    style = ET.fromstring(archive.read('score_style.mss')).find('./Style') \
        if 'score_style.mss' in archive.namelist() else root.find('./Score/Style')
assert style is not None, 'Saved score style is missing'
assert style.findtext('evanStemLengthScale') == '0.8', 'Saved score lost its 80-percent stem scale'
assert style.findtext('shortenStem') == '0', 'Conditional shortening changed in the saved score'
assert len(root.findall('.//Note')) == 25, 'Stem styling changed the notes'
assert not root.findall('.//LayoutBreak'), 'Style editing leaked a score spacing shortcut'
style_path = Path(json.loads(Path('checks/score/default-style.json').read_text())['path'])
assert style_path.is_file(), style_path
saved_data = style_path.read_bytes()
Path('checks/score/saved-default-style.mss').write_bytes(saved_data)
saved = ET.fromstring(saved_data)
assert saved.findtext('.//Style/evanStemLengthScale') == '0.8', 'Saved default lost its 80-percent stem scale'
assert saved.findtext('.//Style/shortenStem') == '0', 'Saved default changed conditional shortening'
print('Native stem style score and saved default persistence passed')
