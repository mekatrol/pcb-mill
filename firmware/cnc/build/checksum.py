import sys
import struct

with open(sys.argv[1], 'r+b') as f:
    f.seek(0)
    vectors = struct.unpack('<8I', f.read(32))
    checksum = (-(sum(vectors[:7]))) & 0xFFFFFFFF
    f.seek(28)
    f.write(struct.pack('<I', checksum))
