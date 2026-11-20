# CGEventTap fallback behavior

This document describes the behavior when the CGEventTap fallback is enabled via `enable_cgeventtap_fallback`.

Even when the CGEventTap fallback is enabled, devices that can be handled through `IOHIDDeviceOpen` are still processed through that path in preference to `CGEventTap`.
Therefore, only devices that meet one of the following conditions are handled via `CGEventTap`:

- Devices for which `IOHIDDeviceOpen` fails for some reason.
- Devices for which `Modify events` is turned off.
  (This is not a typo. Since `CGEventTap` doesn't provide a way to determine the source device of an event, all events received through `CGEventTap` must be processed.
  This means it is not possible to enable or disable processing on a per-device basis.)

## Avoid processing events sent via the virtual device in CGEventTap

As described in [The difference of event posting methods](../DEVELOPMENT.md#the-difference-of-event-posting-methods), `CGEventPost` has several limitations.
For that reason, even when device events are captured via the CGEventTap fallback, the processed key events are still sent through the virtual device.
(In addition, events processed through HID are also sent via the virtual device.)

The event flow is therefore as follows:

- Capture key events using `CGEventTap`.
- Modify them through the manipulator pipeline.
- Send the modified events to the virtual device.
- Since events sent via the virtual device re-enter through `CGEventTap`, ignore events that have already been sent.

The most challenging part is filtering out re-entrant events.
`CGEventTap` does not provide reliable information about which device generated an event, so there is no stable way to detect events originating from the virtual device. (`kCGEventSourceUnixProcessID` and `kCGEventSourceUserData` both become `0` after passing through the virtual device.)

To work around this, we keep track of the events sent to the virtual device and ignore matching events when they reappear.

In theory, using `CGEventCreateKeyboardEvent` would simplify this, since re-entrant events could be identified and ignored via `kCGEventSourceUnixProcessID`.
However, key events generated with `CGEventCreateKeyboardEvent` and posted via `CGEventPost` have behavioral limitations. For example, `fn` key events are not treated by macOS in the same way as true hardware `fn` key presses for certain system-level behaviors, such as tapping `fn` by itself or using `fn+control+arrow keys` for window tiling.

Because of this, using a virtual device is still necessary in order to generate key events that behave naturally.

## Handling internal state before and after Secure Event Input

Another challenge with CGEventTap is how it behaves when Secure Event Input is enabled, for example while entering a password.
In this state, no events are delivered to CGEventTap.
While this is correct from a security perspective, it complicates things for software that relies on CGEventTap.

Because events are suppressed during Secure Event Input, once it is disabled,
the internal state of modifiers and pressed keys can suddenly become inconsistent.
To avoid this, key states must be reset properly when Secure Event Input is enabled and disabled;
otherwise, keys may be incorrectly treated as still being pressed.

There is also a related issue: if Secure Event Input becomes active while a key is being held down, the corresponding key_up event is never observed.
This can result in unintended key repeats.
This can be easily reproduced by running a command such as sudo ls in Terminal.

To mitigate this, all currently pressed keys must be forcibly released by emitting key_up events when Secure Event Input is detected.

## Potential approaches for handling pointing events

The CGEventTap fallback currently does not treat incoming pointing-button events, pointer motion, or scroll-wheel events as input to be manipulated.
Those original CGEvents remain in the event stream, and Karabiner-Elements observes them only for internal housekeeping such as updating modifier-related state.
This does not prevent a key manipulation from producing pointing-button output, which is sent through the virtual pointing device.
Manipulation of input from a pointing device itself is supported only through the normal `IOHIDDeviceOpen` and virtual pointing device path when `Modify events` is enabled.

If pointing-event manipulation through the CGEventTap fallback is considered in the future, there are two possible output approaches.

### Re-create and post CGEvents

One approach is to drop the original pointing events in the event tap, manipulate them, create replacement events with APIs such as `CGEventCreateMouseEvent` and `CGEventCreateScrollWheelEvent`, and post them with `CGEventPost`.

This keeps both input and output in the CoreGraphics event domain and makes it possible to attach source metadata to directly posted events for re-entry detection.
It also avoids requiring the virtual pointing device merely to repost an unchanged mouse event.

However, faithfully reconstructing pointing events is difficult:

- Pointer events contain both an absolute cursor location and relative movement deltas. The replacement event must preserve the location associated with that specific input event; using the latest cursor location after asynchronous manipulation can make the pointer jump.
- Button state affects whether movement is represented as `MouseMoved`, `LeftMouseDragged`, `RightMouseDragged`, or `OtherMouseDragged`.
- Click count, click number, double-click timing and distance, scroll properties, event coalescing, and event ordering would have to be reproduced correctly.
- Dropping the original event makes delivery depend on successfully creating and posting its replacement, adding a failure path that does not exist when the event is passed through unchanged.

An earlier experimental implementation used this approach, including custom tracking of cursor location and double-click state, but it was removed because reliably reproducing pointing events required too much state and was error-prone.

### Post reports through the virtual pointing device

Another approach is to drop the original pointing events that are to be manipulated and send HID reports through the virtual pointing device, as in the normal pointing-device manipulation path.

This lets macOS construct the resulting CGEvents from HID reports and avoids manually reconstructing click counts, drag event types, and other CoreGraphics fields.
It is also less dependent on `CGEventPost` behavior and Accessibility authorization for output.

There are two possible variants of this approach, but each has a different blocking issue.

#### Route all pointing events through the virtual device

To preserve button position, drag behavior, and event ordering, button events and pointer motion could both be dropped and routed through the same queue and virtual pointing device.
The main blocker for this variant is re-entry suppression for pointer motion.

Pointer motion emitted by the virtual pointing device re-enters CGEventTap without a reliable source identifier that distinguishes it from physical pointer motion.
Unlike discrete button events, HID motion reports do not necessarily map one-to-one to CGEvents because macOS may apply acceleration, rounding, and event coalescing.
Matching events by delta, count, or timing could therefore either miss re-entered motion or suppress simultaneous physical motion.
There is currently no robust way to distinguish virtual-device motion from physical motion in CGEventTap.

Routing high-frequency motion events through the manipulator pipeline would also add latency and require careful handling of relative movement, event coalescing, virtual-device readiness, device reconnection, and button-state synchronization.

#### Route only button events through the virtual device

Another variant is to send only button reports through the virtual device while allowing physical pointer motion to remain in the original CGEvent stream.
This avoids the need to suppress re-entered virtual pointer motion, but it cannot guarantee consistency between button events and pointer motion because the two paths have different latency and ordering:

- A physical button-down event can be dropped and queued for asynchronous manipulation.
- A subsequent physical motion event can pass through immediately.
- The virtual button-down report can then arrive after the motion and take effect at the cursor's new position.

This can change the click location, turn an intended drag into ordinary motion, or extend a drag until a delayed button-up report arrives.
Remapping one button to another can also make the physical drag event type disagree with the logical button state produced by the virtual device.

Both the `CGEventPost` and virtual-device approaches also retain a fundamental CGEventTap limitation: the source pointing device cannot be identified, so per-device `Modify events` settings and device conditions cannot be honored.

For these reasons, the current design deliberately does not manipulate pointing events through the CGEventTap fallback.
The fallback remains a limited recovery path for key events, while pointing-event manipulation requires the normal HID input path.
