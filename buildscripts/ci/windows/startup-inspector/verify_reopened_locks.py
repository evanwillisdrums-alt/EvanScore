"""Verify locks after the actual Windows executable reopens and exports a score."""
from pathlib import Path
import xml.etree.ElementTree as ET

files = list(Path('checks').glob('*/locks/reopened.mscx'))
assert len(files) == 2, 'Missing reopened locked score'
for path in files:
    notes = ET.parse(path).findall('.//Note')
    assert notes and all(n.findtext('dynamicsLock', '') for n in notes), f'Locks lost on reopen: {path}'
    print(f'{path}: {len(notes)} reopened locks')
