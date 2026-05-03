import zlib
import struct

APP_BIN     = "/home/tanmay/Projects/app_v1/Debug/app_v1.bin"
FULL_SIZE   = 0x1C000      # 112KB full app region
META_OFFSET = 0x1BFF8      # app size stored here (last 8 bytes)
CRC_OFFSET  = 0x1BFFC      # CRC stored here (last 4 bytes)

with open(APP_BIN, "rb") as f:
    data = bytearray(f.read())

# pad to full region size
data += b'\xFF' * (FULL_SIZE - len(data))

# app size = everything before metadata
app_size = META_OFFSET

# compute CRC over app data only (excluding metadata)
crc = zlib.crc32(bytes(data[:app_size])) & 0xFFFFFFFF

# write size and CRC into metadata slots
data[META_OFFSET:META_OFFSET+4] = struct.pack("<I", app_size)
data[CRC_OFFSET:CRC_OFFSET+4]   = struct.pack("<I", crc)

with open(APP_BIN, "wb") as f:
    f.write(data)

print(f"App size written: 0x{app_size:08X} ({app_size} bytes)")
print(f"CRC written:      0x{crc:08X}")
