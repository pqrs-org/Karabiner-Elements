#pragma once

// From AppleHIDUsageTables.h in IOHIDFamily.

namespace krbn {
/* Usage Pages */
enum {
  kHIDPage_AppleVendorKeyboard = 0xff01,
  kHIDPage_AppleVendorTopCase = 0x00ff,
};

/* AppleVendor Keyboard Page (0xff01) */
enum {
  kHIDUsage_AppleVendorKeyboard_Spotlight = 0x0001,
  kHIDUsage_AppleVendorKeyboard_Dashboard = 0x0002,
  kHIDUsage_AppleVendorKeyboard_Function = 0x0003,
  kHIDUsage_AppleVendorKeyboard_Launchpad = 0x0004,
  kHIDUsage_AppleVendorKeyboard_Reserved = 0x000a,
  kHIDUsage_AppleVendorKeyboard_CapsLockDelayEnable = 0x000b,
  kHIDUsage_AppleVendorKeyboard_PowerState = 0x000c,
  kHIDUsage_AppleVendorKeyboard_Expose_All = 0x0010,
  kHIDUsage_AppleVendorKeyboard_Expose_Desktop = 0x0011,
  kHIDUsage_AppleVendorKeyboard_Brightness_Up = 0x0020,
  kHIDUsage_AppleVendorKeyboard_Brightness_Down = 0x0021,
  kHIDUsage_AppleVendorKeyboard_Language = 0x0030
};

/* AppleVendor Page Top Case (0x00ff) */
enum {
  kHIDUsage_AV_TopCase_KeyboardFn = 0x0003,
  kHIDUsage_AV_TopCase_BrightnessUp = 0x0004,
  kHIDUsage_AV_TopCase_BrightnessDown = 0x0005,
  kHIDUsage_AV_TopCase_VideoMirror = 0x0006,
  kHIDUsage_AV_TopCase_IlluminationToggle = 0x0007,
  kHIDUsage_AV_TopCase_IlluminationUp = 0x0008,
  kHIDUsage_AV_TopCase_IlluminationDown = 0x0009,
  kHIDUsage_AV_TopCase_ClamshellLatched = 0x000a,
  kHIDUsage_AV_TopCase_Reserved_MouseData = 0x00c0
};
} // namespace krbn
