# Classes

* VirtualHIDRoot
  * VirtualHIDRoot_UserClient
    * VirtualHIDKeyboard
    * VirtualHIDPointing

# Restrictions

## We should not add virtual pointing device until user really want to add it

macOS UI changes the scroll bar design if a mouse device (not trackpad) is connected.
The virtual pointing device affect this behavior.
