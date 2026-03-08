#!/usr/bin/env python3
"""
patch_header.py — post-build script for dual-slot bootloader system.

Patches fw_size, fw_crc and hdr_crc into both the .bin and .elf files
after every build.

Usage:
    python3 patch_header.py <firmware.bin> <slot> <fw_version>

Example (in CubeIDE post-build steps):
    python3 ../patch_header.py ${BuildArtifactFileBaseName}.bin 0 1
"""

import sys
import struct
import zlib
import os

# ── Constants — must match flash_layout.h ────────────────────────────────────

APP_MAGIC_WORD       = 0xDEADBEEF
APP_HEADER_VERSION   = 1
APP_HEADER_SIZE      = 0x4000       # 16KB

SLOT_A_BASE          = 0x08010000
SLOT_B_BASE          = 0x08040000

OFFSET_MAGIC        = 0
OFFSET_HDR_VERSION  = 4
OFFSET_FW_VERSION   = 8
OFFSET_FW_SIZE      = 12
OFFSET_FW_CRC       = 16
OFFSET_HDR_CRC      = 20
HEADER_STRUCT_SIZE  = 32

# ── CRC32 ─────────────────────────────────────────────────────────────────────

def crc32(data: bytes) -> int:
    return zlib.crc32(data) & 0xFFFFFFFF

# ── ELF patcher ──────────────────────────────────────────────────────────────

def patch_elf(elf_path: str, slot_base: int, fw_size: int, fw_crc: int, hdr_crc: int, fw_version: int):
    with open(elf_path, "rb") as f:
        elf = bytearray(f.read())

    e_phoff     = struct.unpack_from("<I", elf, 0x1C)[0]
    e_phentsize = struct.unpack_from("<H", elf, 0x2A)[0]
    e_phnum     = struct.unpack_from("<H", elf, 0x2C)[0]

    header_file_offset = None

    for i in range(e_phnum):
        ph_offset = e_phoff + i * e_phentsize
        p_type   = struct.unpack_from("<I", elf, ph_offset)[0]
        p_offset = struct.unpack_from("<I", elf, ph_offset + 0x04)[0]
        p_paddr  = struct.unpack_from("<I", elf, ph_offset + 0x0C)[0]
        p_filesz = struct.unpack_from("<I", elf, ph_offset + 0x10)[0]

        if p_type == 1 and p_paddr == slot_base and p_filesz >= HEADER_STRUCT_SIZE:
            header_file_offset = p_offset
            break

    if header_file_offset is None:
        print(f"Warning: could not find header segment in ELF — ELF not patched")
        return

    struct.pack_into("<I", elf, header_file_offset + OFFSET_MAGIC,       APP_MAGIC_WORD)
    struct.pack_into("<I", elf, header_file_offset + OFFSET_HDR_VERSION, APP_HEADER_VERSION)
    struct.pack_into("<I", elf, header_file_offset + OFFSET_FW_VERSION,  fw_version)
    struct.pack_into("<I", elf, header_file_offset + OFFSET_FW_SIZE,     fw_size)
    struct.pack_into("<I", elf, header_file_offset + OFFSET_FW_CRC,      fw_crc)
    struct.pack_into("<I", elf, header_file_offset + OFFSET_HDR_CRC,     hdr_crc)

    with open(elf_path, "wb") as f:
        f.write(elf)

    print(f"ELF patched: {elf_path}")

# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    if len(sys.argv) != 4:
        print("Usage: patch_header.py <firmware.bin> <slot> <fw_version>")
        sys.exit(1)

    bin_path   = sys.argv[1]
    slot       = int(sys.argv[2])
    fw_version = int(sys.argv[3])

    if slot not in (0, 1):
        print("Error: slot must be 0 or 1")
        sys.exit(1)

    slot_base = SLOT_A_BASE if slot == 0 else SLOT_B_BASE
    slot_name = "Slot A" if slot == 0 else "Slot B"

    with open(bin_path, "rb") as f:
        data = bytearray(f.read())

    print(f"Patching {bin_path} for {slot_name} (base 0x{slot_base:08X})")
    print(f"Binary size: {len(data)} bytes")

    if len(data) <= APP_HEADER_SIZE:
        print("Error: .bin smaller than APP_HEADER_SIZE")
        sys.exit(1)

    struct.pack_into("<I", data, OFFSET_MAGIC,       APP_MAGIC_WORD)
    struct.pack_into("<I", data, OFFSET_HDR_VERSION, APP_HEADER_VERSION)
    struct.pack_into("<I", data, OFFSET_FW_VERSION,  fw_version)

    fw_size = len(data) - APP_HEADER_SIZE
    struct.pack_into("<I", data, OFFSET_FW_SIZE, fw_size)
    print(f"fw_size:  {fw_size} bytes ({fw_size / 1024:.1f} KB)")

    fw_bytes = data[APP_HEADER_SIZE:]
    fw_crc   = crc32(fw_bytes)
    struct.pack_into("<I", data, OFFSET_FW_CRC, fw_crc)
    print(f"fw_crc:   0x{fw_crc:08X}")

    hdr_bytes = data[:HEADER_STRUCT_SIZE - 4]
    hdr_crc   = crc32(hdr_bytes)
    struct.pack_into("<I", data, OFFSET_HDR_CRC, hdr_crc)
    print(f"hdr_crc:  0x{hdr_crc:08X}")

    with open(bin_path, "wb") as f:
        f.write(data)
    print(f"BIN patched: {bin_path}")

    elf_path = bin_path.replace(".bin", ".elf")
    if os.path.exists(elf_path):
        patch_elf(elf_path, slot_base, fw_size, fw_crc, hdr_crc, fw_version)
    else:
        print(f"Warning: {elf_path} not found — ELF not patched")

    print("Done.")

if __name__ == "__main__":
    main()
