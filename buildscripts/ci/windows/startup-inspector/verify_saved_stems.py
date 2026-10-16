"""Check the score and actual Make Default Style output from native UI checks."""
from pathlib import Path
import json
import xml.etree.ElementTree as ET
from zipfile import ZipFile

path = Path('checks/score/keypad-notes.mscz')
with ZipFile(path) as archive:
    xml = next(name for name in archive.namelist() if name.endswith('.mscx'))
    root = ET.fromstring(archive.read(xml))
assert float(root.findtext('.//Style/evanStemLengthScale')) == 0.8
assert root.findtext('.//Style/shortenStem') == '0'
assert len(root.findall('.//Note')) == 25
assert not root.findall('.//LayoutBreak')
style_path = Path(json.loads(Path('checks/score/default-style.json').read_text())['path'])
assert style_path.is_file(), style_path
saved = ET.parse(style_path).getroot()
assert float(saved.findtext('.//Style/evanStemLengthScale')) == 0.8
assert saved.findtext('.//Style/shortenStem') == '0'
print('Native stem style score and saved default persistence passed')
