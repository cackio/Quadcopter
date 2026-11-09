"""GY-86 中文引导：板上 200 Hz 静态统计、独立验证、磁场拟合与烧录复查。"""
import argparse
import copy
import csv
import json
import struct
import sys
import time
from collections import Counter, deque
from pathlib import Path
from calibrate import (np, fit_accelerometer_statistics, fit_magnetometer,
                       validate_magnetometer, export_header, c_vector, AccelerometerFaceError)
from protocol import (ROOT, SCHEMA as S, Parser, pack_command,
                      decode_diagnostic, decode_cal_reply)

FACES = ("px", "nx", "py", "ny", "pz", "nz")
REASONS = ("正常", "移动，重新等待静止", "原始加速度模长异常", "噪声过大",
           "MPU I2C 读取失败", "连续两秒没有新样本", "30秒内未完成，查看最后重启原因",
           "有效样本间隔超过20ms", "命令参数错误", "另一个采样任务正在运行",
           "请求编号过期或板子已复位", "当前固件不是独立校准模式")
STATES = ("空闲", "等待静止", "采集中", "完成", "失败", "取消")


class CaptureFailure(RuntimeError):
    def __init__(self, reply):
        self.reply=reply
        message=reason_text(reply["reason"])
        if reply.get("last_reason",0):
            message+="；最后原因："+reason_text(reply["last_reason"])
        super().__init__(message)


def reason_text(code):
    return REASONS[code] if 0 <= code < len(REASONS) else f"未知原因 {code}"


def write_csv(path, rows):
    if not rows:
        return
    fields = list(rows[0])
    path=Path(path)
    temporary=path.with_name(path.name+".tmp")
    with temporary.open("w", newline="", encoding="utf-8-sig") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)
    temporary.replace(path)


def save_statistics(path, jobs):
    flat = []
    for name, row in jobs.items():
        item = {"label": name, **{k: v for k, v in row.items() if k not in ("mean_raw", "std_raw")}}
        for field in ("mean_raw", "std_raw"):
            item.update({f"{field}_{axis}": value for axis, value in zip(("ax","ay","az","gx","gy","gz"), row[field])})
        flat.append(item)
    write_csv(path, flat)


def check_gyro(row, bias):
    mean = np.asarray(row["mean_raw"], float)[3:]/65.5 - np.asarray(bias,float)
    std = np.asarray(row["std_raw"], float)[3:]/65.5
    if row["count"] < 1000 or not np.isfinite([mean,std]).all() or np.any(std<0):
        raise ValueError("陀螺仪独立验证样本不足或数值异常")
    result = {"mean_dps": mean.tolist(), "std_dps": std.tolist(),
              "temperature_c": row["temperature_c"], "passed": bool((np.abs(mean)<=0.1).all() and (std<=0.2).all())}
    if not result["passed"]:
        raise ValueError("陀螺仪独立验证未通过："+json.dumps(result,ensure_ascii=False))
    return result


def check_accel(poses, bias, scale):
    if len(poses)<12:
        raise ValueError("需要12个独立静止倾角")
    norms, noises = [], []
    for row in poses:
        if row["state"]!=S["CAL_DONE"] or row["count"]<1000:
            raise ValueError("独立倾角采样未完成")
        value = (np.asarray(row["mean_raw"][:3])/16384-bias)*scale
        noise = np.asarray(row["std_raw"][:3])/16384*np.abs(scale)
        if not np.isfinite([value,noise]).all() or (noise<0).any() or noise.max()>0.02:
            raise ValueError("独立倾角采样不稳定")
        norms.append(float(np.linalg.norm(value))); noises.append(noise.tolist())
    means = np.asarray([row["mean_raw"][:3] for row in poses],float)
    directions = means/np.linalg.norm(means,axis=1)[:,None]
    # Independent poses must actually span 3D; 12 copies of one pose are not evidence.
    eigen = np.linalg.eigvalsh(np.cov(directions.T))
    if eigen[0]<0.02 or eigen[-1]/eigen[0]>30:
        raise ValueError("12个倾角覆盖不足：请改变各轴方向，不要只重复平放")
    out = {"norms_g":norms,"mean_norm_g":float(np.mean(norms)),
           "max_norm_error_g":float(np.max(np.abs(np.asarray(norms)-1))),"std_g":noises,
           "passed":all(0.98<=v<=1.02 for v in norms)}
    if not out["passed"]:
        raise ValueError("加速度独立验证未达到0.98–1.02g："+json.dumps(out,ensure_ascii=False))
    return out


def mapping_from_directions(directions):
    """Each native positive axis points to one FLU direction."""
    options = {"前": [1,0,0], "后": [-1,0,0], "左": [0,1,0],
               "右": [0,-1,0], "上": [0,0,1], "下": [0,0,-1]}
    if len(directions)!=3 or any(d not in options for d in directions):
        raise ValueError("每个轴只能输入：前、后、左、右、上、下")
    matrix = np.asarray([options[d] for d in directions],float).T
    if not np.allclose(matrix@matrix.T,np.eye(3)) or np.linalg.det(matrix)<0.99:
        raise ValueError("轴映射不是正交右手系，请重新核对模块轴标记")
    return matrix


