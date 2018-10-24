#!/bin/bash

if [ -e $(xcode-select --print-path)/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.12.sdk ]; then
    exit 0
fi

echo -ne '\033[33;40m'
echo "----------------------------------------------------------------------"
echo "WARNING                                                               "
echo "----------------------------------------------------------------------"
echo "                                                                      "
echo "The current macOS SDK version is not macOS 10.12.                     "
echo "                                                                      "
echo "If Karabiner-VirtualHIDDevice is built with newer SDK,                "
echo "it does not work on macOS 10.12 with the following error.             "
echo "                                                                      "
echo "    The super class vtable '__ZTV11IOHIDDevice' for vtable            "
echo "    '...' is out of date.                                             "
echo "                                                                      "
echo "                                                                      "
echo "Download Xcode8.3.3 and switch SDK by xcode-select command.           "
echo "                                                                      "
echo "  $ sudo xcode-select -s /opt/xcode/Xcode8.3.3.app/Contents/Developer "
echo "  $ make                                                              "
echo "  $ sudo xcode-select --reset                                         "
echo "                                                                      "
echo "Or continue to build with the current SDK by the following command.   "
echo "                                                                      "
echo "  *** NOT RECOMMENDED ***                                             "
echo "  *** PLEASE CONSIDER USING XCODE 8 ***                               "
echo "  $ make build                                                        "
echo "                                                                      "
echo -ne '\033[0m'

exit 1
