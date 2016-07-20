#pragma once

class hid_report {
public:
  class __attribute__((packed)) keyboard_input {
  public:
    keyboard_input(void) : modifiers(0), reserved(0), keys{0} {}

    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
  };
};
