# OptiKey Pro modernization plan

Create a new OptiKey Pro desktop application for Linux, Windows, and macOS. Deliver a usable build on this Arch Linux/KDE Wayland machine first, then complete Linux Pro functionality, then Windows and macOS. Implementation has started in the sibling project. See `host-acceptance.md` for verified prototype results and remaining acceptance gates; the milestones below describe the complete intended scope.

## 1. Baseline and scope

Inspection on 2026-10-18 found:

| Area | Observed state | Implication |
| --- | --- | --- |
| Host | Arch Linux x86_64, KDE Wayland, KWin/Plasma 6.8 | First supported environment and manual acceptance target |
| CPU | AMD Ryzen 9 8940HX, 32 logical CPUs | Native desktop build is practical; benchmark actual latency rather than assuming it |
| Installed components | Qt base/declarative 6.12, Wayland platform plugin, layer-shell-qt 6.8, portals including KDE backend, libei, Speech Dispatcher | Useful foundation; package presence does not prove runtime capability |
| Tooling | GCC, Rust/Cargo, Python available; CMake, Ninja, dotnet and node not found on PATH | Set up a minimal native build toolchain during implementation |
| Eye tracker | Unknown; USB enumeration failed in this execution environment | Do not claim hardware support until the actual tracker is identified and exercised |
| Existing code | Pro and Core target .NET Framework 4.6; WPF, Win32 interop, WindowsInput, Windows speech and vendor integrations | A framework upgrade alone cannot produce the Linux application |

Only ship **OptiKey Pro**. Do not create Chat, Mouse, Symbol, EyeMine, or other product variants. Shared functionality that Pro uses remains in scope, including speech and custom keyboards. Pro startup also references CommuniKate; inventory its actual Pro behavior before deciding how to migrate it. Do not equate excluding the Symbol product with deleting every communication feature from Pro.

Preserve `/home/q/Downloads/OptiKey` as the original application and behavior reference. Perform all rewrite work, including planning, source code, tests, build output, and packaging, in the sibling directory `/home/q/Downloads/OptiKey-Pro`. Do not add rewrite files to or restructure the original repository. Paths to existing source below are relative to the original repository; new project paths are relative to the sibling directory. Retain the repository's existing GPL licensing and attribution, and inventory licenses for reused dictionaries, fonts, icons, and third-party components before packaging.

Relevant source anchors:

- `src/JuliusSweetland.OptiKey.Pro/App.xaml.cs`: Pro composition, initialization, dynamic keyboard launch and integrations.
- `src/JuliusSweetland.OptiKey.Core/Services/`: input, keyboard output, mouse output, suggestions, speech and window management.
- `src/JuliusSweetland.OptiKey.Core/Observables/TriggerSources/`: fixation, switch, mouse, keyboard, touch and controller triggers.
- `src/JuliusSweetland.OptiKey.Core/Native/PInvoke.cs`: Windows dependencies that need adapters.
- `src/JuliusSweetland.OptiKey.UnitTests/Services/`: existing keyboard output and completion behavior to preserve as fixtures.

## Current priority: Plasma default system keyboard

User direction on 2026-10-18: research Plasma's key/symbol generation and make OptiKey the default system on-screen keyboard before returning to the outstanding hardware and permission-restoration acceptance tasks. See [version-matched research and implementation design](plasma-system-keyboard-research.md). Add a KWin-launched input-method-v1 backend, native text commits, input-panel integration and desktop registration while retaining XML compatibility and physical shortcut/pointer output. Validate the backend before changing the real session's default. Login-screen and lock-screen support require separate acceptance.

## 2. Architecture decision

