# Mouse motion and wheel to key

`mouse_motion_and_wheel_to_key` converts pointer movement or wheel input into
key taps. This example converts movement to arrow keys while fn is pressed:

```json
{
    "description": "fn + pointer movement to arrow keys",
    "manipulators": [
        {
            "type": "mouse_motion_and_wheel_to_key",
            "from": {
                "source": "xy",
                "threshold": 32,
                "sampling_interval_milliseconds": 100,
                "modifiers": {
                    "mandatory": ["fn"],
                    "optional": ["any"]
                }
            },
            "to": {
                "up": [{ "key_code": "up_arrow" }],
                "down": [{ "key_code": "down_arrow" }],
                "left": [{ "key_code": "left_arrow" }],
                "right": [{ "key_code": "right_arrow" }]
            }
        }
    ]
}
```

## Configuration

| Field                                 | Values                                                            | Default  |
| ------------------------------------- | ----------------------------------------------------------------- | -------- |
| `from.source`                         | `xy`, `wheels`, `horizontal_wheel`, `vertical_wheel`              | Required |
| `from.threshold`                      | Positive integer in input delta units, not pixels or scroll lines | `1`      |
| `from.sampling_interval_milliseconds` | Positive integer in milliseconds                                  | `100`    |

Both numeric fields accept values up to 2147483647.

`xy` selects both pointer axes; `wheels` selects both wheel axes. Use
`horizontal_wheel` with `to.left` / `to.right` for tilt only, or `vertical_wheel`
with `to.up` / `to.down` for vertical scrolling only.

`to` maps directions to output arrays. At least one array must be nonempty;
nonempty outputs must belong to a selected axis. Arrays support output event
definitions with `modifiers`, `lazy`, and per-output `conditions`.

Direction names refer to input deltas before macOS scrolling preferences:

| Input   | `to.left`      | `to.right`     | `to.up`      | `to.down`    |
| ------- | -------------- | -------------- | ------------ | ------------ |
| Pointer | X < 0          | X > 0          | Y < 0        | Y > 0        |
| Wheel   | Horizontal < 0 | Horizontal > 0 | Vertical > 0 | Vertical < 0 |

## Sampling and output

The first nonzero selected input starts a fixed sampling window. Signed deltas
accumulate until its deadline: opposite movements cancel, and further input,
zero reports, or pauses do not extend the window. Nothing is emitted before the
deadline; evaluation happens even if input stops.

At the deadline:

- **Pointer:** choose the larger of `abs(X)` and `abs(Y)`; ties favor horizontal.
  Emit the chosen direction's array once if it reaches the threshold.
  An unmapped direction produces no output, with no fallback to another direction.
- **Wheel:** both axes share one window. Evaluate each axis against the threshold
  independently, emitting at most one array per axis, horizontal first.

Each selected array runs all its actions as down/up pairs. Then all accumulated
values are discarded, including surplus and subthreshold movement. The next input
starts a new window. A longer interval reduces repeated actions; a shorter one
responds faster.

Windows are independent per rule and device: pointer and wheel rules have separate
windows, as do separately configured horizontal and vertical wheel rules.
Device release/disconnection or configuration replacement cancels pending windows.
Outputs enter the normal output queue without being rematched in the same manager.

## Conditions and modifiers

The usual manipulator `conditions` and `from.modifiers` are checked when a window
starts. Once accepted, the rule keeps consuming and accumulating selected input
until the deadline, even if those conditions or modifiers change. The next window
checks them again. Per-output `conditions` are checked at the deadline.

**All selected axes are consumed immediately in every direction**, including
unmapped, empty, filtered, or subthreshold output. The first applicable rule takes
those axes. Unselected axes and button clicks pass through immediately, including
in mixed reports. If no window is active and its input conditions do not match,
input passes through unchanged.

Output modifiers are captured at the start of each window:

- Mandatory modifiers are removed; optional modifiers are retained.
- Later presses or releases do not change the captured state.
- Output-specific `modifiers` are added for the tap; afterward, the actual modifier
  state from just before emission is restored.

For example, starting the rule above with fn + shift produces shift + arrow even
if shift is released during sampling. Without `optional`, extra modifiers prevent
a new window from starting.
