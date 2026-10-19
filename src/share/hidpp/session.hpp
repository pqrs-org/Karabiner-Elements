#pragma once

// HID++ 2.0 protocol state for routing one control of a Logitech device to a pointing button.
//
// The control is temporarily diverted with REPROG_CONTROLS_V4 (0x1b04), so that the device reports it in HID++
// notifications instead of performing its firmware function.
// Only fixed discovery, reporting-state queries, temporary diversion and its restoration are sent.
// Persistent diversion and remapping are never requested.
//
// This class contains no IOKit calls so that it can be tested without hardware.
// All methods must be called from the same thread.

#include "logger.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace krbn::hidpp {
using long_report = std::array<uint8_t, 20>;

constexpr uint8_t long_report_id = 0x11;
// Directly connected (Bluetooth Low Energy) devices are addressed with 0xff.
// Receivers use other indices and are not supported.
constexpr uint8_t direct_device_index = 0xff;
constexpr uint8_t error_feature_index = 0xff;
constexpr uint16_t reprog_controls_v4_feature_id = 0x1b04;
constexpr int max_attempts = 3;
constexpr std::chrono::milliseconds reply_timeout(2000);

// REPROG_CONTROLS_V4 getCidInfo flags
constexpr uint16_t control_flag_divertable = 0x0020;

// REPROG_CONTROLS_V4 getCidReporting/setCidReporting flags
constexpr uint8_t reporting_flag_diverted = 0x01;
constexpr uint8_t reporting_flag_diverted_valid = 0x02;
constexpr uint8_t reporting_flag_persistently_diverted = 0x04;
constexpr uint8_t reporting_flag_raw_xy_diverted = 0x10;
constexpr uint8_t reporting_flag_force_raw_xy_diverted = 0x40;

// Returns whether a raw input report is a HID++ long report from a directly connected device.
// This function is stateless so it can be called in any thread.
[[nodiscard]] inline bool is_long_report(uint32_t report_id,
                                         std::span<const uint8_t> report) noexcept {
  return report_id == long_report_id &&
         report.size() == std::tuple_size_v<long_report> &&
         report[0] == long_report_id &&
         report[1] == direct_device_index;
}

// Returns whether a request is REPROG_CONTROLS_V4 setCidReporting, which changes the reporting of a control.
// All other requests are read-only queries.
[[nodiscard]] inline bool is_set_reporting_request(const long_report& request) noexcept {
  return request[2] != 0x00 && (request[3] >> 4) == 3;
}

[[nodiscard]] inline std::string make_control_id_string(uint16_t control_id) {
  return fmt::format("{0} (0x{0:04x})", control_id);
}

struct control final {
  uint16_t control_id;
  uint16_t flags;

  [[nodiscard]] bool divertable() const {
    return (flags & control_flag_divertable) != 0;
  }
};

class session final {
public:
  enum class state {
    initial,
    waiting_feature_index,
    waiting_control_count,
    waiting_control_info,
    waiting_reporting,
    waiting_divert,
    diverted,
    // The control was not diverted and nothing has to be restored.
    refused,
    // A request failed or was not answered. A divert request that might have been applied was undone.
    failed,
    stopped,
  };

  struct result final {
    // A HID++ output report to send.
    std::optional<long_report> request;
    // A control state change to report.
    std::optional<bool> pressed;
  };

  // software_id (1-15) is put into requests so that replies to another session are ignored.
  session(uint16_t control_id,
          uint8_t software_id,
          const std::string& log_prefix)
      : control_id_(control_id),
        software_id_(normalize_software_id(software_id)),
        log_prefix_(log_prefix) {
  }

  [[nodiscard]] state get_state() const {
    return state_;
  }

  [[nodiscard]] bool get_pressed() const {
    return pressed_;
  }

  [[nodiscard]] uint64_t get_generation() const {
    return generation_;
  }

  [[nodiscard]] bool waiting_reply() const {
    return pending_request_.has_value();
  }

