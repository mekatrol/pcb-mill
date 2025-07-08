# verify_checksum.py
import struct
import sys

with open(sys.argv[1], 'rb') as f:
    data = f.read(32)
    words = struct.unpack('<8I', data)
    for i, w in enumerate(words):
        print(f"Word {i}: 0x{w:08X}")
    checksum = sum(words) & 0xFFFFFFFF
    print(f"\nChecksum = 0x{checksum:08X} => {'OK' if checksum == 0 else 'BAD'}")
