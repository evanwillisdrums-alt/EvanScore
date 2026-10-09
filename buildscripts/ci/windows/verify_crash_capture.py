"""Validate the actual fault record and x64 context in a captured native minidump."""
from pathlib import Path
import struct

dump = next(Path('capture-test/fault-report').glob('*.dmp')).read_bytes()
assert dump[:4] == b'MDMP'
streams, directory = struct.unpack_from('<II', dump, 8)
exception_stream = None
for i in range(streams):
    stream_type, size, rva = struct.unpack_from('<III', dump, directory + i * 12)
    if stream_type == 6:
        exception_stream = rva
        break
assert exception_stream is not None, 'Minidump has no exception record'
code = struct.unpack_from('<I', dump, exception_stream + 8)[0]
address = struct.unpack_from('<Q', dump, exception_stream + 24)[0]
assert code == 0xC0000005 and address != 0, 'Missing native access violation'
context_size, context_rva = struct.unpack_from('<II', dump, exception_stream + 160)
assert context_size >= 256
rip = struct.unpack_from('<Q', dump, context_rva + 248)[0]
assert rip == address, 'Dump did not preserve the faulting x64 instruction pointer'
print('::notice title=Crash capture context::Native access violation and matching faulting x64 instruction pointer verified')
