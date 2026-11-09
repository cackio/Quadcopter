import ctypes as ct
import struct
import json
import subprocess
import sys
import numpy as np
import pytest
from conftest import ImuRaw
from protocol import SCHEMA as S, Parser, pack_command, decode_cal_reply
from calibrate import fit_accelerometer_statistics, validate_magnetometer
from sensor_check import (check_gyro, check_accel, mapping_from_directions,
                          fresh_magnetic, compare_coefficients, Client,
                          collect_magnetic, header_coefficients)


class Moments(ct.Structure):
    _fields_=[('count',ct.c_uint32),('mean',ct.c_float*7),('m2',ct.c_float*7),
              ('origin',ct.c_float*7),('centered',ct.c_float*7)]


class Command(ct.Structure):
    _fields_=[('id',ct.c_uint32),('op',ct.c_uint8),('face',ct.c_uint8),('samples',ct.c_uint16)]


class Service(ct.Structure):
    _fields_=[(key,ct.c_uint32) for key in ('id','started_us','last_new_us','last_sequence',
              'first_sequence','capture_us','duration_us','dt_bad')]+[
        ('target',ct.c_uint16),('resets',ct.c_uint16)]+[
        (key,ct.c_uint8) for key in ('op','face','state','reason','last_reason','have_sequence','gyro_ready')]+[
        ('bias_dps',ct.c_float*3),('moments',Moments)]


class WireParser(ct.Structure):
    _fields_=[('bytes',ct.c_uint8*18),('used',ct.c_uint),('bad_frames',ct.c_uint32)]


class Diagnostic(ct.Structure):
    _fields_=[(name,ct.c_uint8) for name in ('address','id0','id1','id2','stage','reg','expected','actual','status','initialized')]+[
        (name,ct.c_uint32) for name in ('hal_error','read_errors','overflow','not_ready','new_samples')]


@pytest.fixture
def service(native):
    native.CalService_Init.argtypes=[ct.POINTER(Service)]
    native.CalService_Command.argtypes=[ct.POINTER(Service),ct.POINTER(Command),ct.c_uint32]
    native.CalService_Feed.argtypes=[ct.POINTER(Service),ct.POINTER(ImuRaw),ct.c_int,ct.c_uint32,ct.c_uint32]
    native.CalWire_Result.argtypes=[ct.POINTER(Service),ct.c_void_p]
    native.CalParser_Feed.argtypes=[ct.POINTER(WireParser),ct.c_uint8,ct.POINTER(Command)]
    native.Sensor_Diagnostic.argtypes=[ct.c_uint]
    native.Sensor_Diagnostic.restype=ct.POINTER(Diagnostic)
    state=Service(); native.CalService_Init(ct.byref(state))
    return native,state


def start(native,state,job=1,op=2,now=0,face=0):
    return native.CalService_Command(ct.byref(state),ct.byref(Command(job,op,face,1000)),now)


def feed(native,state,index,raw=None,now=None,result=1):
    if raw is None:
        raw=ImuRaw((ct.c_int16*3)(0,0,16384),(ct.c_int16*3)(52,-31,18),340)
    native.CalService_Feed(ct.byref(state),ct.byref(raw),result,index,(index*5000 if now is None else now)&0xffffffff)


def test_local_capture_1000_fresh_identical_values_and_independent_window(service):
    n,s=service
    assert start(n,s)==S['CAL_OK']
    for i in range(1,1201): feed(n,s,i)
    assert s.state==S['CAL_DONE'] and s.moments.count==1000 and s.gyro_ready
    assert list(s.bias_dps)==pytest.approx(np.array([52,-31,18])/65.5,abs=1e-6)
    assert s.first_sequence==201 and s.last_sequence==1200
    assert s.duration_us==999*5000
    assert list(s.moments.m2)==[0]*7
    assert start(n,s)==S['CAL_OK']  # Same command does not reset DONE.
    assert s.state==S['CAL_DONE']
    assert start(n,s,2,3,6000000)==S['CAL_OK']
    for i in range(1201,2401): feed(n,s,i)
    frame=(ct.c_uint8*96)(); size=n.CalWire_Result(ct.byref(s),frame)
    [(fid,payload)]=Parser().feed(bytes(frame[:size]))
    assert fid==S['SCAL_REPLY_ID'] and size==90
    row=decode_cal_reply(payload)
    assert row['request_id']==2 and row['first_seq']>1200
    assert row['temperature_c']==pytest.approx(37.53)
    assert check_gyro(row,list(s.bias_dps))['passed']


