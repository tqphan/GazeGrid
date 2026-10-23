# OptiKey Pro — Linux prototype

Qt 6 Quick/QML and C++20 implementation in its own sibling project. The original OptiKey repository is unchanged. This is the initial integration prototype, not the completed Pro rewrite.

## Build and run on this machine

```sh
cd /home/q/Downloads/OptiKey-Pro
./build.sh
./run.sh
```

CMake and Ninja are installed locally in `.tools/`. The build uses the host Qt 6 development packages, layer-shell-qt, libei and libspeechd (Speech Dispatcher). Settings also uses the KDE Kirigami and QQC2 desktop-style QML modules, with Qt Widgets providing native desktop control styling. No system helper was installed. `build.sh` also runs the core tests. Runtime settings and caches stay under `runtime/` when using `run.sh`.

The default SDK is the exact user-supplied `stream_engine_linux_3.0.4.6031` directory. Its headers are used for compilation and its x64 shared library is loaded by absolute path at runtime. The sibling 4.24 SDK is not used. SDK files are not copied into this project. The CMake cache variable `TOBII_SDK` allows relocating the same SDK.

## Current controls and implementation

- **Authorize** requests keyboard/pointer access through KDE's portal. The preferred backend is libei; unsupported EIS can use portal D-Bus methods in a separate session mode.
- The imported XML keyboard opens directly. **Settings** honors XML menu visibility; enable **Include layouts hidden by the XML pack** to select any of the 29 layouts; navigation keys and **Home / recover** also change layouts. Geometry, symbols, key groups, ordered macros, modifier toggles, scratchpad text, dictionary suggestions and editable platform mappings have implementations. Full behavior and appearance parity remain unverified.
- **Modifier keys** (Shift, Ctrl, Alt, and Win) cycle through off → latched → locked → off. A **Latched** badge means the modifier releases after the next text or function key; a filled highlight and **Locked** badge mean it stays held until selected again. Latched and locked modifiers carry over between keyboard layouts. Pause and cancellation clear both states. Explicit XML `KeyToggle` commands retain their held/released behavior.
- **Tobii** connects through the selected SDK. **Gaze** also connects automatically and enables gaze selection on the primary display, with a gaze marker drawn directly on the keyboard. Settings → Status reports whether valid on-screen gaze is arriving. The tracker is assumed calibrated to that display. Live streaming has been verified; calibration and dwell acceptance remain pending. Display changes pause input and require checking calibration before resuming.
- **Pause** releases owned input and cancels pending actions. A keyboard key containing a single `<Action>Sleep</Action>` toggles pause/resume, including while paused or a macro is running. The Pause key uses the same active highlight as Magnifier; it is highlighted while paused. Click again to resume, or look away and dwell on the key again. Dwell can activate Pause and recovery controls during a waiting macro. Move away and back before dwelling on Resume.
- Pointer actions support target selection, clicks, double-clicks, relative movement, scrolling and held buttons. **Drag** chooses a source and destination, then performs a cancellable sequence. Repeat reuses the completed target for targeted clicks and drags. Direction keys move by the original default of 25 pixels. A plain left click acts at the current desktop pointer; use gaze selection when that must remain independent of the panel.
- **Magnetic cursor** follows valid gaze while Gaze is enabled. **Look to scroll** selects an anchor, then scrolls in either axis beyond a 50-pixel deadzone, with the original default medium-speed calculation. Looking over the keyboard or opening Settings suppresses scrolling. The Look to scroll key stays highlighted while active. Brief tracking loss pauses scrolling for up to 750 ms; fresh gaze resumes it without accumulated movement. Longer tracking loss, cancellation, or pause stops it. Magnetic cursor stops immediately on tracking loss. Settings → Look to scroll offers speed (10–300%, default 100%), centre dead zone (10–200 pixels, default 50), and direction (both axes, vertical only, or horizontal only). Changes apply immediately; Save preferences keeps them after restart. Advanced scroll-area bounds and its overlay remain pending.
- **Speak** and **Stop speech** use an owned Speech Dispatcher connection on a worker thread. Stop cancels this app’s speech, and failures are reported without blocking typing. Prediction uses a bundled offline English frequency and context model, ready on first launch. It ranks completions using the preceding three words and offers next-word suggestions after a space. Optional local learning gives personal words a bounded boost; it is not required for useful defaults. Model attribution, rebuilding, and evaluation are documented in [resources/prediction/README.md](resources/prediction/README.md). Live voice and prediction usability acceptance remain pending.

