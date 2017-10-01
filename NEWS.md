# Version 11.0.1

* Fixed an issue that modifier flags becomes improperly state by events from Apple trackpads.


# Version 11.0.0

* The first stable release of Karabiner-Elements.
  (There is no changes from Karabiner-Elements 0.91.16.)


# Version 0.91.16

* Karabiner-Elements waits grabbing device until all modifier keys are released in order to avoid modifier flags stuck issue in mouse events.
* Support consumer keys (e.g., media key events in Logitech keyboards.)


# Version 0.91.13

* Add per device support in `Simple Modifications` and `Fn Function Keys`.
  ![Simple Modifications](https://pqrs.org/osx/karabiner/img/news/v0.91.13_0.png)
* The modifier flag event handling has been improved.


# Version 0.91.12

* `device_if` and `device_unless` has been added to `conditions`.
  * An example: https://github.com/pqrs-org/KE-complex_modifications/blob/master/docs/json/example_device.json


# Version 0.91.11

* Fixed an issue that modifier flags might become improperly state in complex_modifications.
  (In complex_modifications rules which changes modifier+modifier to modifier.)


# Version 0.91.10

* macOS 10.13 (High Sierra) support has been improved.


# Version 0.91.9

* `variable_if` and `variable_unless` has been added to `conditions`.
  You can use `set_variable` to change the variables.
  * An example: https://github.com/pqrs-org/KE-complex_modifications/blob/ef8074892e5fff8a4781a898869f8d341b5a815a/docs/json/personal_tekezo.json
* `to_after_key_up` has been added to `complex_modifications > basic`.
* `"from": { "any": "key_code" }` has been added to `complex_modifications > basic`.
   You can use this to disable untargeted keys in your mode. (e.g., disable untargeted keys in Launcher Mode.)
  * An example: https://github.com/pqrs-org/KE-complex_modifications/blob/ef8074892e5fff8a4781a898869f8d341b5a815a/docs/json/personal_tekezo.json#L818-L844
* `Variables` tab has been added into `EventViewer`.
  You can confirm the `set_variable` result in `Variables` tab.


# Version 0.91.8

* Fixed an issue that karabiner_grabber might be crashed when frontmost application is changed.


# Version 0.91.7

* Shell command execution has been supported. (e.g., Launch apps in https://pqrs.org/osx/karabiner/complex_modifications/ )


# Version 0.91.6

* The conditional event manipulation has been supported. (`frontmost_application_if` and `frontmost_application_unless`)


# Version 0.91.5

* GUI for complex_modifications has been added. https://github.com/tekezo/Karabiner-Elements/tree/master/usage#how-to-use-complex-modifications
* Syntax of `complex_modifications > parameters` has been changed.


# Version 0.91.4

* The modifier flag event handling has been improved.
* Show warning and error logs by colored text in Log tab.


# Version 0.91.3

* Add timeout to `to_if_alone`.


# Version 0.91.2

* Initial support of `complex_modifications > basic > to_if_alone`.


# Version 0.91.1

* Fixed an issue that Karabiner-Elements stops working after user switching.
* Initial support of `complex_modifications` (No GUI yet).


# Version 0.91.0

* event manipulation has been changed to `src/core/grabber/include/manipulator/details/basic.hpp`.