class Client:
    """One outstanding request. Record received bytes before parsing, never infer
    200 Hz timing from the downsampled F1 rows. Requests can safely be retried."""
    def __init__(self, port, binary):
        self.port, self.binary = port, binary
        self.parser = Parser()
        self.replies = deque(maxlen=64)
        self.rows = []
        self.request_id = 0
        self.active_job = None
        self.statistics = []
        self.received_bytes = 0
        self.sent_bytes = 0
        self.frame_counts = Counter()
        self.first_received = bytearray()
        self.last_command_hex = ""

    def pump(self):
        chunk = self.port.read(self.port.in_waiting or 1)
        if not chunk:
            return
        self.binary.write(chunk)
        self.binary.flush()
        self.received_bytes += len(chunk)
        self.first_received.extend(chunk[:max(0,64-len(self.first_received))])
        for frame_id,payload in self.parser.feed(chunk):
            self.frame_counts[f"{frame_id:02X}"] += 1
            if frame_id==S["SCAL_REPLY_ID"]:
                self.replies.append(decode_cal_reply(payload))
            elif frame_id==S["DIAG_FRAME_ID"]:
                self.rows.append(decode_diagnostic(payload))

    def request(self, op, request_id, face=0, allow_failed=False):
        command = pack_command(op,request_id,face)
        self.replies.clear()
        for _ in range(3):
            self.port.write(command)
            self.sent_bytes += len(command)
            self.last_command_hex = command.hex(" ")
            deadline=time.monotonic()+1.5
            while time.monotonic()<deadline:
                self.pump()
                while self.replies:
                    row=self.replies.popleft()
                    if row["request_id"]!=request_id:
                        continue
                    if op==S["CAL_INFO"] and (row["op"]!=op or row["face"]!=face):
                        continue
                    if row["state"]==S["CAL_FAILED"] and not allow_failed:
                        raise CaptureFailure(row)
                    return row
        raise TimeoutError(self.timeout_message())

    def link_summary(self):
        return {"sent_bytes":self.sent_bytes,"received_bytes":self.received_bytes,
                "frames":dict(self.frame_counts),"first_received_hex":self.first_received.hex(" "),
                "last_command_hex":self.last_command_hex}

    def timeout_message(self):
        if not self.received_bytes:
            detail="一个字节都没有收到；先确认烧录后已复位并运行、COM及模块供电，不要先排查零偏或姿态算法"
        elif self.frame_counts.get("F1"):
            detail="已收到F1原始诊断，但没有匹配应答；检查JDY TXD→PA10及板上接收，也可能仍运行不支持F2/F3的旧固件"
        elif self.frame_counts.get("03") or self.frame_counts.get("04"):
            detail="已收到姿态帧，但没有匹配应答；核对实际烧录版本和反向接收线路"
        elif not self.frame_counts:
            detail="收到字节但没有有效协议帧；核对模块实际UART波特率及是否混入文本日志"
        else:
            detail="收到其他帧但没有匹配应答；查看通信记录的帧类型和请求编号"
        return (f"未收到命令应答：发送{self.sent_bytes}字节，接收{self.received_bytes}字节，"
                f"有效帧={dict(self.frame_counts)}。{detail}")

    def info(self, page=0):
        return self.request(S["CAL_INFO"],0,page)

    def connect(self):
        info=self.info()
        if info.get("firmware_magic")!=0x53434131:
            raise RuntimeError("固件协议不匹配，请烧录新校准固件")
        self.request_id=info["last_job_id"]
        if self.request_id:
            retained=self.request(S["CAL_CANCEL"],self.request_id,allow_failed=True)
            print("上一会话任务：",STATES[retained["state"]],"；本次采样使用新编号，不拼接上次未完成窗口")
        return info

    def capture(self, gyro=False, face=0):
        self.request_id=(self.request_id+1)&0xffffffff or 1
        job=self.request_id
        self.active_job=job
        try:
            row=self.request(S["CAL_GYRO"] if gyro else S["CAL_STATIC"],job,face)
            deadline=time.monotonic()+36
            while row["state"] not in (S["CAL_DONE"],S["CAL_CANCELLED"]):
                if time.monotonic()>deadline:
                    raise TimeoutError("板上采样未按时结束；请查询传感器错误和供电")
                print(f"\r{STATES[row['state']]} {row['count']}/{row['target']}  重启{row['resets']}次：{reason_text(row['last_reason'])}        ",end="",flush=True)
                until=time.monotonic()+0.8
                while time.monotonic()<until:
                    self.pump()
                row=self.request(S["CAL_STATUS"],job)
            print()
            if row["state"]!=S["CAL_DONE"]:
                raise RuntimeError("采样已取消")
            print(f"完成 {row['count']}/{row['target']}，重启 {row['resets']} 次")
            self.statistics.append((f"job_{job}_{'gyro' if gyro else 'static'}_{face}",row.copy()))
            hz=(row["count"]-1)*1e6/max(row["duration_us"],1)
            if not 180<=hz<=220 or row["dt_bad"]>60:
                raise RuntimeError(f"板上周期未达标：有效采样率 {hz:.1f} Hz，异常间隔 {row['dt_bad']} 次；先检查 TIM4 和 I2C 耗时")
            return row
        finally:
            # On disconnect/timeout, best effort cancel; a new connection must
            # explicitly query the retained job rather than combining sessions.
            self.active_job=None

    def observe(self, seconds, show=True):
        start=len(self.rows); deadline=time.monotonic()+seconds; display=0
        while time.monotonic()<deadline:
            self.pump()
            if show and self.rows and time.monotonic()>=display:
                r=self.rows[-1]
                a=[round(r[k]/16384,3) for k in ("ax","ay","az")]
                g=[round(r[k]/65.5,3) for k in ("gx","gy","gz")]
                print(f"RAW seq={r['sequence']} a(g)={a} gyro(°/s)={g} mag={[r[k] for k in ('mx','my','mz')]} err={r['i2c_errors']}")
                display=time.monotonic()+1
        return self.rows[start:]