Standalone portal output uses US physical keycodes. Experimental KWin input-method mode also supports native Unicode text commits. **Magnifier** highlights when enabled, automatically remembers its state between launches, and enables magnified target selection using the desktop Screenshot portal. Set zoom from 2× to 8× (default 4×) in **Settings → Magnifier**, then save preferences. Enable **Progressive zoom** in the same settings category to grow magnification from 1× to the selected zoom over 0.5–5 seconds (default 1.5 seconds). During the animation, zoom stays focused on the initially selected target; gaze and pointer movement do not steer it. The view holds still for precise selection after the animation finishes. Leave it off for immediate magnification. This uses one screenshot per target, with no live screen-sharing session. To use it: select a pointer action, dwell on an approximate point, approve capture if prompted, then dwell on the precise point after zoom finishes. No physical mouse click is needed. **Select pointer targets by dwell** is enabled by default in **Settings → Magnifier**, with separate start (750 ms) and confirmation (1000 ms) sliders and a movement tolerance slider (40 px). Both gaze and pointer hover work; a ring shows dwell progress. Save preferences to retain changes. Capture cancellation cancels the action. OptiKey registers its application identity before normal portal use so supported desktops can remember screenshot approval for this app. Screenshot approval is managed by the desktop and can be revoked; it does not use the keyboard/mouse restore token. This currently requires one display; native KDE capture and precise-click acceptance passed; live gaze acceptance remains pending. Unknown actions are rejected before any part of their macro executes. Absolute pointer targeting requires one EIS display region. Successful XML loading or action dispatch does not establish full pack compatibility. No existing plugins or legacy settings are loaded.

## Diagnostics and tests

Preferences now use a versioned `profile.json` under the application configuration directory (`runtime/config/OptiKey/OptiKey-Pro/` with `run.sh`). In Settings, choose **Save preferences and startup keyboard** to persist dwell time, keyboard height, high contrast, literal-key mode, local-learning preference, and the currently selected keyboard. The interface and keyboard follow the system light/dark theme; high contrast is an explicit override. The XML is unchanged, and minimized recovery layouts use a borderless restore button flush with the selected screen corner. Its width, height, and opacity (10–100%) are configurable under Settings → Keyboard → Restore button. Changes take effect immediately; saving is explicit. **Reset preferences** restores defaults and leaves input paused. Reset does not delete learned words or shortcut mappings; learned words have their own reset control.

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

`--gaze` starts with gaze enabled. `--gaze-check` runs a 20-second live check through
the Tobii worker, controller and QML interface, with dwell disabled and output
paused. It reports aggregate valid-sample and keyboard-hit counts, without
recording individual gaze coordinates or requesting desktop authorization.

`--validate-keyboards` verifies XML parsing, geometry, command syntax and local navigation targets. It reports per-layout actions, unsupported actions, symbols and attribute names, preserving command order and Text whitespace. Implementation categories are not an action compatibility certification; `behavioralParityVerified` remains false.

`--smoke` displays the panel briefly and closes it. Set `OPTIKEY_SCREENSHOT` to a file path to capture the panel itself.

`--portal-check` opens a permission session and waits for usable libei devices without emitting input. `--input-check` opens a separate disposable receiver after authorization, waits for receiver focus, and sends `optikey` plus Enter. It reports receipt and closes. Click the receiver if KDE does not activate it automatically. Do not switch focus during this short test. The check also clicks the XML q key and verifies that the receiver keeps focus and receives q. It stops if the receiver loses focus. Passing does not establish browser/XWayland compatibility.

See `docs/host-acceptance.md` for observed results and outstanding gates, and `docs/OptiKey-Pro-modernization-plan.md` for the agreed decisions.

`tests/render_layouts.py` creates offscreen snapshots and a coverage report in `runtime/render-audit/`, using an isolated temporary profile. The build test suite includes a fake screenshot portal on an isolated D-Bus session (`dbus-run-session`); it never invokes the real screenshot portal. Restricted sandboxes must allow creation of the local test-bus socket.

`--pointer-check` opens a disposable full-screen Wayland receiver and exercises targeted left click, double-click, drag, scroll and right click. It requires one mapped display and stops on receiver focus loss. Keep that receiver focused during the short test. `--settings` opens settings without changing the input pause state.

`--magnifier-check` verifies screenshot capture and a precise target click against the disposable receiver. Keep it focused and approve capture if prompted.

