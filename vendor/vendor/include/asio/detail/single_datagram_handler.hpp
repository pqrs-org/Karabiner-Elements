//
// detail/single_datagram_handler.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_SINGLE_DATAGRAM_HANDLER_HPP
#define ASIO_DETAIL_SINGLE_DATAGRAM_HANDLER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include <cstddef>
#include "asio/associator.hpp"
#include "asio/detail/empty_range.hpp"
#include "asio/detail/unwrap_ref.hpp"
#include "asio/error_code.hpp"
#include "asio/socket_base.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

template <typename Sequence, typename Value>
inline void write_first_element(Sequence& sequence, const Value& value)
{
  unwrap_ref(*unwrap_ref(sequence).begin()) = value;
}

template <typename Value>
inline void write_first_element(empty_range&, const Value&)
{
}

// Helper class to adapt a single-datagram completion to a multiple-datagram
// handler.
template <typename Handler, typename SizeSequence, typename FlagsSequence>
class single_datagram_handler
{
public:
  template <typename H, typename S, typename F>
  single_datagram_handler(H&& handler, S&& sizes, F&& out_flags, bool empty)
    : handler_(static_cast<H&&>(handler)),
      sizes_(static_cast<S&&>(sizes)),
      out_flags_(static_cast<F&&>(out_flags)),
      empty_(empty)
  {
  }

  void operator()(const asio::error_code& ec,
      std::size_t bytes_transferred)
  {
    std::size_t messages = 0;
    if (!empty_ && !ec)
    {
      write_first_element(sizes_, bytes_transferred);
      write_first_element(out_flags_, socket_base::message_flags(0));
      messages = 1;
    }
    static_cast<Handler&&>(handler_)(ec, messages);
  }

//private:
  Handler handler_;
  SizeSequence sizes_;
  FlagsSequence out_flags_;
  bool empty_;
};

} // namespace detail

template <template <typename, typename> class Associator,
    typename Handler, typename SizeSequence, typename FlagsSequence,
    typename DefaultCandidate>
struct associator<Associator,
    detail::single_datagram_handler<Handler, SizeSequence, FlagsSequence>,
    DefaultCandidate>
  : Associator<Handler, DefaultCandidate>
{
  static typename Associator<Handler, DefaultCandidate>::type get(
      const detail::single_datagram_handler<
        Handler, SizeSequence, FlagsSequence>& h) noexcept
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_);
  }

  static auto get(
      const detail::single_datagram_handler<
        Handler, SizeSequence, FlagsSequence>& h,
      const DefaultCandidate& c) noexcept
    -> decltype(Associator<Handler, DefaultCandidate>::get(h.handler_, c))
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_, c);
  }
};

ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_SINGLE_DATAGRAM_HANDLER_HPP
