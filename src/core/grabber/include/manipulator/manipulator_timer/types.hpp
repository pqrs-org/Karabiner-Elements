#pragma once

struct timer_id : type_safe::strong_typedef<timer_id, uint64_t>,
                  type_safe::strong_typedef_op::equality_comparison<timer_id>,
                  type_safe::strong_typedef_op::relational_comparison<timer_id>,
                  type_safe::strong_typedef_op::integer_arithmetic<timer_id> {
  using strong_typedef::strong_typedef;
};

class entry final {
public:
  entry(absolute_time when) : when_(when) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static timer_id id(0);
    timer_id_ = ++id;
  }

  timer_id get_timer_id(void) const {
    return timer_id_;
  }

  absolute_time get_when(void) const {
    return when_;
  }

  bool compare(const entry& other) {
    if (when_ != other.when_) {
      return when_ < other.when_;
    } else {
      return timer_id_ < other.timer_id_;
    }
  }

private:
  timer_id timer_id_;
  absolute_time when_;
};