def fresh_magnetic(rows):
    values=[]; seen=set()
    for r in rows:
        seq=r["mag_sequence"]
        if not seq or seq in seen or not r["flags"]&S["STATUS_MAG_PRESENT"] or r["mag_age_us"]>100000:
            continue
        seen.add(seq)
        value=[r[k] for k in ("mx","my","mz")]
        if -4096 not in value:
            values.append(value)
    return np.asarray(values,float)


def get_coefficients(client):
    return {str(p):client.info(p)["coefficients"] for p in (3,4,5)}


def collect_magnetic(client,seconds):
    # Serial FIFO ordering makes INFO an acknowledgement barrier. Fence by the
    # actual fresh mag counter as well, so buffered training samples cannot be
    # counted as independent validation data after an input() pause.
    before=client.info(); marker=client.info(2)["new_samples"]
    rows=client.observe(seconds,show=False)
    after=client.info()
    previous=None
    for row in rows:
        if previous is not None and ((row["sequence"]-previous)&0xffffffff)>=0x80000000:
            raise RuntimeError("采样序号倒退，疑似板子复位，不能合并磁场数据")
        previous=row["sequence"]
    if (after["uptime_ms"]<before["uptime_ms"] or
        after["imu_errors"]!=before["imu_errors"] or
        after["mag_errors"]!=before["mag_errors"]):
        raise RuntimeError("采集期间板子复位或 I2C 错误增长，本次磁场数据作废")
    return [r for r in rows if 0<((r["mag_sequence"]-marker)&0xffffffff)<0x80000000]


def compare_coefficients(actual, expected):
    for page,values in expected.items():
        if len(actual[page])!=len(values) or not np.allclose(actual[page],values,rtol=2e-6,atol=1e-8):
            raise ValueError(f"板上参数第{page}页与候选参数不一致，请重新编译烧录")


def valid_saved_window(row):
    """Restore only complete, finite, stationary, correctly timed RAW windows."""
    try:
        mean=np.asarray(row["mean_raw"],float); std=np.asarray(row["std_raw"],float)
        count=int(row["count"]); duration=int(row["duration_us"])
        span=(int(row["last_seq"])-int(row["first_seq"]))&0xffffffff
        return (row["op"]==S["CAL_STATIC"] and row["state"]==S["CAL_DONE"] and
                count==1000 and row["target"]==1000 and duration>0 and
                180<=(count-1)*1e6/duration<=220 and row["dt_bad"]<=60 and
                count-1<=span<0x80000000 and mean.shape==(6,) and std.shape==(6,) and
                np.isfinite([mean,std]).all() and (std>=0).all() and
                std[:3].max()/16384<=0.020001 and std[3:].max()/65.5<=0.200001)
    except (KeyError,ValueError,TypeError):
        return False


def load_resume_checkpoint(folder, stage="accel"):
    folder=Path(folder)
    source=json.loads((folder/"session.json").read_text(encoding="utf-8"))
    if stage not in ("accel","verify") or source.get("stage")!=stage:
        raise ValueError("续采阶段须与原会话的 accel 或 verify 阶段一致")
    if not all(k in source.get("firmware",{}) for k in ("imu_hz","cal_flags","firmware_magic","uptime_ms")):
        raise ValueError("原会话缺少完整固件配置，不能确认续采条件")
    if not all(k in source.get("initial_coefficients",{}) for k in ("3","4","5")):
        raise ValueError("原会话缺少固件参数信息，不能确认采样配置一致")
    faces={};poses={};discarded=[]
    def accept(row):
        face=int(row.get("face",0))
        if not 1<=face<=18:
            return
        if not valid_saved_window(row):
            discarded.append({"face":face,"request_id":row.get("request_id"),"reason":"窗口不完整、噪声或周期不合格"})
            return
        if face<=6:
            faces[FACES[face-1]]=copy.deepcopy(row)
        else:
            poses[face]=copy.deepcopy(row)
    # Old versions saved completed poses only in the final statistics CSV.
    for row in source.get("accel",{}).get("faces",{}).values(): accept(row)
    for row in source.get("accel",{}).get("poses",[]): accept(row)
    for row in source.get("post_flash",{}).get("checks",{}).get("accel_poses",[]): accept(row)
    csv_file=folder/"采样统计.csv"
    if csv_file.is_file():
        with csv_file.open(encoding="utf-8-sig",newline="") as stream:
            for raw in csv.DictReader(stream):
                try:
                    row={key:int(raw[key]) for key in ("op","face","state","request_id","reason","last_reason",
                         "count","target","resets","first_seq","last_seq","duration_us","dt_bad")}
                    row["temperature_c"]=float(raw["temperature_c"])
                    for field in ("mean_raw","std_raw"):
                        row[field]=[float(raw[field+"_"+axis]) for axis in ("ax","ay","az","gx","gy","gz")]
                    accept(row)
                except (KeyError,ValueError,TypeError):
                    raise ValueError("原采样统计CSV字段缺失或损坏，未恢复不完整记录")
    return {"source":source,"faces":faces,"poses":[poses[k] for k in sorted(poses)],
            "discarded_windows":discarded,"folder":str(folder.resolve())}


