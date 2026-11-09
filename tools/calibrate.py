"""Offline native-frame accelerometer and magnetometer calibration."""
import argparse
import csv
import json
import re
import os
import sys
from pathlib import Path
# Anaconda invoked from ordinary PowerShell has not activated its DLL search path.
_dll_directory = None
if os.name == "nt" and (Path(sys.prefix)/"Library/bin").is_dir():
    _conda_bin = str(Path(sys.prefix)/"Library/bin")
    os.environ["PATH"] = _conda_bin + os.pathsep + os.environ.get("PATH", "")
    _dll_directory = os.add_dll_directory(_conda_bin)
import numpy as np
from scipy.optimize import least_squares
from protocol import ROOT, SCHEMA


def fit_magnetometer(samples):
    samples = np.asarray(samples, dtype=float)
    if samples.ndim != 2 or samples.shape[1] != 3 or len(samples) < 1000:
        raise ValueError("Need at least 1000 fresh 3D magnetic samples")
    if not np.isfinite(samples).all() or (samples == -4096).any():
        raise ValueError("Invalid/overflow magnetic samples")
    if len(np.unique(samples, axis=0)) < 300:
        raise ValueError("Too few distinct magnetic samples")
    eigenvalues = np.linalg.eigvalsh(np.cov(samples.T))
    if eigenvalues[0] <= 0 or eigenvalues[-1] / eigenvalues[0] > 100:
        raise ValueError("Poor 3D coverage: rotate around all axes, not just flat yaw")
    origin = np.median(samples, axis=0)
    scale = np.median(np.linalg.norm(samples-origin, axis=1))
    if scale < 1:
        raise ValueError("Magnetic signal too small")
    unit = (samples-origin)/scale

    def unpack(p):
        lower = np.array([[np.exp(p[3]), 0, 0],
                          [p[6], np.exp(p[4]), 0],
                          [p[7], p[8], np.exp(p[5])]])
        return p[:3], lower

    def residual(p):
        center, lower = unpack(p)
        return np.linalg.norm((unit-center) @ lower, axis=1)-1

    lower_bounds = [-5]*3 + [-5]*3 + [-10]*3
    upper_bounds = [5]*3 + [5]*3 + [10]*3
    solution = least_squares(residual, np.zeros(9), loss="soft_l1", f_scale=0.03,
                             bounds=(lower_bounds, upper_bounds), max_nfev=1500)
    if not solution.success:
        raise ValueError("Ellipsoid fit did not converge")
    center, lower = unpack(solution.x)
    # Symmetric positive root preserves sensor axes; a Cholesky factor itself
    # would introduce a spurious rotation into heading fusion.
    values, vectors = np.linalg.eigh(lower @ lower.T)
    if values[0] <= 0 or values[-1]/values[0] > 100:
        raise ValueError("Unreasonable soft-iron condition number")
    matrix = (vectors*np.sqrt(values)) @ vectors.T / scale
    bias = origin+scale*center
    corrected = (samples-bias) @ matrix.T
    norms = np.linalg.norm(corrected, axis=1)
    good = np.abs(norms-1) < 0.15
    rms = float(np.sqrt(np.mean((norms[good]-1)**2))) if good.any() else float("inf")
    if good.mean() < 0.90 or rms > 0.05:
        raise ValueError("Fit residual too large / too many magnetic outliers")
    octants = ((corrected[good] > 0)*np.array([1,2,4])).sum(axis=1)
    coverage = np.bincount(octants, minlength=8)
    if coverage.min() < 20:
        raise ValueError("Incomplete sphere coverage; collect all eight octants")
    return bias, matrix, {"samples": len(samples), "rms_relative": rms,
                          "inlier_fraction": float(good.mean()), "octants": coverage.tolist()}


def fit_accelerometer(faces):
    """Six named native sensor faces: px,nx,py,ny,pz,nz, >=30 stationary samples each."""
    bias = np.zeros(3)
    scale = np.ones(3)
    face_means = {}
    for name in ("px", "nx", "py", "ny", "pz", "nz"):
        values = np.asarray(faces[name], dtype=float)/16384
        if values.ndim != 2 or values.shape[1] != 3 or len(values) < 30 or not np.isfinite(values).all():
            raise ValueError(f"Face {name}: need >=30 finite stationary raw samples")
        if np.max(np.std(values, axis=0)) > 0.02:
            raise ValueError(f"Face {name}: sensor moved during capture")
        face_means[name] = np.mean(values, axis=0)
    return _fit_accel_means(face_means)


