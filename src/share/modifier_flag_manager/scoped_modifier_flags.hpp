#pragma once

class scoped_modifier_flags final {
public:
  scoped_modifier_flags(modifier_flag_manager& modifier_flag_manager,
                        std::unordered_set<modifier_flag> modifier_flags)
      : modifier_flag_manager_(modifier_flag_manager) {
    for (const auto& m : {
             modifier_flag::caps_lock,
             modifier_flag::left_control,
             modifier_flag::left_shift,
             modifier_flag::left_option,
             modifier_flag::left_command,
             modifier_flag::right_control,
             modifier_flag::right_shift,
             modifier_flag::right_option,
             modifier_flag::right_command,
             modifier_flag::fn,
         }) {
      if (modifier_flags.contains(m)) {
        while (!modifier_flag_manager_.is_pressed(m)) {
          active_modifier_flag f(active_modifier_flag::type::increase, m, device_id(0));
          modifier_flag_manager_.push_back_active_modifier_flag(f);
          scoped_active_modifier_flags_.push_back(f);
        }
      } else {
        if (modifier_flag_manager_.is_pressed(m)) {
          auto copy = modifier_flag_manager_.get_active_modifier_flags();
          for (const auto& f : copy) {
            if (f.get_modifier_flag() == m) {
              active_modifier_flag inverse(f.get_inverse_type(), f.get_modifier_flag(), device_id(0));
              modifier_flag_manager_.push_back_active_modifier_flag(inverse);
              scoped_active_modifier_flags_.push_back(inverse);
            }
          }
        }
      }
    }
  }

  ~scoped_modifier_flags(void) {
    for (const auto& f : get_inverse_active_modifier_flags()) {
      modifier_flag_manager_.push_back_active_modifier_flag(f);
    }
  }

  const std::vector<active_modifier_flag>& get_scoped_active_modifier_flags(void) const {
    return scoped_active_modifier_flags_;
  }

  std::vector<active_modifier_flag> get_inverse_active_modifier_flags(void) const {
    std::vector<active_modifier_flag> flags;

    for (const auto& f : scoped_active_modifier_flags_) {
      flags.push_back(active_modifier_flag(f.get_inverse_type(), f.get_modifier_flag(), f.get_device_id()));
    }

    return flags;
  }

private:
  modifier_flag_manager& modifier_flag_manager_;
  std::vector<active_modifier_flag> scoped_active_modifier_flags_;
};
