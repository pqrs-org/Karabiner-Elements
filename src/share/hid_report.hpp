#pragma once

class hid_report {
public:
  struct __attribute__((packed)) keyboard_input {
    UInt8 modifiers;
    UInt8 reserved;
    UInt8 keys[6];
  };
};
