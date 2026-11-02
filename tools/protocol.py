"""ANO V7 framing and private diagnostics. Schema comes from the C header."""
import math
import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCHEMA = {
    name: int(value, 0)
    for name, value in re.findall(
        r"^#define\s+((?:DIAG_|STATUS_)\w+)\s+(0x[0-9A-Fa-f]+|\d+)\s*$",
        (ROOT / "Core/Inc/telemetry_schema.h").read_text(encoding="utf-8"),
        re.MULTILINE,
    )
}


def checksum(data):
    sc = ac = 0
    for value in data:
        sc = (sc + value) & 255
        ac = (ac + sc) & 255
    return bytes((sc, ac))


def pack(frame_id, payload):
    if len(payload) > 255:
        raise ValueError("Payload exceeds V7 length field")
    body = bytes((0xAA, 0xFF, frame_id, len(payload))) + bytes(payload)
    return body + checksum(body)


class Parser:
    """Incremental framing; retain incomplete input, resync after bad lengths/CRC.

    Bound the accepted length to 96 bytes: all frames used by this project fit.
    Known function IDs have fixed lengths. Reject corrupt lengths early while
    preserving header-like byte sequences inside a valid, fragmented payload.
    """

    def __init__(self):
        self.buffer = bytearray()
        self.bad_checksums = 0
        self.discarded_bytes = 0

    def feed(self, chunk):
        self.buffer.extend(chunk)
        frames = []
        while True:
            start = self.buffer.find(b"\xaa\xff")
            if start < 0:
                keep = 1 if self.buffer.endswith(b"\xaa") else 0
                remove = len(self.buffer) - keep
                self.discarded_bytes += remove
                del self.buffer[:remove]
                break
            if start:
                self.discarded_bytes += start
                del self.buffer[:start]
            if len(self.buffer) < 4:
                break
            length = self.buffer[3]
            expected = {3: 7, 4: 9, SCHEMA["DIAG_FRAME_ID"]: SCHEMA["DIAG_PAYLOAD_SIZE"]}
            if length > 96 or (self.buffer[2] in expected and length != expected[self.buffer[2]]):
                del self.buffer[0]
                self.discarded_bytes += 1
                continue
            size = length + 6
            if len(self.buffer) < size:
                break
            candidate = self.buffer[:size]
            if checksum(candidate[:-2]) != candidate[-2:]:
                self.bad_checksums += 1
                self.discarded_bytes += 1
                del self.buffer[0]
                continue
            frames.append((candidate[2], bytes(candidate[4:-2])))
            del self.buffer[:size]
        return frames


def decode_diagnostic(payload):
    if len(payload) != SCHEMA["DIAG_PAYLOAD_SIZE"]:
        raise ValueError("Wrong diagnostic payload size")

    def field(name, fmt):
        return struct.unpack_from("<" + fmt, payload, SCHEMA["DIAG_OFF_" + name])

    version = field("VERSION", "B")[0]
    if version != SCHEMA["DIAG_VERSION"]:
        raise ValueError(f"Unsupported diagnostic schema {version}")
    result = {}
    for name, fmt in (
        ("FLAGS", "H"), ("SEQUENCE", "I"), ("TIMESTAMP_US", "I"),
        ("DT_US", "H"), ("EXEC_US", "H"), ("MAG_SEQUENCE", "I"),
        ("MAG_AGE_US", "I"), ("RELEASE_MISSED", "I"),
        ("I2C_ERRORS", "I"), ("CAL_SAMPLES", "H"), ("TX_DROPPED", "H"),
    ):
        result[name.lower()] = field(name, fmt)[0]
    for name, value in zip(("ax", "ay", "az", "gx", "gy", "gz"), field("IMU_RAW", "6h")):
        result[name] = value
    for name, value in zip(("mx", "my", "mz"), field("MAG_RAW", "3h")):
        result[name] = value
    q = [v / 10000 for v in field("QUATERNION", "4h")]
    for name, value in zip(("qw", "qx", "qy", "qz"), q):
        result[name] = value
    w, x, y, z = q
    result["roll_deg"] = math.degrees(math.atan2(2*(w*x+y*z), 1-2*(x*x+y*y)))
    result["pitch_deg"] = math.degrees(math.asin(max(-1, min(1, 2*(w*y-z*x)))))
    result["yaw_deg"] = math.degrees(math.atan2(2*(w*z+x*y), 1-2*(y*y+z*z)))
    return result