def can_reuse_gyro(resume, info, imu, gyro):
    old=resume["source"].get("gyro",{})
    if not old or not gyro.get("ready"):
        return False
    try:
        check_gyro(old["independent"],old["bias_dps"])
        # Anchor to the retained gyro epoch, not older restored accel windows
        # which may legitimately come from a previous boot.
        last_id=int(old["independent"]["request_id"])
        last_seq=int(old["independent"]["last_seq"])
        return (np.allclose(gyro["bias_dps"],old["bias_dps"],rtol=0,atol=1e-6) and
                info["uptime_ms"]>=resume["source"]["firmware"]["uptime_ms"] and
                info["last_job_id"]>=last_id and imu["new_samples"]>=last_seq)
    except (KeyError,ValueError,TypeError):
        return False


def capture_with_retry(client, gyro=False, face=0):
    while True:
        try:
            return client.capture(gyro=True,face=face) if gyro else client.capture(face=face)
        except CaptureFailure as error:
            # Retrying starts a NEW request/window. Never join partial samples,
            # relax thresholds, or automatically recover an I2C failure.
            if (error.reply["reason"]!=S["CAL_TIMEOUT"] or
                error.reply.get("last_reason") not in (S["CAL_NOISE"],S["CAL_MOTION"],S["CAL_ACCEL_RANGE"])):
                raise
            print("当前窗口未完成：",error,"。前面完成的数据已保留。")
            choice=input("调整夹具、松开手并放稳；回车只重试当前姿态，输入 q 保存退出：").strip().lower()
            if choice in ("q","quit","exit"):
                raise


def acquire_poses(client, existing=None, checkpoint=None):
    poses={int(row["face"]):copy.deepcopy(row) for row in (existing or [])}
    if any(face<7 or face>18 for face in poses):
        raise ValueError("独立姿态编号无效，不能续采")
    for i in range(12):
        if 7+i in poses:
            print(f"独立验证 {i+1}/12：已恢复完整1000点，跳过采集")
            continue
        input(f"独立验证 {i+1}/12：换一个新的静止倾角，覆盖朝上/朝下及左右前后倾斜；放稳后回车：")
        poses[7+i]=capture_with_retry(client,face=7+i)
        if checkpoint:
            checkpoint([poses[face] for face in sorted(poses)])
    return [poses[face] for face in sorted(poses)]


def gyro_stage(client, report):
    info=client.info()
    wait=max(0,30-info["uptime_ms"]/1000)
    if wait:
        print(f"预热还需约 {wait:.0f} 秒；这不是完整温度补偿，请保持供电和静止")
        client.observe(wait,show=False)
    input("把机架静止放在稳固桌面，保持不动；回车开始零偏校准：")
    fit=capture_with_retry(client,gyro=True)
    info=client.info(6)
    if not info["ready"]:
        raise RuntimeError("固件未标记陀螺仪零偏有效")
    print("本次零偏 °/s：",info["bias_dps"])
    independent=capture_with_retry(client)
    result=check_gyro(independent,info["bias_dps"])
    report["gyro"]={"fit":fit,"independent":independent,"bias_dps":info["bias_dps"],"validation":result,
                    "status":"独立验证通过；零偏仅在本次运行有效"}
    return report["gyro"]


def ask_mapping(client, report, changes):
    # Preserve prior confirmations only if compiled parameters match the base;
    # the wizard never derives magnetometer mapping from an ellipsoid.
    for sensor,prefix in (("MPU6050","IMU"),("磁力计","MAG")):
        text=input(f"{sensor}安装方向已核对时，输入原生+X/+Y/+Z指向（例：前 右 下）；不确定直接回车：").strip()
        if not text:
            report.setdefault("mapping",{})[prefix]="未确认，保持原矩阵并清除确认标志；不能据此验收九轴"
            changes[f"CAL_{prefix}_AXES_CONFIRMED"]="0"
            continue
        matrix=mapping_from_directions(text.split())
        changes[f"CAL_{prefix}_TO_BODY"]="{"+",".join(c_vector(v) for v in matrix)+"}"
        changes[f"CAL_{prefix}_AXES_CONFIRMED"]="1"
        report.setdefault("mapping",{})[prefix]=matrix.tolist()


