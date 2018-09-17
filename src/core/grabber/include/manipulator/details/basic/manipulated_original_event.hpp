#pragma once

class manipulated_original_event final {
public:
  class from_event final {
  public:
    from_event(void) : device_id_(device_id::zero) {
    }

    from_event(device_id device_id,
               const event_queue::event& event,
               const event_queue::event& original_event) : device_id_(device_id),
                                                           event_(event),
                                                           original_event_(original_event) {
    }

    device_id get_device_id(void) const {
      return device_id_;
    }

    const event_queue::event& get_event(void) const {
      return event_;
    }

    const event_queue::event& get_original_event(void) const {
      return original_event_;
    }

    bool operator==(const from_event& other) const {
      return device_id_ == other.device_id_ &&
             event_ == other.event_ &&
             original_event_ == other.original_event_;
    }

    friend size_t hash_value(const from_event& value) {
      size_t h = 0;
      boost::hash_combine(h, value.device_id_);
      boost::hash_combine(h, value.event_);
      boost::hash_combine(h, value.original_event_);
      return h;
    }

  private:
    device_id device_id_;
    event_queue::event event_;
    event_queue::event original_event_;
  };

  struct from_event_hash final {
    std::size_t operator()(const from_event& v) const noexcept {
      return hash_value(v);
    }
  };

  class events_at_key_up final {
  public:
    class entry {
    public:
      entry(device_id device_id,
            const event_queue::event& event,
            event_type event_type,
            const event_queue::event& original_event,
            bool lazy) : device_id_(device_id),
                         event_(event),
                         event_type_(event_type),
                         original_event_(original_event),
                         lazy_(lazy) {
      }

      event_queue::entry make_entry(const event_queue::event_time_stamp& event_time_stamp) const {
        return event_queue::entry(device_id_,
                                  event_time_stamp,
                                  event_,
                                  event_type_,
                                  original_event_,
                                  lazy_);
      }

    private:
      device_id device_id_;
      event_queue::event event_;
      event_type event_type_;
      event_queue::event original_event_;
      bool lazy_;
    };

    const std::vector<entry>& get_events(void) const {
      return events_;
    }

    void emplace_back_event(device_id device_id,
                            const event_queue::event& event,
                            event_type event_type,
                            const event_queue::event& original_event,
                            bool lazy) {
      events_.emplace_back(device_id,
                           event,
                           event_type,
                           original_event,
                           lazy);
    }

    void clear_events(void) {
      events_.clear();
    }

  private:
    std::vector<entry> events_;
  };

  manipulated_original_event(const std::unordered_set<from_event, from_event_hash>& from_events,
                             const std::unordered_set<modifier_flag>& from_mandatory_modifiers,
                             absolute_time key_down_time_stamp) : from_events_(from_events),
                                                                  from_mandatory_modifiers_(from_mandatory_modifiers),
                                                                  key_down_time_stamp_(key_down_time_stamp),
                                                                  alone_(true),
                                                                  halted_(false),
                                                                  key_up_posted_(false) {
  }

  const std::unordered_set<from_event, from_event_hash>& get_from_events(void) const {
    return from_events_;
  }

  const std::unordered_set<modifier_flag>& get_from_mandatory_modifiers(void) const {
    return from_mandatory_modifiers_;
  }

  std::unordered_set<modifier_flag>& get_key_up_posted_from_mandatory_modifiers(void) {
    return key_up_posted_from_mandatory_modifiers_;
  }

  absolute_time get_key_down_time_stamp(void) const {
    return key_down_time_stamp_;
  }

  bool get_alone(void) const {
    return alone_;
  }

  bool get_halted(void) const {
    return halted_;
  }

  void set_halted(void) {
    halted_ = true;
  }

  const events_at_key_up& get_events_at_key_up(void) const {
    return events_at_key_up_;
  }
  events_at_key_up& get_events_at_key_up(void) {
    return const_cast<events_at_key_up&>(static_cast<const manipulated_original_event&>(*this).get_events_at_key_up());
  }

  bool get_key_up_posted(void) const {
    return key_up_posted_;
  }

  void set_key_up_posted(bool value) {
    key_up_posted_ = true;
  }

  void unset_alone(void) {
    alone_ = false;
  }

  bool from_event_exists(const from_event& from_event) const {
    return from_events_.find(from_event) != std::end(from_events_);
  }

  void erase_from_event(const from_event& from_event) {
    from_events_.erase(from_event);
  }

  void erase_from_events_by_device_id(device_id device_id) {
    for (auto it = std::begin(from_events_); it != std::end(from_events_);) {
      if (it->get_device_id() == device_id) {
        it = from_events_.erase(it);
      } else {
        std::advance(it, 1);
      }
    }
  }

  void erase_from_events_by_event(const event_queue::event& event) {
    for (auto it = std::begin(from_events_); it != std::end(from_events_);) {
      if (it->get_event() == event) {
        it = from_events_.erase(it);
      } else {
        std::advance(it, 1);
      }
    }
  }

private:
  std::unordered_set<from_event, from_event_hash> from_events_;
  std::unordered_set<modifier_flag> from_mandatory_modifiers_;
  std::unordered_set<modifier_flag> key_up_posted_from_mandatory_modifiers_;
  absolute_time key_down_time_stamp_;
  bool alone_;
  bool halted_;
  events_at_key_up events_at_key_up_;
  bool key_up_posted_;
};