Settings can opt into remembering desktop-control permission where the portal supports it. Save preferences to retain the choice and automatically connect keyboard/mouse control on subsequent launches. The desktop may still ask for permission if it cannot reuse the saved authorization. Diagnostic and smoke modes do not automatically connect; input checks manage their own sessions. Restore tokens are stored with owner-only permissions and consumed once before requesting a replacement. **Disconnect and forget desktop permission** closes input and deletes the local token; save preferences afterward to keep automatic connection disabled on future launches. Desktop permission management remains available in KDE settings.

## Experimental Plasma input method

`--input-method` must be launched by KWin with its inherited Wayland socket.
CMake generates `build/org.optikey.Pro.InputMethod.desktop` pointing to this build;
the build does not automatically install or select it. On this machine the entry
has been registered in the user application directory and Plasma’s cache refreshed,
but Plasma Keyboard is still selected.

Run `python3 tests/native_input_method.py` to check 22 top/bottom placement-button
clicks, focus retention, Unicode commits, Backspace, Return and password-field
input against a separate Qt receiver in a private,
headless KWin session. The test needs permission to create local D-Bus sockets.
It writes `runtime/native-input-method.log` and does not change the desktop's
keyboard selection. The receiver keeps its original field focused. Add `--late-start` to delay the
keyboard by one second and verify recovery of that already focused field.

Native mode supports top and bottom layouts. The first placement key moves the
keyboard between them. Top placement uses a non-focus-taking overlay; returning
to the bottom restores the native input panel. Native text input stays connected
through both transitions. Settings is a regular movable, resizable desktop window. Top mode
covers application content instead of asking applications to resize, and remains
visible outside text fields for gaze/pointer controls. Plasma's native panel
visibility controls apply to bottom placement. Offset/minimized layouts still
require standalone mode and are labeled unavailable in Plasma mode.
Pointer actions and held physical shortcuts still require desktop authorization.
Real-session focus/recovery, GTK/browser/XWayland compatibility, and gaze
acceptance remain outstanding. Selectable registration alone does not establish
that OptiKey is ready to replace Plasma Keyboard.

Click the OptiKey system tray icon, or choose **Settings…** from its menu, to open
Settings. Authorize, Tobii, recovery, gaze, and speech controls are
available there in every category. There is no separate controls window.
Opening Settings keeps the keyboard’s current pause state. Closing Settings keeps OptiKey
running. Choose **Quit OptiKey** in Settings or the tray menu to exit.
The keyboard's Settings command and `--settings` also open the same window.
Settings controls use pointer clicks; gaze hit testing remains on the keyboard
and target-selection overlay. To send text, authorize desktop output and focus
the destination text field.

Settings uses KDE desktop controls and theme colors, with a left sidebar grouping options into **Status**, **Keyboard**, **Gaze & dwell**, **Magnifier**, **Typing & speech**,
**Desktop access**, and **Shortcuts**. Live gaze, tracker, desktop-control, and last-action
messages appear under **Status**. Magnifier groups zoom behavior and target dwell controls. Keyboard groups layout, appearance, and restore-button options. Tracker, speech, and desktop connection actions live on their respective pages. Save/reset preferences remain available in every category.

Gaze smoothing is available in **Settings → Gaze & dwell**: Off (0), Low (1), Medium (2), High
(3), and Extreme (4), plus **Smooth when changing gaze target**. Low is the
prototype's default; target-change smoothing defaults off. Changes apply
immediately; **Save preferences and startup keyboard** persists both options.
Older profiles without these fields use the same defaults.

The filter ports the original OptiKey `DataFilters/KalmanFilter.cs` equations and
optional weighted measurement history. It processes display-coordinate distances
before converting back to normalized gaze, so the marker, dwell selection, and
pointer actions share one result. Unlike upstream's zero initialization, the
first valid point seeds the estimate and history. Invalid/stale tracking, display
changes, and smoothing setting changes reset the filter. Tracking validity and
freshness checks still run before smoothing.

When minimized in standalone mode, look at the **↗ restore button** for 750 ms
to bring the keyboard back. Settings offers **Restore button corner** and
**Restore button width** and **Restore button height** (40–128 logical pixels each). The default is 64 × 64 pixels at
the bottom left, flush with the screen edges. Changes apply immediately;
use Save preferences to retain them. The square stays fully on-screen and
ignores reserved panel space so its displayed position matches gaze detection.
Imported XML files are unchanged.