def render_report(folder, report):
    def safe(value):
        if isinstance(value,float) and not np.isfinite(value):
            return str(value)
        if isinstance(value,dict):
            return {k:safe(v) for k,v in value.items()}
        if isinstance(value,list):
            return [safe(v) for v in value]
        return value
    temporary=folder/"session.json.tmp"
    temporary.write_text(json.dumps(safe(report),ensure_ascii=False,indent=2,allow_nan=False),encoding="utf-8")
    temporary.replace(folder/"session.json")
    lines=["# GY-86 传感器校准与验收记录","",f"总体状态：{report.get('status','进行中')}","",
           "数据来自本次串口采集；软件自动测试结果不能替代该硬件记录。","",
           "| 项目 | 状态 |","|---|---|"]
    for name in ("raw","gyro","accel","mag","mapping","post_flash"):
        if name in report:
            item=report[name]
            lines.append(f"| {name} | {item.get('status','见 session.json 详情')} |")
    if report.get("error"):
        lines.extend(["", "失败原因："+report["error"]])
    lines.extend(["","完整均值、噪声、独立残差、覆盖和板上参数见 session.json；原始接收字节见 session.bin。",
                  "","模长正确不代表轴方向正确；磁场椭球校准不能确定安装方向或真北。"])
    (folder/"验收报告.md").write_text("\n".join(lines)+"\n",encoding="utf-8")


def raw_stage(client, seconds):
    before=client.info()
    print("先静止观察，再分别转动三个轴；这里显示 RAW 数据，不运行姿态融合。")
    rows=client.observe(seconds)
    after=client.info()
    count=(after["dt_total"]-before["dt_total"])&0xffffffff
    band=(after["dt_in_band"]-before["dt_in_band"])&0xffffffff
    elapsed=(after["uptime_ms"]-before["uptime_ms"])&0xffffffff
    if not rows or not count:
        raise RuntimeError("没有有效 MPU 新样本，请查看身份、读取和数据就绪诊断")
    if after["imu_errors"]!=before["imu_errors"]:
        raise RuntimeError("原始检查期间 MPU I2C 错误增长")
    rate=count*1000/max(elapsed,1)
    if not 180<=rate<=220 or band/count<0.95:
        raise RuntimeError(f"原始采样周期未达标：{rate:.1f}Hz，4.5–5.5ms 占比 {band/count:.1%}；先检查调度和 I2C")
    return {"status":"原始数据已采集，轴方向需人工核对", "full_rate_hz":count*1000/max(elapsed,1),
            "period_in_band_fraction":band/count,"before":before,"after":after,"display_rows":len(rows)}


