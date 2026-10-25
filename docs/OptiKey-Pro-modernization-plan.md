# GazeGrid modernization — remaining work

Reviewed against the local implementation on 2026-10-24. Completed implementation tasks have been removed. Usage is documented in [README](../README.md); recorded validation is in [host acceptance](host-acceptance.md). Implementation alone does not close a live acceptance gate.

## Scope and retained decisions

- Deliver Pro functionality only, first on Arch Linux/KDE Wayland, then Windows and macOS. Preserve the original `/home/q/Downloads/OptiKey` as the behavior reference; keep rewrite work in this project.
- Use Qt 6 Quick/QML, C++20 and CMake. Keep platform APIs behind input/output interfaces and selection logic independent of SDKs.
- Support direct loading of the required XML keyboard pack, including game and mouse actions. Preserve source layouts and make optional visual overrides explicit. Full original-app behavioral parity remains a release gate.
- Use explicit, visible platform shortcut mappings with a literal option; preserve physical game-control semantics.
- Use the supplied Tobii Stream Engine 3.0.4.6031 library and matching headers. Do not substitute the separate 4.24 SDK. Determine SDK redistribution separately before packaging.
- Keep prediction offline and learning optional and resettable. Do not load legacy plugins or migrate legacy OptiKey settings. A new plugin system and automatic updater are outside the agreed scope.
- Deliver locally first; produce an Arch package after desktop-control and live Tobii acceptance. Retain GPL licensing and resource attribution.
- Prefer RemoteDesktop portal/libei. A narrowly scoped, unprivileged-client system helper is an accepted fallback only after demonstrating a specific blocker. Any helper design and installation requirements must be reviewable; this is not authorization to install a privileged service.

## First usable Linux release

See [remaining feature work](feature-inventory.md) for the detailed backlog. Outstanding release checks are:

- Compare the required pack's geometry, symbols, navigation, state persistence and action semantics with the original application; exercise mapped and literal shortcuts in target applications.
- Validate typing and focus in real editors, browsers and XWayland applications, including Unicode, dead keys, alternate layouts and IME behavior where supported.
- Confirm live gaze calibration, dwell typing, pointer targeting, magnifier selection and minimized recovery. Exercise tracker disconnect/reconnect and suspend/resume.
- Verify remembered portal authorization, denial/revocation, session recreation and recovery using the intended assistive input.
- Verify speech and cancellation, settings operation and preference recovery with the intended input method.
- Exercise mixed-DPI, rotated displays, hotplug and coordinate remapping on hardware.
- Complete a 60-minute session without crashes, duplicate selections or stuck input. Measure selection-to-output latency separately from dwell; initial target is p95 below 50 ms on this host.

Plasma system-keyboard acceptance remains a priority: validate real-session startup, focus and recovery before selecting it as the default. Offset/minimized native layouts, login-screen and lock-screen integration remain separate work. See [Plasma research](plasma-system-keyboard-research.md).

## Remaining Linux delivery

Complete advanced selection and trigger adapters, text composition, profile selection, calibration and localization as detailed in the inventory. Inventory CommuniKate's actual Pro behavior before deciding its migration scope.

Establish reproducible CI and extract representative legacy behavior fixtures where coverage is missing. Inventory dependency/resource licenses and pin the tested toolchain for packaging. Add Windows/macOS shared-code compilation when portable.

After local acceptance, produce an Arch package with desktop registration and diagnostics. Validate a second distribution and GNOME Wayland separately. An X11 adapter remains conditional on the support matrix; evaluate Flatpak after input, portal and surface behavior is proven inside its sandbox.

## Windows

Implement native keyboard/pointer output, non-activating windows, display/DPI mapping, speech and tracker adapters. Define elevated-application and secure-desktop limitations. Use the legacy build as a regression oracle on Windows 11 x64. Deliver a signed installer with rollback and fresh settings, then pass the shared usability checks on real hardware.

## macOS

Implement a non-activating panel bridge, native event output, speech, display transforms and Accessibility permission onboarding/recovery. Start with Apple Silicon; establish the minimum OS against the selected Qt release. Validate Spaces, fullscreen, scaling, keyboard layouts and permission revocation on hardware. Deliver signed, notarized artifacts. Report tracker support per device and OS.

## Acceptance rules

Keep deterministic replay and fake-backend tests separate from real desktop/device evidence. Test owned-key/button cleanup and cancellation through failures; never replay stale actions after reconnecting. Do not claim a supported environment or tracker based only on compilation, rendering or handler availability. Keep typed text and gaze coordinates out of default diagnostics.
