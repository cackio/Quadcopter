"""ANO V7 framing and private diagnostics. Schema comes from the C header."""
import math
import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCHEMA = {}
for header in ("telemetry_schema.h", "cal_wire.h"):
    SCHEMA.update({name: int(value, 0) for name, value in re.findall(
        r"^#define\s+((?:DIAG_|STATUS_|SCAL_)\w+)\s+(0x[0-9A-Fa-f]+|\d+)\s*$",
        (ROOT/"Core/Inc"/header).read_text(encoding="utf-8"), re.MULTILINE)})
# Read the command/state/reason enum values from the actual C service header.
for body in re.findall(r"enum\s*\{([^}]+)\}", (ROOT/"Core/Inc/cal_service.h").read_text()):
    value = -1
    for entry in body.split(","):
        name, *explicit = entry.strip().split("=")
        value = int(explicit[0], 0) if explicit else value+1
        SCHEMA[name.strip()] = value


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
            expected = {3: 7, 4: 9, SCHEMA["DIAG_FRAME_ID"]: SCHEMA["DIAG_PAYLOAD_SIZE"],
                        SCHEMA["SCAL_CMD_ID"]: SCHEMA["SCAL_CMD_SIZE"],
                        SCHEMA["SCAL_REPLY_ID"]: SCHEMA["SCAL_REPLY_SIZE"]}
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


def pack_command(op, request_id, face=0, samples=1000):
    payload = struct.pack("<4BIHH", SCHEMA["SCAL_VERSION"], op, face, 0, request_id, samples, 0)
    return pack(SCHEMA["SCAL_CMD_ID"], payload)


def decode_cal_reply(payload):
    if len(payload) != SCHEMA["SCAL_REPLY_SIZE"] or payload[0] != SCHEMA["SCAL_VERSION"]:
        raise ValueError("Unsupported calibration protocol")
    def field(name, fmt):
        return struct.unpack_from("<"+fmt, payload, SCHEMA["SCAL_OFF_"+name])
    out = {"op": payload[1], "face": payload[2], "state": payload[3], "request_id": field("ID", "I")[0],
           "reason": payload[8], "last_reason": payload[9]}
    if out["op"] != SCHEMA["CAL_INFO"]:
        for name, fmt in (("COUNT","H"),("TARGET","H"),("RESETS","H"),("FIRST_SEQ","I"),
                          ("LAST_SEQ","I"),("DURATION_US","I"),("DT_BAD","I"),("TEMPERATURE_C","f")):
            out[name.lower()] = field(name,fmt)[0]
        out["mean_raw"] = list(field("MEAN","6f"))
        out["std_raw"] = list(field("STD","6f"))
    elif out["face"] == 0:
        for name in ("APP_MODE","IMU_OK","MAG_PRESENT","CONFIG_OK"):
            out[name.lower()] = struct.unpack_from("<B",payload,SCHEMA["SCAL_INFO_OFF_"+name])[0]
        for name in ("BAUD","IMU_HZ","CAL_FLAGS","IMU_ERRORS","MAG_ERRORS","RX_OVERFLOW","BAD_COMMANDS",
                     "RX_ERRORS","UPTIME_MS","DT_TOTAL","DT_IN_BAND","EXEC_MAX_US","MISSED","LAST_JOB_ID",
                     "FIRMWARE_MAGIC","RX_BYTES","DIAGNOSTIC_VERSION"):
            out[name.lower()] = struct.unpack_from("<I",payload,SCHEMA["SCAL_INFO_OFF_"+name])[0]
    elif out["face"] in (1,2):
        for name in ("ADDRESS","STAGE","REGISTER","EXPECTED","ACTUAL","HAL_STATUS","INITIALIZED"):
            out[name.lower()] = struct.unpack_from("<B",payload,SCHEMA["SCAL_DRIVER_OFF_"+name])[0]
        for i,value in enumerate(struct.unpack_from("<3B",payload,SCHEMA["SCAL_DRIVER_OFF_ID"])):
            out["id"+str(i)] = value
        for name in ("HAL_ERROR","READ_ERRORS","OVERFLOW","NOT_READY","NEW_SAMPLES"):
            out[name.lower()] = struct.unpack_from("<I",payload,SCHEMA["SCAL_DRIVER_OFF_"+name])[0]
    elif out["face"] in (3,4,5):
        count = payload[SCHEMA["SCAL_COEFF_OFF_COUNT"]]
        if count != {3:6,4:12,5:18}[out["face"]]:
            raise ValueError("Wrong coefficient page size")
        out["coefficients"] = list(struct.unpack_from("<"+str(count)+"f",payload,SCHEMA["SCAL_COEFF_OFF_VALUES"]))
    elif out["face"] == 6:
        out["bias_dps"] = list(struct.unpack_from("<3f",payload,SCHEMA["SCAL_GYRO_OFF_BIAS_DPS"]))
        out["ready"] = bool(payload[SCHEMA["SCAL_GYRO_OFF_READY"]])
    return out
