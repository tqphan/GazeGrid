#!/usr/bin/env python3
"""Render the XML keyboard pack offscreen; never authorize desktop input."""
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
output = root / "runtime" / "render-audit"
output.mkdir(parents=True, exist_ok=True)
icons = json.loads((root / "resources" / "icons.json").read_text())
for layout in (root / "references" / "optikey-keyboard-en-us").glob("*.xml"):
    for symbol in ET.parse(layout).iter("Symbol"):
        if not icons.get((symbol.text or "").strip()):
            raise SystemExit(f"Missing icon in {layout.name}: {symbol.text}")
results = []
with tempfile.TemporaryDirectory(prefix="gaze-grid-render-") as temporary:
    environment = dict(os.environ, QT_QPA_PLATFORM="offscreen", QT_QUICK_BACKEND="software",
                       XDG_CONFIG_HOME=temporary + "/config", XDG_CACHE_HOME=temporary + "/cache")
    for layout in sorted((root / "references" / "optikey-keyboard-en-us").glob("*.xml")):
        screenshot = output / (layout.stem + ".png")
        screenshot.unlink(missing_ok=True)
        environment["GAZE_GRID_SCREENSHOT"] = str(screenshot)
        run = subprocess.run([str(root / "build" / "gaze-grid"), "--smoke", "--layout", layout.stem],
                             env=environment, text=True, capture_output=True, timeout=15)
        dimensions = struct.unpack(">II", screenshot.read_bytes()[16:24]) if screenshot.exists() else None
        results.append(dict(layout=layout.stem, exitCode=run.returncode, dimensions=dimensions,
                            diagnostics=run.stderr + run.stdout))
        print(layout.stem, run.returncode, dimensions, flush=True)
    (output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    audit = subprocess.run([str(root / "build" / "gaze-grid"), "--validate-keyboards"],
                           env=environment, text=True, capture_output=True, check=True)
    (output / "coverage.json").write_text(audit.stdout)
if len(results) != 29 or any(r["exitCode"] or not r["dimensions"] or r["diagnostics"] for r in results):
    raise SystemExit("Render failures or diagnostics: inspect runtime/render-audit/results.json")
print("All 29 layouts rendered without diagnostics.")
