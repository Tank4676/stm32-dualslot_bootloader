import sys
import zlib
import struct

if len(sys.argv) != 2 or sys.argv[1] not in ("a", "b"):
    print("Usage: python3 import_crc.py <a|b>")
    sys.exit(1)

slot = sys.argv[1]

if slot == "a":
    APP_BIN     = "/home/tanmay/STM32CubeIDE/workspace_2.0.0/app_v1_dualslot/Debug/app_v1_dualslot.bin"
    FULL_SIZE   = 0x1C000      # 112 KB
    META_OFFSET = 0x1BFF8
    CRC_OFFSET  = 0x1BFFC
else:
    APP_BIN     = "/home/tanmay/STM32CubeIDE/workspace_2.0.0/app_v1_dualslot/Debug_slotb/app_v1_dualslot.bin"
    FULL_SIZE   = 0x20000      # 128 KB
    META_OFFSET = 0x1FFF8
    CRC_OFFSET  = 0x1FFFC

with open(APP_BIN, "rb") as f:
    data = bytearray(f.read())

data += b'\xFF' * (FULL_SIZE - len(data))
app_size = META_OFFSET
crc = zlib.crc32(bytes(data[:app_size])) & 0xFFFFFFFF

data[META_OFFSET:META_OFFSET+4] = struct.pack("<I", app_size)
data[CRC_OFFSET:CRC_OFFSET+4]   = struct.pack("<I", crc)

with open(APP_BIN, "wb") as f:
    f.write(data)

print(f"Slot {slot.upper()} | size: 0x{app_size:08X} | CRC: 0x{crc:08X}")
