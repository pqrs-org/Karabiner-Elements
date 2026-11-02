#pragma once

#include "event_queue/entry.hpp"
#include "modifier_flag_manager.hpp"
#include <pqrs/hash.hpp>
#include <unordered_map>
#include <unordered_set>

namespace krbn::event_queue {

// Owns the modifier states maintained by an event queue.
//
// `modifier_flag_manager` represents one stream of modifier changes. This
// class combines the streams needed by the event pipeline: the effective
// output state, the state shared by ordinary matching, and output-only state
// associated with a specific source event.
class modifier_state_manager final {
public:
  modifier_state_manager(const modifier_state_manager&) = delete;

  modifier_state_manager() = default;

  [[nodiscard]] const modifier_flag_manager& get_output_state() const {
    return output_state_;
  }

  [[nodiscard]] modifier_flag_manager& get_output_state() {
    return const_cast<modifier_flag_manager&>(static_cast<const modifier_state_manager&>(*this).get_output_state());
  }

  [[nodiscard]] const modifier_flag_manager& get_regular_matching_state() const {
    return regular_matching_state_;
  }

  [[nodiscard]] std::unordered_set<modifier_flag> make_modifier_flags_for_matching(const entry& entry) const {
    auto result = regular_matching_state_.make_modifier_flags();

    auto output_only_modifier_flags = make_output_only_modifier_flags_for_matching(entry);
    result.insert(std::begin(output_only_modifier_flags),
                  std::end(output_only_modifier_flags));

    return result;
  }

  [[nodiscard]] std::unordered_set<modifier_flag> make_output_only_modifier_flags_for_matching(const entry& entry) const {
    // Include output-only modifiers only for entries generated from the same
    // source event. For example, `a -> shift+1` matches a downstream
    // `shift+1` rule, while a later physical key does not inherit shift.
    // Tracking this state per queue also reflects intermediate transformations:
    // if another manager transforms or consumes shift, the original shift is
    // no longer included when the associated `1` is matched downstream.
    auto it = output_only_states_.find(make_output_only_state_key(entry));
    if (it != std::end(output_only_states_)) {
      return it->second.make_modifier_flags();
    }

    return {};
  }

  [[nodiscard]] std::unordered_set<modifier_flag> make_output_only_modifier_flags_for_manipulation(const entry& entry) const {
    auto result = make_output_only_modifier_flags_for_matching(entry);

    // The current entry has not been applied to this state yet. Include its
    // modifier as provenance for transformations, but not in the modifier set
    // used to match the entry itself.
    if (entry.get_output_only_modifier()) {
      if (auto e = entry.get_event().get_if<momentary_switch_event>()) {
        if (auto modifier_flag = e->make_modifier_flag()) {
          result.insert(*modifier_flag);
        }
      }
    }

    return result;
  }

  // Apply the modifier portion of an entry without enqueueing it.
  //
  // A deferred key_up event can be posted directly to a downstream queue. In
  // that case, the queues it skipped must still update their modifier state so
  // future output does not restore a modifier that has already been released.
  void apply(const entry& entry) {
    update_state(output_state_, entry);

    if (entry.get_output_only_modifier()) {
      update_output_only_state(entry);
    } else {
      update_state(regular_matching_state_, entry);
    }
  }

  void erase_all_active_modifier_flags_except_lock_and_sticky(device_id device_id) {
    output_state_.erase_all_active_modifier_flags_except_lock_and_sticky(device_id);
    regular_matching_state_.erase_all_active_modifier_flags_except_lock_and_sticky(device_id);

    for (auto& pair : output_only_states_) {
      pair.second.erase_all_active_modifier_flags_except_lock_and_sticky(device_id);
    }
    erase_empty_output_only_states();
  }

  void erase_all_active_modifier_flags(device_id device_id) {
    output_state_.erase_all_active_modifier_flags(device_id);
    regular_matching_state_.erase_all_active_modifier_flags(device_id);

    for (auto& pair : output_only_states_) {
      pair.second.erase_all_active_modifier_flags(device_id);
    }
    erase_empty_output_only_states();
  }

private:
  struct output_only_state_key final {
    krbn::device_id device_id;
    event original_event;

    bool operator==(const output_only_state_key&) const = default;
  };

  struct output_only_state_key_hash final {
    std::size_t operator()(const output_only_state_key& value) const {
      std::size_t h = 0;

      pqrs::hash::combine(h, value.device_id);
      pqrs::hash::combine(h, value.original_event);

      return h;
    }
  };

  [[nodiscard]] static output_only_state_key make_output_only_state_key(const entry& entry) {
    return {
        .device_id = entry.get_device_id(),
        .original_event = entry.get_original_event(),
    };
  }

  void update_output_only_state(const entry& entry) {
    auto it = output_only_states_.try_emplace(make_output_only_state_key(entry)).first;

    update_state(it->second, entry);
    erase_empty_output_only_states();
  }

  void erase_empty_output_only_states() {
    std::erase_if(output_only_states_,
                  [](const auto& pair) {
                    return pair.second.make_modifier_flags().empty();
                  });
  }

