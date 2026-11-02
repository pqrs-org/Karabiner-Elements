#include "hidpp/device_support.hpp"
#include "hidpp/session.hpp"
#include <boost/ut.hpp>

namespace {
namespace hidpp = krbn::hidpp;

constexpr uint16_t top_button = 0x00c4;

hidpp::long_report make_report(std::initializer_list<uint8_t> bytes) {
  hidpp::long_report r{};
  size_t i = 0;
  for (auto b : bytes) {
    r[i++] = b;
  }
  return r;
}

hidpp::session::result input(hidpp::session& s, const hidpp::long_report& r) {
  return s.handle_input_report(r);
}

// Replies recorded from a Logitech MX Anywhere 3S over Bluetooth Low Energy with software id 1.
const auto feature_index_reply = make_report({0x11, 0xff, 0x00, 0x01, 0x0a, 0x00, 0x05});
const auto control_count_reply = make_report({0x11, 0xff, 0x0a, 0x01, 0x07});
const std::vector<hidpp::long_report> control_info_replies = {
    make_report({0x11, 0xff, 0x0a, 0x11, 0x00, 0x50, 0x00, 0x38, 0x01, 0x00, 0x01, 0x01, 0x04}),
    make_report({0x11, 0xff, 0x0a, 0x11, 0x00, 0x51, 0x00, 0x39, 0x01, 0x00, 0x01, 0x01, 0x04}),
    make_report({0x11, 0xff, 0x0a, 0x11, 0x00, 0x52, 0x00, 0x3a, 0x31, 0x00, 0x02, 0x03, 0x05}),
    make_report({0x11, 0xff, 0x0a, 0x11, 0x00, 0x53, 0x00, 0x3c, 0x31, 0x00, 0x02, 0x03, 0x0d}),
    make_report({0x11, 0xff, 0x0a, 0x11, 0x00, 0x56, 0x00, 0x3e, 0x31, 0x00, 0x02, 0x03, 0x0d}),
    make_report({0x11, 0xff, 0x0a, 0x11, 0x00, 0xc4, 0x00, 0x9d, 0x31, 0x00, 0x02, 0x03, 0x05}),
    make_report({0x11, 0xff, 0x0a, 0x11, 0x00, 0xd7, 0x00, 0xb4, 0xa0, 0x00, 0x03, 0x00, 0x03}),
};

hidpp::long_report reporting_reply(uint16_t cid, uint8_t flags, uint16_t remap = 0) {
  return make_report({0x11, 0xff, 0x0a, 0x21,
                      static_cast<uint8_t>(cid >> 8), static_cast<uint8_t>(cid & 0xff),
                      flags,
                      static_cast<uint8_t>(remap >> 8), static_cast<uint8_t>(remap & 0xff)});
}

hidpp::long_report divert_reply(uint16_t cid, uint8_t flags) {
  return make_report({0x11, 0xff, 0x0a, 0x31,
                      static_cast<uint8_t>(cid >> 8), static_cast<uint8_t>(cid & 0xff),
                      flags, 0x00, 0x00});
}

hidpp::long_report notification(std::initializer_list<uint16_t> cids) {
  auto r = make_report({0x11, 0xff, 0x0a, 0x00});
  size_t i = 4;
  for (auto cid : cids) {
    r[i++] = cid >> 8;
    r[i++] = cid & 0xff;
  }
  return r;
}

const auto get_feature_request = make_report({0x11, 0xff, 0x00, 0x01, 0x1b, 0x04});
const auto get_count_request = make_report({0x11, 0xff, 0x0a, 0x01});
const auto get_reporting_request = make_report({0x11, 0xff, 0x0a, 0x21, 0x00, 0xc4});
const auto divert_request = make_report({0x11, 0xff, 0x0a, 0x31, 0x00, 0xc4, 0x03, 0x00, 0x00});
const auto undivert_request = make_report({0x11, 0xff, 0x0a, 0x31, 0x00, 0xc4, 0x02, 0x00, 0x00});

hidpp::long_report get_control_info_request(uint8_t index) {
  return make_report({0x11, 0xff, 0x0a, 0x11, index});
}

// Runs discovery up to the getCidReporting request.
void discover(hidpp::session& s) {
  s.start();
  input(s, feature_index_reply);
  input(s, control_count_reply);
  for (const auto& r : control_info_replies) {
    input(s, r);
  }
}

void make_diverted(hidpp::session& s) {
  discover(s);
  input(s, reporting_reply(top_button, 0x00));
  input(s, divert_reply(top_button, 0x03));
}

hidpp::session make_session(uint16_t cid = top_button, uint8_t software_id = 1) {
  return hidpp::session(cid, software_id, "test hidpp_button:");
}

// Lets every attempt of the pending request time out.
void time_out(hidpp::session& s) {
  for (int i = 0; i < hidpp::max_attempts; ++i) {
    s.handle_timeout(s.get_generation());
  }
}

hidpp::long_report with_software_id(hidpp::long_report r, uint8_t software_id) {
  r[3] = static_cast<uint8_t>((r[3] & 0xf0) | software_id);
  return r;
}

// Sessions waiting for a reply in each discovery phase, and the HID++ error that answers the request.
struct discovery_phase final {
  void (*enter)(hidpp::session&);
  hidpp::long_report error;
};

const std::vector<discovery_phase> discovery_phases = {
    {[](hidpp::session& s) { s.start(); },
     make_report({0x11, 0xff, 0xff, 0x00, 0x01, 0x02})},
    {[](hidpp::session& s) { s.start(); input(s, feature_index_reply); },
     make_report({0x11, 0xff, 0xff, 0x0a, 0x01, 0x02})},
    {[](hidpp::session& s) { s.start(); input(s, feature_index_reply); input(s, control_count_reply); input(s, control_info_replies[0]); },
     make_report({0x11, 0xff, 0xff, 0x0a, 0x11, 0x02})},
    {[](hidpp::session& s) { discover(s); },
     make_report({0x11, 0xff, 0xff, 0x0a, 0x21, 0x02})},
};

std::vector<uint8_t> mx_anywhere_3s_descriptor() {
  // The report descriptor of Logitech MX Anywhere 3S (046d:b037) over Bluetooth Low Energy.
  // clang-format off
  return {
      0x05, 0x01, 0x09, 0x02, 0xa1, 0x01, 0x85, 0x02, 0x09, 0x01, 0xa1, 0x00,
      0x95, 0x10, 0x75, 0x01, 0x15, 0x00, 0x25, 0x01,
      0x05, 0x09, 0x19, 0x01, 0x29, 0x10, 0x81, 0x02, // Buttons 1-16
      0x05, 0x01, 0x16, 0x01, 0xf8, 0x26, 0xff, 0x07, 0x75, 0x0c, 0x95, 0x02, 0x09, 0x30, 0x09, 0x31, 0x81, 0x06,
      0x15, 0x81, 0x25, 0x7f, 0x75, 0x08, 0x95, 0x01, 0x09, 0x38, 0x81, 0x06,
      0x95, 0x01, 0x05, 0x0c, 0x0a, 0x38, 0x02, 0x81, 0x06,
      0xc0, 0xc0,
      0x06, 0x43, 0xff, 0x0a, 0x02, 0x02, 0xa1, 0x01, // Usage Page (0xff43), Usage (0x0202)
      0x85, 0x11, 0x75, 0x08, 0x95, 0x13, 0x15, 0x00, 0x26, 0xff, 0x00, // Report ID (0x11), 19 bytes
      0x09, 0x02, 0x81, 0x00,
      0x09, 0x02, 0x91, 0x00,
      0xc0,
  };
  // clang-format on
}

krbn::device_identifiers make_identifiers(uint16_t vendor_id,
                                          bool is_pointing_device) {
  return krbn::device_identifiers({
      .vendor_id = pqrs::hid::vendor_id::value_t(vendor_id),
      .product_id = pqrs::hid::product_id::value_t(0xb037),
      .is_pointing_device = is_pointing_device,
  });
}

const auto logitech_mouse = make_identifiers(0x046d, true);
} // namespace

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "is_long_report"_test = [] {
    auto r = notification({top_button});
    expect(hidpp::is_long_report(0x11, r));
    expect(!hidpp::is_long_report(0x10, r));
    expect(!hidpp::is_long_report(0x11, std::span<const uint8_t>(r).first(19)));

    auto receiver = r;
    receiver[1] = 0x01;
    expect(!hidpp::is_long_report(0x11, receiver));

    auto mismatched_id = r;
    mismatched_id[0] = 0x10;
    expect(!hidpp::is_long_report(0x11, mismatched_id));
  };

  "is_set_reporting_request"_test = [] {
    // Divert and restore are written synchronously; all other requests are read-only queries.
    expect(hidpp::is_set_reporting_request(divert_request));
    expect(hidpp::is_set_reporting_request(undivert_request));
    expect(!hidpp::is_set_reporting_request(get_feature_request));
    expect(!hidpp::is_set_reporting_request(get_count_request));
    expect(!hidpp::is_set_reporting_request(get_control_info_request(0)));
    expect(!hidpp::is_set_reporting_request(get_reporting_request));
  };

  "discovery and divert"_test = [] {
    auto s = make_session();

    expect(s.start().request == get_feature_request);
    expect(s.get_state() == hidpp::session::state::waiting_feature_index);
    expect(!s.start().request) << "start twice";

    expect(input(s, feature_index_reply).request == get_count_request);
    expect(input(s, control_count_reply).request == get_control_info_request(0));

    for (size_t i = 0; i < control_info_replies.size(); ++i) {
      auto r = input(s, control_info_replies[i]);
      if (i + 1 < control_info_replies.size()) {
        expect(r.request == get_control_info_request(static_cast<uint8_t>(i + 1)));
      } else {
        expect(r.request == get_reporting_request);
      }
    }

    expect(s.get_controls().size() == 7_ul);
    expect(s.get_controls()[5].control_id == top_button);
    expect(s.get_controls()[5].flags == 0x0531);
    expect(s.get_controls()[5].divertable());
    expect(!s.get_controls()[0].divertable());

    expect(input(s, reporting_reply(top_button, 0x00)).request == divert_request);
    expect(s.get_state() == hidpp::session::state::waiting_divert);

    auto r = input(s, divert_reply(top_button, 0x03));
    expect(!r.request);
    expect(s.get_state() == hidpp::session::state::diverted);
    expect(!s.waiting_reply());
  };

  "routing"_test = [] {
    auto s = make_session();
    make_diverted(s);

    expect(input(s, notification({top_button})).pressed == std::optional<bool>(true));
    expect(!input(s, notification({top_button})).pressed) << "duplicate";
    expect(!input(s, notification({0x0052, top_button})).pressed) << "another control";
    expect(input(s, notification({0x0052})).pressed == std::optional<bool>(false));
    expect(!input(s, notification({})).pressed) << "duplicate";

    // Only the configured control is routed.
    expect(!input(s, notification({0x0053})).pressed);
    expect(input(s, notification({0x0052, 0x0053, 0x0056, top_button})).pressed == std::optional<bool>(true));
  };

  "routing another control"_test = [] {
    auto s = make_session(0x0053);
    discover(s);
    input(s, reporting_reply(0x0053, 0x00));
    input(s, divert_reply(0x0053, 0x03));
    expect(s.get_state() == hidpp::session::state::diverted);

    expect(!input(s, notification({top_button})).pressed);
    expect(input(s, notification({0x0053})).pressed == std::optional<bool>(true));
  };

  "notifications are ignored unless diverted"_test = [] {
    auto s = make_session();
    expect(!input(s, notification({top_button})).pressed);
    discover(s);
    expect(!input(s, notification({top_button})).pressed);

    make_diverted(s);
    auto other_feature = notification({top_button});
    other_feature[2] = 0x0b;
    expect(!input(s, other_feature).pressed);

    auto other_event = notification({top_button});
    other_event[3] = 0x10;
    expect(!input(s, other_event).pressed);
  };

  "refused: feature is not supported"_test = [] {
    auto s = make_session();
    s.start();
    auto r = input(s, make_report({0x11, 0xff, 0x00, 0x01, 0x00}));
    expect(!r.request);
    expect(s.get_state() == hidpp::session::state::refused);
    expect(!s.stop().request) << "nothing to restore";
  };

  "refused: no controls"_test = [] {
    auto s = make_session();
    s.start();
    input(s, feature_index_reply);
    expect(!input(s, make_report({0x11, 0xff, 0x0a, 0x01, 0x00})).request);
    expect(s.get_state() == hidpp::session::state::refused);
  };

  "refused: control does not exist"_test = [] {
    auto s = make_session(0x00c5);
    discover(s);
    expect(s.get_state() == hidpp::session::state::refused);
    expect(s.get_reason().find("197 (0x00c5) does not exist") != std::string::npos);
    expect(s.get_reason().find("82 (0x0052), 83 (0x0053), 86 (0x0056), 196 (0x00c4), 215 (0x00d7)") != std::string::npos)
        << s.get_reason();
    expect(!s.stop().request);
  };

  "refused: control cannot be diverted"_test = [] {
    auto s = make_session(0x0050);
    discover(s);
    expect(s.get_state() == hidpp::session::state::refused);
    expect(s.get_reason().find("cannot be temporarily diverted") != std::string::npos);
    expect(!s.stop().request);
  };

  "refused: control is owned by another client"_test = [] {
    for (uint8_t flags : {0x01, 0x03, 0x04, 0x10, 0x40}) {
      auto s = make_session();
      discover(s);
      expect(!input(s, reporting_reply(top_button, flags)).request);
      expect(s.get_state() == hidpp::session::state::refused) << static_cast<int>(flags);
      expect(s.get_reason().find("already diverted") != std::string::npos);
      expect(!s.stop().request) << "another client's setting must not be restored";
    }

    {
      auto s = make_session();
      discover(s);
      expect(!input(s, reporting_reply(top_button, 0x00, 0x0053)).request);
      expect(s.get_state() == hidpp::session::state::refused);
      expect(s.get_reason().find("remapped") != std::string::npos);
    }

    // A remap to itself is not a remap.
    {
      auto s = make_session();
      discover(s);
      expect(input(s, reporting_reply(top_button, 0x00, top_button)).request == divert_request);
    }
  };

  "malformed packets and wrong replies"_test = [] {
    auto s = make_session();
    s.start();

    auto ignored = [&](std::span<const uint8_t> r) {
      auto result = s.handle_input_report(r);
      expect(!result.request && !result.pressed);
      expect(s.get_state() == hidpp::session::state::waiting_feature_index);
    };

    ignored(std::span<const uint8_t>(feature_index_reply).first(19));
    ignored({});

    auto wrong_report_id = feature_index_reply;
    wrong_report_id[0] = 0x10;
    ignored(wrong_report_id);

    auto receiver = feature_index_reply;
    receiver[1] = 0x01;
    ignored(receiver);

    auto wrong_software_id = feature_index_reply;
    wrong_software_id[3] = 0x02;
    ignored(wrong_software_id);

    auto wrong_function = feature_index_reply;
    wrong_function[3] = 0x11;
    ignored(wrong_function);

    auto wrong_feature_index = feature_index_reply;
    wrong_feature_index[2] = 0x0a;
    ignored(wrong_feature_index);

    // An error for another request.
    ignored(make_report({0x11, 0xff, 0xff, 0x0a, 0x31, 0x05}));

    expect(input(s, feature_index_reply).request == get_count_request);

    // Wrong control in replies.
    for (const auto& r : {control_count_reply}) {
      input(s, r);
    }
    for (const auto& r : control_info_replies) {
      input(s, r);
    }
    expect(s.get_state() == hidpp::session::state::waiting_reporting);
    expect(!input(s, reporting_reply(0x0052, 0x00)).request);
    expect(s.get_state() == hidpp::session::state::waiting_reporting);
    input(s, reporting_reply(top_button, 0x00));
    expect(!input(s, divert_reply(0x0052, 0x03)).request);
    expect(s.get_state() == hidpp::session::state::waiting_divert);
  };

  "duplicated control info reply"_test = [] {
    auto s = make_session();
    s.start();
    input(s, feature_index_reply);
    input(s, control_count_reply);

    expect(input(s, control_info_replies[0]).request == get_control_info_request(1));
    auto r = input(s, control_info_replies[0]);
    expect(!r.request);
    expect(s.get_controls().size() == 1_ul);

    for (size_t i = 1; i < control_info_replies.size(); ++i) {
      input(s, control_info_replies[i]);
    }
    expect(s.get_state() == hidpp::session::state::waiting_reporting);
  };

  "HID++ errors"_test = [] {
    // During discovery: nothing to restore.
    {
      auto s = make_session();
      s.start();
      auto r = input(s, make_report({0x11, 0xff, 0xff, 0x00, 0x01, 0x02}));
      expect(!r.request);
      expect(s.get_state() == hidpp::session::state::failed);
      expect(!s.stop().request);
    }

    // During divert: the divert request is undone.
    {
      auto s = make_session();
      discover(s);
      input(s, reporting_reply(top_button, 0x00));
      auto r = input(s, make_report({0x11, 0xff, 0xff, 0x0a, 0x31, 0x02}));
      expect(r.request == undivert_request);
      expect(s.get_state() == hidpp::session::state::failed);
      expect(!s.stop().request) << "undo once";
    }

    // The device did not divert.
    {
      auto s = make_session();
      discover(s);
      input(s, reporting_reply(top_button, 0x00));
      auto r = input(s, divert_reply(top_button, 0x02));
      expect(r.request == undivert_request);
      expect(s.get_state() == hidpp::session::state::failed);
    }
  };

  "timeouts"_test = [] {
    auto s = make_session();
    s.start();
    auto generation = s.get_generation();

    expect(!s.handle_timeout(generation - 1).request) << "stale";

    for (int i = 1; i < hidpp::max_attempts; ++i) {
      auto r = s.handle_timeout(generation);
      expect(r.request == get_feature_request);
      expect(!s.handle_timeout(generation).request) << "stale after retry";
      generation = s.get_generation();
    }

    auto r = s.handle_timeout(generation);
    expect(!r.request);
    expect(s.get_state() == hidpp::session::state::failed);

    // A timer of a finished request is ignored.
    {
      auto s2 = make_session();
      s2.start();
      auto g = s2.get_generation();
      input(s2, feature_index_reply);
      expect(!s2.handle_timeout(g).request);
      expect(s2.get_state() == hidpp::session::state::waiting_control_count);
    }

    // No reply to the divert request.
    {
      auto s2 = make_session();
      discover(s2);
      input(s2, reporting_reply(top_button, 0x00));
      for (int i = 1; i < hidpp::max_attempts; ++i) {
        expect(s2.handle_timeout(s2.get_generation()).request == divert_request);
      }
      expect(s2.handle_timeout(s2.get_generation()).request == undivert_request);
      expect(s2.get_state() == hidpp::session::state::failed);
    }
  };

  "recovery after a discovery timeout"_test = [] {
    for (const auto& phase : discovery_phases) {
      auto s = make_session();
      phase.enter(s);
      auto old_generation = s.get_generation();
      auto old_state = s.get_state();

      // Activity while waiting changes nothing.
      expect(!s.can_recover_on_physical_activity());
      expect(!s.restart_after_discovery_timeout(2).request);
      expect(s.get_state() == old_state);
      expect(s.get_generation() == old_generation);
      expect(s.waiting_reply());

      time_out(s);
      expect(s.get_state() == hidpp::session::state::failed);
      expect(s.can_recover_on_physical_activity());
      expect(s.can_recover_on_physical_activity()) << "the getter does not consume the allowance";

      // Nothing is sent without activity.
      expect(!s.waiting_reply());
      expect(!s.handle_timeout(s.get_generation()).request);
      expect(!input(s, feature_index_reply).request);
      expect(s.can_recover_on_physical_activity());

      auto failed_generation = s.get_generation();
      auto r = s.restart_after_discovery_timeout(2);
      expect(r.request == with_software_id(get_feature_request, 2));
      expect(!r.pressed);
      expect(s.get_state() == hidpp::session::state::waiting_feature_index);
      expect(s.waiting_reply());
      expect(s.get_controls().empty());
      expect(s.get_reason().empty());
      expect(s.get_generation() > failed_generation) << "the generation is never reset";
      expect(!s.can_recover_on_physical_activity());

      // Repeated activity does not restart again.
      auto restarted_generation = s.get_generation();
      expect(!s.restart_after_discovery_timeout(3).request);
      expect(s.get_generation() == restarted_generation);

      // Timers of the failed attempt and replies to its software id are ignored.
      expect(!s.handle_timeout(old_generation).request);
      expect(!s.handle_timeout(failed_generation).request);
      expect(!input(s, feature_index_reply).request);
      expect(s.get_state() == hidpp::session::state::waiting_feature_index);

      // The second failure is final.
      {
        auto failed_again = s;
        time_out(failed_again);
        expect(failed_again.get_state() == hidpp::session::state::failed);
        expect(!failed_again.can_recover_on_physical_activity());
        expect(!failed_again.restart_after_discovery_timeout(4).request);
        expect(failed_again.get_state() == hidpp::session::state::failed);
        expect(!failed_again.stop().request);
      }

      // Discovery starts from the beginning with the new software id.
      expect(input(s, with_software_id(feature_index_reply, 2)).request == with_software_id(get_count_request, 2));
      expect(input(s, with_software_id(control_count_reply, 2)).request == with_software_id(get_control_info_request(0), 2));
      for (size_t i = 0; i < control_info_replies.size(); ++i) {
        input(s, with_software_id(control_info_replies[i], 2));
      }
      expect(s.get_controls().size() == 7_ul);
      expect(input(s, with_software_id(reporting_reply(top_button, 0x00), 2)).request == with_software_id(divert_request, 2));
      input(s, with_software_id(divert_reply(top_button, 0x03), 2));
      expect(s.get_state() == hidpp::session::state::diverted);
      expect(!s.can_recover_on_physical_activity());
      expect(input(s, notification({top_button})).pressed == std::optional<bool>(true));
      expect(s.stop().request == with_software_id(undivert_request, 2));
    }

    // The new software id is normalized like the constructor argument.
    {
      auto s = make_session(top_button, 2);
      s.start();
      time_out(s);
      expect(s.restart_after_discovery_timeout(0x10).request == get_feature_request);
    }
  };

  "recovery: control ownership is checked again"_test = [] {
    auto recover = [] {
      auto s = make_session();
      s.start();
      time_out(s);
      s.restart_after_discovery_timeout(2);
      input(s, with_software_id(feature_index_reply, 2));
      input(s, with_software_id(control_count_reply, 2));
      for (const auto& r : control_info_replies) {
        input(s, with_software_id(r, 2));
      }
      return s;
    };

    {
      auto s = recover();
      expect(s.get_state() == hidpp::session::state::waiting_reporting);
      expect(!input(s, with_software_id(reporting_reply(top_button, 0x01), 2)).request);
      expect(s.get_state() == hidpp::session::state::refused);
      expect(s.get_reason().find("already diverted") != std::string::npos);
      expect(!s.can_recover_on_physical_activity());
      expect(!s.stop().request) << "another client's setting must not be restored";
    }

    {
      auto s = recover();
      expect(!input(s, with_software_id(reporting_reply(top_button, 0x00, 0x0053), 2)).request);
      expect(s.get_state() == hidpp::session::state::refused);
      expect(s.get_reason().find("remapped") != std::string::npos);
      expect(!s.restart_after_discovery_timeout(3).request);
      expect(!s.stop().request);
    }
  };

  "recovery: only discovery timeouts"_test = [] {
    auto expect_no_recovery = [](hidpp::session& s, hidpp::session::state expected) {
      auto generation = s.get_generation();
      expect(!s.can_recover_on_physical_activity());
      expect(!s.restart_after_discovery_timeout(2).request);
      expect(s.get_state() == expected);
      expect(s.get_generation() == generation);
    };

    // Not started.
    {
      auto s = make_session();
      expect_no_recovery(s, hidpp::session::state::initial);
    }

    // HID++ errors.
    for (const auto& phase : discovery_phases) {
      auto s = make_session();
      phase.enter(s);
      input(s, phase.error);
      expect(s.get_state() == hidpp::session::state::failed);
      expect_no_recovery(s, hidpp::session::state::failed);
    }

    // Refusals.
    {
      auto s = make_session();
      s.start();
      input(s, make_report({0x11, 0xff, 0x00, 0x01, 0x00}));
      expect_no_recovery(s, hidpp::session::state::refused);
    }
    {
      auto s = make_session(0x0050);
      discover(s);
      expect_no_recovery(s, hidpp::session::state::refused);
    }
    {
      auto s = make_session();
      discover(s);
      input(s, reporting_reply(top_button, 0x01));
      expect_no_recovery(s, hidpp::session::state::refused);
    }

    // No reply to the divert request.
    {
      auto s = make_session();
      discover(s);
      input(s, reporting_reply(top_button, 0x00));
      time_out(s);
      expect(s.get_state() == hidpp::session::state::failed);
      expect_no_recovery(s, hidpp::session::state::failed);
      expect(!s.stop().request) << "undo once";
    }

    // The device did not divert.
    {
      auto s = make_session();
      discover(s);
      input(s, reporting_reply(top_button, 0x00));
      input(s, divert_reply(top_button, 0x02));
      expect_no_recovery(s, hidpp::session::state::failed);
    }

    // Stop cancels the recovery.
    {
      auto s = make_session();
      s.start();
      time_out(s);
      expect(s.can_recover_on_physical_activity());
      expect(!s.stop().request);
      expect_no_recovery(s, hidpp::session::state::stopped);
    }

    // A diverted and held control is not affected.
    {
      auto s = make_session();
      make_diverted(s);
      input(s, notification({top_button}));
      expect_no_recovery(s, hidpp::session::state::diverted);
      expect(s.get_pressed());
      expect(input(s, notification({})).pressed == std::optional<bool>(false));
      expect(input(s, notification({top_button})).pressed == std::optional<bool>(true));
      auto r = s.stop();
      expect(r.request == undivert_request);
      expect(r.pressed == std::optional<bool>(false));
    }
  };

  "failed writes"_test = [] {
    // A request which could not be written is not answered, so it is handled by the bounded reply timeout.
    // No further request is sent after the last attempt.
    {
      auto s = make_session();
      s.start();
      input(s, feature_index_reply);
      for (int i = 1; i < hidpp::max_attempts; ++i) {
        expect(s.handle_timeout(s.get_generation()).request == get_count_request);
      }
      expect(!s.handle_timeout(s.get_generation()).request);
      expect(s.get_state() == hidpp::session::state::failed);
      expect(!s.handle_timeout(s.get_generation()).request);
      expect(!s.stop().request) << "nothing was diverted";
    }

    // A restore request is sent once and never retried.
    {
      auto s = make_session();
      make_diverted(s);
      auto generation = s.get_generation();
      expect(s.stop().request == undivert_request);
      expect(!s.waiting_reply());
      expect(!s.handle_timeout(generation).request);
      expect(!s.handle_timeout(s.get_generation()).request);
      expect(!s.stop().request);
    }
  };

  "stop"_test = [] {
    // While held: release and restore.
    {
      auto s = make_session();
      make_diverted(s);
      input(s, notification({top_button}));

      auto r = s.stop();
      expect(r.request == undivert_request);
      expect(r.pressed == std::optional<bool>(false));
      expect(s.get_state() == hidpp::session::state::stopped);

      auto r2 = s.stop();
      expect(!r2.request && !r2.pressed) << "stop twice";

      expect(!input(s, notification({})).pressed);
      expect(!input(s, notification({top_button})).pressed);
    }

    // While not held.
    {
      auto s = make_session();
      make_diverted(s);
      auto r = s.stop();
      expect(r.request == undivert_request);
      expect(!r.pressed);
    }

    // Before the divert request.
    {
      auto s = make_session();
      s.start();
      expect(!s.stop().request);
    }

    // While waiting for the divert reply: the request may already have been applied.
    {
      auto s = make_session();
      discover(s);
      input(s, reporting_reply(top_button, 0x00));
      auto generation = s.get_generation();
      expect(s.stop().request == undivert_request);
      expect(!s.handle_timeout(generation).request) << "stale timer";
      expect(!input(s, divert_reply(top_button, 0x03)).request) << "late reply";
      expect(s.get_state() == hidpp::session::state::stopped);
    }
  };

  "stale replies of a previous session"_test = [] {
    // Configuration replacement creates a session with another software id.
    auto s = make_session(top_button, 2);
    expect(s.start().request == make_report({0x11, 0xff, 0x00, 0x02, 0x1b, 0x04}));

    // Replies with software id 1 are ignored.
    expect(!input(s, feature_index_reply).request);
    expect(s.get_state() == hidpp::session::state::waiting_feature_index);
    expect(!input(s, divert_reply(top_button, 0x02)).request);

    auto own = feature_index_reply;
    own[3] = 0x02;
    expect(input(s, own).request == make_report({0x11, 0xff, 0x0a, 0x02}));
  };

  "device_support"_test = [] {
    auto descriptor = mx_anywhere_3s_descriptor();

    {
      hidpp::device_support support(logitech_mouse, "Bluetooth Low Energy", descriptor);
      expect(!support.get_unsupported_reason());
      expect(support.make_input_buttons_string() == "declared: button1-button16");
      expect(!support.find_pointing_button_conflict(pqrs::hid::usage::button::button_17));
      expect(!support.find_pointing_button_conflict(pqrs::hid::usage::button::button_32));

      // button6 is declared by the mouse report, so it is refused.
      auto conflict = support.find_pointing_button_conflict(pqrs::hid::usage::button::button_6);
      expect(conflict.has_value());
      expect(conflict->find("button1-button16") != std::string::npos) << *conflict;
      expect(support.find_pointing_button_conflict(pqrs::hid::usage::button::button_1).has_value());
      expect(support.find_pointing_button_conflict(pqrs::hid::usage::button::button_16).has_value());
    }

    expect(!hidpp::device_support(logitech_mouse, "BluetoothLowEnergy", descriptor).get_unsupported_reason());

    auto unsupported = [&](const krbn::device_identifiers& identifiers,
                           std::string_view transport,
                           std::span<const uint8_t> d,
                           std::string_view expected) {
      hidpp::device_support support(identifiers, transport, d);
      expect(support.get_unsupported_reason().has_value());
      if (support.get_unsupported_reason()) {
        expect(support.get_unsupported_reason()->find(expected) != std::string::npos) << *support.get_unsupported_reason();
      }
      expect(support.find_pointing_button_conflict(pqrs::hid::usage::button::button_17).has_value());
    };

    unsupported(make_identifiers(0x056e, true), "Bluetooth Low Energy", descriptor, "only Logitech");
    unsupported(make_identifiers(0x046d, false), "Bluetooth Low Energy", descriptor, "only pointing devices");
    unsupported(logitech_mouse, "USB", descriptor, "transport `USB`");
    unsupported(logitech_mouse, "Bluetooth", descriptor, "receivers are not supported");
    unsupported(logitech_mouse, "Bluetooth Low Energy", {}, "HID++ long reports");

    // Without the HID++ output report.
    {
      auto d = descriptor;
      d.erase(d.end() - 5, d.end() - 1);
      unsupported(logitech_mouse, "Bluetooth Low Energy", d, "HID++ long reports");
    }

    // A HID++ report with another size.
    {
      auto d = descriptor;
      d[d.size() - 15] = 0x06; // Report Count (6)
      unsupported(logitech_mouse, "Bluetooth Low Energy", d, "HID++ long reports");
    }

    expect(hidpp::device_support().get_unsupported_reason().has_value());
  };

  "device_support: explicit button usages"_test = [] {
    auto d = mx_anywhere_3s_descriptor();
    // Usage (Button 1), Usage (Button 6) instead of Usage Minimum (1), Usage Maximum (16)
    d[23] = 0x01;
    d[22] = 0x09;
    d[24] = 0x09;
    d[25] = 0x06;

    hidpp::device_support support(logitech_mouse, "Bluetooth Low Energy", d);
    expect(!support.get_unsupported_reason());
    expect(support.make_input_buttons_string() == "declared: button1, button6");
    expect(support.find_pointing_button_conflict(pqrs::hid::usage::button::button_6).has_value());
    expect(!support.find_pointing_button_conflict(pqrs::hid::usage::button::button_17));
  };

  "device_support: button input without usages"_test = [] {
    auto d = mx_anywhere_3s_descriptor();
    // Remove Usage Minimum/Maximum of the buttons.
    d.erase(d.begin() + 22, d.begin() + 26);

    hidpp::device_support support(logitech_mouse, "Bluetooth Low Energy", d);
    expect(!support.get_unsupported_reason());
    auto conflict = support.find_pointing_button_conflict(pqrs::hid::usage::button::button_17);
    expect(conflict.has_value());
    expect(conflict->find("cannot be determined") != std::string::npos) << *conflict;
  };

  return 0;
}
