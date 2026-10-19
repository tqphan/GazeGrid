#!/usr/bin/env python3
"""Render the unchanged XML fixture offscreen; never authorize desktop input."""
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
output = root / "runtime" / "render-audit"
output.mkdir(parents=True, exist_ok=True)
results = []
with tempfile.TemporaryDirectory(prefix="optikey-render-") as temporary:
    environment = dict(os.environ, QT_QPA_PLATFORM="offscreen", QT_QUICK_BACKEND="software",
                       XDG_CONFIG_HOME=temporary + "/config", XDG_CACHE_HOME=temporary + "/cache")
    for layout in sorted((root / "references" / "optikey-keyboard-en-us").glob("*.xml")):
        screenshot = output / (layout.stem + ".png")
        screenshot.unlink(missing_ok=True)
        environment["OPTIKEY_SCREENSHOT"] = str(screenshot)
        run = subprocess.run([str(root / "build" / "optikey-pro"), "--smoke", "--layout", layout.stem],
                             env=environment, text=True, capture_output=True, timeout=15)
        dimensions = struct.unpack(">II", screenshot.read_bytes()[16:24]) if screenshot.exists() else None
        results.append(dict(layout=layout.stem, exitCode=run.returncode, dimensions=dimensions,
                            diagnostics=run.stderr + run.stdout))
        print(layout.stem, run.returncode, dimensions, flush=True)
    (output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    audit = subprocess.run([str(root / "build" / "optikey-pro"), "--validate-keyboards"],
                           env=environment, text=True, capture_output=True, check=True)
    (output / "coverage.json").write_text(audit.stdout)
if len(results) != 22 or any(r["exitCode"] or not r["dimensions"] or r["diagnostics"] for r in results):
    raise SystemExit("Render failures or diagnostics: inspect runtime/render-audit/results.json")
print("All 22 layouts rendered without diagnostics.")