@pytest.mark.parametrize('failure,expected',[(0,'CAL_NO_SAMPLE'),(-1,'CAL_IO_ERROR')])
def test_faults_do_not_silently_wait(service,failure,expected):
    n,s=service; start(n,s)
    feed(n,s,0,result=failure,now=2000000)
    assert s.state==S['CAL_FAILED'] and s.reason==S[expected]


def test_wrap_duplicate_sequence_cancel_stale_and_reconnect(service):
    n,s=service; base=0xfffff000
    assert start(n,s,now=base)==0
    feed(n,s,1,now=base+5000)
    feed(n,s,1,now=base+10000)  # duplicate ID is not another sample
    assert s.moments.count==1
    assert start(n,s,2,now=base+10000)==S['CAL_BUSY']
    assert s.id==1 and s.moments.count==1
    cancel=Command(1,S['CAL_CANCEL'],0,1000)
    assert n.CalService_Command(ct.byref(s),ct.byref(cancel),0)==0
    assert s.state==S['CAL_CANCELLED']
    assert start(n,s,2,now=10000)==0
    assert start(n,s,1,now=10000)==S['CAL_STALE']
    # A reset discards the active token; STATUS must fail rather than appear complete.
    n.CalService_Init(ct.byref(s))
    assert n.CalService_Command(ct.byref(s),ct.byref(Command(2,S['CAL_STATUS'],0,1000)),0)==S['CAL_STALE']


def test_motion_noise_timeout_and_time_gap(service):
    n,s=service; start(n,s)
    for i in range(1,50): feed(n,s,i)
    moving=ImuRaw((ct.c_int16*3)(0,0,16384),(ct.c_int16*3)(1000,0,0),0)
    feed(n,s,50,moving)
    assert s.resets==1 and s.moments.count==0 and s.last_reason==S['CAL_MOTION']
    feed(n,s,51,now=30000000)
    assert s.reason==S['CAL_TIMEOUT']
    start(n,s,2,now=30000000)
    feed(n,s,1,now=30005000); feed(n,s,2,now=30030000)
    assert s.reason==S['CAL_TIME_GAP']
    start(n,s,3,now=40000000)
    for i in range(1,201):
        noisy=ImuRaw((ct.c_int16*3)(0,0,16384),(ct.c_int16*3)(52+(-20 if i%2 else 20),-31,18),0)
        feed(n,s,i,noisy,now=40000000+i*5000)
    assert s.state==S['CAL_WAIT'] and s.last_reason==S['CAL_NOISE']


def test_actual_c_request_parser_resync_and_validation(service):
    n,s=service; parser=WireParser(); cmd=Command()
    good=pack_command(S['CAL_STATIC'],123,6)
    corrupt=bytearray(good);corrupt[-1]^=1
    data=b'noise\xaa\xff\xf2\xff'+bytes(corrupt)+good
    received=[]
    for byte in data:
        if n.CalParser_Feed(ct.byref(parser),byte,ct.byref(cmd)):
            received.append((cmd.id,cmd.op,cmd.face,cmd.samples))
    assert received==[(123,3,6,1000)] and parser.bad_frames>=1
    assert n.CalService_Command(ct.byref(s),ct.byref(Command(1,3,19,1000)),0)==S['CAL_BAD_COMMAND']


def test_diagnostics_identify_register_and_error_counter(service):
    n,s=service
    n.mock_reset(); n.mock_corrupt_register(0x1B)
    assert n.IMU_Init()==0
    d=n.Sensor_Diagnostic(0).contents
    assert (d.stage,d.reg,d.expected,d.actual)==(4,0x1B,8,9)
    n.mock_reset(); assert n.IMU_Init()==1
    n.mock_fail_register(0x3A)
    raw=ImuRaw();assert n.IMU_Read(ct.byref(raw))==-1
    assert n.Sensor_Diagnostic(0).contents.read_errors==1
    assert n.Sensor_Diagnostic(1).contents.read_errors==0


def fake_stat(vector):
    return {'count':1000,'state':S['CAL_DONE'],'mean_raw':list(vector)+[52,-31,18],
            'std_raw':[1]*6,'temperature_c':37}


