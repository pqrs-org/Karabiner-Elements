#pragma once

// HID++ 2.0 protocol state for discovering controls and optionally routing
// controls of a Logitech device to logical button events.
//
// Controls are temporarily diverted with REPROG_CONTROLS_V4 (0x1b04), so that the device reports them in HID++
// notifications instead of performing its firmware function.
// Only fixed discovery, reporting-state queries, temporary diversion and its restoration are sent.
// Persistent diversion and remapping are never requested.
//
// This class contains no IOKit calls so that it can be tested without hardware.
// All methods must be called from the same thread.

#include "logger.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace krbn::logitech::hidpp {
using short_report = std::array<uint8_t, 7>;
using long_report = std::array<uint8_t, 20>;

constexpr uint8_t short_report_id = 0x10;
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

// Returns whether a raw input report is a HID++ short report from a directly connected device.
// Diverted button notifications can use this format even when requests and replies use long reports.
[[nodiscard]] inline bool is_short_report(uint32_t report_id,
                                          std::span<const uint8_t> report) noexcept {
  return report_id == short_report_id &&
         report.size() == std::tuple_size_v<short_report> &&
         report[0] == short_report_id &&
         report[1] == direct_device_index;
}

[[nodiscard]] inline bool is_hidpp_report(uint32_t report_id,
                                          std::span<const uint8_t> report) noexcept {
  return is_short_report(report_id, report) ||
         is_long_report(report_id, report);
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
    discovered,
    waiting_reporting,
    waiting_divert,
    diverted,
    // No control remains diverted and nothing has to be restored.
    refused,
    // A request failed or was not answered. Divert requests that might have been applied were undone.
    failed,
    stopped,
  };

  struct result final {
    // A HID++ output report to send.
    std::optional<long_report> request;
    // Additional requests used when multiple controls must be restored at once.
    std::vector<long_report> additional_requests;
    struct button_event final {
      uint16_t control_id;
      bool pressed;

      bool operator==(const button_event&) const = default;
    };
    std::vector<button_event> button_events;
    // Kept as a convenience for single-control callers and tests.
    std::optional<bool> pressed;
  };

  // software_id (1-15) is put into requests so that replies to another session are ignored.
  session(std::optional<uint16_t> control_id,
          uint8_t software_id,
          const std::string& log_prefix)
      : session(control_id ? std::vector<uint16_t>{*control_id} : std::vector<uint16_t>{},
                software_id,
                log_prefix) {
  }

  session(std::vector<uint16_t> control_ids,
          uint8_t software_id,
          const std::string& log_prefix)
      : control_ids_(std::move(control_ids)),
        software_id_(normalize_software_id(software_id)),
        log_prefix_(log_prefix) {
    std::ranges::sort(control_ids_);
    control_ids_.erase(std::unique(std::begin(control_ids_), std::end(control_ids_)), std::end(control_ids_));
  }

  [[nodiscard]] state get_state() const {
    return state_;
  }

  [[nodiscard]] bool get_pressed() const {
    return !pressed_control_ids_.empty();
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

  [[nodiscard]] bool controls_discovered() const {
    return controls_discovered_;
  }

  [[nodiscard]] std::vector<uint16_t> get_divertable_control_ids() const {
    std::vector<uint16_t> result;

    for (const auto& control : controls_) {
      if (control.divertable()) {
        result.push_back(control.control_id);
      }
    }

    return result;
  }

  // The reason why the controls were refused or the session failed.
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
    controls_discovered_ = false;
    current_control_index_ = 0;
    divert_requested_control_ids_.clear();
    pressed_control_ids_.clear();
    pending_request_ = std::nullopt;
    reason_.clear();

    return start();
  }

  result handle_input_report(std::span<const uint8_t> report) {
    long_report normalized_report{};

    if (is_short_report(short_report_id, report) ||
        is_long_report(long_report_id, report)) {
      std::ranges::copy(report, normalized_report.begin());
      report = normalized_report;
    } else {
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

        controls_discovered_ = true;

        if (control_ids_.empty()) {
          state_ = state::discovered;
          pending_request_ = std::nullopt;
          return {};
        }

        for (auto control_id : control_ids_) {
          auto it = std::ranges::find_if(controls_, [control_id](const auto& c) {
            return c.control_id == control_id;
          });
          if (it == std::end(controls_)) {
            return refuse(fmt::format("control {0} does not exist (divertable controls: {1})",
                                      make_control_id_string(control_id),
                                      make_divertable_controls_string()));
          }
          if (!it->divertable()) {
            return refuse(fmt::format("control {0} cannot be temporarily diverted (flags 0x{1:04x}, divertable controls: {2})",
                                      make_control_id_string(control_id),
                                      it->flags,
                                      make_divertable_controls_string()));
          }
        }

        current_control_index_ = 0;
        return send(state::waiting_reporting,
                    make_get_reporting_request(current_control_id()));
      }

      case state::waiting_reporting: {
        // getCidReporting reply: [cid(2), flags, remap(2), ...]
        if (!same_control_id(report)) {
          return {};
        }

        auto flags = report[6];
        auto remap = static_cast<uint16_t>((report[7] << 8) | report[8]);

        if ((flags & (reporting_flag_persistently_diverted |
                      reporting_flag_raw_xy_diverted |
                      reporting_flag_force_raw_xy_diverted)) != 0) {
          return refuse(fmt::format("control {0} is already diverted (reporting flags 0x{1:02x}). "
                                    "Karabiner-Elements does not override persistent or raw XY diversion",
                                    make_control_id_string(current_control_id()),
                                    flags));
        }
        if (remap != 0 && remap != current_control_id()) {
          return refuse(fmt::format("control {0} is remapped to {1} by another HID++ client",
                                    make_control_id_string(current_control_id()),
                                    make_control_id_string(remap)));
        }

        if ((flags & reporting_flag_diverted) != 0) {
          // Configuring this control is an explicit opt-in to manage it with
          // Karabiner-Elements. Take responsibility for an existing temporary
          // diversion, including restoring it when this session stops. This
          // also recovers a diversion left behind by an earlier session.
          divert_requested_control_ids_.push_back(current_control_id());
          return finish_current_control_diversion(true, true);
        }

        divert_requested_control_ids_.push_back(current_control_id());
        return send(state::waiting_divert,
                    make_set_reporting_request(current_control_id(), reporting_flag_diverted | reporting_flag_diverted_valid));
      }

      case state::waiting_divert: {
        // setCidReporting reply echoes [cid(2), flags, remap(2)].
        if (!same_control_id(report)) {
          return {};
        }

        if ((report[6] & reporting_flag_diverted) == 0) {
          return fail(fmt::format("the device did not divert control {0}",
                                  make_control_id_string(current_control_id())));
        }

        return finish_current_control_diversion(true);
      }

      case state::initial:
      case state::discovered:
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

    if (state_ == state::waiting_divert) {
      // Some devices, including M585/M590, apply setCidReporting but do not
      // echo the request as specified. The synchronous IOKit write has already
      // succeeded, so do not spend three protocol retries on every selected
      // control. Retain it as a temporary diversion and make sure stop() later
      // sends the matching restore request.
      return finish_current_control_diversion(false);
    }

    if (attempts_ >= max_attempts) {
      return fail(fmt::format("no HID++ reply after {0} attempts while {1}",
                              attempts_,
                              make_pending_request_description()),
                  state_ == state::waiting_feature_index ||
                      state_ == state::waiting_control_count ||
                      state_ == state::waiting_control_info ||
                      (state_ == state::waiting_reporting && divert_requested_control_ids_.empty()));
    }

    ++attempts_;
    ++generation_;
    return result{.request = pending_request_};
  }

  // Returns requests that restore controls which might be diverted, and releases for pressed controls.
  result stop() {
    result r;

    if (state_ == state::stopped) {
      return r;
    }

    append_restore_requests(r);
    for (auto control_id : pressed_control_ids_) {
      r.button_events.push_back({.control_id = control_id, .pressed = false});
    }
    if (r.button_events.size() == 1) {
      r.pressed = false;
    }
    pressed_control_ids_.clear();

    state_ = state::stopped;
    discovery_timed_out_ = false;
    pending_request_ = std::nullopt;
    ++generation_;

    return r;
  }

