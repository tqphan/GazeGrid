# Host acceptance — initial prototype

Date: 2026-10-18. Host: Arch Linux x86-64, KDE Wayland, Qt 6.12.0. This report records the initial implementation, not completion of the modernization plan.

| Check | Result |
| --- | --- |
| Native C++/Qt build | Pass with project-local CMake 4.4.4 and Ninja 1.13.2 |
| Core tests | Dwell continuity, target changes, explicit activation, XML parsing, command ordering/whitespace and entity rejection |
| Required pack | All 22 XML files parse; 570 DynamicKey entries; all local keyboard references resolve |
| Wayland panel | Native bottom layer-shell window rendered and exited successfully; keyboard interactivity disabled |
| Portal discovery | RemoteDesktop version 2; AvailableDeviceTypes 7 |
| Authorized input session | Pass: libei keyboard/pointer devices became ready after desktop permission approval |
| Separate-process input delivery | Pass: disposable Wayland Qt receiver received `optikey` followed by Enter |
| Supplied Tobii SDK | Exact x64 library loads and reports 3.0.4.6031; no substitution |
| Tobii enumeration | Library returned zero devices; live gaze remains unverified |
| Tobii services | `tobii_engine` and `tobii_usb` reported active |
| Original repository | No rewrite files or changes added |

The separate-process test proves keystroke delivery to a focused Qt target. It does not yet establish focus preservation when physically clicking the panel, input delivery into a browser/XWayland application, or pointer-action correctness.

Still required: live Tobii streaming/calibration/reconnect testing; full XML styling, symbols, navigation semantics and actions; platform mappings; text composition and prediction; speech; mouse drag/held buttons/magnification; permission restoration and revocation tests; suspend/resume and mixed-DPI/display changes; accessible settings/recovery flows; a sustained usage session. Windows/macOS adapters and packaging remain future milestones.

No privileged helper or Tobii service installation was performed. CMake/Ninja were installed into `.tools/` after system package installation could not proceed without a sudo password. The selected Tobii SDK remains in its original user-supplied location.

## Action-runner regression follow-up — 2026-10-18

The current source builds and `./build.sh` passes the core suite, including all 22 layout compilation fixtures. Added regression coverage verifies that either held Shift key records effective US punctuation in scratchpad text (for example, Shift+1 records `!`) and remains held until explicitly released. Backspace combined with either Ctrl, Alt, or Win key still sends the physical key sequence but no longer reports an ordinary single-character scratchpad deletion. Unmodified Backspace continues to report that deletion. Shortcut effects in the destination application are not inferred or synchronized into the scratchpad.

These checks use a fake output backend; no desktop-input or live Tobii acceptance was performed in this follow-up. The initial prototype descriptions above remain historical and do not certify the newer implementation's full pack compatibility.

## Preferences recovery follow-up — 2026-10-18

Added a versioned JSON preference store, explicit Save and Reset settings controls, atomic writes, and a validated backup. Regression tests pass for round-trip persistence, startup keyboard persistence, corrupt-primary recovery, preserving the backup after corruption, resetting both copies, rejecting malformed/out-of-range preferences, preserving newer-version files, and reporting filesystem write failures. Reset leaves input paused and keeps learned words and shortcut mappings separate. The former prototype QSettings file is no longer read or written.

`./build.sh` passes (23 QtTest results, including initialization and cleanup). An offscreen application smoke run with an isolated temporary configuration also exits successfully without QML errors. These automated checks do not establish assistive-input usability of the settings window or desktop/hardware acceptance.

## Pointer, XML and lifecycle follow-up — 2026-10-18

Implemented a separate pointer executor with two-point drag, timed movement/button release, repeat of completed targeted actions, cancellation, click/held-button conflict prevention, and relative direction actions matching the original 25-pixel default. Magnetic cursor and selected-anchor look-to-scroll have implementations; look-to-scroll currently covers the original default free-axis/medium-speed/50-pixel-deadzone calculation, not every original settings mode.

Added Screenshot-portal magnification with a four-times cropped selection image, coordinate remapping, cancellation and single-display checks. Five isolated fake-portal tests cover image delivery, denial, cancellation with a late response, nonlocal URI rejection and unexpected request handles. The fake service does not capture the desktop. Real KDE capture consent/focus and assistive selection remain unverified.

Dwell recovery controls now remain usable while a macro waits. Display changes pause/cancel output, and Tobii samples retain their age across worker-to-UI queuing. All 86 navigation links and back-return flags pass Controller tests. XML menu visibility and symbol margins are applied, with an explicit settings option to include hidden layouts.

The final automated run passed all three suites (47 test cases excluding setup/cleanup). All 22 layouts rendered offscreen without diagnostics; artifacts and per-layout coverage are in `runtime/render-audit/`. All 51 fixture action names now have implementation paths, but full behavioral parity remains unverified. No live Tobii probe or streaming test was performed.

See `feature-inventory.md` for remaining work, including advanced settings/selection behavior, full text composition, live mouse and magnifier acceptance, mixed-DPI calibration, speech lifecycle, profile usability and packaging.

## Native desktop follow-up — 2026-10-18

