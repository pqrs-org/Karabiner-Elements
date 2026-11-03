# Logitech HID++ button

`logitech_hidpp_button` diverts controls of a Logitech device, such as buttons
that normally perform firmware functions, and maps them directly to ordinary
Karabiner-Elements output events.

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
    "logitech_hidpp_button": {
        "controls": [
            {
                "control_id": 82,
                "to": [{ "pointing_button": "button4" }]
            },
            {
                "control_id": 196,
                "to": [{ "key_code": "right_control" }]
            }
        ]
    }
}
```

Remove a control from `controls` to give that control back to its firmware
function. Remove `logitech_hidpp_button` when no controls are selected.

## Configuration

| Field        | Values                                                 |
| ------------ | ------------------------------------------------------ |
| `controls`   | Non-empty array of control mappings. Required          |
| `control_id` | Integer from 1 to 65535 (the HID++ control ID, CID)    |
| `to`         | Non-empty Simple Modifications-compatible output array |

Other keys are errors. An invalid `logitech_hidpp_button` is not applied.
Duplicate control IDs are errors. Controls are stored in ascending ID order.

## Supported devices

Only devices that meet all of the following:

- Logitech vendor ID (`0x046d`, 1133).
- A pointing device.
- Connected directly over Bluetooth Low Energy.
  USB, classic Bluetooth, and Logitech receivers (Unifying, Bolt) are not supported.
- Its report descriptor declares the HID++ long report (report ID `0x11`).
- It supports `REPROG_CONTROLS_V4` (`0x1b04`) and the control can be temporarily diverted.

## Choosing the values

For a supported Logitech device, the Devices tab lists the controls that can be
used. Enable a checkbox and choose its output from the picker. Clearing a
checkbox returns that control to the device firmware.

`core_service.log` also lists the detected and activated controls:

```text
... logitech_hidpp_button: divertable controls: 82 (0x0052), 83 (0x0053), 86 (0x0056), 196 (0x00c4), 215 (0x00d7)
... logitech_hidpp_button: activated: control 196 (0x00c4) is diverted (REPROG_CONTROLS_V4 feature index 0x0a)
```

When `logitech_hidpp_button` is not activated, the log shows the reason, and the
control keeps its firmware function.

## Behavior

- The controls are diverted temporarily. Nothing is stored in the device, and the
  device resets the diversion when it reconnects.
- If any selected control is already diverted or remapped, for example by another
  application, none of the selected controls are taken over.
- The controls are restored when the setting is removed or changed, when the device
  is no longer modified, and when Karabiner-Core-Service stops. Held buttons are
  released at the same time.
- If the device does not answer or rejects a request, the controls are restored and
  left to the firmware until the device is grabbed again (reconnect, wake, restart,
  or configuration change).
- If only the discovery before the diversion times out (the device did not answer
  while Karabiner-Elements looked up the feature, the controls, or the reporting
  state), it is retried once for the same device when the device next reports
  pointer movement or scrolling. Pressing the button does not retry it. A refusal,
  an error reply, a failed diversion, or a second timeout is not retried, and the
  controls stay with the firmware until the device is grabbed again. This is
  not guaranteed to fix a device that stays unresponsive, and until the retry
  succeeds, presses of the button may still perform its firmware function.
- If Karabiner-Core-Service stops unexpectedly, controls can stay diverted
  until the device reconnects. Turn the device off and on to restore it.
- Do not let another application, such as Logi Options+, manage the same controls.