class AccelerometerFaceError(ValueError):
    def __init__(self, quality):
        self.quality = quality
        failed = [name for name, row in quality.items() if not row["passed"]]
        details = "; ".join(
            f"{name}: residual={quality[name]['residual_g']:.4f}g, "
            f"corrected={np.round(quality[name]['corrected_g'],4).tolist()}"
            for name in failed)
        super().__init__("misaligned face or cross-axis error; "+details)


def accelerometer_face_quality(face_means, bias, scale):
    """Diagnostic directions assume the diagonal calibration model; angular
    errors are NOT independent measurements of the fixture's actual tilt."""
    quality = {}
    for name, mean in face_means.items():
        expected = np.zeros(3)
        expected["xyz".index(name[1])] = 1 if name[0] == "p" else -1
        corrected = (mean-bias)*scale
        residual = float(np.linalg.norm(corrected-expected))
        angle = float(np.degrees(np.arccos(np.clip(np.dot(corrected,expected)/np.linalg.norm(corrected),-1,1))))
        quality[name] = {"raw_mean_g":np.asarray(mean).tolist(), "corrected_g":corrected.tolist(),
                         "expected_g":expected.tolist(), "residual_g":residual,
                         "model_direction_error_deg":angle, "passed":residual<=0.08}
    return quality


def _fit_accel_means(face_means):
    bias = np.zeros(3)
    scale = np.ones(3)
    for axis, label in enumerate("xyz"):
        positive = face_means["p"+label][axis]
        negative = face_means["n"+label][axis]
        if not 1.5 < positive-negative < 2.5:
            raise ValueError(f"Axis {label}: wrong face/sign/range")
        bias[axis] = (positive+negative)/2
        scale[axis] = 2/(positive-negative)
    quality = accelerometer_face_quality(face_means,bias,scale)
    if any(not row["passed"] for row in quality.values()):
        raise AccelerometerFaceError(quality)
    return bias, scale


def read_samples(path, sensor):
    with Path(path).open(encoding="utf-8-sig", newline="") as source:
        rows = list(csv.DictReader(source))
    values = []
    seen = set()
    for row in rows:
        flags = int(row.get("flags", "0"))
        if sensor == "mag":
            seq = int(row.get("mag_sequence", "0"))
            # The driver publishes raw data only after a valid complete read.
            # Fusion rejection may be caused by an OLD calibration; do not use
            # that model to censor the raw dataset for its replacement.
            if not flags & SCHEMA["STATUS_MAG_PRESENT"]:
                continue
            if int(row.get("mag_age_us", "4294967295")) > 100000:
                continue
            names = ("mx", "my", "mz")
        else:
            seq = int(row.get("sequence", "0"))
            if not flags & SCHEMA["STATUS_IMU_NEW"]:
                continue
            names = ("ax", "ay", "az")
        if seq == 0 or seq in seen:
            continue
        seen.add(seq)
        values.append([float(row[name]) for name in names])
    return np.asarray(values)


def c_vector(values):
    def number(value):
        text = format(float(value), ".9g")
        return text + ("" if "." in text or "e" in text else ".0") + "f"
    return "{" + ",".join(number(value) for value in values) + "}"


