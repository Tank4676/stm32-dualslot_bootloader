"""
OTA firmware sender for STM32 bootloader.

Usage:
    python3 ota_send.py /dev/ttyACM0 /home/tanmay/Projects/app_v1/Debug/app_v1.bin

Protocol:
    1. Send 'UPDATE' to trigger bootloader update mode
    2. Wait for 'READY' from bootloader
    3. Send 4 bytes: firmware size (little endian)
    4. Loop: send 256 byte chunk + 4 byte CRC32, wait for 'ACK'
    5. Wait for 'DONE'
"""

import sys
import struct
import zlib
import time
import serial

CHUNK_SIZE = 256
BAUD = 115200

def wait_for(ser, expected, timeout=5):
    """Read lines until we see the expected token or timeout."""
    start = time.time()
    buf = b""
    while time.time() - start < timeout:
        data = ser.read(64)
        if data:
            buf += data
            if expected.encode() in buf:
                return True, buf.decode(errors="ignore")
    return False, buf.decode(errors="ignore")

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 ota_send.py <port> <firmware.bin>")
        sys.exit(1)

    port = sys.argv[1]
    fw_path = sys.argv[2]

    with open(fw_path, "rb") as f:
        firmware = f.read()

    fw_size = len(firmware)
    print(f"Firmware size: {fw_size} bytes")

    ser = serial.Serial(port, BAUD, timeout=1)
    time.sleep(0.5)

    # 1. Trigger update mode
    print("Sending UPDATE trigger...")
    ser.write(b"UPDATE")

    # 2. Wait for READY
    ok, msg = wait_for(ser, "READY", timeout=5)
    if not ok:
        print(f"No READY received. Got: {msg}")
        sys.exit(1)
    print("Bootloader ready.")

    # 3. Send firmware size
    print("Sending firmware size...")
    ser.write(struct.pack("<I", fw_size))

    # 4. Send chunks
    sent = 0
    chunk_index = 0
    while sent < fw_size:
        chunk = firmware[sent:sent + CHUNK_SIZE]
        crc = zlib.crc32(chunk) & 0xFFFFFFFF

        ser.write(chunk)
        ser.write(struct.pack("<I", crc))

        ok, msg = wait_for(ser, "ACK", timeout=5)
        if not ok:
            print(f"\nNo ACK at chunk {chunk_index}. Got: {msg}")
            sys.exit(1)

        sent += len(chunk)
        chunk_index += 1
        print(f"\rChunk {chunk_index}: {sent}/{fw_size} bytes", end="", flush=True)

    print()

    # 5. Wait for DONE
    ok, msg = wait_for(ser, "DONE", timeout=5)
    if ok:
        print("Update complete.")
    else:
        print(f"No DONE received. Got: {msg}")

    ser.close()

if __name__ == "__main__":
    main()
