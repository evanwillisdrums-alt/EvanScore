"""Check files written by real first-save/Save As interactions in the Windows app."""
from pathlib import Path
from zipfile import ZipFile
import xml.etree.ElementTree as ET


def score(name):
    path = Path("checks/score") / f"tuplet1-ref{name}.mscz"
    with ZipFile(path) as archive:
        roots = [n for n in archive.namelist() if n.endswith(".mscx")]
        assert len(roots) == 1, (path, roots)
        return ET.fromstring(archive.read(roots[0])).find("Score")


def music(root):
    """Compare notation content without view/audio/project metadata."""
    return [
        [
            [(node.tag, node.text) for node in measure.iter()
             if node.tag in {"pitch", "tpc", "tpc2", "durationType", "duration", "dots",
                             "sigN", "sigD", "actualNotes", "normalNotes"}]
            for measure in staff.findall("Measure")
        ]
        for staff in root.findall("Staff")
    ]


original = score("")
saved_as = score("-saved as")
assert music(saved_as) == music(original), "Save As changed notes, durations or measures"
assert saved_as.findall(".//Note"), "The Save As test score must contain notes"

first = score("-first save")
staves = first.findall("Staff")
assert len(staves) == 2, "First save lost a piano staff"
assert all(len(staff.findall("Measure")) == 2 for staff in staves), "First save lost measures"
assert first.findtext(".//metaTag[@name='workTitle']") == "tuplet1-ref-new score"
print("Native first Save and Save As archives retain their score contents")
