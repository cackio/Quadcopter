import ctypes as ct
import math
import struct
import numpy as np
import pytest
from conftest import Filter, Float3, Float4, GyroCal, ImuRaw
from protocol import Parser, SCHEMA, decode_diagnostic, pack


def euler(native, quaternion):
    result = Float3()
    native.Attitude_ToEuler(quaternion, result)
    return list(result)


@pytest.mark.parametrize("axis", range(3))
def test_actual_c_gyro_ninety_degrees(native, axis):
    f = Filter(Float4(1,0,0,0), 0)
    gyro = Float3()
    gyro[axis] = math.pi/2
    for _ in range(200):
        assert native.Attitude_Update(ct.byref(f), gyro, Float3(0,0,0), None, 0.005, 0) == 1
    expected = Float4()
    angles = [0,0,0]
    angles[axis] = 90
    native.Attitude_FromEuler(*angles, expected)
    assert abs(np.dot(f.q, expected)) > 0.999999
    assert np.linalg.norm(f.q) == pytest.approx(1, abs=1e-6)


def test_c_six_axis_static_convergence(native):
    f = Filter(Float4(1,0,0,0), 0.05)
    q = Float4()
    native.Attitude_FromEuler(30, -20, 0, q)
    w,x,y,z = q
    acc = Float3(2*(x*z-w*y), 2*(y*z+w*x), 1-2*(x*x+y*y))
    for _ in range(5000):
        assert native.Attitude_Update(ct.byref(f), Float3(0,0,0), acc, None, 0.005, 1) == 2
    assert euler(native, f.q)[:2] == pytest.approx([30,-20], abs=0.1)


def test_c_nine_axis_heading_and_fallback(native):
    f = Filter(Float4(1,0,0,0), 0.1)
    target = Float4()
    native.Attitude_FromEuler(-25,20,70,target)
    w,x,y,z=target
    rotation = np.array([[1-2*(y*y+z*z),2*(x*y-w*z),2*(x*z+w*y)],
                         [2*(x*y+w*z),1-2*(x*x+z*z),2*(y*z-w*x)],
                         [2*(x*z-w*y),2*(y*z+w*x),1-2*(x*x+y*y)]])
    acc = Float3(*(rotation.T @ [0,0,1]))
    mag = Float3(*(rotation.T @ [0.7,0,0.4]))
    for _ in range(6000):
        assert native.Attitude_Update(ct.byref(f),Float3(0,0,0),acc,mag,0.005,1) == 3
    assert euler(native,f.q) == pytest.approx([-25,20,70],abs=0.15)
    previous = np.array(f.q)
    assert native.Attitude_Update(ct.byref(f),Float3(0,0,0),acc,None,0.005,1) == 2
    assert abs(np.dot(previous,f.q)) > 0.99999
    assert native.Attitude_Update(ct.byref(f),Float3(0,0,0),acc,mag,0.005,1) == 3


@pytest.mark.parametrize("dt", [0,-0.005,0.1,float("nan"),float("inf")])
def test_c_rejects_invalid_time_without_modifying_q(native, dt):
    f=Filter(Float4(1,0,0,0),0.05)
    assert native.Attitude_Update(ct.byref(f),Float3(0,0,1),Float3(0,0,1),None,dt,1)==0
    assert list(f.q)==[1,0,0,0]


def test_c_nan_zero_vectors_and_zero_gradient(native):
    f=Filter(Float4(1,0,0,0),0.05)
    assert native.Attitude_Update(ct.byref(f),Float3(float("nan"),0,0),Float3(0,0,1),None,0.005,1)==0
    for _ in range(500):
        assert native.Attitude_Update(ct.byref(f),Float3(0,0,0),Float3(0,0,1),Float3(1,0,0),0.005,1)==3
    assert list(f.q)==pytest.approx([1,0,0,0])
    assert native.Attitude_Update(ct.byref(f),Float3(0,0,0),Float3(0,0,0),Float3(0,0,0),0.005,1)==1
    assert native.Attitude_Update(ct.byref(f),Float3(0,0,0),Float3(0,0,1),Float3(0,0,0),0.005,1)==2
    assert native.Attitude_Normalize(Float4(0,0,0,0))==0


def test_c_tilt_compensated_initial_heading_and_display(native):
    f=Filter(Float4(1,0,0,0),0.05)
    assert native.Attitude_Align(ct.byref(f),Float3(0,0,1),Float3(0,-1,0))==1
    assert euler(native,f.q)==pytest.approx([0,0,90],abs=0.001)
    q=Float4();native.Attitude_FromEuler(20,30,40,q)
    result=Float4();native.Attitude_Display(q,Float4(0,1,0,0),result)
    assert euler(native,result)==pytest.approx([20,-30,-40],abs=0.001)


def test_c_mapping_and_time_wrap(native):
    assert native.Attitude_MatrixValid((ct.c_float*9)(1,0,0,0,1,0,0,0,1))==1
    assert native.Attitude_MatrixValid((ct.c_float*9)(1,0,0,0,1,0,0,0,-1))==0
    assert native.Attitude_MatrixValid((ct.c_float*9)(1,1,0,0,1,0,0,0,1))==0
    assert native.test_elapsed(0x20,0xFFFFFFF0)==48


def test_expired_sample_does_not_revive_on_counter_wrap(native):
    valid=ct.c_uint8(1)
    native.test_expire(1000,0xFFFFFFF0,100000,ct.byref(valid))
    assert valid.value==1
    native.test_expire(200000,0xFFFFFFF0,100000,ct.byref(valid))
    assert valid.value==0
    native.test_expire(0x10,0xFFFFFFF0,100000,ct.byref(valid))
    assert valid.value==0