User decision: **Qt 6 Quick/QML with C++**. C++20 and CMake/Ninja remain proposed implementation defaults. Milestone 1 validates desktop integration; changing the selected language/framework requires a new user decision. Qt supports Linux, Windows and macOS and provides a native Wayland client integration. Arch itself is our project-tested target, not a claim of official Qt distribution support. [Qt platforms](https://doc.qt.io/qt-6/supported-platforms.html), [Qt Wayland client](https://doc.qt.io/qt-6/qtwaylandclient-index.html).

This choice favors the immediate KDE/Wayland integration work and a single native language behind QML. It requires rewriting C# logic; mitigate that cost with behavior fixtures and incremental feature delivery. Avoid introducing a Rust/C++ bridge or a web UI in the first implementation without a demonstrated need.

Separate responsibilities:

| Module | Responsibility |
| --- | --- |
| `core` | Pure selection state machines, dwell timing, smoothing, cancellation, modifiers, text composition, prediction interfaces |
| `ui` | QML keyboard, suggestions, dwell feedback, mouse controls, accessible settings and recovery controls |
| `platform` | Keyboard and pointer output, desktop surfaces, display coordinates, speech, clipboard, permissions and lifecycle |
| `inputs` | Mouse/touch input, replayed gaze, real gaze adapters and switch sources |
| `resources` | Layouts, translations, dictionaries, fonts and icons with provenance |
| `keyboard-import` | Direct loading of legacy keyboard XML into the internal layout model, with actionable incompatibility reports |
| `tests` | Behavioral fixtures, deterministic input replay and native integration harnesses |

Define interfaces for point samples, selection triggers, output actions, window placement, display transforms, speech and capability reporting. Keep platform APIs out of selection logic. Samples carry monotonic timestamps, validity and explicit coordinate space. Output distinguishes text, physical keys and shortcuts; a string is not interchangeable with a sequence of keycodes.

Use a single ordered output queue with cancellation and tracked key/button ownership. Release owned modifiers and buttons on pause, device loss, shutdown and permission loss where the backend still permits release. Test backend disconnect cleanup explicitly. Never replay stale actions after reconnecting.

## Required keyboard compatibility — user requirement

Load the original XML files from [tqphan/optikey-keyboard-en-us](https://github.com/tqphan/optikey-keyboard-en-us) directly, without requiring the user to rewrite them as QML or convert them manually. This is a first usable release requirement, not deferred general XML compatibility. All 29 layouts are in scope, including the game and mouse layouts within Pro; these are not separate product variants.

Reference checkout: `references/optikey-keyboard-en-us`, based on upstream `experimental` commit `c44d4b7a1df273852b676087089e87871c0f4a55` (29 XML files). Local repairs populate the three empty Dolphin variants: the top strip mirrors the bottom strip, and both two-row variants add the existing mouse tools row. Other XML files remain unchanged from that revision. The loader supports automatic row-major placement and Scratchpad content; regression checks cover all 29 layouts and 732 dynamic keys. Full behavioral parity remains unverified.

Observed requirements:

- Grid geometry, spans, percentage sizes, top/bottom floating placement, transparent backgrounds and near-off-screen minimized panels.
- Mixed `row`/`Row` attribute spelling, key-name aliases/case, key groups, colors, opacity, named OptiKey symbols, labels and shifted labels.
- Relative extensionless `ChangeKeyboard` references, `BackReturnsHere`, back navigation, menu visibility and state persistence rules.
- Suggestion rows, scratchpad actions, per-key `CompletionTimes`, and ordered `KeyDown`, `Text`, `Wait`, `KeyUp` sequences. Preserve control-character text, including tab; do not trim it away.
- Modifier and WASD toggles, function/navigation keys, sleep, and 51 distinct action names observed in the XML (derive the authoritative list from the fixture during implementation).
- Advanced mouse operations including drag, held buttons, repeated actions, magnifier, magnetic cursor and look-to-scroll. Inspect the original implementation for their semantics before porting.

Build a loader/action coverage report for every XML file, referenced symbol, navigation target, attribute and command. Validate all 29 files, render every layout, exercise every navigation link, and replay action sequences with assertions for ordering, waits and release behavior. Compare geometry and key behavior against the original implementation. Unsupported operations must be reported; reporting alone does not meet this pack's compatibility acceptance gate.

The first usable release must load the pack unchanged and demonstrate its actions on this machine, including a recoverable minimized panel on Wayland. Move the features required by this pack forward from milestone 3 into milestone 2. If a feature is blocked, bring the specific limitation to the user rather than silently reducing the required scope.

User decision: use explicit platform mappings for shortcuts, with literal behavior available and every override visible. Keep the source XML unchanged. Store mappings separately from the imported pack and show the original sequence and effective platform action. Use deliberate mappings for known shortcuts rather than globally replacing Ctrl or Win modifiers; preserve physical key and game-control semantics. Where intent or an equivalent is unknown, report the unmapped shortcut and expose the literal option rather than inventing behavior. Distinguish successful input delivery from whether the target OS/application assigns the same meaning to that shortcut. Test both mapped and literal execution, including modifier release and ordered macro steps.

## Primary input — user decision

Use **Tobii through the Linux Stream Engine SDK** as the primary input on this machine. The user has confirmed the tracker is currently unplugged: continue development assuming it will be connected during use, do not repeat hardware checks, and leave live-device acceptance pending rather than treating it as a blocker. Integrate against the exact user-supplied library and its matching headers. The user does not need to identify a tracker model before implementation. Discover devices and capabilities at runtime. Mouse and replay input remain development/recovery paths, not substitutes for live library-backed acceptance.

User-supplied references:

- API document: `/home/q/Downloads/tobii_eye_tracker_linux_installer/tobii_stream_engine_api.pdf`.
- Selected library: `/home/q/Downloads/tobii_eye_tracker_linux_installer/stream_engine_linux_3.0.4.6031/lib/x64/libtobii_stream_engine.so`.
- Matching headers exist under that SDK's `include/tobii/` directory. Static inspection identifies the library as Linux x86-64 ELF. No live streaming has been verified.

User decision: target the supplied `stream_engine_linux_3.0.4.6031/lib/x64/libtobii_stream_engine.so` with its matching headers. A separate `tobii-stream-engine-4.24.0` installation exists but is not selected. Do not substitute it or mix headers and binaries. Treat bundled documents and installer scripts as reference material, not authorization to install services or alter the host.

The inspected API provides runtime version queries, device enumeration and information, gaze subscriptions, callback processing and reconnection. Implement a C++ adapter with an owned worker lifecycle, validity filtering, timestamp handling, explicit display mapping and queued delivery into the UI/core. Invalid or stale gaze must cancel selection rather than trigger output. Record model and capabilities through device discovery without logging device serial numbers by default.

Before declaring local Tobii support complete, enumerate devices through the selected library, confirm required services and permissions on Arch, prove live gaze delivery, query calibration availability where supported by this SDK, measure display accuracy and latency, and exercise unplug/replug and suspend/resume. Missing devices or unsupported capabilities must produce actionable diagnostics; they must not require a manually supplied model name to start the application. Separate streaming support from calibration support. Visible USB manufacturer/product strings did not identify a Tobii device during inspection; this is not definitive device discovery.

Keep SDK-specific code behind the input interface so the UI and selection core do not depend on a particular ABI. Decide SDK distribution separately before packaging. Keep all adapter code, probe source, build output and reports in the sibling project.

## UI compatibility — user decision

Preserve imported XML keyboard geometry, colors, symbols, labels, navigation and interaction behavior faithfully, including top/bottom placement and minimized states. Modernize the surrounding Qt interface for settings, device setup, diagnostics and profile management. Visual changes to imported keyboards are optional and must not overwrite the source XML. Compare rendered layouts and behavior against the reference pack during acceptance testing; platform constraints must be surfaced rather than silently redesigning the layouts.

## Existing plugins — user decision

Existing OptiKey plugins are out of scope, not deferred release requirements. No legacy plugin loading or compatibility host will be implemented. This does not reduce compatibility with the required 29 XML keyboard layouts; their required actions will be implemented directly in the new application.

## Settings and profiles — user decision

Start with fresh settings and a new profile format. Do not implement import or migration of legacy OptiKey configuration files. Keep required keyboard XML loading unchanged and separate from application settings. The new format must support versioning, atomic writes, last-known-good recovery and reset; future migrations between versions of this new application remain supported as needed.

## Word prediction — user decision

Use offline dictionary and frequency-based suggestions for the first release, including the required XML keyboard suggestion rows. Local learning is optional and must have controls to disable it and reset learned data. No cloud service or language-model dependency is required. Keep dictionaries and learned data separate so resetting learning preserves the base dictionary. Advanced prediction is outside the first-release scope unless separately agreed.

## Local delivery and updates — user decision

Deliver a build-and-run application in `/home/q/Downloads/OptiKey-Pro` first. Keep source, build output, launch instructions and diagnostics in that sibling project. An installable package is not required for the first prototype. Produce an Arch package after desktop control and live Tobii integration pass acceptance on this machine. Updates are manual initially; an automatic updater is outside the initial scope.

## 3. Milestone 0 — Define Pro behavior and prepare a reproducible build

Deliver a feature inventory marking each Pro feature as first build, Linux parity, hardware-dependent, or explicitly deferred. Include dwell and repeat behavior, multi-key selection, modifiers, text editing, prediction, mouse positioning/click/drag/scroll, speech, layouts, localization, profiles, calibration and extension actions.

Create the build skeleton in `/home/q/Downloads/OptiKey-Pro`, dependency manifest and one-command configure/build/test instructions. Install missing development tooling during implementation; pin a tested Qt/compiler baseline and record the actual host package versions. Establish a Linux build/test job, with shared-code compilation jobs for Windows and macOS once the skeleton is portable.

Extract representative existing keyboard-output and prediction test cases as language-independent fixtures. Inventory WPF-specific behavior rather than attempting automatic XAML conversion.

Exit: a minimal window builds and runs natively on this machine, and the feature inventory makes the first release boundary explicit.

## Linux desktop control — user decision

Use the desktop RemoteDesktop portal and libei first. A narrowly scoped system helper is an accepted fallback only if testing establishes a concrete blocker, including inability to start or recover with the intended assistive input. Record the failure and why the helper resolves it before implementing that fallback.

The main OptiKey application remains unprivileged. Any helper exposes only the required input operations to the authorized local user/session, cannot execute arbitrary shell commands, and releases owned keys/buttons on disconnect. Choose its permissions and input mechanism based on the demonstrated need. A helper does not automatically solve window placement, focus, or compositor restrictions.

This decision approves the architectural fallback, not immediate installation of a privileged service. Present the concrete helper design and installation requirements if it becomes necessary. Test portal permission restoration, denied/revoked access, startup and recovery without depending on a physical mouse.

## 4. Milestone 1 — Prove desktop control on this machine

This is the critical feasibility gate. Build a small native Wayland keyboard panel with a few keys, a dwell indicator, pause/resume, and a pointer test control before investing in the complete interface.

1. Query live portal capabilities and establish an authorized RemoteDesktop session. Prefer its EIS connection with libei; use supported D-Bus input methods as a separate fallback. Do not mix both event paths in the same EIS-connected session. The portal defines user consent and optional session restoration; verify what this KDE backend actually grants. [RemoteDesktop protocol](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.RemoteDesktop.html).
2. Prototype a non-focus-taking keyboard surface using layer-shell-qt on this compositor. Verify pointer interaction, stacking and reserved screen space. Treat these as experiments, not consequences of setting an always-on-top flag. Settings can use a normal focusable window. Opening or closing Settings must preserve ongoing input and the user’s current pause state.
3. Establish display/region mapping for gaze and absolute pointer output, including logical versus physical coordinates. If the backend requires a screen-cast stream for absolute coordinates, request and explain that capability; test the EIS region path separately.
4. Type into a native Wayland editor and browser, plus an XWayland application. Confirm that selecting a keyboard key leaves input focus in the destination. Test letters, Shift, Backspace, Enter and a shortcut.
5. Prove pointer movement, click, drag and scrolling. Handle panel hit-testing independently of injected desktop pointer movement to avoid feedback loops.
6. Exercise denied consent, revoked permission, panel closure, sleep/resume and session recreation. Check whether consent can be completed with the user's available assistive input; record any one-time helper requirement.

Exit: repeated end-to-end typing and pointer operation work in real applications on this KDE Wayland session. A local text box or a screenshot is insufficient. Capture the tested applications, package versions and results in a host acceptance report.

If this gate fails, investigate the specific compositor/backend gap first. A narrowly scoped helper is an accepted architectural fallback under the policy above; its concrete design remains reviewable. Compositor-specific integration beyond that policy is a separate design decision. X11 may be a diagnostic fallback, but switching away from this machine's Wayland session does not satisfy the primary gate. Do not make running the entire application as root the implementation strategy.

## 5. Milestone 2 — First usable OptiKey Pro build here

Deliver a full basic keyboard with letters, numbers, punctuation, navigation, modifiers, repeat/backspace, configurable dwell, visible selection progress and cancellation. Include a scratchpad, basic offline word suggestions, mouse click/right-click/double-click/drag/scroll, Speech Dispatcher speech, adjustable key size, contrast, and persistent settings.

Start with mouse-driven dwell and deterministic gaze replay so work is testable immediately. Implement the Tobii adapter against the selected user-supplied library and matching headers, using runtime enumeration and capability discovery. Verify service availability and calibration requirements through the integration. Test live gaze before describing the build as usable with eye tracking. A webcam mode is a separate deliverable and must not be presented as equivalent without measurement.

Provide a versioned local settings format with atomic writes, last-known-good recovery, and an obvious reset path. Keep typed text and gaze data out of default diagnostics. Provide pause/resume and recovery controls usable through the configured assistive input, plus a caregiver stop shortcut where supported.

Exit checklist:

- Pass the required 29-layout keyboard-pack compatibility gate above.
- Compose, correct and send text to the tested editor and browser without focus loss.
- Move the pointer, select text, drag an item and scroll a page.
- Speak and cancel a scratchpad utterance; speech failure does not disable typing.
- Change dwell and key size, restart, and recover those preferences.
- Complete a 60-minute usage session without a crash, duplicate selections or stuck keys/buttons.
- Pause/recover after input loss and permission loss without unintended output.
- Measure selection-to-output latency separately from configured dwell; initially target p95 under 50 ms on this host and report the measured result.

Record mouse/replay acceptance separately from real-tracker acceptance. This milestone establishes local usability, not full Pro parity.

## 6. Milestone 3 — Complete the Linux Pro feature set

Port advanced selection, multi-key typing, per-key dwell settings, switch/controller inputs as required by the inventory, mouse precision controls, profiles and calibration. Preserve useful Pro layouts, translations, dictionaries and assets. Reimplement prediction behind an interface; implement the agreed offline dictionary/frequency approach, reusing applicable basic completion behavior rather than taking a dependency on the Windows Presage binary. Advanced n-gram prediction is not a first-release requirement.

Implement a compatibility reader for supported dynamic keyboard XML and convert it to an internal layout model. Report unsupported commands explicitly. Never silently execute imported program-launch actions. User decision: do not support existing OptiKey plugins. Do not build a legacy .NET plugin host, compatibility bridge, or plugin migration adapters. Implement the required XML keyboard actions directly. A new plugin system is not part of the current scope and requires a separate user decision.

Add Unicode, dead-key, alternate-layout and IME test cases. Evaluate text commit and explicit clipboard paste separately from physical key injection; clipboard replacement must be visible and restoration must avoid overwriting newer user clipboard content. Keep prediction local and provide a way to disable learning.

Test mixed-DPI monitors, display hotplug, scale changes, rotated screens, disconnecting a tracker, suspend/resume and corrupted settings. Freeze injection until coordinate transforms are valid after display changes. Prefer testing these on the actual host, adding hardware or test rigs where needed.

After the sibling-folder build passes desktop-control and live Tobii acceptance, produce an Arch-installable package with a desktop entry and a diagnostic command. Use manual updates initially. Add a second Linux distribution and GNOME Wayland after KDE acceptance, then an X11 adapter if retained in the support matrix. Treat each compositor as a separate integration target. Evaluate Flatpak only after portal, input and surface behavior is proven inside its sandbox.

Exit: every inventory entry has either passing Linux evidence or an explicit, visible limitation. An unsupported tracker must not appear supported simply because the keyboard runs.

## 7. Milestone 4 — Windows

Implement native keyboard/pointer output, non-activating window behavior, display/DPI mapping, speech and tracker adapters behind the same contracts. Use SendInput where appropriate and test its integrity-level limitations; elevated applications and secure desktop behavior require an explicit support policy. [Microsoft SendInput](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput).

Reuse the legacy build as a regression oracle on Windows, comparing fixtures and observable behavior. Ship a signed installer with rollback and fresh settings; do not import legacy OptiKey configuration. Start with Windows 11 x64; expand architectures only with test hardware and a stated requirement.

Exit: the local-usability checklist and agreed Pro inventory pass on real Windows hardware, including tracker disconnect, focus, permissions and mixed DPI.

## 8. Milestone 5 — macOS

Implement a non-activating native panel bridge, keyboard/pointer output through native event APIs, speech and display transforms. Prototype Accessibility permission onboarding and recovery early in this milestone; determine any additional permissions from the APIs actually used. Plan an Objective-C++ bridge where Qt's abstractions do not cover the required behavior. [Apple CGEvent](https://developer.apple.com/documentation/coregraphics/cgevent).

Start with Apple Silicon and establish the minimum supported OS against the chosen Qt release. Add Intel builds if required and tested. Validate Spaces, full-screen applications, display scaling, keyboard layouts and permission revocation on a real Mac. Produce a signed, notarized application and installer artifact.

Exit: the shared acceptance checklist passes on hardware. Report tracker compatibility per device and OS; Windows SDK availability is not evidence of macOS support.

## 9. Testing and delivery rules

Use deterministic sample replay for dwell timing, jitter, loss of tracking, repeat/cancel behavior and coordinate mapping. Test output sequences for modifier ownership and cleanup, not just individual key presses. Reuse existing behavior fixtures for composition and predictions. Add integration tests for portal/backend failures and platform-contract tests using fake adapters.

CI compilation and offscreen UI tests do not validate desktop injection. Require a real desktop acceptance run for each supported environment and a real device run for each supported tracker. Include accessible names, readable focus/selection feedback and a settings flow operable with the intended input method.

Keep milestones independently runnable. Do not replace the legacy release until the replacement meets its advertised scope. Re-estimate effort after the Wayland experiment and the first tracker integration; those are the largest unknowns, so a firm whole-project timeline now would be misleading.

The first implementation batch is deliberately small: build skeleton, portal capability probe, non-focus-taking panel, dwell selection, external key injection, pointer test, and a recorded acceptance run on this machine. Full UI redesign follows that proof.
