#!/usr/bin/env python3
"""Exercise OptiKey on a private, headless KWin compositor; never the user's session."""
import os
from pathlib import Path
import shlex
import signal
import subprocess
import tempfile
import argparse

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--late-start", action="store_true", help="Start the keyboard after the receiver has focused its field")
args = parser.parse_args()

root = Path(__file__).resolve().parents[1]
binary = root / "build/optikey-pro"
with tempfile.TemporaryDirectory(prefix="optikey-native-im-") as directory:
    work = Path(directory)
    for name in ("config", "cache", "runtime"):
        (work / name).mkdir(mode=0o700)
    (work / "config/kwinrc").write_text("[Wayland]\nVirtualKeyboardMode=2\n[Plugins]\nshakecursorEnabled=false\n")
    for name, arguments in (("keyboard", "--input-method --input-method-check"), ("receiver", "--input-method-target")):
        script = work / name
        delay = "sleep 1\n" if name == "keyboard" and args.late_start else ""
        script.write_text("#!/bin/sh\nexport QT_QPA_PLATFORM=wayland\n" + delay + "exec " + shlex.quote(str(binary)) + " " + arguments + "\n")
        script.chmod(0o700)
    env = os.environ.copy()
    for name in ("WAYLAND_SOCKET", "WAYLAND_DISPLAY", "DISPLAY", "DBUS_SESSION_BUS_ADDRESS", "QT_IM_MODULE"):
        env.pop(name, None)
    env.update(XDG_RUNTIME_DIR=str(work / "runtime"), XDG_CONFIG_HOME=str(work / "config"),
               XDG_CACHE_HOME=str(work / "cache"), QT_QPA_PLATFORM="offscreen", QT_QUICK_BACKEND="software",
               KWIN_COMPOSE="Q", OPTIKEY_ISOLATED_IM_TEST="1", QT_QPA_PLATFORMTHEME="generic")
    command = ["dbus-run-session", "--", "kwin_wayland", "--virtual", "--width", "1024", "--height", "768",
               "--socket", "optikey-test", "--no-lockscreen", "--no-global-shortcuts", "--no-kactivities",
               "--inputmethod", str(work / "keyboard"), "--exit-with-session", str(work / "receiver")]
    # Use a file: portal services may inherit stdout after KWin exits, so a
    # PIPE would keep communicate() waiting for unrelated service lifetimes.
    with (work / "session.log").open("w+") as stream:
        process = subprocess.Popen(command, env=env, stdout=stream, stderr=subprocess.STDOUT,
                                   text=True, start_new_session=True)
        try:
            process.wait(timeout=35)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGTERM)
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
        finally:
            try:
                os.killpg(process.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
        stream.seek(0)
        output = stream.read()
    log = root / "runtime/native-input-method.log"
    log.parent.mkdir(exist_ok=True)
    log.write_text(output)
    passed = (process.returncode == 0
              and "PASS: native Unicode commit, Backspace, Return and private field reached the Qt receiver" in output
              and "Native placement check: top (11 UI clicks)" in output
              and "Native placement check: bottom (11 UI clicks)" in output
              and "Wayland protocol error" not in output)
    print(output if not passed else "PASS: native Unicode commit, Backspace, Return and private field reached the Qt receiver")
    print("Log:", log)
    raise SystemExit(0 if passed else 1)