def run_wizard(client, args, folder, report):
    info=client.connect(); report["firmware"]=info
    report["drivers"]={"imu":client.info(1),"mag":client.info(2)}
    report["initial_coefficients"]=get_coefficients(client)
    print(json.dumps(report["drivers"],ensure_ascii=False,indent=2))
    if args.stage=="link":
        report["status"]="双向通信及固件信息查询通过；传感器结果见drivers，尚未校准"
        print("双向通信已通过，当前 app_mode=",info["app_mode"],"（1为独立校准，0为姿态）")
        return
    if info["app_mode"]!=1:
        raise RuntimeError("请烧录 FLIGHT_APP_MODE=1 的独立校准固件；普通姿态固件只支持信息查询")
    if not info["imu_ok"]:
        raise RuntimeError("MPU 初始化失败：上述 stage/register/expected/actual/hal_error 是具体原因")
    resume=None
    if getattr(args,"resume",None):
        resume=load_resume_checkpoint(args.resume,args.stage)
        compare_coefficients(report["initial_coefficients"],resume["source"]["initial_coefficients"])
        old_info=resume["source"]["firmware"]
        if any(info[k]!=old_info[k] for k in ("imu_hz","cal_flags","firmware_magic")):
            raise ValueError("原会话与当前采样配置或参数标志不一致，拒绝续采")
        report["resume"]={"source":resume["folder"],"restored_faces":list(resume["faces"]),
                          "restored_pose_numbers":[r["face"]-6 for r in resume["poses"]],
                          "discarded_windows":resume["discarded_windows"]}
        if args.stage=="accel":
            report["accel"]={"faces":copy.deepcopy(resume["faces"]),"poses":copy.deepcopy(resume["poses"]),
                             "status":"从原目录恢复完整窗口，独立验证尚未完成"}
        else:
            report["post_flash"]={"checks":{"accel_poses":copy.deepcopy(resume["poses"])},
                                  "status":"已恢复完整窗口，烧录后独立复查尚未完成"}
        for row in list(resume["faces"].values())+resume["poses"]:
            client.statistics.append((f"restored_{row['face']}_{row['request_id']}",copy.deepcopy(row)))
        if args.stage=="accel":
            print(f"恢复 {len(resume['faces'])}/6 个面、{len(resume['poses'])}/12 个独立姿态。请保持同一传感器、量程和安装。")
        else:
            print(f"恢复烧录后复查的 {len(resume['poses'])}/12 个独立姿态。请保持同一传感器、量程和安装。")
        render_report(folder,report)
    previous=None
    if args.stage=="verify":
        previous=json.loads((args.verify_from/"session.json").read_text(encoding="utf-8"))
        expected=previous.get("candidate_coefficients")
        if not expected:
            raise ValueError("原会话没有通过验证的候选参数")
        compare_coefficients(report["initial_coefficients"],expected)
        if info["cal_flags"]!=previous["candidate_flags"]:
            raise ValueError("板上有效/轴确认标志不匹配，请核对构建宏与参数头文件")
    changes={}; stage=args.stage
    if stage in ("all","raw"):
        report["raw"]=raw_stage(client,10 if stage=="all" else args.seconds)
    if stage in ("all","gyro","accel","verify"):
        if resume and can_reuse_gyro(resume,info,report["drivers"]["imu"],client.info(6)):
            report["gyro"]=copy.deepcopy(resume["source"]["gyro"])
            report["resume"]["gyro_reused"]=True
            print("板上零偏与原会话一致，复用原零偏及独立验证记录。")
        else:
            if resume:
                report["resume"]["gyro_reused"]=False
                print("板上零偏已改变或板子已重启：只重做零偏，不重做已保存的六面和独立姿态。")
            gyro_stage(client,report)
        render_report(folder,report)
    if stage in ("all","accel"):
        faces=copy.deepcopy(resume["faces"]) if resume else {}
        for i,name in enumerate(FACES):
            if name in faces:
                print(f"六面校准 {i+1}/6：{name} 已恢复完整1000点，跳过采集")
                continue
            input(f"六面校准 {i+1}/6：把传感器原生 {name[1].upper()}{'+' if name[0]=='p' else '-'} 轴朝上，固定后回车：")
            faces[name]=capture_with_retry(client,face=i+1)
            report.setdefault("accel",{}).update(faces=dict(faces),status="已采集，尚未通过拟合和独立验证")
            save_statistics(folder/"六面统计.csv",faces)
            render_report(folder,report)
        save_statistics(folder/"六面统计.csv",faces)
        try:
            bias,scale=fit_accelerometer_statistics(faces)
        except AccelerometerFaceError as error:
            report["accel"]["face_quality"]=error.quality
            print("六面一致性检查（补偿后向量单位g；角度仅为当前模型的方向误差）：")
            failed=[]
            for name,q in error.quality.items():
                label=name[1].upper()+("+" if name[0]=="p" else "-")
                print(f"{label}: {np.round(q['corrected_g'],4).tolist()}，残差 {q['residual_g']:.4f}g，"
                      f"方向误差约 {q['model_direction_error_deg']:.1f}°，{'通过' if q['passed'] else '未通过'}")
                if not q['passed']: failed.append(label)
            raise ValueError("六面方向一致性未通过："+"、".join(failed)+
                             "。请按传感器原生轴用夹具摆正，勿直接按机架面或仅凭静止判断；若准确摆正后仍失败，再检查交叉轴误差。") from error
        def checkpoint_poses(poses):
            report["accel"].update(faces=dict(faces),poses=copy.deepcopy(poses),bias_g=bias.tolist(),scale=scale.tolist(),
                                   status=f"拟合通过，独立姿态已完成{len(poses)}/12，尚未验收")
            save_statistics(folder/"采样统计.csv",dict(client.statistics))
            render_report(folder,report)
        existing=resume["poses"] if resume else []
        checkpoint_poses(existing)
        poses=acquire_poses(client,existing=existing,checkpoint=checkpoint_poses)
        quality=check_accel(poses,bias,scale)
        report["accel"]={"faces":faces,"poses":poses,"bias_g":bias.tolist(),"scale":scale.tolist(),
                         "validation":quality,"status":"拟合通过，独立验证通过，待烧录复查"}
        changes.update(CAL_ACCEL_VALID="1",CAL_ACCEL_BIAS_G=c_vector(bias),CAL_ACCEL_SCALE=c_vector(scale))
    if stage in ("all","mag"):
        if not info["mag_present"]:
            if stage=="mag":
                raise RuntimeError("H43 磁力计未识别，请查磁力计诊断；MPU 校准仍然可用")
            report["mag"]={"status":"磁力计缺失，跳过；九轴未验收"}
        else:
            input("保持最终装配与供电，远离铁桌/磁铁。回车后缓慢翻转整个机架，覆盖所有三维方向：")
            train_rows=collect_magnetic(client,args.seconds)
            train=fresh_magnetic(train_rows)
            write_csv(folder/"mag_train.csv",train_rows)
            bias,matrix,fit=fit_magnetometer(train)
            input("拟合通过。现在重新采集独立数据；不要重放之前动作记录。回车开始多方向转动：")
            hold_rows=collect_magnetic(client,args.mag_verify_seconds)
            hold=fresh_magnetic(hold_rows)
            write_csv(folder/"mag_independent.csv",hold_rows)
            quality=validate_magnetometer(hold,bias,matrix)
            report["mag"]={"bias_raw":bias.tolist(),"matrix":matrix.tolist(),"fit":fit,"validation":quality,
                           "status":"拟合通过，独立验证通过，待烧录复查"}
            changes.update(CAL_MAG_VALID="1",CAL_MAG_BIAS_RAW=c_vector(bias),
                           CAL_MAG_MATRIX="{"+",".join(c_vector(row) for row in matrix)+"}")
    if stage in ("all","mapping"):
        ask_mapping(client,report,changes)
    if stage=="verify":
        if not args.verify_from:
            raise ValueError("verify 需要 --verify-from 原采集目录")
        actual=get_coefficients(client); compare_coefficients(actual,expected)
        if info["cal_flags"]!=previous["candidate_flags"]:
            raise ValueError("板上有效/轴确认标志不匹配，请核对构建宏与参数头文件")
        checks=report.get("post_flash",{}).get("checks",{})
        if "accel" in previous and "validation" in previous["accel"]:
            def checkpoint_verification(poses):
                checks["accel_poses"]=copy.deepcopy(poses)
                report["post_flash"]={"status":f"烧录后独立姿态已完成{len(poses)}/12，尚未复查通过",
                                      "coefficients":actual,"checks":checks}
                save_statistics(folder/"采样统计.csv",dict(client.statistics))
                render_report(folder,report)
            existing=resume["poses"] if resume else []
            checkpoint_verification(existing)
            poses=acquire_poses(client,existing=existing,checkpoint=checkpoint_verification)
            checks["accel_poses"]=poses
            checks["accel"]=check_accel(poses,np.array(actual['3'][:3]),np.array(actual['3'][3:]))
        if "mag" in previous and "validation" in previous["mag"]:
            input("板上参数一致。回车后再次做独立磁场三维旋转：")
            rows=collect_magnetic(client,args.mag_verify_seconds)
            write_csv(folder/"mag_post_flash.csv",rows)
            checks["mag"]=validate_magnetometer(fresh_magnetic(rows),actual['4'][:3],np.array(actual['4'][3:]).reshape(3,3))
        report["post_flash"]={"status":"板上编译参数一致，独立复查通过", "coefficients":actual,"checks":checks}
    if changes:
        # Export only after the requested fits AND independent checks succeeded.
        # Keep unrelated coefficients and mappings from the selected base file.
        output=folder/"sensor_calibration.h"
        candidate=export_header(args.base,None,changes)
        coefficients,flags=header_coefficients(None,text=candidate)
        validate_coefficients(coefficients)
        output.write_text(candidate,encoding="utf-8")
        report["candidate_coefficients"],report["candidate_flags"]=coefficients,flags
        report["candidate_header"]=str(output.resolve())
        report["status"]="独立验证通过，候选参数已生成，待烧录复查"
        print("候选参数：",output,"。复制到 Core/Inc/sensor_calibration.h 后重新编译、烧录，再运行 --stage verify。")
    else:
        report["status"]="烧录复查通过" if stage=="verify" else "所选检查完成；未生成新的固定校准参数"
    report["final_firmware"]=client.info()


