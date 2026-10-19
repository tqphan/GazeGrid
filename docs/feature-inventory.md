# Linux implementation inventory

Updated 2026-10-18. This inventory describes the local Qt implementation, not a release certification. The original OptiKey tree and the 22-layout reference pack remain unchanged.

| Area | Current implementation | Remaining acceptance or work |
| --- | --- | --- |
| Selection | Continuous dwell, per-key dwell, cancellation, explicit click consumption | Configurable repeat, advanced multi-key selection, additional trigger adapters |
| Keyboard XML | 22 layouts / 570 DynamicKeys compile; geometry, key groups, labels, shifted labels, symbol margins, symbols and menu visibility | Original-app visual comparison, full state persistence semantics, disabled/held visual parity |
| Navigation | All 86 ChangeKeyboard entries and back-return flags tested through Controller; recovery path retained | Live minimized-panel recovery and assistive usability |
| Action execution | Ordered macros, waits, transient versus persistent key ownership, unsupported-action preflight, cancellation | Full target-application compatibility and sustained latency measurements |
| Keyboard output | Linux portal/libei with separate D-Bus fallback; US physical key encoder | Unicode, dead keys, alternate layouts and IME/text commit |
| Shortcuts | Explicit editable mappings with literal execution option; unknown Windows combinations reported | Curated Linux mappings and target-application verification |
| Pointer actions | Clicks, double-clicks, held buttons, relative movement, scrolling, two-point drag and repeat | Native click/double-click/drag/scroll/right-click passed; configurable movement/drag parameters, suppression/restoration of modifiers |
| Magnetic cursor | Valid gaze follows desktop pointer; pause/loss/cancellation stop it | Live gaze and multi-display mapping |
| Look to scroll | Selected anchor, two axes, medium-speed/default deadzone calculation; panel/control exclusion | Additional bounds/modes/speeds, visible deadzone overlay, physical-pixel/DPI verification, desktop acceptance |
| Magnifier | Screenshot portal, four-times cropped image, precise point remapping; cancellation and failure paths | Native KDE capture and precise click passed; live gaze selection pending; currently one display only |
| Scratchpad | Tracks text emitted by OptiKey and plain Backspace; shifted punctuation represented correctly | Destination edits/shortcuts are not synchronized; full composition model |
| Prediction | Offline ASCII dictionary completion, punctuation/whitespace-aware prefixes, casing, learned frequency ranking and reset; held-key guard for accepting suggestions | Base frequency data, multilingual tokenization, full composition fixtures, performance and usability acceptance |
| Speech | Owned Speech Dispatcher connection on a worker thread, app-scoped cancellation, queued-request cancellation and error/reconnect handling | Voice settings, live speech/cancellation acceptance and stalled-daemon shutdown behavior |
| Profiles | Versioned atomic preferences with validated backup, explicit save/reset, future-version protection; optional keyboard-height/high-contrast overrides | Profile selection and full assistive settings operation |
| Tobii | Exact supplied SDK, worker lifecycle, validity and timestamp filtering, reconnect path; UI delivery age checked | Tracker is unplugged by user instruction: live streaming/calibration/reconnect acceptance pending; no new hardware probe performed |
| Displays | Geometry/DPI/orientation and screen changes pause/cancel output before remapping | Mixed-DPI/hotplug tests, explicit tracker/display calibration mapping |
| Desktop lifecycle | Output ownership cleanup, portal denial/disconnection handling, focus-aware acceptance harness | Opt-in single-use restore tokens and Forget implemented/tested with fake portal; real restoration/revocation, suspend/resume and assistive recovery pending |
| Packaging | Local build/run scripts and diagnostic commands | Arch package follows desktop and live Tobii acceptance; dependency/license inventory still required |
| Windows/macOS | Shared core architecture | Native adapters, hardware testing and packaging remain future milestones |
| Legacy plugins/settings | Intentionally excluded by user decision | No compatibility host or legacy configuration migration planned |

## Automated evidence

`./build.sh` runs core, Controller, Screenshot portal, speech and RemoteDesktop portal suites. There are 66 test cases, excluding QtTest setup/cleanup: 43 core, 9 Controller, 5 screenshot, 4 speech and 5 RemoteDesktop tests. Both portal suites use fake services on separate D-Bus sessions, never the real desktop portal. Socket-restricted sandboxes need permission to create that local test bus.

`python3 tests/render_layouts.py` renders all 22 layouts using the offscreen Qt backend and an isolated temporary profile. It writes images, render diagnostics, and per-layout action/symbol/attribute coverage under `runtime/render-audit/`. This verifies rendering without QML diagnostics, not visual equality with WPF or actual Wayland placement.

All 51 action names in this fixture have an implementation path. `behavioralParityVerified` remains false in the audit because handler availability does not establish full semantics or real desktop delivery. See `host-acceptance.md` for observed host results and remaining gates.