def export_header(base, output, replacements):
    text = Path(base).read_text(encoding="utf-8")
    for name, value in replacements.items():
        lines = text.splitlines(keepends=True)
        updated = []
        count = index = 0
        while index < len(lines):
            line = lines[index]
            if re.match(rf"^#define[ \t]+{name}[ \t]+", line):
                count += 1
                while line.rstrip().endswith("\\") and index + 1 < len(lines):
                    index += 1
                    line = lines[index]
                updated.append(f"#define {name} {value}\n")
            else:
                updated.append(line)
            index += 1
        text = "".join(updated)
        if count != 1:
            raise ValueError(f"Expected exactly one {name} in base header")
    if output is None:
        return text
    output = Path(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text, encoding="utf-8")
    return text


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("kind", choices=("mag", "accel"))
    parser.add_argument("--input", type=Path, help="Magnetic diagnostic CSV")
    parser.add_argument("--faces", type=Path, help="Directory containing px.csv ... nz.csv")
    parser.add_argument("--base", type=Path, default=ROOT/"Core/Inc/sensor_calibration.h")
    parser.add_argument("--output", type=Path, default=ROOT/"build/calibration/sensor_calibration.h")
    args = parser.parse_args()
    try:
        if args.kind == "mag":
            if not args.input:
                parser.error("mag requires --input")
            bias, matrix, quality = fit_magnetometer(read_samples(args.input, "mag"))
            changes = {"CAL_MAG_VALID": "1", "CAL_MAG_BIAS_RAW": c_vector(bias),
                       "CAL_MAG_MATRIX": "{"+",".join(c_vector(row) for row in matrix)+"}"}
        else:
            if not args.faces:
                parser.error("accel requires --faces")
            faces = {name: read_samples(args.faces/(name+".csv"), "accel")
                     for name in ("px","nx","py","ny","pz","nz")}
            bias, scale = fit_accelerometer(faces)
            changes = {"CAL_ACCEL_VALID": "1", "CAL_ACCEL_BIAS_G": c_vector(bias),
                       "CAL_ACCEL_SCALE": c_vector(scale)}
            quality = {"bias_g": bias.tolist(), "scale": scale.tolist()}
        export_header(args.base, args.output, changes)
        print(json.dumps({"header": str(args.output), "quality": quality}, ensure_ascii=False, indent=2))
    except (ValueError, OSError) as error:
        parser.exit(1, f"Calibration refused: {error}\n")


if __name__ == "__main__":
    main()


def fit_accelerometer_statistics(faces):
    """Native LSB Welford summaries of >=1000 new samples per face."""
    means = {}
    for name in ("px","nx","py","ny","pz","nz"):
        if name not in faces:
            raise ValueError(f"Missing face {name}")
        row = faces[name]
        mean, std = np.asarray(row["mean_raw"],float), np.asarray(row["std_raw"],float)
        if row["count"] < 1000 or row["state"] != SCHEMA["CAL_DONE"]:
            raise ValueError(f"Face {name}: incomplete local capture")
        if mean.shape != (6,) or std.shape != (6,) or not np.isfinite([mean,std]).all() or (std<0).any():
            raise ValueError(f"Face {name}: invalid statistics")
        if std[:3].max()/16384 > 0.02 or std[3:].max()/65.5 > 0.2:
            raise ValueError(f"Face {name}: sensor moved/noisy")
        means[name] = mean[:3]/16384
    return _fit_accel_means(means)


def validate_magnetometer(samples,bias,matrix):
    samples=np.asarray(samples,float); bias=np.asarray(bias,float); matrix=np.asarray(matrix,float)
    if samples.ndim!=2 or samples.shape[1]!=3 or len(samples)<300 or not np.isfinite(samples).all() or (samples==-4096).any():
        raise ValueError("Independent magnetic validation needs >=300 valid raw points")
    if bias.shape!=(3,) or matrix.shape!=(3,3) or not np.isfinite(bias).all() or not np.isfinite(matrix).all():
        raise ValueError("Invalid magnetic coefficients")
    if not np.allclose(matrix,matrix.T,atol=1e-8) or np.linalg.eigvalsh(matrix).min()<=0:
        raise ValueError("Magnetic matrix must be symmetric positive definite")
    cov=np.linalg.eigvalsh(np.cov(samples.T))
    if cov[0]<=0 or cov[-1]/cov[0]>100:
        raise ValueError("Independent magnetic validation has poor 3D coverage")
    corrected=(samples-bias)@matrix.T
    norms=np.linalg.norm(corrected,axis=1); good=np.abs(norms-1)<0.15
    rms=float(np.sqrt(np.mean((norms[good]-1)**2))) if good.any() else float("inf")
    octants=((corrected[good]>0)*[1,2,4]).sum(axis=1)
    coverage=np.bincount(octants,minlength=8)
    if good.mean()<0.9 or rms>0.05 or coverage.min()<5:
        raise ValueError("Independent magnetic residual/outlier/coverage check failed")
    return {"samples":len(samples),"rms_relative":rms,"inlier_fraction":float(good.mean()),
            "max_norm_error":float(np.max(np.abs(norms-1))),"octants":coverage.tolist()}
