# OptiKey as Plasma's system keyboard

Research date: 2026-10-18. Priority requested by the user before the outstanding Tobii and permission-restoration acceptance work. This is a researched implementation design; the system keyboard selection has not been changed.

## Host and source baseline

Installed package metadata reports `kwin`, `plasma-workspace` and `plasma-keyboard` version **6.8.0-1**. `~/.config/kwinrc` selects `/usr/share/applications/org.kde.plasma.keyboard.desktop` under `[Wayland]`, with `VirtualKeyboardMode=1`. In this version, the modes are 0 disabled, 1 touch/tablet, 2 touch/tablet/mouse. The display-manager service points to `plasmalogin.service`; login-screen integration must therefore be investigated against Plasma Login, not assumed to use SDDM.

Downloaded and inspected official KDE **v6.8.0** source archives under `/tmp/optikey-plasma-research/`. Current master has a different keyboard framework, so its README is not an accurate description of all installed implementation details. No source files were imported into OptiKey.

## How the installed keyboard generates text, keys and symbols

Plasma Keyboard 6.8 uses Qt Virtual Keyboard internally. Its process sets `QT_IM_MODULE=qtvirtualkeyboard`, loads QML layouts and adapts the resulting Qt events to Wayland input-method-v1. This environment setting is local to the keyboard process; it is not an instruction to set every application's input module. [Startup](https://github.com/KDE/plasma-keyboard/blob/v6.8.0/src/main.cpp).

| Layer | What it means |
| --- | --- |
| QML `key` | Logical Qt key identity, such as `Qt.Key_Q` |
| QML `text` | Character/string produced by the key |
| `displayText` | Visible key label; it can differ from the output |
| `alternativeKeys` | Additional characters offered by a long press |
| Style icons | Visual assets for actions such as Backspace, not text to type |

For example, the German layout defines `Qt.Key_Udiaeresis` with text `ü`, and an `s` key with alternatives `sß`. Breeze renders labels with `control.displayText` and action icons with `Kirigami.Icon`, such as `edit-clear-symbolic` for Backspace. Symbol pages are layout definitions, with fallbacks to Qt's built-in layouts where provided. [Layout example](https://github.com/KDE/plasma-keyboard/blob/v6.8.0/src/layouts/de_DE/main.qml), [Breeze style](https://github.com/KDE/plasma-keyboard/blob/v6.8.0/src/styles/breeze/style.qml), [Qt key properties](https://doc.qt.io/qt-6.10/qml-qtquick-virtualkeyboard-components-basekey.html).

The event adapter follows two main paths:

- **Text:** Qt text/input-method events become `commit_string(serial, text)` and, where appropriate, preedit and surrounding-text operations. This supports Unicode strings rather than requiring a US physical-key sequence for every character.
- **Nontext keys:** Qt key events are converted to XKB keysyms and sent through the input-method context. Return is deliberately treated as a key despite having a newline text representation. Physical keyboard events intercepted for composition have a separate forwarding path.

