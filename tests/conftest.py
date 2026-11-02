import ctypes as ct
import os
import shutil
import subprocess
import sys
from pathlib import Path
import pytest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/"tools"))
Float3 = ct.c_float*3
Float4 = ct.c_float*4


class Filter(ct.Structure):
    _fields_ = [("q", Float4), ("beta", ct.c_float)]


class GyroCal(ct.Structure):
    _fields_ = [("count", ct.c_uint32), ("mean", ct.c_float*6), ("m2", ct.c_float*6),
                ("bias_dps", Float3), ("ready", ct.c_uint8)]


class ImuRaw(ct.Structure):
    _fields_ = [("accel", ct.c_int16*3), ("gyro", ct.c_int16*3), ("temperature", ct.c_int16)]


@pytest.fixture(scope="session")
def native():
    compiler = os.environ.get("QUADCOPTER_HOST_CC") or shutil.which("gcc")
    if not compiler:
        for candidate in ("F:/qt/Tools/mingw1310_64/bin/gcc.exe", "F:/qt/Tools/mingw1120_64/bin/gcc.exe"):
            if Path(candidate).exists():
                compiler = candidate
                break
    if not compiler:
        pytest.fail("Native C compiler missing: set QUADCOPTER_HOST_CC")
    output = ROOT/"build/tests"
    output.mkdir(parents=True, exist_ok=True)
    library = output/("flight_native.dll" if os.name == "nt" else "libflight_native.so")
    command = [compiler, "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", "-shared",
               "-I"+str(ROOT/"tests/stubs"), "-I"+str(ROOT/"Core/Inc")]
    if os.name == "nt":
        command += ["-static-libgcc", "-Wl,--export-all-symbols"]
    else:
        command += ["-fPIC"]
    command += [str(ROOT/p) for p in ("Core/Src/attitude.c", "Core/Src/ano_protocol.c",
                                    "Core/Src/imu_driver.c", "tests/native_bindings.c")]
    command += ["-lm", "-o", str(library)]
    environment = os.environ.copy()
    environment["PATH"] = str(Path(compiler).parent)+os.pathsep+environment.get("PATH", "")
    result = subprocess.run(command, capture_output=True, text=True, env=environment)
    assert result.returncode == 0, result.stdout+result.stderr
    dll = ct.CDLL(str(library))
    dll.host_compiler = compiler
    dll.host_environment = environment
    ptr = ct.POINTER(ct.c_float)
    dll.Attitude_Update.argtypes = [ct.POINTER(Filter), ptr, ptr, ptr, ct.c_float, ct.c_int]
    dll.Attitude_FromEuler.argtypes = [ct.c_float, ct.c_float, ct.c_float, ptr]
    dll.Attitude_ToEuler.argtypes = [ptr, ptr]
    dll.Attitude_Align.argtypes = [ct.POINTER(Filter), ptr, ptr]
    dll.Attitude_Display.argtypes = [ptr, ptr, ptr]
    dll.Attitude_MatrixValid.argtypes = [ptr]
    dll.Attitude_Normalize.argtypes = [ptr]
    dll.GyroCalibration_Push.argtypes = [ct.POINTER(GyroCal), ptr, ptr, ct.c_uint32]
    dll.GyroCalibration_Reset.argtypes = [ct.POINTER(GyroCal)]
    dll.ANO_PackEuler.argtypes = [ptr, ct.c_void_p, ct.c_size_t]
    dll.ANO_PackQuaternion.argtypes = [ptr, ct.c_void_p, ct.c_size_t]
    dll.ANO_Scale.argtypes = [ct.c_float, ct.c_float]
    dll.ANO_Scale.restype = ct.c_int16
    dll.test_diagnostic.argtypes = [ct.c_void_p, ct.c_size_t]
    dll.test_elapsed.argtypes = [ct.c_uint32, ct.c_uint32]
    dll.test_elapsed.restype = ct.c_uint32
    dll.test_expire.argtypes = [ct.c_uint32,ct.c_uint32,ct.c_uint32,ct.POINTER(ct.c_uint8)]
    dll.IMU_Read.argtypes = [ct.POINTER(ImuRaw)]
    dll.Magnetometer_Read.argtypes = [ct.POINTER(ct.c_int16)]
    return dll