def test_c_stationary_gyro_calibration_and_motion_rejection(native):
    calibration=GyroCal()
    for _ in range(300):
        assert native.GyroCalibration_Push(ct.byref(calibration),Float3(0,0,1),Float3(.2,-.3,1),1000)==0
    assert calibration.count==300
    assert native.GyroCalibration_Push(ct.byref(calibration),Float3(0,0,1),Float3(10,0,0),1000)==0
    assert calibration.count==0
    for _ in range(1000):
        native.GyroCalibration_Push(ct.byref(calibration),Float3(0,0,1),Float3(.2,-.3,1),1000)
    assert calibration.ready==1
    assert list(calibration.bias_dps)==pytest.approx([.2,-.3,1])


def test_actual_c_v7_fixed_frames(native):
    out=(ct.c_ubyte*96)()
    n=native.ANO_PackEuler(Float3(0,0,0),out,96)
    assert bytes(out[:n])==bytes.fromhex("AA FF 03 07 00 00 00 00 00 00 00 B3 97")
    n=native.ANO_PackQuaternion(Float4(1,0,0,0),out,96)
    assert bytes(out[:n])==bytes.fromhex("AA FF 04 09 10 27 00 00 00 00 00 00 00 ED E4")
    n=native.ANO_PackEuler(Float3(30,-30,90),out,96)
    assert struct.unpack("<3h",bytes(out[4:10]))==(3000,-3000,9000)
    assert native.ANO_PackEuler(Float3(0,0,0),out,12)==0
    assert native.ANO_PackEuler(Float3(float("nan"),0,0),out,96)==0
    assert native.ANO_Scale(999,100)==32767
    assert native.ANO_Scale(-999,100)==-32768


def test_actual_c_diagnostic_schema_matches_python(native):
    out=(ct.c_ubyte*96)();n=native.test_diagnostic(out,96)
    assert n==67
    [(frame_id,payload)]=Parser().feed(bytes(out[:n]))
    assert frame_id==SCHEMA["DIAG_FRAME_ID"]
    row=decode_diagnostic(payload)
    assert row["sequence"]==0x12345678
    assert row["timestamp_us"]==0xFFFFFFF0
    assert [row[k] for k in ("ax","ay","az","gx","gy","gz")]==[-2500,-1500,-500,500,1500,2500]
    assert [row[k] for k in ("mx","my","mz")]==[-123,456,-789]
    assert row["dt_us"]==5000 and row["exec_us"]==1200
    assert row["qw"]==1 and row["cal_samples"]==1000 and row["i2c_errors"]==2


def test_parser_noise_bad_checksum_bad_length_and_fragmentation():
    valid=pack(3,bytes(7))
    broken=bytearray(valid);broken[-1]^=1
    stream=b"garbage\xaa"+bytes(broken)+b"\xaa\xff\x03\x60"+valid
    parser=Parser();frames=[]
    for value in stream:
        frames.extend(parser.feed(bytes([value])))
    assert frames==[(3,bytes(7))]
    assert parser.bad_checksums>=1
    # A frame-looking sequence inside a fragmented payload is still DATA.
    payload=pack(3,bytes(7))+bytes(48)
    outer=pack(SCHEMA["DIAG_FRAME_ID"],payload)
    parser=Parser()
    assert parser.feed(outer[:30])==[]
    assert parser.feed(outer[30:])==[(SCHEMA["DIAG_FRAME_ID"],payload)]


def test_c_driver_setup_and_readback_failure(native):
    native.mock_reset()
    assert native.IMU_Init()==1
    assert native.Magnetometer_Init()==1
    native.mock_reset();native.mock_corrupt_register(0x1B)
    assert native.IMU_Init()==0
    native.mock_reset();native.mock_imu_addr(0xD2)
    assert native.IMU_Init()==1


def set_s16(native,mag,reg,value):
    native.mock_register(mag,reg,(value>>8)&255)
    native.mock_register(mag,reg+1,value&255)


def test_c_driver_freshness_burst_signed_values_and_busy(native):
    native.mock_reset();assert native.IMU_Init()==1
    raw=ImuRaw();raw.accel[0]=123
    assert native.IMU_Read(ct.byref(raw))==0 and raw.accel[0]==123
    for i,value in enumerate((-32768,123,16384,77,-655,32767,-1)):
        set_s16(native,0,0x3B+2*i,value)
    native.mock_register(0,0x3A,1)
    assert native.IMU_Read(ct.byref(raw))==1
    assert list(raw.accel)==[-32768,123,16384]
    assert list(raw.gyro)==[-655,32767,-1] and raw.temperature==77
    assert native.IMU_Read(ct.byref(raw))==0
    reads=ct.c_uint.in_dll(native,"mock_reads").value
    ct.c_int.in_dll(native,"mock_busy").value=1
    assert native.IMU_Read(ct.byref(raw))==-1
    assert ct.c_uint.in_dll(native,"mock_reads").value==reads


def test_c_hmc_axis_order_overflow_and_wrong_identity(native):
    native.mock_reset();assert native.Magnetometer_Init()==1
    raw=(ct.c_int16*3)()
    for reg,value in ((3,100),(5,-300),(7,200)):
        set_s16(native,1,reg,value)
    native.mock_register(1,9,1)
    assert native.Magnetometer_Read(raw)==1 and list(raw)==[100,200,-300]
    native.mock_register(1,9,1);set_s16(native,1,3,-4096)
    assert native.Magnetometer_Read(raw)==-2
    native.mock_register(1,0x0A,0)
    assert native.Magnetometer_Init()==0