The adapter translates Qt character offsets to UTF-8 byte offsets for surrounding-text replacement and preedit. It also receives password/sensitive-field hints. [Event adapter](https://github.com/KDE/plasma-keyboard/blob/v6.8.0/src/inputlisteneritem.cpp), [Protocol calls](https://github.com/KDE/plasma-keyboard/blob/v6.8.0/src/inputplugin.cpp).

KWin delivers input-method text to applications through text-input-v2/v3, or its internal text context. Its implementation also has a fallback that maps text to keysyms/keycodes, using a temporary keymap for otherwise unmapped symbols. This fallback is a compatibility mechanism, not proof that every browser, terminal, game or XWayland application behaves identically. [KWin text delivery](https://github.com/KDE/kwin/blob/v6.8.0/src/inputmethod.cpp).

## What makes a keyboard the system default

The Virtual Keyboard settings module discovers application desktop entries marked `X-KDE-Wayland-VirtualKeyboard=true`. The selected desktop-file path is stored as `[Wayland] InputMethod` in `kwinrc`. KWin reads its `Exec` command, launches the process, and supplies a dedicated inherited `WAYLAND_SOCKET`. Only that designated input-method connection is allowed to bind `zwp_input_method_v1` and `zwp_input_panel_v1`. Registering a desktop entry alone therefore does not turn the current OptiKey process into an input method. [Discovery](https://github.com/KDE/kwin/blob/v6.8.0/src/kcms/virtualkeyboard/kcmvirtualkeyboard.cpp), [Launch configuration](https://github.com/KDE/kwin/blob/v6.8.0/src/main_wayland.cpp), [Interface access](https://github.com/KDE/kwin/blob/v6.8.0/src/wayland_server.cpp).

Plasma Keyboard assigns its QQuickWindow the input-panel surface role through a Qt Wayland shell integration. Its keyboard panel requests bottom-center placement. OptiKey currently uses LayerShellQt for top/bottom XML layouts and independent control/target windows. A Wayland surface cannot simultaneously have both roles. We need an explicit surface strategy, including how top-positioned and near-off-screen minimized XML layouts coexist with the compositor's input-panel geometry. [Panel integration](https://github.com/KDE/plasma-keyboard/blob/v6.8.0/src/inputpanelintegration.cpp), [Panel placement](https://github.com/KDE/plasma-keyboard/blob/v6.8.0/src/qwaylandinputpanelsurface.cpp).

The normal system text-input path does not need a RemoteDesktop portal session. Pointer control and persistent physical shortcuts remain separate capabilities. Current upstream development mentions KDE fake-input for modifier emulation; the installed 6.8 source inspected here uses Qt Virtual Keyboard and the input-method adapter. Do not treat master-only behavior as installed behavior.

## Recommended OptiKey design

Retain the XML renderer, gaze/dwell logic, ordered macros and platform boundary. Implement a native Plasma input-method mode, with standalone portal mode retained for other environments and testing.

1. **Wayland input-method backend.** Bind the v1 interfaces through the KWin-provided Qt Wayland connection. Handle context activation/deactivation, serials, reset, content hints, surrounding text and UTF-8 offsets. Discard queued text when its destination context changes; never replay it into the next field.
2. **Separate output intents.** Add committed text and preedit operations alongside physical key down/up. Plain XML text and suggestion insertion can use text commits; tabs/newlines and other control characters require explicit key semantics. Ctrl/Alt/Meta chords, held WASD and literal physical actions must retain ordered press/release behavior. Do not commit `c` as text while executing Ctrl+C. Verify Shift/Caps interaction against existing XML fixtures.
3. **Surface and visibility integration.** Add an input-panel surface for system text entry and preserve a reachable gaze control/recovery surface outside text fields. Resolve top/bottom XML placement before claiming parity. Use Plasma's current visibility controls and explicit invocation; touch-only automatic popup is insufficient for gaze users. Ordinary app focus must remain on the destination.
4. **Sensitive fields.** Use content hints to suppress scratchpad mirroring, prediction learning and automatic speech for passwords/sensitive text; clear context-derived data when leaving a field. This is a functional requirement once OptiKey receives surrounding text.
5. **Desktop registration.** Install a stable executable and desktop entry with `X-KDE-Wayland-VirtualKeyboard=true`, `Type=Application`, `Name=OptiKey Pro`, and `Exec=... --input-method`. KWin owns startup in this mode; avoid a second autostart instance. That flag is proposed and is not implemented yet.
6. **Validation then selection.** First test in nested KWin: discovery, inherited socket, text commits, nontext keys, panel behavior, focus transitions and recovery. Then test native Qt/GTK, browsers, terminals and XWayland, followed by selecting OptiKey in System Settings > Keyboard > Virtual Keyboard. Preserve the previous desktop-file setting for rollback. Live gaze remains a separate hardware gate.

Session-default, lock-screen and login-screen operation are separate acceptance scopes. A user-session desktop setting does not establish login-screen support. Investigate Plasma Login and screen-locker requirements separately, including tracker availability and settings ownership, before advertising a complete system-wide replacement.

## Concrete next implementation milestone

Build `--input-method` with text commit and nontext-key support, a minimal input-panel surface, a desktop-entry template, and an isolated/nested KWin test harness. Preserve all 22 reference layouts and existing standalone diagnostics. Only after that milestone works should the real session's InputMethod setting be switched. The existing app lacks these interfaces today, so changing the setting now would register an incomplete replacement.

## Implementation update — 2026-10-19

The proposed backend, input-panel role and isolated harness are now implemented.
The headless KWin Qt-receiver test passes Unicode, editing keys and private-field
input after field entry is retried following keyboard startup. CMake generates a
validated desktop entry in `build/`. Earlier statements above describing missing
interfaces are historical. Immediate startup activation, real-session integration
and selecting the default keyboard remain pending; see `host-acceptance.md`.

## Startup recovery update — 2026-10-19

The receiver focus retry has been removed. OptiKey now asks KWin to re-adopt a
context once when activation arrives without committed state; it still waits for
protocol state before enabling output. This uses the existing writable `active`
property, whose setter calls `setActive`, which re-adopts the current context.
See [KWin property implementation](https://github.com/KDE/kwin/blob/v6.8.0/src/virtualkeyboard_dbus.cpp)
and [context adoption](https://github.com/KDE/kwin/blob/v6.8.0/src/inputmethod.cpp).
Both normal and deliberately delayed startup pass the isolated receiver check.
The desktop entry is now registered locally but not selected as the default.

## Top placement implementation — 2026-10-19

Top placement now uses layer-shell while native text commits remain connected.
Returning to bottom placement recreates the surface with the input-panel role.
The same Wayland surface is never assigned both roles. This supersedes the earlier
top-placement limitation; offset layouts remain deferred. The top overlay stays
visible for assistive controls outside text fields and does not participate in
KWin's native panel visibility or application-resizing behavior.
