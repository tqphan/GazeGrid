# Virtual game controller

Open **Gamepad** from either game toolbar, or choose **Virtual Game Controller**
in the keyboard selector. Both circular joystick keys respond continuously to gaze
or mouse hover. The center 15% is a dead zone; moving farther out increases the
axis value. Leaving a joystick centers it. LB, RB and D-pad keys toggle held
buttons; use **Release all** to clear them. Face buttons tap for 100 ms.

This implements virtual output only, using the standard Linux gamepad mapping:
https://kernel.org/doc/html/latest/input/gamepad.html
The device appears as `OptiKey Virtual Controller`. Games must support Linux
controllers (or use Steam Input). This does not impersonate every proprietary
controller, implement motion sensors/touchpads, or receive force feedback.

## Device access

The Linux `uinput` module must be loaded and the current user must have write
access to `/dev/uinput`. Desktop portal authorization does not grant that access.
Use your distribution's controller-device permissions package or ask your
administrator for a session-scoped udev rule for uinput. Do not run OptiKey as root.
An access failure is shown in OptiKey's action status. The device is created on
first non-neutral output; a game may take a moment to discover it.

## Actions

Every action below works in `<Action>...</Action>` in a DynamicKey or macro.
Names are case-sensitive. Buttons use `Gamepad` + name + operation:

- Names: `A`, `B`, `X`, `Y`, `C`, `Z`, `LeftShoulder`, `RightShoulder`,
  `LeftTriggerButton`, `RightTriggerButton`, `Back`, `Start`, `Guide`,
  `LeftStick`, `RightStick`, `DpadUp`, `DpadDown`, `DpadLeft`, `DpadRight`,
  and `Button1` through `Button32`.
- Operations: `Press`, `Release`, `Toggle`, `Tap` (100 ms).
- Example: `<Action>GamepadAPress</Action><Wait>500</Wait><Action>GamepadARelease</Action>`.

Axes use `Gamepad` + axis + `:` + normalized value:

- `LeftX`, `LeftY`, `RightX`, `RightY`, `Throttle`, `Rudder`, `Wheel`: -1 to 1.
- `LeftTrigger`, `RightTrigger`: 0 to 1.
- `HatX`, `HatY`: -1, 0, 1 (fractional values round to the nearest position).
- Example: `<Action>GamepadLeftX:-0.5</Action>`; positive Y means down.
- `GamepadReset` releases every button and centers/zeros every axis.

Buttons and axes persist until released/reset. Pause, tracking loss, layout
changes, recovery and action failure reset controller state. Conversation layouts
with `SimulateKeystrokes=false` never send controller output. Desktop input
permission is not required for controller-only macros.

## Joystick keys

Place this in a keyboard's Content, using normal grid or canvas geometry:

```xml
<Joystick row="0" col="0" width="3" height="3" stick="Left">
  <Label>Joystick</Label>
</Joystick>
```

`stick` is `Left` (default) or `Right`. Joystick keys operate continuously without
dwell activation. Looking away centers the stick so users can safely select
another action. Use discrete axis actions when a stick must stay deflected while
selecting other keys.