  [[nodiscard]] const std::vector<control>& get_controls() const {
    return controls_;
  }

  // The reason why the control was refused or failed.
  [[nodiscard]] const std::string& get_reason() const {
    return reason_;
  }

  // Whether the session failed because the device did not answer a discovery request
  // (ROOT.getFeature, getCount, getCidInfo or getCidReporting) and has not been restarted yet.
  // A session is restarted at most once in its lifetime.
  [[nodiscard]] bool can_recover_on_physical_activity() const {
    return state_ == state::failed &&
           discovery_timed_out_ &&
           !recovery_used_;
  }

  // ROOT.getFeature(REPROG_CONTROLS_V4)
  result start() {
    if (state_ != state::initial) {
      return {};
    }

    return send(state::waiting_feature_index,
                make_request(0x00,
                             0,
                             {static_cast<uint8_t>(reprog_controls_v4_feature_id >> 8),
                              static_cast<uint8_t>(reprog_controls_v4_feature_id & 0xff)}));
  }

  // Discovers again with a new software_id after can_recover_on_physical_activity() became true.
  // The generation is not reset, so timers of the previous attempt are ignored.
  result restart_after_discovery_timeout(uint8_t software_id) {
    if (!can_recover_on_physical_activity()) {
      return {};
    }

    recovery_used_ = true;
    discovery_timed_out_ = false;
    software_id_ = normalize_software_id(software_id);
    state_ = state::initial;
    feature_index_ = 0;
    control_count_ = 0;
    controls_.clear();
    pending_request_ = std::nullopt;
    reason_.clear();

    return start();
  }