A fresh `./run.sh --input-check` passed on the real KDE Wayland session: portal authorization produced usable libei devices; imported XML commands delivered `optikey` plus Enter to a separate receiver; an injected click on the XML `q` key preserved receiver focus and delivered `q`. The harness now stops on receiver focus loss. This adds panel-click focus evidence; browser/XWayland, live drag/scroll/magnifier and sustained-use acceptance remain pending.

## Speech lifecycle follow-up — 2026-10-18

Replaced detached `spd-say` calls with an owned libspeechd connection on a worker thread. Stop cancels only this connection's messages. Pending requests cannot speak after cancellation; submission failures close the connection and the next request reconnects. Errors are reported without exposing scratchpad text in diagnostics. Four fake-backend tests cover ownership/cleanup, connection failure, cancellation during connection establishment, and reconnect after failed submission. No audible speech was generated during these tests; live voice and cancellation acceptance remain pending.

## Prediction, visual preferences and dwell controls — 2026-10-18

Prediction now handles tabs, punctuation and prefix casing, avoids duplicate learning across repeated separators, and refuses suggestion activation while keys are held. Settings, Speak and Stop speech support dwell activation; entering Settings pauses and cancels output. Keyboard height and high contrast are optional, validated, persisted overrides. Minimized recovery layouts keep their imported size even when height scaling is active.

All four automated suites pass (55 test cases excluding setup/cleanup). Additional offscreen renders verified the scrollable settings window, a 125% high-contrast keyboard and unchanged minimized-panel dimensions, without QML diagnostics. These checks do not establish full gaze operation of the settings fields.

## Native pointer, magnifier and permission follow-up

The separate full-screen Wayland receiver passed targeted left click, double-click, two-point drag, scroll and right click. The double-click harness deduplicates Qt press/double-click reports using their shared event timestamp. The magnifier check passed actual KDE Screenshot capture, crop-content verification and precise click delivery after receiver focus returned. These checks use synthetic target points, not live gaze.

Optional desktop permission storage now uses private single-use restore tokens, version-gates persistence, and provides Disconnect and Forget. Five isolated RemoteDesktop tests verify default opt-out, token rotation, older portal fallback, denial and forgetting. All five automated suites passed (66 cases excluding setup/cleanup). Real remembered-session restoration and revocation remain pending. Shortcut mapping validation and corrupt-file fallback are covered; the default KDE previous-screen mapping preserves ordered modifier release, and unknown shortcuts remain explicit.

After the portal changes, native `--input-check` passed again: imported XML typing reached the separate Wayland receiver, and clicking XML q preserved receiver focus and delivered q. The complete 22-layout offscreen render audit also passed without QML diagnostics.

## Native input-method continuation — 2026-10-19

The current source builds. Core, controller and speech suites pass inside the
sandbox; screenshot and portal suites pass when allowed to create their private
D-Bus sockets. No real portal authorization was requested by these suites.

The isolated headless KWin harness passes native Unicode text (`é`, `€`, emoji),
Backspace, Return and password-field delivery to its separate Qt receiver. The
initial run exposed a startup race: the receiver enabled its field before the
input-method interface was bound, and the context received no content/commit
state. The receiver now retries field entry until text arrives. This validates
post-startup field entry, not automatic recovery of an already focused field.

CMake now generates the concrete input-method desktop entry; desktop-file-validate
passes. The entry has not been installed and the real session default remains
unchanged. Real-session application compatibility, panel placement/focus,
startup recovery and live gaze remain outstanding.

## Startup recovery and desktop registration — 2026-10-19

Supersedes the receiver retry workaround above. The backend now makes one
asynchronous KWin context re-adoption request when activation has received no
committed state after 250 ms. Output remains unavailable until protocol state
arrives; unknown field privacy remains conservative. Deactivation cancels the
pending timer, and there is no repeating recovery loop.

Both the ordinary isolated test and `python3 tests/native_input_method.py
--late-start` pass with the receiver's original field left focused. The latter
deliberately starts the keyboard one second late. Unicode, Backspace, Return and
password-field delivery all pass. The harness now uses a log file instead of a
pipe held open by descendant portal services, and requires a successful compositor
exit as well as the receiver's pass marker. All five build regression suites pass.

The validated desktop entry now launches the project `run.sh`, retaining local
preferences/cache behavior. It is installed at
`~/.local/share/applications/org.optikey.Pro.InputMethod.desktop`, and
`kbuildsycoca6 --noincremental` succeeded. The existing kwinrc selection remains
`/usr/share/applications/org.kde.plasma.keyboard.desktop` with mode 1.
Real-session application compatibility and gaze acceptance remain outstanding.

## Placement button feedback — 2026-10-19

Native-mode keys that target unsupported top/offset layouts now show an explicit
unavailable label, with a tooltip and accessible explanation. Click/dwell reports
the reason before starting any commands. Original XML remains unchanged.
The main bottom bar's first key is covered by controller regression checks for
native rejection and standalone navigation to the top bar. The five regression
suites and isolated delayed-start native input check pass; no QML errors were
reported by that native run. This improves feedback, not native top placement.
