#pragma once

#include "boost_defs.hpp"

#include <boost/signals2.hpp>

namespace krbn {
class boost_utility final {
public:
  struct signals2_combiner_call_while_true final {
    typedef bool result_type;

    template <typename input_iterator>
    result_type operator()(input_iterator first_observer,
                           input_iterator last_observer) const {
      result_type value = true;
      for (;
           first_observer != last_observer && value;
           std::advance(first_observer, 1)) {
        value = *first_observer;
      }
      return value;
    }
  };
};
} // namespace krbn
