# Remaining Linux feature work

Reviewed against source and existing documentation on 2026-10-24. Completed implementation tasks are omitted; live acceptance remains open where only automated or isolated evidence exists. See [README](../README.md) for implemented controls and [host acceptance](host-acceptance.md) for recorded results.

| Area | Remaining implementation or acceptance |
| --- | --- |
| Selection | Advanced multi-key typing and additional switch/controller trigger adapters; assistive usability of existing dwell/repeat modes |
| Keyboard XML | Original-app visual and behavioral comparison, full state-persistence and disabled/held visual parity; live minimized recovery |
| Action execution | Target-application compatibility, mapped/literal shortcut verification and sustained latency measurements |
| Keyboard output | Real-session native text input; dead keys, alternate layouts, IME and browser/GTK/XWayland compatibility; standalone physical output remains US-based |
| Pointer actions | Configurable movement/drag parameters, modifier suppression/restoration, live gaze targeting and multi-display mapping |
| Look to scroll | Advanced bounds and deadzone overlay; physical-pixel/DPI and real desktop acceptance |
| Magnifier | Live gaze selection and multi-display support |
| Scratchpad | Full composition model and synchronization limitations after destination edits/shortcuts |
| Prediction | Multilingual tokenization, full composition fixtures and live usability acceptance; context is limited to GazeGrid-entered text |
| Speech | Voice settings, live speech/cancellation acceptance and stalled-daemon shutdown behavior |
| Profiles/settings | Profile selection and full assistive settings operation |
| Tobii | Calibration availability/accuracy, live dwell typing, unplug/replug and suspend/resume; streaming through the UI already has recorded evidence |
| Displays | Mixed-DPI, hotplug, rotation and explicit tracker/display calibration mapping |
| Desktop lifecycle | Real permission restoration/revocation, suspend/resume and assistive recovery |
| Plasma input method | Real-session startup/focus/recovery, default selection after validation, native offset/minimized layouts; separate lock/login-screen investigation |
| Pro parity | CommuniKate behavior inventory, remaining layouts/translations and legacy behavioral fixtures |
| Delivery | Reproducible CI, dependency/license inventory, Arch packaging after acceptance, other compositor/distribution validation |
| Windows/macOS | Native adapters, hardware testing and packaging |

The virtual game controller is output support, not an implementation of physical controller trigger input. Existing legacy plugins and settings migration remain explicitly excluded.

Automated checks are defined in `CMakeLists.txt`; `./build.sh` runs them. `tests/render_layouts.py` audits rendering and coverage, and `tests/native_input_method.py` checks isolated KWin integration. These do not certify full pack parity or live assistive usability.
