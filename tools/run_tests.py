"""Run this project's tests without loading unrelated system pytest plugins."""
import os
import sys
from pathlib import Path

os.environ["PYTEST_DISABLE_PLUGIN_AUTOLOAD"] = "1"
os.environ.setdefault("OPENBLAS_NUM_THREADS", "1")
os.environ.setdefault("OMP_NUM_THREADS", "1")
_dll_directory = None
if os.name == "nt" and (Path(sys.prefix)/"Library/bin").is_dir():
    conda_bin = str(Path(sys.prefix)/"Library/bin")
    os.environ["PATH"] = conda_bin + os.pathsep + os.environ.get("PATH", "")
    _dll_directory = os.add_dll_directory(conda_bin)
import pytest

if __name__ == "__main__":
    root = Path(__file__).resolve().parents[1]
    raise SystemExit(pytest.main([str(root/"tests"), "-q", *sys.argv[1:]]))
