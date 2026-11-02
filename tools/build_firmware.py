"""Build actual Keil-listed sources with ARMCLANG, without opening an IDE.

Creates build/<name>/{quadcopter.axf,quadcopter.hex,quadcopter.map,build.log}.
Never downloads to a board. Keil itself remains the primary IDE project.
"""
import argparse
import concurrent.futures
import json
import os
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler-bin", type=Path, default=Path("F:/keilmdk/ARM/ARMCLANG/bin"))
    parser.add_argument("--name", default="firmware")
    parser.add_argument("--define", action="append", default=[])
    args = parser.parse_args()
    if not args.name or any(c not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-" for c in args.name):
        parser.error("--name must be a simple directory name")
    out = ROOT/"build"/args.name
    out.mkdir(parents=True, exist_ok=True)
    compiler = args.compiler_bin/"armclang.exe"
    assembler = args.compiler_bin/"armasm.exe"
    linker = args.compiler_bin/"armlink.exe"
    project = ROOT/"MDK-ARM/hello.uvprojx"
    target = ET.parse(project).find(".//Target")
    controls = target.find("TargetOption/TargetArmAds/Cads/VariousControls")
    includes = [(project.parent/p.replace("\\", "/")).resolve()
                for p in controls.findtext("IncludePath", "").split(";") if p]
    defines = [d for d in controls.findtext("Define", "").split(",") if d] + args.define
    commands, objects = [], []
    for entry in target.findall("Groups/Group/Files/File"):
        if entry.findtext("FileOption/CommonProperty/IncludeInBuild") == "0":
            continue
        source = (project.parent/entry.findtext("FilePath").replace("\\", "/")).resolve()
        if source.suffix.lower() not in (".c", ".s"):
            continue
        obj = out/(str(len(objects))+"_"+source.stem+".o")
        objects.append(obj)
        if source.suffix.lower() == ".s":
            command = [str(assembler), "--cpu", "Cortex-M4.fp.sp", "--apcs=interwork", "-g", str(source), "-o", str(obj)]
        else:
            command = [str(compiler), "--target=arm-arm-none-eabi", "-mcpu=cortex-m4",
                       "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard", "-std=c99", "-O1", "-g",
                       "-ffunction-sections", "-fdata-sections", "-fshort-enums", "-fshort-wchar", "-Wall"]
            command += ["-I"+str(p) for p in includes]
            command += ["-D"+d for d in defines]
            command += ["-c", str(source), "-o", str(obj)]
        commands.append(command)

    def run(command):
        result = subprocess.run(command, cwd=ROOT, text=True, encoding="utf-8", errors="replace", capture_output=True)
        return result.returncode, subprocess.list2cmdline(command)+"\n"+result.stdout+result.stderr

    with concurrent.futures.ThreadPoolExecutor(max_workers=min(6, os.cpu_count() or 1)) as pool:
        results = list(pool.map(run, commands))
    log = "\n".join(text for _, text in results)
    if any(code for code, _ in results):
        (out/"build.log").write_text(log, encoding="utf-8")
        for code, text in results:
            if code:
                print(text)
        raise SystemExit("Compile failed; see "+str(out/"build.log"))
    scatter = out/"firmware.sct"
    scatter.write_text('''LR_FLASH 0x08000000 0x00080000 {
 ER_FLASH 0x08000000 0x00080000 {
  *.o (RESET, +First)
  *(InRoot$$Sections)
  .ANY (+RO)
  .ANY (+XO)
 }
 RW_RAM 0x20000000 0x00018000 { .ANY (+RW +ZI) }
}
''', encoding="ascii")
    command = [str(linker), "--cpu=Cortex-M4.fp.sp", "--strict", "--scatter", str(scatter),
               "--map", "--symbols", "--callgraph", "--info=sizes,totals,unused,veneers",
               "--list", str(out/"quadcopter.map"), "-o", str(out/"quadcopter.axf")]
    command += [str(p) for p in objects]
    code, text = run(command)
    log += "\n"+text
    (out/"build.log").write_text(log, encoding="utf-8")
    if code:
        print(text)
        raise SystemExit("Link failed")
    code, text = run([str(args.compiler_bin/"fromelf.exe"), "--i32combined", "--output",
                      str(out/"quadcopter.hex"), str(out/"quadcopter.axf")])
    if code:
        print(text)
        raise SystemExit("HEX export failed")
    print(json.dumps({"sources_compiled": len(objects), "defines": args.define,
                      "axf": str(out/"quadcopter.axf"), "hex": str(out/"quadcopter.hex"),
                      "map": str(out/"quadcopter.map"), "log": str(out/"build.log")}, indent=2))


if __name__ == "__main__":
    main()