  result handle_input_report(std::span<const uint8_t> report) {
    if (!is_long_report(long_report_id, report)) {
      return {};
    }

    // HID++ 2.0 error: [report_id, device_index, 0xff, feature_index, function|software_id, error_code, ...]
    if (report[2] == error_feature_index) {
      if (pending_request_ &&
          report[3] == (*pending_request_)[2] &&
          report[4] == (*pending_request_)[3]) {
        return fail(fmt::format("HID++ error 0x{0:02x} for feature index 0x{1:02x} function {2}",
                                report[5],
                                report[3],
                                report[4] >> 4));
      }
      return {};
    }

    // Notifications use software id 0.
    if ((report[3] & 0x0f) == 0) {
      return handle_notification(report);
    }

    // A reply has the feature index and function|software_id of the pending request.
    if (!pending_request_ ||
        report[2] != (*pending_request_)[2] ||
        report[3] != (*pending_request_)[3]) {
      return {};
    }

    switch (state_) {
      case state::waiting_feature_index: {
        // ROOT.getFeature reply: [feature_index, feature_type, feature_version]
        auto index = report[4];
        if (index == 0x00 || index == error_feature_index) {
          return refuse("the device does not support REPROG_CONTROLS_V4 (0x1b04)");
        }

        feature_index_ = index;
        return send(state::waiting_control_count,
                    make_request(feature_index_, 0, {}));
      }

      case state::waiting_control_count: {
        // getCount reply: [count]
        control_count_ = report[4];
        if (control_count_ == 0) {
          return refuse("the device reports no REPROG_CONTROLS_V4 controls");
        }

        controls_.clear();
        return send(state::waiting_control_info,
                    make_request(feature_index_, 1, {0}));
      }

      case state::waiting_control_info: {
        // getCidInfo reply: [cid(2), task_id(2), flags, position, group, group_mask, additional_flags]
        control c{
            .control_id = static_cast<uint16_t>((report[4] << 8) | report[5]),
            .flags = static_cast<uint16_t>(report[8] | (report[12] << 8)),
        };

        // getCidInfo replies do not echo the index.
        // A control id appears only once, so a repeated id is a duplicated reply to an earlier request.
        for (const auto& existing : controls_) {
          if (existing.control_id == c.control_id) {
            return {};
          }
        }

        controls_.push_back(c);

        if (controls_.size() < control_count_) {
          return send(state::waiting_control_info,
                      make_request(feature_index_, 1, {static_cast<uint8_t>(controls_.size())}));
        }

        logger::get_logger()->info("{0} divertable controls: {1}",
                                   log_prefix_,
                                   make_divertable_controls_string());

        auto it = std::ranges::find_if(controls_, [this](const auto& c) {
          return c.control_id == control_id_;
        });
        if (it == std::end(controls_)) {
          return refuse(fmt::format("control {0} does not exist (divertable controls: {1})",
                                    make_control_id_string(control_id_),
                                    make_divertable_controls_string()));
        }
        if (!it->divertable()) {
          return refuse(fmt::format("control {0} cannot be temporarily diverted (flags 0x{1:04x}, divertable controls: {2})",
                                    make_control_id_string(control_id_),
                                    it->flags,
                                    make_divertable_controls_string()));
        }

        return send(state::waiting_reporting,
                    make_request(feature_index_, 2, {static_cast<uint8_t>(control_id_ >> 8), static_cast<uint8_t>(control_id_ & 0xff)}));
      }

      case state::waiting_reporting: {
        // getCidReporting reply: [cid(2), flags, remap(2), ...]
        if (!same_control_id(report)) {
          return {};
        }

        auto flags = report[6];
        auto remap = static_cast<uint16_t>((report[7] << 8) | report[8]);

        if ((flags & (reporting_flag_diverted |
                      reporting_flag_persistently_diverted |
                      reporting_flag_raw_xy_diverted |
                      reporting_flag_force_raw_xy_diverted)) != 0) {
          return refuse(fmt::format("control {0} is already diverted (reporting flags 0x{1:02x}). "
                                    "Another HID++ client may own it, or an earlier diversion was not restored; "
                                    "reconnecting the device resets temporary diversion",
                                    make_control_id_string(control_id_),
                                    flags));
        }
        if (remap != 0 && remap != control_id_) {
          return refuse(fmt::format("control {0} is remapped to {1} by another HID++ client",
                                    make_control_id_string(control_id_),
                                    make_control_id_string(remap)));
        }

        divert_requested_ = true;
        return send(state::waiting_divert,
                    make_set_reporting_request(reporting_flag_diverted | reporting_flag_diverted_valid));
      }

      case state::waiting_divert: {
        // setCidReporting reply echoes [cid(2), flags, remap(2)].
        if (!same_control_id(report)) {
          return {};
        }

        pending_request_ = std::nullopt;

        if ((report[6] & reporting_flag_diverted) == 0) {
          return fail(fmt::format("the device did not divert control {0}",
                                  make_control_id_string(control_id_)));
        }

        state_ = state::diverted;
        logger::get_logger()->info("{0} activated: control {1} is diverted (REPROG_CONTROLS_V4 feature index 0x{2:02x})",
                                   log_prefix_,
                                   make_control_id_string(control_id_),
                                   feature_index_);
        return {};
      }

      case state::initial:
      case state::diverted:
      case state::refused:
      case state::failed:
      case state::stopped:
        return {};
    }

    return {};
  }

  result handle_timeout(uint64_t generation) {
    if (generation != generation_ ||
        !pending_request_) {
      return {};
    }

    if (attempts_ >= max_attempts) {
      return fail(fmt::format("no HID++ reply after {0} attempts", attempts_),
                  state_ == state::waiting_feature_index ||
                      state_ == state::waiting_control_count ||
                      state_ == state::waiting_control_info ||
                      state_ == state::waiting_reporting);
    }

    ++attempts_;
    ++generation_;
    return result{.request = pending_request_};
  }

