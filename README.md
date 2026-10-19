# OptiKey Pro — Linux prototype

Qt 6 Quick/QML and C++20 implementation in its own sibling project. The original OptiKey repository is unchanged. This is the initial integration prototype, not the completed Pro rewrite.

## Build and run on this machine

```sh
cd /home/q/Downloads/OptiKey-Pro
./build.sh
./run.sh
```

CMake and Ninja are installed locally in `.tools/`. The build uses the host Qt 6 development packages, layer-shell-qt, libei and libspeechd (Speech Dispatcher). No system helper was installed. `build.sh` also runs the core tests. Runtime settings and caches stay under `runtime/` when using `run.sh`.

The default SDK is the exact user-supplied `stream_engine_linux_3.0.4.6031` directory. Its headers are used for compilation and its x64 shared library is loaded by absolute path at runtime. The sibling 4.24 SDK is not used. SDK files are not copied into this project. The CMake cache variable `TOBII_SDK` allows relocating the same SDK.

## Current controls and implementation

- **Authorize** requests keyboard/pointer access through KDE's portal. The preferred backend is libei; unsupported EIS can use portal D-Bus methods in a separate session mode.
- The imported XML keyboard opens directly. **Settings** honors XML menu visibility; enable **Include layouts hidden by the XML pack** to select any of the 22 layouts; navigation keys and **Home / recover** also change layouts. Geometry, symbols, key groups, ordered macros, modifier toggles, scratchpad text, dictionary suggestions and editable platform mappings have implementations. Full behavior and appearance parity remain unverified.
- **Tobii** connects through the selected SDK. **Gaze** enables gaze selection on the primary display. The tracker is assumed calibrated to that display. Live-device acceptance remains pending; display changes pause input and require checking calibration before resuming.
- **Pause** releases owned input and cancels pending actions. Dwell can activate Pause and recovery controls during a waiting macro. Move away and back before dwelling on Resume.
- Pointer actions support target selection, clicks, double-clicks, relative movement, scrolling and held buttons. **Drag** chooses a source and destination, then performs a cancellable sequence. Repeat reuses the completed target for targeted clicks and drags. Direction keys move by the original default of 25 pixels. A plain left click acts at the current desktop pointer; use gaze selection when that must remain independent of the panel.
- **Magnetic cursor** follows valid gaze while Gaze is enabled. **Look to scroll** selects an anchor, then scrolls in either axis beyond a 50-pixel deadzone, with the original default medium-speed calculation. Looking over the keyboard or control bar suppresses scrolling. Both modes stop on cancellation, tracking loss, or pause. Advanced look-to-scroll bounds/settings and its overlay remain pending.
- **Speak** and **Stop speech** use an owned Speech Dispatcher connection on a worker thread. Stop cancels this app’s speech, and failures are reported without blocking typing. Prediction uses the bundled offline dictionary with optional learned frequencies. Live voice and prediction usability acceptance remain pending.

Standalone portal output uses US physical keycodes. Experimental KWin input-method mode also supports native Unicode text commits. **Magnifier** enables a four-times magnified target-selection flow using the desktop Screenshot portal: choose an approximate point, approve capture if prompted, then choose the precise point. Capture cancellation cancels the action. This currently requires one display; native KDE capture and precise-click acceptance passed; live gaze acceptance remains pending. Unknown actions are rejected before any part of their macro executes. Absolute pointer targeting requires one EIS display region. Successful XML loading or action dispatch does not establish full pack compatibility. No existing plugins or legacy settings are loaded.

## Diagnostics and tests

Preferences now use a versioned `profile.json` under the application configuration directory (`runtime/config/OptiKey/OptiKey-Pro/` with `run.sh`). In Settings, choose **Save preferences and startup keyboard** to persist dwell time, keyboard height, high contrast, literal-key mode, local-learning preference, and the currently selected keyboard. Visual overrides are optional; the XML is unchanged, and minimized recovery layouts retain their original size. Changes take effect immediately; saving is explicit. **Reset preferences** restores defaults and leaves input paused. Reset does not delete learned words or shortcut mappings; learned words have their own reset control.