  static void update_state(modifier_flag_manager& state,
                           const entry& entry) {
    const auto& event = entry.get_event();
    auto event_type = entry.get_event_type();
    auto device_id = entry.get_device_id();

    if (auto e = event.get_if<momentary_switch_event>()) {
      if (auto modifier_flag = e->make_modifier_flag()) {
        auto type = (event_type == event_type::key_down ? modifier_flag_manager::active_modifier_flag::type::increase
                                                        : modifier_flag_manager::active_modifier_flag::type::decrease);
        state.push_back_active_modifier_flag(
            modifier_flag_manager::active_modifier_flag(type,
                                                        *modifier_flag,
                                                        device_id));
      }

      if (event_type == event_type::key_down &&
          entry.get_validity() == validity::valid &&
          !e->modifier_flag()) {
        state.erase_all_sticky_modifier_flags();
      }

    } else if (event.get_type() == event::type::sticky_modifier) {
      if (event_type == event_type::key_down || event_type == event_type::single) {
        if (auto sticky_modifier = event.get_sticky_modifier()) {
          auto type = modifier_flag_manager::active_modifier_flag::type::increase_sticky;

          if (sticky_modifier->second == sticky_modifier_type::toggle) {
            if (state.sticky_size(sticky_modifier->first) > 0) {
              type = modifier_flag_manager::active_modifier_flag::type::decrease_sticky;
            }
          } else if (sticky_modifier->second != sticky_modifier_type::on) {
            type = modifier_flag_manager::active_modifier_flag::type::decrease_sticky;
          }

          state.push_back_active_modifier_flag(
              modifier_flag_manager::active_modifier_flag(type,
                                                          sticky_modifier->first,
                                                          device_id));
        }
      }

    } else if (event.get_type() == event::type::caps_lock_state_changed) {
      if (auto integer_value = event.get_integer_value()) {
        auto type = (*integer_value ? modifier_flag_manager::active_modifier_flag::type::increase_led_lock
                                    : modifier_flag_manager::active_modifier_flag::type::decrease_led_lock);
        state.push_back_active_modifier_flag(
            modifier_flag_manager::active_modifier_flag(type,
                                                        modifier_flag::caps_lock,
                                                        device_id));
      }
    }
  }

  // The modifier state is split into effective output, regular matching, and
  // source-specific output-only states. The following examples show why these
  // states cannot be represented by a single modifier set.
  //
  // The modifiers used to match an entry are composed as follows:
  //
  //   regular_matching_state_
  //     + output_only_states_ entries with the same device and original event
  //
  // `output_state_` is not used directly for matching.
  //
  // Consider the following rules in two consecutive manipulator managers:
  //
  //   Simple Modifications:  1       -> shift+1
  //   Complex Modifications: shift+1 -> x
  //
  // The shift and 1 generated by Simple Modifications have the same
  // `original_event`. Before Complex Modifications matches the generated 1,
  // the output queue of Simple Modifications contains:
  //
  // - output_state_:           [shift]
  // - regular_matching_state_: []
  // - output_only_states_:     [shift (device:1, original event: 1)]
  // - modifiers for matching:  [shift] (regular_matching_state_ + output_only_states_)
  //
  // The selected output-only entry contributes shift to matching, so the
  // second rule produces x.
  //
  // In contrast, consider these rules:
  //
  //   Simple Modifications:  1       -> shift+1
  //   Complex Modifications: shift+b -> y
  //
  // If a physical b is pressed while 1 is held for key repeat, b has a
  // different `original_event`. Before Complex Modifications matches b, the
  // output queue of Simple Modifications contains:
  //
  // - output_state_:           [shift]
  // - regular_matching_state_: []
  // - output_only_states_:     [shift (device:1, original event: 1)]
  // - modifiers for matching:  [] (regular_matching_state_ + output_only_states_)
  //
  // Thus, the second rule does not produce y. The deferred generated shift is
  // released before b is output, and its key_up removes it from
  // `output_state_` and removes the output-only entry once no modifiers
  // remain. A physically held shift is part of the regular matching state, so
  // it still makes the second rule match and produce y.
  //
  // A standalone generated modifier is also different from an output-only
  // modifier. For example:
  //
  //   Simple Modifications:  1       -> shift
  //   Complex Modifications: shift+b -> y
  //
  // Here, shift is the primary output of the first rule. Before Complex
  // Modifications matches b, the output queue of Simple Modifications
  // contains:
  //
  // - output_state_:           [shift]
  // - regular_matching_state_: [shift]
  // - output_only_states_:     []
  // - modifiers for matching:  [shift] (regular_matching_state_ + output_only_states_)
  //
  // Pressing physical b while holding 1 therefore matches the second rule and
  // produces y. In contrast, the shift kept pressed solely to accompany the
  // repeating 1 in `1 -> shift+1` is isolated by source event in
  // `output_only_states_`.

  // Effective modifier state sent to the virtual device, including both
  // regular and output-only modifiers.
  modifier_flag_manager output_state_;

  // Modifier state used for matching unrelated subsequent input.
  modifier_flag_manager regular_matching_state_;

  // Source-specific modifiers used only when matching associated output.
  // States are grouped by `(device_id, original_event)` and removed when empty.
  std::unordered_map<output_only_state_key,
                     modifier_flag_manager,
                     output_only_state_key_hash>
      output_only_states_;
};

} // namespace krbn::event_queue
