# HID++ button

`hidpp_button` reports one control of a Logitech device, such as a button that
normally performs a firmware function, as an ordinary pointing button. The
pointing button can then be used in any rule.

Add it to the device entry of the selected profile. Karabiner-Elements must be
modifying the device (`"ignore": false`):

```json
{
    "identifiers": {
        "is_pointing_device": true,
        "product_id": 45111,
        "vendor_id": 1133
    },
    "ignore": false,
    "hidpp_button": {
        "control_id": 196,
        "pointing_button": "button17"
    }
}
```

The control is then reported as `button17` and can be changed by a rule, for
example to hold `right_control` while it is pressed:

```json
{
    "description": "MX Anywhere 3S top button to right_control",
    "manipulators": [
        {
            "type": "basic",
            "from": {
                "pointing_button": "button17",
                "modifiers": { "optional": ["any"] }
            },
            "to": [{ "key_code": "right_control" }],
            "conditions": [
                {
                    "type": "device_if",
                    "identifiers": [{ "vendor_id": 1133, "product_id": 45111 }]
                }
            ]
        }
    ]
}
```

Remove `hidpp_button` to give the control back to its firmware function.

## Configuration

| Field             | Values                                                        |
| ----------------- | ------------------------------------------------------------- |
| `control_id`      | Integer from 1 to 65535 (the HID++ control ID, CID). Required |
| `pointing_button` | Pointing button name, such as `button17`. Required            |

Other keys are errors. An invalid `hidpp_button` is not applied.
One control can be configured per device.
Use `button1` to `button32` if the button is not changed by a rule, because the
virtual pointing device reports only those buttons.

## Supported devices

Only devices that meet all of the following:

- Logitech vendor ID (`0x046d`, 1133).
- A pointing device.
- Connected directly over Bluetooth Low Energy.
  USB, classic Bluetooth, and Logitech receivers (Unifying, Bolt) are not supported.
- Its report descriptor declares the HID++ long report (report ID `0x11`).
- It supports `REPROG_CONTROLS_V4` (`0x1b04`) and the control can be temporarily diverted.

## Choosing the values

`core_service.log` lists the controls that can be used when `hidpp_button` is
configured for a supported device:

```text
... hidpp_button: divertable controls: 82 (0x0052), 83 (0x0053), 86 (0x0056), 196 (0x00c4), 215 (0x00d7)
... hidpp_button: activated: control 196 (0x00c4) is diverted (REPROG_CONTROLS_V4 feature index 0x0a)
```

`pointing_button` must be a button which the device does not report by itself.
Karabiner-Elements cannot tell a physical press from a press of the same button
reported through `hidpp_button`. For example, the MX Anywhere 3S declares
`button1` to `button16`, so use `button17` or higher:

```text
... hidpp_button: not activated: the device can report pointing button 6 itself (declared: button1-button16); choose a pointing_button which the device does not declare
```

When `hidpp_button` is not activated, the log shows the reason, and the control
keeps its firmware function.

## Behavior

- The control is diverted temporarily. Nothing is stored in the device, and the
  device resets the diversion when it reconnects.
- A control which is already diverted or remapped, for example by another
  application, is not taken over.
- The control is restored when the setting is removed or changed, when the device
  is no longer modified, and when Karabiner-Core-Service stops. A held button is
  released at the same time.
- If the device does not answer or rejects a request, the control is restored and
  left to the firmware until the device is grabbed again (reconnect, wake, restart,
  or configuration change).
- If only the discovery before the diversion times out (the device did not answer
  while Karabiner-Elements looked up the feature, the controls, or the reporting
  state), it is retried once for the same device when the device next reports
  pointer movement or scrolling. Pressing the button does not retry it. A refusal,
  an error reply, a failed diversion, or a second timeout is not retried, and the
  control stays with the firmware until the device is grabbed again. This is
  not guaranteed to fix a device that stays unresponsive, and until the retry
  succeeds, presses of the button may still perform its firmware function.
- If Karabiner-Core-Service stops unexpectedly, the control can stay diverted
  until the device reconnects. Turn the device off and on to restore it.
- Do not let another application, such as Logi Options+, manage the same control.