Writes are atomic and retain a validated `.bak` recovery copy. If the main profile is corrupt, startup uses that copy or defaults and reports recovery in Settings. Profiles from newer versions are preserved and cannot be overwritten without an explicit reset. This fresh format does not import the prototype's QSettings file or legacy OptiKey settings.

```sh
./run.sh --diagnose --validate-keyboards
./run.sh --smoke
./run.sh --portal-check
./run.sh --input-check
./run.sh --pointer-check
./run.sh --magnifier-check
python3 tests/render_layouts.py
```

`--diagnose` loads the real library and enumerates devices, and queries portal capabilities. It prints no device URLs or serial numbers. A zero device count is not live-gaze success.

`--validate-keyboards` verifies XML parsing, geometry, command syntax and local navigation targets. It reports per-layout actions, unsupported actions, symbols and attribute names, preserving command order and Text whitespace. Implementation categories are not an action compatibility certification; `behavioralParityVerified` remains false.

`--smoke` displays the panel briefly and closes it. Set `OPTIKEY_SCREENSHOT` to a file path to capture the panel itself.

`--portal-check` opens a permission session and waits for usable libei devices without emitting input. `--input-check` opens a separate disposable receiver after authorization, waits for receiver focus, and sends `optikey` plus Enter. It reports receipt and closes. Click the receiver if KDE does not activate it automatically. Do not switch focus during this short test. The check also clicks the XML q key and verifies that the receiver keeps focus and receives q. It stops if the receiver loses focus. Passing does not establish browser/XWayland compatibility.

See `docs/host-acceptance.md` for observed results and outstanding gates, and `docs/OptiKey-Pro-modernization-plan.md` for the agreed decisions.

`tests/render_layouts.py` creates offscreen snapshots and a coverage report in `runtime/render-audit/`, using an isolated temporary profile. The build test suite includes a fake screenshot portal on an isolated D-Bus session (`dbus-run-session`); it never invokes the real screenshot portal. Restricted sandboxes must allow creation of the local test-bus socket.

`--pointer-check` opens a disposable full-screen Wayland receiver and exercises targeted left click, double-click, drag, scroll and right click. It requires one mapped display and stops on receiver focus loss. Keep that receiver focused during the short test. `--settings` opens settings with output paused.

`--magnifier-check` verifies screenshot capture and a precise target click against the disposable receiver. Keep it focused and approve capture if prompted.

Settings can opt into remembering desktop-control permission where the portal supports it. Save preferences to retain the choice. Restore tokens are stored with owner-only permissions and consumed once before requesting a replacement. **Disconnect and forget desktop permission** closes input and deletes the local token; desktop permission management remains available in KDE settings.

## Experimental Plasma input method

`--input-method` must be launched by KWin with its inherited Wayland socket.
CMake generates `build/org.optikey.Pro.InputMethod.desktop` pointing to this build;
the build does not automatically install or select it. On this machine the entry
has been registered in the user application directory and Plasma’s cache refreshed,
but Plasma Keyboard is still selected.

Run `python3 tests/native_input_method.py` to check Unicode commits, Backspace,
Return and password-field input against a separate Qt receiver in a private,
headless KWin session. The test needs permission to create local D-Bus sockets.
It writes `runtime/native-input-method.log` and does not change the desktop's
keyboard selection. The receiver keeps its original field focused. Add `--late-start` to delay the
keyboard by one second and verify recovery of that already focused field.

Native mode currently restricts XML placement to supported bottom layouts.
Buttons targeting top or offset layouts are labeled “Unavailable in Plasma mode”;
hovering, clicking, or dwelling explains the restriction. Standalone mode retains
the original placement buttons and behavior.
Pointer actions and held physical shortcuts still require desktop authorization.
Real-session focus/recovery, GTK/browser/XWayland compatibility, and gaze
acceptance remain outstanding. Selectable registration alone does not establish
that OptiKey is ready to replace Plasma Keyboard.