  // Returns the request that restores the control if it might be diverted, and a release if it is pressed.
  result stop() {
    result r;

    if (state_ == state::stopped) {
      return r;
    }

    if (divert_requested_) {
      divert_requested_ = false;
      r.request = make_set_reporting_request(reporting_flag_diverted_valid);
    }

    if (pressed_) {
      pressed_ = false;
      r.pressed = false;
    }

    state_ = state::stopped;
    discovery_timed_out_ = false;
    pending_request_ = std::nullopt;
    ++generation_;

    return r;
  }

private:
  static uint8_t normalize_software_id(uint8_t software_id) {
    return static_cast<uint8_t>(software_id & 0x0f) != 0 ? static_cast<uint8_t>(software_id & 0x0f) : 1;
  }

  long_report make_request(uint8_t feature_index,
                           uint8_t function,
                           std::initializer_list<uint8_t> parameters) const {
    long_report r{};
    r[0] = long_report_id;
    r[1] = direct_device_index;
    r[2] = feature_index;
    r[3] = static_cast<uint8_t>((function << 4) | software_id_);

    size_t i = 4;
    for (auto p : parameters) {
      r[i++] = p;
    }

    return r;
  }

  // setCidReporting(cid, flags, remap = 0 (unchanged))
  long_report make_set_reporting_request(uint8_t flags) const {
    return make_request(feature_index_,
                        3,
                        {static_cast<uint8_t>(control_id_ >> 8),
                         static_cast<uint8_t>(control_id_ & 0xff),
                         flags,
                         0x00,
                         0x00});
  }

  result send(state new_state, const long_report& request) {
    state_ = new_state;
    pending_request_ = request;
    attempts_ = 1;
    ++generation_;
    return result{.request = request};
  }

  bool same_control_id(std::span<const uint8_t> report) const {
    return ((report[4] << 8) | report[5]) == control_id_;
  }

  result handle_notification(std::span<const uint8_t> report) {
    // REPROG_CONTROLS_V4 divertedButtonsEvent: up to four pressed control ids (big endian).
    if (state_ != state::diverted ||
        report[2] != feature_index_ ||
        (report[3] >> 4) != 0) {
      return {};
    }

    bool pressed = false;
    for (size_t i = 4; i <= 10; i += 2) {
      if (((report[i] << 8) | report[i + 1]) == control_id_) {
        pressed = true;
      }
    }

    if (pressed == pressed_) {
      return {};
    }

    pressed_ = pressed;
    return result{.pressed = pressed};
  }

  std::string make_divertable_controls_string() const {
    std::string s;
    for (const auto& c : controls_) {
      if (c.divertable()) {
        if (!s.empty()) {
          s += ", ";
        }
        s += make_control_id_string(c.control_id);
      }
    }
    return s.empty() ? "none" : s;
  }

  result refuse(const std::string& reason) {
    state_ = state::refused;
    pending_request_ = std::nullopt;
    reason_ = reason;

    logger::get_logger()->warn("{0} not activated: {1}", log_prefix_, reason);

    return {};
  }

  // Undo a divert request that the device might have applied, so the control keeps its firmware function.
  // discovery_timed_out is decided by the caller because divert_requested_ is cleared here.
  result fail(const std::string& reason, bool discovery_timed_out = false) {
    state_ = state::failed;
    discovery_timed_out_ = discovery_timed_out;
    pending_request_ = std::nullopt;
    reason_ = reason;

    logger::get_logger()->warn("{0} not activated: {1}", log_prefix_, reason);

    result r;
    if (divert_requested_) {
      divert_requested_ = false;
      r.request = make_set_reporting_request(reporting_flag_diverted_valid);
    }
    return r;
  }

  uint16_t control_id_;
  uint8_t software_id_;
  std::string log_prefix_;
  state state_ = state::initial;
  uint8_t feature_index_ = 0;
  uint8_t control_count_ = 0;
  std::vector<control> controls_;
  bool divert_requested_ = false;
  bool discovery_timed_out_ = false;
  bool recovery_used_ = false;
  bool pressed_ = false;
  std::optional<long_report> pending_request_;
  int attempts_ = 0;
  uint64_t generation_ = 0;
  std::string reason_;
};
} // namespace krbn::hidpp