Gaze indicator appearance is configurable in **Settings → Gaze & dwell**: diameter
(4–120 px), opacity (5–100%), and a solid RGB color (`#RRGGBB`). The default is
a solid blue circle, 18 px at full opacity. Optional **Morph with gaze acceleration
and direction** stretches it along gaze movement, with bounded deformation based
on acceleration. It returns to a circle at rest and resets after tracking loss.
These visual effects do not change gaze targeting. Use **Save preferences and
startup keyboard** to retain changes.

The gaze indicator is drawn directly on the keyboard and clipped at its edges,
including the minimized restore button. It hides when looking outside the keyboard
or when the keyboard is hidden. Size, opacity, color, and morphing remain configurable.

**Settings → Keyboard → Temporarily hide keyboard during mouse actions** hides
the panel from the start of a mouse action through target selection and execution.
The keyboard returns on completion, failure, or cancellation. Target selection
and its Cancel button remain available. Continuous magnetic cursor and look-to-scroll
modes restore the keyboard after activation so their controls remain accessible.
The option defaults off and is retained with Save preferences.

Enable **Settings → Gaze & dwell → Cycle modifiers while hovering** to advance
Shift, Ctrl, Alt, and Win at the configured repeat rate without looking away. The option
defaults off, applies to modifier keys only, and is retained with Save preferences.
Explicit KeyToggle modifier keys retain their two-state toggle behavior.

**Settings → Gaze & dwell** also has independent **Repeat letters**, **Repeat
Backspace**, and **Repeat arrow keys while hovering** options. All default off.
Enabled keys repeat at the configured repeat rate; moving away stops repetition.
The arrow option covers Left, Right, Up, and Down. Letter repetition applies to
single-letter text keys, not predictions, digits, punctuation, or macros. Save
preferences to retain these choices.

**Repeat rate** sets the interval between repeat activations from 100–2000 ms
(10–0.5 repeats per second), default 750 ms. It applies to enabled letter,
Backspace, arrow, and modifier repetition. The initial activation still uses
Keyboard dwell. Each repeat waits for the previous action to finish.

Existing restore-button size preferences initialize both width and height to the
previous square size. New width and height values save independently.

**Repeat punctuation while hovering** independently enables repetition for
single-character punctuation and symbol keys, such as `.`, `,`, `!`, `?`, `+`,
and `$`. It defaults off and uses the shared repeat rate. Digits, spaces,
predictions, and multi-character text keys are excluded. Save preferences to
retain the option.

**Enable gaze input** saves automatically. When enabled, the next launch restores
gaze input and reconnects the eye tracker. When no valid gaze is available, mouse
dwell takes over for keys, predictions, and target selection. Valid gaze automatically
takes priority again; switching inputs starts a fresh dwell. Reset preferences turns it off.

In **Settings → Typing & speech → Word prediction**, choose **Context-aware**
(the default), **Common words** (frequency ranking without sentence context),
or **Off**. You can separately disable suggestions after a space, or enable
**Use and learn personal words locally**. Turning personalization off stops
learning and ignores existing personal words without deleting them. Changes
apply immediately; use **Save preferences and startup keyboard** to keep them.

## Freely positioned keys

XML keyboards can replace `<Grid>` with `<Canvas><Width>100</Width><Height>100</Height></Canvas>`.
On a canvas, every `DynamicKey`, `Scratchpad`, and `SuggestionRow` specifies
`X`, `Y`, `Width`, and `Height`, including decimal values:

```xml
<DynamicKey X="12.5" Y="20" Width="18" Height="9.5">
  <Label>A</Label><Text>a</Text>
</DynamicKey>
```

Coordinates start at the canvas's top-left and scale with the keyboard window.
A 100 × 100 canvas makes each unit one percent of the window; any positive canvas
size is supported. Set the keyboard's top-level `Width` and `Height` to `100%`
(and keyboard height scaling to 100%) to place keys across the screen.
Each rectangle must have positive dimensions and fit inside the canvas.
Keys may overlap; the last item in XML is drawn on top and receives targeting.
Empty areas do not activate keys. Key contents are clipped to their rectangles.
`SuggestionRow` accepts `Count` (1–64, default 4), dividing its width equally.
Grid layouts retain their existing cell sizing and automatic placement.

See [the complete example](docs/examples/freeform.xml). Copy it into your keyboard
pack directory and select it in Settings → Keyboard after restarting. Placement
is configured through XML; there is no drag-and-drop layout editor.

Virtual controller output and gaze/mouse joystick keys are available from the game toolbar’s **Gamepad** key. See [controller actions and setup](docs/game-controller.md).