def header_coefficients(path,text=None):
    """Read only numeric calibration macros, never execute header contents."""
    import re
    text=(Path(path).read_text(encoding="utf-8") if text is None else text).replace("\\\n", "")
    def values(name):
        match=re.search(r"^#define\s+"+name+r"\s+(.+)$",text,re.M)
        if not match:
            raise ValueError("参数头缺少 "+name)
        return [float(x) for x in re.findall(r"[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?",match[1])]
    coefficient={"3":values("CAL_ACCEL_BIAS_G")+values("CAL_ACCEL_SCALE"),
                 "4":values("CAL_MAG_BIAS_RAW")+values("CAL_MAG_MATRIX"),
                 "5":values("CAL_IMU_TO_BODY")+values("CAL_MAG_TO_BODY")}
    if any(len(coefficient[k])!=n for k,n in (("3",6),("4",12),("5",18))):
        raise ValueError("参数头的系数数量错误")
    flags=sum(int(values(name)[0])<<bit for bit,name in enumerate(("CAL_ACCEL_VALID","CAL_MAG_VALID",
               "CAL_IMU_AXES_CONFIRMED","CAL_MAG_AXES_CONFIRMED")))
    return coefficient,flags


def validate_coefficients(pages):
    if any(not np.isfinite(v).all() for v in pages.values()) or np.min(pages['3'][3:])<=0:
        raise ValueError("候选系数非有限或加速度比例无效；请先修复基础参数")
    soft=np.array(pages['4'][3:]).reshape(3,3)
    if not np.allclose(soft,soft.T,atol=1e-8) or np.linalg.eigvalsh(soft).min()<=0:
        raise ValueError("候选磁场矩阵必须对称正定")
    for start in (0,9):
        rotation=np.array(pages['5'][start:start+9]).reshape(3,3)
        if not np.allclose(rotation@rotation.T,np.eye(3),atol=1e-3) or abs(np.linalg.det(rotation)-1)>1e-3:
            raise ValueError("候选轴映射必须正交且为右手系")


def offline_check(folder, output):
    report=json.loads((folder/"session.json").read_text(encoding="utf-8"))
    checks={}
    if "gyro" in report:
        checks["gyro"]=check_gyro(report["gyro"]["independent"],report["gyro"]["bias_dps"])
    if "accel" in report:
        bias,scale=fit_accelerometer_statistics(report["accel"]["faces"])
        if not report["accel"].get("poses"):
            raise ValueError("尚未采集12个独立倾角，不能完成独立验收")
        checks["accel"]=check_accel(report["accel"]["poses"],bias,scale)
    if (folder/"mag_train.csv").exists():
        def load(path):
            with path.open(encoding="utf-8-sig",newline="") as f:
                return [{k:int(v) if k in ("mag_sequence","flags","mag_age_us","mx","my","mz") else v for k,v in row.items()} for row in csv.DictReader(f)]
        bias,matrix,fit=fit_magnetometer(fresh_magnetic(load(folder/"mag_train.csv")))
        checks["mag"]={"fit":fit,"validation":validate_magnetometer(fresh_magnetic(load(folder/"mag_independent.csv")),bias,matrix)}
    if not checks:
        raise ValueError("目录里没有可离线复核的独立校准数据")
    (output/"离线复核.json").write_text(json.dumps(checks,ensure_ascii=False,indent=2),encoding="utf-8")
    print("离线复核通过；不能替代烧录后板上参数查询。")


