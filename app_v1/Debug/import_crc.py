import struct

APP_BIN = "/home/tanmay/Projects/app_v1/Debug/app_v1.bin"
CRC_OFFSET = 0x1FFFC

def stm32_crc32(data):
    crc = 0xFFFFFFFF
    for i in range(0, len(data), 4):
        word = struct.unpack(">I", data[i:i+4])[0]
        crc ^= word
        for _ in range(32):
            if crc & 0x80000000:
                crc = (crc << 1) ^ 0x04C11DB7
            else:
                crc <<= 1
            crc &= 0xFFFFFFFF
    return crc

with open(APP_BIN, "r+b") as f:
    data = bytearray(f.read())
    crc = stm32_crc32(data[:CRC_OFFSET])
    data[CRC_OFFSET:CRC_OFFSET+4] = struct.pack("<I", crc)
    f.seek(0)
    f.write(data)
    print(f"CRC written: 0x{crc:08X}")
