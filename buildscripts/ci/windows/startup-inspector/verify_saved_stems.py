"""Check the score and actual Make Default Style output from native UI checks."""
from pathlib import Path
import os
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
styles = list((Path(os.environ['LOCALAPPDATA']) / 'MuseScore').glob('*/Styles/EvanScore-Default.mss'))
assert len(styles) == 1, styles
saved = ET.parse(styles[0]).getroot()
assert float(saved.findtext('.//Style/evanStemLengthScale')) == 0.8
assert saved.findtext('.//Style/shortenStem') == '0'
print('Native stem style score and saved default persistence passed')
