import zlib
import struct

APP_BIN = "/home/tanmay/Projects/app_v1/Debug/app_v1.bin"

with open(APP_BIN, "r+b") as f:
    data = bytearray(f.read())

crc = zlib.crc32(bytes(data[:-4])) & 0xFFFFFFFF
data[-4:] = struct.pack("<I", crc)

with open(APP_BIN, "wb") as f:
    f.write(data)

print(f"CRC32 = 0x{crc:08X}")
print(f"Size  = {len(data)} bytes (0x{len(data):X})")
