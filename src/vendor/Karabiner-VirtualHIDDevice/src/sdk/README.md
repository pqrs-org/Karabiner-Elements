`MacOSX10.12.sdk/IOHIDEventService` is not compatible with macOS 10.11.
So, we have to use `MacOSX10.11.sdk/IOHIDEventService` to build VirtualHIDEventService.

Note:
`kextload` will be failed with the following error message when you load kexts with `MacOSX10.12.sdk/IOHIDEventService` in macOS 10.11.