private:
  result finish_current_control_diversion(bool acknowledged,
                                          bool already_diverted = false) {
    pending_request_ = std::nullopt;

    if (already_diverted) {
      logger::get_logger()->info("{0} activated: control {1} was already temporarily diverted and is now managed by Karabiner-Elements",
                                 log_prefix_,
                                 make_control_id_string(current_control_id()));
    } else if (acknowledged) {
      logger::get_logger()->info("{0} activated: control {1} is diverted (REPROG_CONTROLS_V4 feature index 0x{2:02x})",
                                 log_prefix_,
                                 make_control_id_string(current_control_id()),
                                 feature_index_);
    } else {
      logger::get_logger()->info("{0} activated without acknowledgement: control {1} is assumed to be diverted",
                                 log_prefix_,
                                 make_control_id_string(current_control_id()));
    }

    ++current_control_index_;
    if (current_control_index_ < control_ids_.size()) {
      return send(state::waiting_reporting,
                  make_get_reporting_request(current_control_id()));
    }

    state_ = state::diverted;
    return {};
  }

  [[nodiscard]] std::string make_pending_request_description() const {
    switch (state_) {
      case state::waiting_feature_index:
        return "looking up REPROG_CONTROLS_V4";

      case state::waiting_control_count:
        return "getting the control count";

      case state::waiting_control_info:
        return fmt::format("getting information for control index {0}", controls_.size());

      case state::waiting_reporting:
        if (current_control_index_ < control_ids_.size()) {
          return fmt::format("getting the reporting state of control {0}",
                             make_control_id_string(current_control_id()));
        }
        break;

      case state::waiting_divert:
        if (current_control_index_ < control_ids_.size()) {
          return fmt::format("diverting control {0}",
                             make_control_id_string(current_control_id()));
        }
        break;

      case state::initial:
      case state::discovered:
      case state::diverted:
      case state::refused:
      case state::failed:
      case state::stopped:
        break;
    }

    return "waiting for a request";
  }

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

  long_report make_get_reporting_request(uint16_t control_id) const {
    return make_request(feature_index_,
                        2,
                        {static_cast<uint8_t>(control_id >> 8),
                         static_cast<uint8_t>(control_id & 0xff)});
  }

  // setCidReporting(cid, flags, remap = 0 (unchanged))
  long_report make_set_reporting_request(uint16_t control_id, uint8_t flags) const {
    return make_request(feature_index_,
                        3,
                        {static_cast<uint8_t>(control_id >> 8),
                         static_cast<uint8_t>(control_id & 0xff),
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
    return current_control_index_ < control_ids_.size() &&
           ((report[4] << 8) | report[5]) == current_control_id();
  }

  result handle_notification(std::span<const uint8_t> report) {
    // REPROG_CONTROLS_V4 divertedButtonsEvent: up to four pressed control ids (big endian).
    if ((state_ != state::waiting_reporting &&
         state_ != state::waiting_divert &&
         state_ != state::diverted) ||
        report[2] != feature_index_ ||
        (report[3] >> 4) != 0) {
      return {};
    }

    std::vector<uint16_t> new_pressed_control_ids;
    for (size_t i = 4; i <= 10; i += 2) {
      auto control_id = static_cast<uint16_t>((report[i] << 8) | report[i + 1]);
      // Accept controls whose setCidReporting request has already been sent,
      // even while later selected controls are still being configured.
      if (std::ranges::find(divert_requested_control_ids_, control_id) != std::end(divert_requested_control_ids_)) {
        new_pressed_control_ids.push_back(control_id);
      }
    }
    std::ranges::sort(new_pressed_control_ids);
    new_pressed_control_ids.erase(std::unique(std::begin(new_pressed_control_ids), std::end(new_pressed_control_ids)),
                                  std::end(new_pressed_control_ids));

    result r;
    for (auto control_id : control_ids_) {
      auto was_pressed = std::ranges::find(pressed_control_ids_, control_id) != std::end(pressed_control_ids_);
      auto pressed = std::ranges::find(new_pressed_control_ids, control_id) != std::end(new_pressed_control_ids);
      if (was_pressed != pressed) {
        r.button_events.push_back({.control_id = control_id, .pressed = pressed});
      }
    }

    pressed_control_ids_ = std::move(new_pressed_control_ids);
    if (r.button_events.size() == 1) {
      r.pressed = r.button_events.front().pressed;
    }
    return r;
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

    result r;
    append_restore_requests(r);
    return r;
  }

  // Undo divert requests that the device might have applied, so the controls keep their firmware functions.
  // discovery_timed_out is decided by the caller before diverted controls are cleared here.
  result fail(const std::string& reason, bool discovery_timed_out = false) {
    state_ = state::failed;
    discovery_timed_out_ = discovery_timed_out;
    pending_request_ = std::nullopt;
    reason_ = reason;

    logger::get_logger()->warn("{0} not activated: {1}", log_prefix_, reason);

    result r;
    append_restore_requests(r);
    return r;
  }

  [[nodiscard]] uint16_t current_control_id() const {
    assert(current_control_index_ < control_ids_.size());
    return control_ids_[current_control_index_];
  }

  void append_restore_requests(result& r) {
    for (auto control_id : divert_requested_control_ids_) {
      auto request = make_set_reporting_request(control_id, reporting_flag_diverted_valid);
      if (!r.request) {
        r.request = request;
      } else {
        r.additional_requests.push_back(request);
      }
    }
    divert_requested_control_ids_.clear();
  }

  std::vector<uint16_t> control_ids_;
  uint8_t software_id_;
  std::string log_prefix_;
  state state_ = state::initial;
  uint8_t feature_index_ = 0;
  uint8_t control_count_ = 0;
  std::vector<control> controls_;
  bool controls_discovered_ = false;
  size_t current_control_index_ = 0;
  std::vector<uint16_t> divert_requested_control_ids_;
  bool discovery_timed_out_ = false;
  bool recovery_used_ = false;
  std::vector<uint16_t> pressed_control_ids_;
  std::optional<long_report> pending_request_;
  int attempts_ = 0;
  uint64_t generation_ = 0;
  std::string reason_;
};
} // namespace krbn::logitech::hidpp
