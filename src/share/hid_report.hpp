#pragma once

class hid_report {
public:
  struct __attribute__((packed)) keyboard_input {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
  };
};
