import numpy as np
import pytest
import subprocess
import csv
from calibrate import fit_magnetometer, fit_accelerometer, export_header,read_samples
from protocol import ROOT,SCHEMA


def test_ellipsoid_recovers_hard_and_full_soft_iron():
    rng=np.random.default_rng(73)
    unit=rng.normal(size=(1800,3));unit/=np.linalg.norm(unit,axis=1)[:,None]
    distortion=np.array([[520,65,-20],[65,390,35],[-20,35,610]])
    expected_bias=np.array([130,-85,40])
    raw=unit@distortion.T+expected_bias+rng.normal(0,0.7,unit.shape)
    bias,matrix,quality=fit_magnetometer(raw)
    assert bias==pytest.approx(expected_bias,abs=0.4)
    assert np.linalg.norm(matrix-np.linalg.inv(distortion))<1e-5
    assert matrix==pytest.approx(matrix.T,abs=1e-10)
    assert quality["rms_relative"]<0.005


def test_ellipsoid_rejects_flat_and_insufficient_data():
    rng=np.random.default_rng(0)
    with pytest.raises(ValueError):
        fit_magnetometer(rng.normal(size=(999,3)))
    angle=np.linspace(0,2*np.pi,1500)
    with pytest.raises(ValueError,match="coverage"):
        fit_magnetometer(np.column_stack((500*np.cos(angle),500*np.sin(angle),np.zeros(1500))))


def test_accelerometer_six_faces_and_motion():
    bias=np.array([.03,-.02,.04]);expected_scale=np.array([1.02,.98,1.01]);faces={}
    for axis,label in enumerate("xyz"):
        for prefix,sign in (("p",1),("n",-1)):
            actual=np.zeros(3);actual[axis]=sign
            faces[prefix+label]=np.tile((actual/expected_scale+bias)*16384,(40,1))
    actual_bias,scale=fit_accelerometer(faces)
    assert actual_bias==pytest.approx(bias)
    assert scale==pytest.approx(expected_scale)
    faces["px"][0,0]+=16384
    with pytest.raises(ValueError,match="moved"):
        fit_accelerometer(faces)


def test_header_export_preserves_unrelated_and_handles_continuations(tmp_path,native):
    base=tmp_path/"base.h"
    text=(ROOT/"Core/Inc/sensor_calibration.h").read_text()
    text=text.replace("#define CAL_MAG_MATRIX ","#define CAL_MAG_MATRIX \\\n    ")
    base.write_text(text)
    output=tmp_path/"generated.h"
    export_header(base,output,{"CAL_MAG_VALID":"1","CAL_MAG_MATRIX":"{{1.0f,0,0},{0,1.0f,0},{0,0,1.0f}}"})
    result=output.read_text()
    assert "#define CAL_MAG_VALID 1" in result
    assert "#define CAL_IMU_TO_BODY" in result and "#define CAL_ACCEL_VALID 0" in result
    assert "#define CAL_MAG_MATRIX {{1.0f,0,0},{0,1.0f,0},{0,0,1.0f}}" in result
    checked=subprocess.run([native.host_compiler,"-std=c99","-fsyntax-only","-x","c",
                            "-include",str(output),"-"],input="float matrix[3][3]=CAL_MAG_MATRIX;\n",
                           text=True,capture_output=True,env=native.host_environment)
    assert checked.returncode==0,checked.stderr


def test_recalibration_uses_fresh_raw_not_the_old_fusion_model(tmp_path):
    source=tmp_path/"raw.csv"
    with source.open("w",newline="") as stream:
        writer=csv.DictWriter(stream,fieldnames=["flags","mag_sequence","mag_age_us","mx","my","mz"])
        writer.writeheader()
        row=dict(flags=SCHEMA["STATUS_MAG_PRESENT"]|SCHEMA["STATUS_MAG_REJECTED"],
                 mag_sequence=1,mag_age_us=1000,mx=100,my=200,mz=300)
        writer.writerow(row);writer.writerow(row)
        writer.writerow(dict(row,mag_sequence=2,mag_age_us=200000))
    assert read_samples(source,"mag").tolist()==[[100,200,300]]