def main():
    if hasattr(sys.stdout,"reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--list",action="store_true")
    parser.add_argument("--port",help="实际电脑上的蓝牙传出 COM，如 COM9")
    parser.add_argument("--baud",type=int,default=9600)
    parser.add_argument("--stage",choices=("all","link","raw","gyro","accel","mag","mapping","verify"),default="all")
    parser.add_argument("--seconds",type=float,default=240,help="磁场训练采集秒数；raw 模式为观察秒数")
    parser.add_argument("--mag-verify-seconds",type=float,default=90)
    parser.add_argument("--output",type=Path,default=ROOT/"captures"/time.strftime("sensor_%Y%m%d_%H%M%S"))
    parser.add_argument("--base",type=Path,default=ROOT/"Core/Inc/sensor_calibration.h")
    parser.add_argument("--verify-from",type=Path)
    parser.add_argument("--resume",type=Path,help="恢复原 accel 或 verify 阶段的完整窗口；新记录写入新的 --output")
    parser.add_argument("--replay",type=Path,help="离线解析已保存的 .bin，不连接硬件")
    parser.add_argument("--offline",type=Path,help="离线重算采集目录，不连接硬件")
    args=parser.parse_args()
    if args.list:
        from serial.tools import list_ports
        for p in list_ports.comports(): print(p.device,p.description)
        return
    if not np.isfinite([args.seconds,args.mag_verify_seconds]).all() or args.seconds<=0 or args.mag_verify_seconds<=0:
        parser.error("采集时间必须大于0")
    if args.stage=="verify" and not args.verify_from:
        parser.error("verify 需要 --verify-from")
    if args.verify_from and not (args.verify_from/"session.json").is_file():
        parser.error("--verify-from 目录中没有 session.json")
    if args.resume and (args.stage not in ("accel","verify") or not (args.resume/"session.json").is_file()):
        parser.error("--resume 需要 --stage accel 或 verify，并指向同一阶段包含 session.json 的原目录")
    if not args.replay and not args.offline and not args.port:
        parser.error("请先 --list 查看电脑串口，再提供 --port；或使用 --replay/--offline")
    try:
        args.output.mkdir(parents=True,exist_ok=False)
    except FileExistsError:
        parser.exit(1,"输出目录已存在，请指定新的 --output，避免覆盖上次校准。\n")
    report={"status":"进行中","stage":args.stage}
    client=None
    try:
        if args.offline:
            offline_check(args.offline,args.output)
            report["status"]="离线复核通过（未连接硬件）"
            return
        if args.replay:
            from capture import consume
            with args.replay.open("rb") as stream:
                summary=consume(iter(lambda:stream.read(4096),b""),args.output/"raw.csv")
            replies=[]; framing=Parser()
            with args.replay.open("rb") as stream:
                for chunk in iter(lambda:stream.read(4096),b""):
                    for frame_id,payload in framing.feed(chunk):
                        if frame_id==S["SCAL_REPLY_ID"]: replies.append(decode_cal_reply(payload))
            (args.output/"replies.json").write_text(json.dumps(replies,ensure_ascii=False,indent=2),encoding="utf-8")
            report["status"]="回放完成（未连接硬件）"
            report["replay"]={**summary,"replies":len(replies)}
            print(json.dumps(summary)); return
        import serial
        print("关闭匿名上位机/串口助手；模块 RXD←PA9、TXD→PA10、共地；拆桨，保持电机输出关闭。")
        with serial.Serial(args.port,args.baud,timeout=0.2,write_timeout=2) as port, (args.output/"session.bin").open("wb") as binary:
            client=Client(port,binary)
            try:
                run_wizard(client,args,args.output,report)
            except BaseException:
                if client.request_id:
                    try: port.write(pack_command(S["CAL_CANCEL"],client.request_id))
                    except (OSError,serial.SerialException): pass
                raise
    except (ValueError,RuntimeError,TimeoutError,OSError,KeyboardInterrupt,EOFError) as error:
        report["status"]="未通过，不能进入下一验收阶段"
        report["error"]=str(error) or "用户中断"
        print("停止：",report["error"])
        raise SystemExit(1)
    finally:
        if client is not None:
            write_csv(args.output/"raw.csv",client.rows)
            save_statistics(args.output/"采样统计.csv",dict(client.statistics))
            report["parser"]={"bad_checksums":client.parser.bad_checksums,"discarded_bytes":client.parser.discarded_bytes}
            report["communication"]=client.link_summary()
        render_report(args.output,report)
    print("记录已保存：",args.output)


if __name__=="__main__":
    main()
