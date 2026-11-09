import copy
import json
from types import SimpleNamespace
import numpy as np
import pytest
import sensor_check as check
from protocol import ROOT, SCHEMA as S


def window(face, vector, job=None):
    job=face+2 if job is None else job
    return dict(op=S['CAL_STATIC'],face=face,state=S['CAL_DONE'],request_id=job,
                reason=0,last_reason=0,count=1000,target=1000,resets=0,
                first_seq=job*2000,last_seq=job*2000+999,duration_us=4995000,dt_bad=0,
                temperature_c=28,mean_raw=(np.asarray(vector,dtype=float)*16384).tolist()+[52,-31,18],std_raw=[1]*6)


@pytest.fixture
def saved_session(tmp_path):
    source=tmp_path/'original';source.mkdir()
    faces={}
    for i,name in enumerate(check.FACES):
        vec=np.zeros(3);vec[i//2]=1 if i%2==0 else -1
        faces[name]=window(i+1,vec)
    rng=np.random.default_rng(3);directions=rng.normal(size=(12,3))
    directions/=np.linalg.norm(directions,axis=1)[:,None]
    poses=[window(i+7,v) for i,v in enumerate(directions)]
    coefficients,flags=check.header_coefficients(ROOT/'Core/Inc/sensor_calibration.h')
    independent=window(0,[0,0,1],job=2)
    bias=(np.array(independent['mean_raw'][3:])/65.5).tolist()
    gyro=dict(independent=independent,bias_dps=bias,validation=check.check_gyro(independent,bias),status='passed')
    firmware=dict(app_mode=1,imu_ok=1,mag_present=1,imu_hz=200,cal_flags=flags,
                  firmware_magic=0x53434131,uptime_ms=1000,last_job_id=15)
    report=dict(stage='accel',firmware=firmware,initial_coefficients=coefficients,
                gyro=gyro,accel={'faces':faces,'status':'尚未验证'},error='noise timeout')
    (source/'session.json').write_text(json.dumps(report),encoding='utf-8')
    # Simulate the user's old version: six completed poses exist ONLY in CSV.
    rows={f'face{i}':r for i,r in enumerate(list(faces.values())+poses[:6])}
    partial=copy.deepcopy(poses[6]);partial.update(state=S['CAL_FAILED'],count=51,duration_us=0)
    rows['unfinished']=partial
    check.save_statistics(source/'采样统计.csv',rows)
    return source,report,poses


def test_load_old_csv_restores_six_faces_six_poses_and_rejects_half_window(saved_session):
    source,report,poses=saved_session
    resumed=check.load_resume_checkpoint(source)
    assert set(resumed['faces'])==set(check.FACES)
    assert [r['face'] for r in resumed['poses']]==list(range(7,13))
    assert resumed['discarded_windows'][0]['face']==13
    info={**report['firmware'],'uptime_ms':200000,'last_job_id':16}
    imu={'new_samples':50000};gyro={'ready':True,'bias_dps':report['gyro']['bias_dps']}
    assert check.can_reuse_gyro(resumed,info,imu,gyro)
    assert not check.can_reuse_gyro(resumed,info,imu,{**gyro,'ready':False})


def test_resume_runs_only_remaining_poses_and_exports_after_full_validation(saved_session,tmp_path,monkeypatch):
    source,old,poses=saved_session
    output=tmp_path/'continued';output.mkdir()
    class Link:
        def __init__(self):self.statistics=[];self.calls=[]
        def connect(self):return {**old['firmware'],'uptime_ms':200000,'last_job_id':16}
        def info(self,page=0):
            if page in (3,4,5):return {'coefficients':old['initial_coefficients'][str(page)]}
            if page==6:return {'ready':True,'bias_dps':old['gyro']['bias_dps']}
            if page==1:return {'new_samples':50000}
            return self.connect()
        def capture(self,face=0,gyro=False):
            assert not gyro and face>=13
            self.calls.append(face);row=copy.deepcopy(poses[face-7])
            self.statistics.append((f'new{face}',row));return row
    client=Link();prompts=[]
    monkeypatch.setattr('builtins.input',lambda text:prompts.append(text) or '')
    monkeypatch.setattr(check,'gyro_stage',lambda *args:pytest.fail('must reuse unchanged gyro'))
    result={'stage':'accel','status':'进行中'}
    args=SimpleNamespace(stage='accel',resume=source,base=ROOT/'Core/Inc/sensor_calibration.h')
    check.run_wizard(client,args,output,result)
    assert client.calls==list(range(13,19))
    assert '7/12' in prompts[0]
    assert len(result['accel']['poses'])==12
    assert result['accel']['validation']['passed']
    assert (output/'sensor_calibration.h').exists()
    assert (output/'session.json').is_file()  # written after every complete pose
    assert not (output/'session.json.tmp').exists()
    assert (source/'session.json').read_text(encoding='utf-8')==json.dumps(old)


def test_noise_timeout_retries_current_pose_without_losing_previous_six(saved_session,monkeypatch):
    _,_,poses=saved_session
    class Link:
        def __init__(self):self.calls=[]
        def capture(self,face=0):
            self.calls.append(face)
            if len(self.calls)==1:
                raise check.CaptureFailure(dict(reason=S['CAL_TIMEOUT'],last_reason=S['CAL_NOISE']))
            return poses[face-7]
    client=Link();prompts=[];progress=[]
    monkeypatch.setattr('builtins.input',lambda text:prompts.append(text) or '')
    result=check.acquire_poses(client,poses[:6],lambda rows:progress.append(copy.deepcopy(rows)))
    assert client.calls==[13,13,14,15,16,17,18]
    assert [len(rows) for rows in progress]==list(range(7,13))
    assert result[:6]==poses[:6]
    assert any('只重试当前姿态' in text for text in prompts)


def test_i2c_error_is_not_automatically_retried(monkeypatch):
    class Link:
        def capture(self,face=0):
            raise check.CaptureFailure(dict(reason=S['CAL_IO_ERROR'],last_reason=0))
    monkeypatch.setattr('builtins.input',lambda *args:pytest.fail('I2C errors must stop'))
    with pytest.raises(check.CaptureFailure,match='I2C'):
        check.capture_with_retry(Link(),face=13)


def test_resume_rejects_changed_compiled_coefficients(saved_session,tmp_path):
    source,old,_=saved_session
    class Link:
        statistics=[]
        def connect(self):return old['firmware']
        def info(self,page=0):
            if page in (3,4,5):
                values=copy.deepcopy(old['initial_coefficients'][str(page)])
                if page==3:values[0]+=.1
                return {'coefficients':values}
            return {'new_samples':50000}
    output=tmp_path/'mismatched';output.mkdir()
    with pytest.raises(ValueError,match='参数'):
        check.run_wizard(Link(),SimpleNamespace(stage='accel',resume=source),output,{})
    assert not (output/'sensor_calibration.h').exists()


def test_post_flash_verification_resumes_only_remaining_poses(saved_session,tmp_path,monkeypatch):
    _,old,poses=saved_session
    interrupted_dir=tmp_path/'verify_interrupted';interrupted_dir.mkdir()
    firmware={**old['firmware'],'cal_flags':old['firmware']['cal_flags']|1}
    interrupted={**old,'stage':'verify','firmware':firmware}
    interrupted.pop('accel')
    (interrupted_dir/'session.json').write_text(json.dumps(interrupted),encoding='utf-8')
    check.save_statistics(interrupted_dir/'采样统计.csv',{f'pose{i}':r for i,r in enumerate(poses[:6])})
    candidate=tmp_path/'candidate';candidate.mkdir()
    previous={'candidate_coefficients':old['initial_coefficients'],
              'candidate_flags':firmware['cal_flags'],'accel':{'validation':{'passed':True}}}
    (candidate/'session.json').write_text(json.dumps(previous),encoding='utf-8')
    output=tmp_path/'continued_verify';output.mkdir()
    class Link:
        def __init__(self):self.statistics=[];self.calls=[]
        def connect(self):return {**firmware,'uptime_ms':200000,'last_job_id':16}
        def info(self,page=0):
            if page in (3,4,5):return {'coefficients':old['initial_coefficients'][str(page)]}
            if page==6:return {'ready':True,'bias_dps':old['gyro']['bias_dps']}
            if page==1:return {'new_samples':50000}
            return self.connect()
        def capture(self,face=0,gyro=False):
            assert not gyro and face>=13
            self.calls.append(face);row=copy.deepcopy(poses[face-7])
            self.statistics.append((f'new{face}',row));return row
    client=Link();prompts=[]
    monkeypatch.setattr('builtins.input',lambda text:prompts.append(text) or '')
    monkeypatch.setattr(check,'gyro_stage',lambda *args:pytest.fail('unchanged gyro should be reused'))
    args=SimpleNamespace(stage='verify',resume=interrupted_dir,verify_from=candidate)
    result={'stage':'verify','status':'进行中'}
    check.run_wizard(client,args,output,result)
    assert client.calls==list(range(13,19)) and '7/12' in prompts[0]
    assert result['post_flash']['checks']['accel']['passed']
    assert result['post_flash']['status']=='板上编译参数一致，独立复查通过'
    assert not (output/'sensor_calibration.h').exists()