def test_face_statistics_independent_data_and_mapping():
    bias=np.array([.02,-.03,.04]);scale=np.array([1.02,.98,1.01]);faces={}
    for i,name in enumerate(('px','nx','py','ny','pz','nz')):
        v=np.zeros(3);v[i//2]=1 if i%2==0 else -1
        faces[name]=fake_stat((v/scale+bias)*16384)
    b,k=fit_accelerometer_statistics(faces)
    assert b==pytest.approx(bias); assert k==pytest.approx(scale)
    rng=np.random.default_rng(3);v=rng.normal(size=(12,3));v/=np.linalg.norm(v,axis=1)[:,None]
    poses=[fake_stat((x/scale+bias)*16384) for x in v]
    assert check_accel(poses,b,k)['passed']
    with pytest.raises(ValueError,match='覆盖'): check_accel([poses[0]]*12,b,k)
    faces['px']['count']=999
    with pytest.raises(ValueError): fit_accelerometer_statistics(faces)
    assert mapping_from_directions(['前','右','下']).tolist()==[[1,0,0],[0,-1,0],[0,0,-1]]
    with pytest.raises(ValueError): mapping_from_directions(['前','左','下'])
    with pytest.raises(ValueError): compare_coefficients({'3':[1,2]}, {'3':[1,3]})


def test_independent_magnetic_rejects_wrong_model_flat_and_duplicates():
    rng=np.random.default_rng(4);v=rng.normal(size=(500,3));v/=np.linalg.norm(v,axis=1)[:,None]
    raw=v*500+[100,-20,40]
    assert validate_magnetometer(raw,[100,-20,40],np.eye(3)/500)['rms_relative']<1e-10
    with pytest.raises(ValueError): validate_magnetometer(raw,[400,0,0],np.eye(3)/500)
    with pytest.raises(ValueError): validate_magnetometer(np.tile(raw[0],(500,1)),[100,-20,40],np.eye(3)/500)
    r=dict(mag_sequence=1,mag_age_us=1000,flags=S['STATUS_MAG_PRESENT']|S['STATUS_MAG_REJECTED'],mx=10,my=20,mz=30)
    assert len(fresh_magnetic([r,r]))==1


def test_client_retries_same_request_and_ignores_wrong_id(tmp_path):
    from protocol import pack
    class Port:
        in_waiting=0
        def __init__(self): self.writes=[]; self.chunks=[]
        def write(self,cmd):
            self.writes.append(cmd)
            if len(self.writes)==2:
                p=bytearray(84);p[0]=1;p[1]=2;p[3]=S['CAL_DONE'];struct.pack_into('<I',p,4,999)
                self.chunks.append(pack(S['SCAL_REPLY_ID'],p))
                struct.pack_into('<I',p,4,123)
                self.chunks.append(pack(S['SCAL_REPLY_ID'],p))
        def read(self,n): return self.chunks.pop(0) if self.chunks else b''
    port=Port()
    with (tmp_path/'received.bin').open('wb') as b:
        c=Client(port,b)
        assert c.request(S['CAL_GYRO'],123)['request_id']==123
    assert len(port.writes)==2 and port.writes[0]==port.writes[1]


def app_frame(native):
    out=(ct.c_uint8*96)(); size=native.app_frame(out)
    return Parser().feed(bytes(out[:size]))[0]


def app_send(native,op,job=0,face=0,now=5000):
    data=pack_command(op,job,face)
    native.app_receive(data,len(data)); native.app_telemetry()  # parse into mailbox
    native.app_cycle(now)  # only sample owner executes commands
    native.app_tx_complete(); native.app_telemetry()
    return app_frame(native)


def test_real_app_commands_info_pages_and_parameter_query(service):
    n,s=service; n.mock_reset(); n.app_reset()
    _,p=app_send(n,S['CAL_INFO']); info=decode_cal_reply(p)
    assert info['app_mode']==1 and info['imu_ok'] and info['mag_present']
    assert info['baud']==9600 and info['imu_hz']==200 and info['firmware_magic']==0x53434131
    assert info['rx_bytes']==18
    from protocol import ROOT
    expected,_=header_coefficients(ROOT/'Core/Inc/sensor_calibration.h')
    actual={}
    for page in (3,4,5):
        _,payload=app_send(n,S['CAL_INFO'],face=page)
        actual[str(page)]=decode_cal_reply(payload)['coefficients']
    compare_coefficients(actual,expected)
    _,payload=app_send(n,S['CAL_INFO'],face=1)
    driver=decode_cal_reply(payload)
    assert driver['id0']==0x68 and driver['initialized']


def test_real_app_rx_overflow_error_recovery_and_persistent_tx(service):
    n,s=service; n.mock_reset();n.app_reset()
    n.app_receive(b'\xaa'*300,300)
    assert n.app_rx_drops()>0
    n.app_telemetry()  # overflow flush resets framing
    assert n.app_pending()
    starts=ct.c_uint.in_dll(n,'app_rx_starts').value
    n.app_rx_error()
    assert ct.c_uint.in_dll(n,'app_rx_starts').value==starts+1
    assert n.app_pending()  # unrelated RX error must not release TX
    _,payload=app_send(n,S['CAL_INFO'])
    assert decode_cal_reply(payload)['rx_errors']==1
    assert decode_cal_reply(payload)['rx_overflow']>0
    # An absent TX completion eventually aborts the buffer instead of wedging telemetry.
    n.app_cycle(500000); n.app_telemetry(); n.app_telemetry()
    assert ct.c_uint.in_dll(n,'app_tx_aborts').value>=1


def test_real_app_raw_mode_ignores_attitude_validity_and_accepts_calibration(service):
    n,s=service; n.mock_reset();n.mock_register(1,0x0A,0);n.app_reset();n.app_invalid_config()
    _,payload=app_send(n,S['CAL_GYRO'],1)
    reply=decode_cal_reply(payload)
    assert reply['state']==S['CAL_WAIT'] and reply['request_id']==1
    for i,value in enumerate((0,0,16384)):
        reg=0x3B+2*i;n.mock_register(0,reg,(value>>8)&255);n.mock_register(0,reg+1,value&255)
    for i in range(1,1201):
        n.mock_register(0,0x3A,1);n.app_cycle(5000+i*5000)
    _,payload=app_send(n,S['CAL_STATUS'],1,now=6010000)
    assert decode_cal_reply(payload)['state']==S['CAL_DONE']
    n.app_tx_complete();n.app_telemetry();n.app_telemetry()
    fid,payload=app_frame(n)
    assert fid==S['DIAG_FRAME_ID']
    from protocol import decode_diagnostic
    row=decode_diagnostic(payload)
    assert not row['flags']&S['STATUS_ATTITUDE_VALID']
    assert row['flags']&S['STATUS_CONFIG_ERROR']
    assert not row['flags']&S['STATUS_MAG_PRESENT']
    assert row['cal_samples']==1000 and row['sequence']==1200


def test_magnetic_barrier_excludes_buffered_training_points():
    class Link:
        def info(self,page=0):
            return dict(new_samples=40) if page else dict(uptime_ms=1000,imu_errors=0,mag_errors=0)
        def observe(self,seconds,show=False):
            return [dict(sequence=100,mag_sequence=39),dict(sequence=101,mag_sequence=40),
                    dict(sequence=102,mag_sequence=41)]
    assert [r['mag_sequence'] for r in collect_magnetic(Link(),1)]==[41]


def test_cli_replay_and_failed_offline_never_exports_parameters(native,tmp_path):
    from protocol import ROOT
    out=(ct.c_uint8*96)(); size=native.test_diagnostic(out,96)
    source=tmp_path/'received.bin';source.write_bytes(bytes(out[:size]))
    destination=tmp_path/'replay'
    result=subprocess.run([sys.executable,str(ROOT/'tools/sensor_check.py'),'--replay',str(source),
                           '--output',str(destination)],capture_output=True)
    assert result.returncode==0,result.stderr
    record=json.loads((destination/'session.json').read_text(encoding='utf-8'))
    assert record['replay']['diagnostic_frames']==1
    assert '未连接硬件' in record['status']
    bad=tmp_path/'bad';bad.mkdir()
    (bad/'session.json').write_text(json.dumps({'gyro':{'independent':fake_stat([0,0,16384]),
                                                       'bias_dps':[20,0,0]}}),encoding='utf-8')
    failed=tmp_path/'failed'
    result=subprocess.run([sys.executable,str(ROOT/'tools/sensor_check.py'),'--offline',str(bad),
                           '--output',str(failed)],capture_output=True)
    assert result.returncode==1
    assert not (failed/'sensor_calibration.h').exists()
    assert json.loads((failed/'session.json').read_text(encoding='utf-8'))['error']


def test_welford_mean_and_sample_std_match_independent_numpy(native):
    native.CalMoments_Push.argtypes=[ct.POINTER(Moments),ct.POINTER(ImuRaw)]
    native.CalMoments_Std.argtypes=[ct.POINTER(Moments),ct.c_uint]
    native.CalMoments_Std.restype=ct.c_float
    moments=Moments();rng=np.random.default_rng(11)
    samples=np.rint(rng.normal([50,-120,16384,52,-31,18,340],[2,3,2,2,3,2,1],(1000,7))).astype(int)
    for row in samples:
        raw=ImuRaw((ct.c_int16*3)(*row[:3]),(ct.c_int16*3)(*row[3:6]),int(row[6]))
        native.CalMoments_Push(ct.byref(moments),ct.byref(raw))
    assert list(moments.mean)==pytest.approx(samples.mean(axis=0),abs=0.002)
    assert [native.CalMoments_Std(ct.byref(moments),i) for i in range(7)]==pytest.approx(samples.std(axis=0,ddof=1),abs=0.01)


def test_shared_sensor_compensation_units_and_bias_applied_once(native):
    from conftest import Float3
    raw=(ct.c_int16*3)(655,0,-655);bias=Float3(1,0,-1);out=Float3()
    values=(ct.c_float*18)();native.Sensor_Coefficients(5,values)
    rotation=np.array(values[:9]).reshape(3,3)
    native.Sensor_Gyro(raw,bias,out)
    assert list(out)==pytest.approx(rotation@np.deg2rad([9,0,-9]),abs=1e-6)
    native.Sensor_Coefficients(3,values)
    native_acc=Float3();acc=(ct.c_int16*3)(16384,0,0)
    native.Sensor_Accel(acc,native_acc,out)
    expected=(np.array([1,0,0])-values[:3])*values[3:6]
    assert list(native_acc)==pytest.approx(expected,abs=1e-6)
    assert list(out)==pytest.approx(rotation@expected,abs=1e-6)


def test_driver_waits_for_previous_stop_but_bounds_a_stuck_bus(native):
    native.mock_reset()
    ct.c_uint.in_dll(native,'mock_stop_cycles').value=3
    assert native.IMU_Init()==1
    assert native.Magnetometer_Init()==1
    reads=ct.c_uint.in_dll(native,'mock_reads').value
    ct.c_int.in_dll(native,'mock_busy').value=1
    raw=ImuRaw()
    assert native.IMU_Read(ct.byref(raw))==-1
    assert ct.c_uint.in_dll(native,'mock_reads').value==reads
    native.mock_reset()


def test_timeout_diagnostic_distinguishes_zero_bytes_from_reverse_path(tmp_path):
    class Port:
        in_waiting=0
        def read(self,n): return b''
    with (tmp_path/'received.bin').open('wb') as stream:
        c=Client(Port(),stream)
        assert '一个字节都没有收到' in c.timeout_message()
        c.received_bytes=67;c.frame_counts['F1']=1
        assert '已收到F1' in c.timeout_message()
        assert c.link_summary()['frames']=={'F1':1}
        c.frame_counts.clear()
        assert '没有有效协议帧' in c.timeout_message()


def test_misaligned_faces_report_all_failures_and_save_before_fitting(tmp_path,monkeypatch):
    from types import SimpleNamespace
    import sensor_check
    from calibrate import AccelerometerFaceError
    # Recorded raw means, not accepted calibration coefficients. X/Z poses have
    # persistent transverse components while the Y pair is nearly aligned.
    raw=[(17933.8535,-80.052,-3065.3779),(-14529.5684,-354.8,901.706),
         (1492.2581,16167.998,-2018.968),(1656.168,-16547.0059,-1156.37),
         (3329.7361,-448.174,14902.8496),(-543.136,-.042,-18322.4004)]
    faces={name:fake_stat(v) for name,v in zip(sensor_check.FACES,raw)}
    with pytest.raises(AccelerometerFaceError) as caught:
        fit_accelerometer_statistics(faces)
    quality=caught.value.quality
    assert {name for name,q in quality.items() if not q['passed']}=={'px','nx','pz','nz'}
    assert quality['nx']['residual_g']==pytest.approx(.15752,abs=.00002)
    assert quality['px']['model_direction_error_deg']==pytest.approx(4.68,abs=.02)
    class Link:
        def connect(self): return dict(app_mode=1,imu_ok=1,mag_present=1)
        def info(self,page=0): return {'coefficients':[0.0]*{3:6,4:12,5:18}.get(page,0)}
        def capture(self,face=0): return faces[sensor_check.FACES[face-1]]
    monkeypatch.setattr(sensor_check,'gyro_stage',lambda client,report:None)
    monkeypatch.setattr('builtins.input',lambda message:'')
    report={}
    with pytest.raises(ValueError,match='六面方向一致性未通过'):
        sensor_check.run_wizard(Link(),SimpleNamespace(stage='accel'),tmp_path,report)
    assert len(report['accel']['faces'])==6
    assert report['accel']['face_quality']['ny']['passed']
    assert (tmp_path/'六面统计.csv').is_file()
    assert not (tmp_path/'sensor_calibration.h').exists()
