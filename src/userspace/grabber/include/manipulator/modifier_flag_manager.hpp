#pragma once

class modifier_flag_manager {
public:
  modifier_flag_manager(void) {
    states_.resize(static_cast<size_t>(modifier_flag::prepared_modifier_flag_end_));

    states_[static_cast<size_t>(modifier_flag::left_control)] = std::make_unique<state>("control", "⌃");
    states_[static_cast<size_t>(modifier_flag::left_shift)] = std::make_unique<state>("shift", "⇧");
    states_[static_cast<size_t>(modifier_flag::left_option)] = std::make_unique<state>("option", "⌥");
    states_[static_cast<size_t>(modifier_flag::left_command)] = std::make_unique<state>("command", "⌘");
    states_[static_cast<size_t>(modifier_flag::right_control)] = std::make_unique<state>("control", "⌃");
    states_[static_cast<size_t>(modifier_flag::right_shift)] = std::make_unique<state>("shift", "⇧");
    states_[static_cast<size_t>(modifier_flag::right_option)] = std::make_unique<state>("option", "⌥");
    states_[static_cast<size_t>(modifier_flag::right_command)] = std::make_unique<state>("command", "⌘");
    states_[static_cast<size_t>(modifier_flag::fn)] = std::make_unique<state>("fn", "fn");
  }

  void reset(void) {
    for (const auto& s : states_) {
      if (s) {
        s->reset();
      }
    }
  }

  enum class operation {
    increase,
    decrease,
  };

  void manipulate(modifier_flag k, operation operation) {
    auto i = static_cast<size_t>(k);
    if (states_[i]) {
      switch (operation) {
      case operation::increase:
        states_[i]->increase();
        break;
      case operation::decrease:
        states_[i]->decrease();
        break;
      }
    }
  }

  bool pressed(modifier_flag m) const {
    if (m == modifier_flag::zero) {
      return true;
    }

    auto i = static_cast<size_t>(m);
    if (i < states_.size() && states_[i]) {
      return states_[i]->pressed();
    }
    return false;
  }

  bool pressed(const std::vector<modifier_flag>& modifier_flags) {
    // return true if all modifier flags are pressed.
    for (const auto& m : modifier_flags) {
      if (!pressed(m)) {
        return false;
      }
    }
    return true;
  }

  uint8_t get_hid_report_bits(void) const {
    uint8_t bits = 0;

    if (pressed(modifier_flag::left_control)) {
      bits |= (0x1 << 0);
    }
    if (pressed(modifier_flag::left_shift)) {
      bits |= (0x1 << 1);
    }
    if (pressed(modifier_flag::left_option)) {
      bits |= (0x1 << 2);
    }
    if (pressed(modifier_flag::left_command)) {
      bits |= (0x1 << 3);
    }
    if (pressed(modifier_flag::right_control)) {
      bits |= (0x1 << 4);
    }
    if (pressed(modifier_flag::right_shift)) {
      bits |= (0x1 << 5);
    }
    if (pressed(modifier_flag::right_option)) {
      bits |= (0x1 << 6);
    }
    if (pressed(modifier_flag::right_command)) {
      bits |= (0x1 << 7);
    }

    return bits;
  }

  IOOptionBits get_io_option_bits(void) const {
    IOOptionBits bits = 0;
    if (pressed(modifier_flag::left_control) ||
        pressed(modifier_flag::right_control)) {
      bits |= NX_CONTROLMASK;
    }
    if (pressed(modifier_flag::left_shift) ||
        pressed(modifier_flag::right_shift)) {
      bits |= NX_SHIFTMASK;
    }
    if (pressed(modifier_flag::left_option) ||
        pressed(modifier_flag::right_option)) {
      bits |= NX_ALTERNATEMASK;
    }
    if (pressed(modifier_flag::left_command) ||
        pressed(modifier_flag::right_command)) {
      bits |= NX_COMMANDMASK;
    }
    if (pressed(modifier_flag::fn)) {
      bits |= NX_SECONDARYFNMASK;
    }
    return bits;
  }

private:
  class state {
  public:
    state(const std::string& name, const std::string& symbol) : name_(name),
                                                                symbol_(symbol),
                                                                count_(0) {}

    const std::string& get_name(void) const { return name_; }
    const std::string& get_symbol(void) const { return symbol_; }

    bool pressed(void) const { return count_ > 0; }

    void reset(void) {
      count_ = 0;
    }

    void increase(void) {
      ++count_;
    }

    void decrease(void) {
      --count_;
    }

  private:
    std::string name_;
    std::string symbol_;
    int count_;
  };

  std::vector<std::unique_ptr<state>> states_;
};
