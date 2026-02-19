# Karabiner-Elements Keybinding Lifecycle (Internals Guide)

This guide explains what happens inside Karabiner-Elements from the moment you press a key to the moment your configured action runs.

It is intentionally written for newcomers who want to understand the internals, debug behavior, and reason about latency.

## 1) Runtime architecture at a glance

For a typical complex modification rule, the path is:

1. Input device emits HID event.
2. Karabiner core receives and normalizes event.
3. Manipulators evaluate `from`/conditions.
4. Matching manipulator emits one or more `to` events.
5. `post_event_to_virtual_devices` schedules output events.
6. Events are dispatched either:
   - to virtual HID (key/mouse reports), or
   - to `console_user_server` (shell command, send user command, input source change, software function).

That split is central to understanding Karabiner internals.

## 2) Primary processes and responsibilities

## 2.1 Core-service side

Karabiner core service is responsible for:

- ingesting low-level input events,
- running manipulator chains,
- building output event queues,
- posting virtual HID reports,
- forwarding user-context operations to `console_user_server`.

## 2.2 `console_user_server` side

`console_user_server` handles operations that should run in the logged-in user context, such as:

- `shell_command_execution`,
- `send_user_command`,
- `select_input_source`,
- software functions and app-history related features.

It verifies peer identity/shared secret before accepting many operations.

## 3) How a rule is represented internally

A JSON rule in `karabiner.json` is parsed into internal event definitions.

Key parser/type entry points:

- `src/share/manipulator/types/event_definition.hpp`
- `src/share/manipulator/types/to_event_definition.hpp`

`to` entries become typed internal events, for example:

- `momentary_switch_event` (virtual keyboard/mouse outputs),
- `shell_command`,
- `send_user_command`,
- `select_input_source`,
- `software_function`.

## 4) Step-by-step: keypress to action

## 4.1 Key is pressed

1. Keyboard sends HID report.
2. macOS HID stack delivers event.
3. Karabiner receives device event and wraps it as internal queue entries.

## 4.2 Manipulator matching

Manipulator evaluation checks:

- `from` key/button code,
- mandatory/optional modifiers,
- conditions (frontmost app, variables, device, etc.),
- event type (`key_down`, `key_up`, `single`).

If matched, `to` actions are emitted as internal events.

## 4.3 Post-event scheduling

`post_event_to_virtual_devices` is where outputs are queued and dispatched.

Main implementation:

- `src/share/manipulator/manipulators/post_event_to_virtual_devices/post_event_to_virtual_devices.hpp`
- `src/share/manipulator/manipulators/post_event_to_virtual_devices/queue.hpp`

Important behavior:

- key/mouse ordering logic uses timestamp adjustments for reliability,
- certain non-keyboard actions (`shell_command`, `send_user_command`, etc.) skip modifier-order timing adjustments and are queued as async work items.

## 4.4 Dispatch path A: virtual HID outputs

For key/mouse-like outputs, queue dispatcher posts reports to virtual HID service client.

This path drives synthetic keypresses/clicks seen by applications.

## 4.5 Dispatch path B: user-context operations

For operations like `send_user_command`:

1. Core side uses `console_user_server_client` to send msgpack IPC.
2. `console_user_server` receiver validates request.
3. Appropriate handler executes operation.

Relevant files:

- `src/share/console_user_server_client.hpp`
- `src/core/console_user_server/include/console_user_server/receiver.hpp`
- `src/core/console_user_server/include/console_user_server/send_user_command_handler.hpp`

For `send_user_command`, handler serializes payload JSON and sends it to configured UNIX datagram endpoint.

## 5) Why this split exists (security model)

Karabiner intentionally separates concerns:

1. Core event manipulation path
2. User-session execution path (`console_user_server`)

Benefits:

- avoids turning core event path into a generic arbitrary-process command runner,
- applies team-id/shared-secret checks on IPC boundary,
- keeps user-targeted actions in user context where filesystem/socket permissions naturally apply.

## 6) Latency model: where time is actually spent

For a keybinding that triggers app activation or command execution, latency typically comes from:

1. HID input polling + OS delivery
2. manipulator evaluation and queue scheduling
3. inter-process hop to `console_user_server` (for user-context operations)
4. endpoint-side action execution
5. WindowServer/frame presentation

For many workflows, stage 4/5 (application behavior + display frame timing) dominates perceived delay.

## 7) Typical failure modes

## 7.1 Rule parsed but never matched

Symptoms:

- binding appears dead,
- no downstream action logs.

Common causes:

- wrong modifier expectations,
- condition mismatch,
- event type mismatch (`key_up`/`key_down` assumptions),
- profile/rule ordering confusion.

## 7.2 Action matched but user-context operation fails

Symptoms:

- keybinding triggers internally, but no external effect.

Common causes:

- `console_user_server` not connected/ready,
- target socket endpoint missing (for `send_user_command`),
- payload shape mismatch for receiver,
- permission/trust mismatch.

## 7.3 Perceived "random" latency spikes

Common causes:

- process spawn in hot path (`shell_command` paths launching shells/tools),
- app activation variance,
- high system load/background indexing,
- display/frame synchronization effects.

## 8) Reading the code efficiently (suggested order)

For newcomers, this order gives the fastest mental model:

1. event definition parsing
   - `src/share/manipulator/types/event_definition.hpp`
   - `src/share/manipulator/types/to_event_definition.hpp`
2. output dispatch manipulator
   - `src/share/manipulator/manipulators/post_event_to_virtual_devices/post_event_to_virtual_devices.hpp`
   - `src/share/manipulator/manipulators/post_event_to_virtual_devices/queue.hpp`
3. user-context client/server boundary
   - `src/share/console_user_server_client.hpp`
   - `src/core/console_user_server/include/console_user_server/receiver.hpp`
4. operation-specific handlers
   - e.g. `send_user_command_handler.hpp`, shell command handler, software function handler

## 9) Practical debugging mindset

When debugging a binding, split the question into stages:

1. Did the manipulator match?
2. Which internal `to` event type was emitted?
3. Which dispatch path was used (virtual HID vs console_user_server)?
4. Did that path complete successfully?
5. Is remaining delay in endpoint/app/UI layer?

This stage-by-stage approach avoids chasing the wrong layer.

## 10) Minimal summary

Karabiner keybindings are not a single monolithic action.
They are a pipeline:

- input ingestion,
- manipulator transformation,
- typed event dispatch,
- context-specific execution.

Understanding that pipeline is the key to both reliability tuning and latency tuning.
