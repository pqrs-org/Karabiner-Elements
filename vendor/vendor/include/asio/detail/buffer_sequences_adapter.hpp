//
// detail/buffer_sequences_adapter.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_BUFFER_SEQUENCES_ADAPTER_HPP
#define ASIO_DETAIL_BUFFER_SEQUENCES_ADAPTER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if !defined(ASIO_WINDOWS_RUNTIME)

#include <cstddef>
#include <iterator>
#include "asio/buffer.hpp"
#include "asio/detail/buffer_sequence_adapter.hpp"
#include "asio/detail/empty_range.hpp"
#include "asio/detail/endpoint_sequence_adapter.hpp"
#include "asio/detail/flags_sequence_adapter.hpp"
#include "asio/detail/size_sequence_adapter.hpp"
#include "asio/detail/socket_ops.hpp"
#include "asio/detail/socket_types.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/detail/unwrap_ref.hpp"
#include "asio/error_code.hpp"
#include "asio/socket_base.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

class buffer_sequences_adapter_base
{
public:
  // The maximum number of messages to support in a single operation.
  enum { max_messages = ASIO_RECVMMSG_SENDMMSG_MAX_BATCH };

  // The maximum number of buffers to support in a single operation.
  enum
  {
    max_buffers =
      static_cast<int>(max_messages)
        > static_cast<int>(buffer_sequence_adapter_base::max_buffers)
      ? static_cast<int>(max_messages)
      : static_cast<int>(buffer_sequence_adapter_base::max_buffers)
  };
};

static_assert(buffer_sequences_adapter_base::max_buffers <= (1 << 16),
    "ASIO_RECVMMSG_SENDMMSG_MAX_BATCH must not exceed 65536");

// Helper trait to determine whether a range has random access iterators.
template <typename Range, typename = void>
struct is_random_access_range : false_type
{
};

template <typename Range>
struct is_random_access_range<Range,
    enable_if_t<
      is_same<decltype(declval<Range&>().begin()),
        decltype(declval<Range&>().end())>::value
      && is_base_of<std::random_access_iterator_tag,
        typename std::iterator_traits<
          decltype(declval<Range&>().begin())>::iterator_category>::value
    >>
  : true_type
{
};

// Helper trait to determine whether a sequence is known never to end.
template <typename Sequence, typename = void>
struct is_unbounded_sequence : false_type
{
};

#if defined(ASIO_HAS_STD_RANGES)
template <typename Sequence>
struct is_unbounded_sequence<Sequence,
    enable_if_t<
      is_same<decay_t<decltype(unwrap_ref(declval<Sequence&>()).end())>,
        std::unreachable_sentinel_t>::value
    >>
  : true_type
{
};
#endif // defined(ASIO_HAS_STD_RANGES)

// Helper class to step through a sequence in lockstep with the messages.
template <typename Sequence>
class sequence_cursor
{
public:
  static constexpr bool unbounded = is_unbounded_sequence<Sequence>::value;

  static constexpr bool sized = unbounded
    || is_random_access_range<
      remove_reference_t<decltype(unwrap_ref(declval<Sequence&>()))>>::value;

  explicit sequence_cursor(Sequence& sequence)
    : iter_(unwrap_ref(sequence).begin()),
      end_(unwrap_ref(sequence).end())
  {
  }

  std::size_t size() const
  {
    return size(integral_constant<bool, unbounded>());
  }

  bool at_end() const
  {
    return !(iter_ != end_);
  }

  void next()
  {
    ++iter_;
  }

private:
  std::size_t size(true_type) const
  {
    return ~std::size_t(0);
  }

  std::size_t size(false_type) const
  {
    return static_cast<std::size_t>(std::distance(iter_, end_));
  }

  decltype(unwrap_ref(declval<Sequence&>()).begin()) iter_;
  decltype(unwrap_ref(declval<Sequence&>()).end()) end_;
};

template <>
class sequence_cursor<empty_range>
{
public:
  static constexpr bool sized = true;

  explicit sequence_cursor(const empty_range&)
  {
  }

  std::size_t size() const
  {
    return ~std::size_t(0);
  }

  bool at_end() const
  {
    return false;
  }

  void next()
  {
  }
};

template <>
class sequence_cursor<const empty_range>
  : public sequence_cursor<empty_range>
{
public:
  explicit sequence_cursor(const empty_range& r)
    : sequence_cursor<empty_range>(r)
  {
  }
};

// Helper class to limit a batch to the shortest of the per-message sequences.
template <typename EndpointSequence,
    typename SizeSequence, typename FlagsSequence>
class transmit_multiple_limit
{
public:
  // Whether the length of the shortest sequence is known without iterating.
  static constexpr bool sized =
    sequence_cursor<EndpointSequence>::sized
      && sequence_cursor<SizeSequence>::sized
      && sequence_cursor<FlagsSequence>::sized;

  transmit_multiple_limit(EndpointSequence& endpoints,
      SizeSequence& sizes, FlagsSequence& flags)
    : endpoints_(endpoints),
      sizes_(sizes),
      flags_(flags)
  {
  }

  bool at_end() const
  {
    return endpoints_.at_end() || sizes_.at_end() || flags_.at_end();
  }

  void next()
  {
    endpoints_.next();
    sizes_.next();
    flags_.next();
  }

  std::size_t size() const
  {
    std::size_t size = endpoints_.size();
    if (sizes_.size() < size)
      size = sizes_.size();
    if (flags_.size() < size)
      size = flags_.size();
    return size;
  }

private:
  sequence_cursor<EndpointSequence> endpoints_;
  sequence_cursor<SizeSequence> sizes_;
  sequence_cursor<FlagsSequence> flags_;
};

// Helper class to translate a range of buffer sequences into native messages.
template <typename Buffer, typename Element>
class buffer_sequences_adapter
  : public buffer_sequences_adapter_base
{
public:
  // Determine the number of messages to include in a batch, and the number of
  // native buffers needed to hold them.
  template <typename BufferSequences, typename Limit>
  static std::size_t count(BufferSequences& buffer_sequences,
      Limit& limit, std::size_t& buffer_count)
  {
    std::size_t count = 0;
    buffer_count = 0;
    auto&& range = unwrap_ref(buffer_sequences);
    for (auto it = range.begin(); it != range.end()
        && count < max_messages && !limit.at_end();
        ++it, ++count, limit.next())
    {
      auto&& buffer_sequence = *it;
      auto iter = asio::buffer_sequence_begin(buffer_sequence);
      auto end = asio::buffer_sequence_end(buffer_sequence);
      std::size_t n = 0;
      for (; iter != end
          && n < buffer_sequence_adapter_base::max_buffers; ++iter)
        ++n;
      if (count > 0 && buffer_count + n > max_buffers)
        break;
      buffer_count += n;
    }
    return count;
  }

  // Fill in the messages and their native buffers.
  template <typename BufferSequences>
  static void fill(BufferSequences& buffer_sequences, mmsghdr* msgs,
      socket_ops::buf* bufs, std::size_t count)
  {
    std::size_t buffer_pos = 0;
    auto it = unwrap_ref(buffer_sequences).begin();
    for (std::size_t i = 0; i < count; ++i, ++it)
    {
      buffer_sequence_adapter<Buffer, Element> adapter(*it);
      for (std::size_t j = 0; j < adapter.count(); ++j)
        bufs[buffer_pos + j] = adapter.buffers()[j];
      msgs[i] = mmsghdr();
      ASIO_MMSGHDR_HDR_PTR(msgs[i]).msg_iov = &bufs[buffer_pos];
      ASIO_MMSGHDR_HDR_PTR(msgs[i]).msg_iovlen = adapter.count();
      buffer_pos += adapter.count();
    }
  }
};

template <typename Buffer>
class buffer_sequences_adapter<Buffer, asio::mutable_buffer>
  : public buffer_sequences_adapter_base
{
public:
  // Determine the number of messages to include in a batch. Each message uses
  // exactly one native buffer.
  template <typename BufferSequences, typename Limit>
  static std::size_t count(BufferSequences& buffer_sequences,
      Limit& limit, std::size_t& buffer_count)
  {
    auto&& range = unwrap_ref(buffer_sequences);
    buffer_count = do_count(range, limit,
        integral_constant<bool, Limit::sized
          && is_random_access_range<
            remove_reference_t<decltype(range)>>::value>());
    return buffer_count;
  }

  // Fill in the messages and their native buffers.
  template <typename BufferSequences>
  static void fill(BufferSequences& buffer_sequences, mmsghdr* msgs,
      socket_ops::buf* bufs, std::size_t count)
  {
    auto it = unwrap_ref(buffer_sequences).begin();
    for (std::size_t i = 0; i < count; ++i, ++it)
    {
      bufs[i] = buffer_sequence_adapter<Buffer, Buffer>(
          Buffer(*it)).buffers()[0];
      msgs[i] = mmsghdr();
      ASIO_MMSGHDR_HDR_PTR(msgs[i]).msg_iov = &bufs[i];
      ASIO_MMSGHDR_HDR_PTR(msgs[i]).msg_iovlen = 1;
    }
  }

private:
  template <typename Range, typename Limit>
  static std::size_t do_count(Range& range, Limit& limit, true_type)
  {
    std::size_t size = static_cast<std::size_t>(
        std::distance(range.begin(), range.end()));
    if (limit.size() < size)
      size = limit.size();
    return size < std::size_t(max_messages)
      ? size : std::size_t(max_messages);
  }

  template <typename Range, typename Limit>
  static std::size_t do_count(Range& range, Limit& limit, false_type)
  {
    std::size_t count = 0;
    for (auto it = range.begin(); it != range.end()
        && count < max_messages && !limit.at_end(); ++it, limit.next())
      ++count;
    return count;
  }
};

template <typename Buffer>
class buffer_sequences_adapter<Buffer, asio::const_buffer>
  : public buffer_sequences_adapter<Buffer, asio::mutable_buffer>
{
};

template <typename BufferSequences>
using transmit_multiple_element_t = decay_t<
    decltype(*unwrap_ref(declval<BufferSequences&>()).begin())>;

template <std::size_t N, typename Adapter, typename Endpoint,
    typename BufferSequences, typename Function>
inline auto transmit_multiple_invoke_inline(
    BufferSequences& buffer_sequences, std::size_t count, const Function& f)
  -> decltype(f(static_cast<mmsghdr*>(0), std::size_t(0),
        declval<endpoint_storage_base<Endpoint>&>()))
{
  mmsghdr msgs[N];
  socket_ops::buf bufs[N];
  endpoint_storage<Endpoint, N> endpoints;
  Adapter::fill(buffer_sequences, msgs, bufs, count);
  return f(msgs, count, endpoints);
}

template <std::size_t N, typename Adapter, typename Endpoint,
    typename BufferSequences, typename Function>
ASIO_NEVER_INLINE auto transmit_multiple_invoke(
    BufferSequences& buffer_sequences, std::size_t count, const Function& f)
  -> decltype(f(static_cast<mmsghdr*>(0), std::size_t(0),
        declval<endpoint_storage_base<Endpoint>&>()))
{
  return transmit_multiple_invoke_inline<N, Adapter, Endpoint>(
      buffer_sequences, count, f);
}

// Batches up to this size are prepared in the caller's frame.
enum { transmit_multiple_inline_size = 8 };

template <int SizeClass>
struct transmit_multiple_storage_size
{
  static const std::size_t value =
    (std::size_t(1) << SizeClass)
      < std::size_t(buffer_sequences_adapter_base::max_buffers)
    ? (std::size_t(1) << SizeClass)
    : std::size_t(buffer_sequences_adapter_base::max_buffers);
};

template <typename Buffer, typename Endpoint, typename BufferSequences,
    typename Limit, typename Function>
inline auto transmit_multiple_dispatch(BufferSequences& buffer_sequences,
    Limit& limit, const Function& f)
  -> decltype(f(static_cast<mmsghdr*>(0), std::size_t(0),
        declval<endpoint_storage_base<Endpoint>&>()))
{
  typedef buffer_sequences_adapter<Buffer,
      transmit_multiple_element_t<BufferSequences>> adapter_type;

  std::size_t buffer_count = 0;
  std::size_t count = adapter_type::count(
      buffer_sequences, limit, buffer_count);
  std::size_t size = count > buffer_count ? count : buffer_count;

  if (size <= std::size_t(transmit_multiple_inline_size))
  {
    return transmit_multiple_invoke_inline<
        std::size_t(transmit_multiple_inline_size),
        adapter_type, Endpoint>(buffer_sequences, count, f);
  }

  int size_class = 0;
  while ((std::size_t(1) << size_class) < size)
    ++size_class;

  switch (size_class)
  {
#define ASIO_PRIVATE_TRANSMIT_MULTIPLE_CASE(n) \
  case n: \
    return transmit_multiple_invoke<transmit_multiple_storage_size<n>::value, \
        adapter_type, Endpoint>(buffer_sequences, count, f); \
  /**/
  ASIO_PRIVATE_TRANSMIT_MULTIPLE_CASE(0)
  ASIO_PRIVATE_TRANSMIT_MULTIPLE_CASE(1)
  ASIO_PRIVATE_TRANSMIT_MULTIPLE_CASE(2)
  ASIO_PRIVATE_TRANSMIT_MULTIPLE_CASE(3)
  ASIO_PRIVATE_TRANSMIT_MULTIPLE_CASE(4)
  ASIO_PRIVATE_TRANSMIT_MULTIPLE_CASE(5)
  ASIO_PRIVATE_TRANSMIT_MULTIPLE_CASE(6)
  ASIO_PRIVATE_TRANSMIT_MULTIPLE_CASE(7)
  ASIO_PRIVATE_TRANSMIT_MULTIPLE_CASE(8)
  ASIO_PRIVATE_TRANSMIT_MULTIPLE_CASE(9)
  ASIO_PRIVATE_TRANSMIT_MULTIPLE_CASE(10)
  ASIO_PRIVATE_TRANSMIT_MULTIPLE_CASE(11)
  ASIO_PRIVATE_TRANSMIT_MULTIPLE_CASE(12)
  ASIO_PRIVATE_TRANSMIT_MULTIPLE_CASE(13)
  ASIO_PRIVATE_TRANSMIT_MULTIPLE_CASE(14)
  ASIO_PRIVATE_TRANSMIT_MULTIPLE_CASE(15)
#undef ASIO_PRIVATE_TRANSMIT_MULTIPLE_CASE
  default:
    return transmit_multiple_invoke<transmit_multiple_storage_size<16>::value,
        adapter_type, Endpoint>(buffer_sequences, count, f);
  }
}

template <typename Endpoint = void,
    typename BufferSequences, typename EndpointSequence>
inline std::size_t send_multiple(socket_type s,
    socket_ops::state_type state, BufferSequences& buffer_sequences,
    EndpointSequence& endpoints, socket_base::message_flags flags,
    asio::error_code& ec)
{
  const empty_range none = empty_range();
  transmit_multiple_limit<EndpointSequence,
      const empty_range, const empty_range> limit(endpoints, none, none);
  typedef endpoint_sequence_adapter<EndpointSequence, Endpoint> adapter_type;
  typedef typename adapter_type::storage_type storage_type;
  return transmit_multiple_dispatch<const_buffer, storage_type>(
      buffer_sequences, limit, [&](mmsghdr* msgs, std::size_t count,
        endpoint_storage_base<storage_type>& storage) -> std::size_t
      {
        adapter_type::set_destinations(msgs, count, endpoints, storage);
        return socket_ops::sync_sendmmsg(s, state, msgs, count, flags, ec);
      });
}

#if !defined(ASIO_HAS_IOCP)

template <typename Endpoint = void,
    typename BufferSequences, typename EndpointSequence>
inline bool non_blocking_send_multiple(socket_type s,
    BufferSequences& buffer_sequences, EndpointSequence& endpoints,
    socket_base::message_flags flags, asio::error_code& ec,
    std::size_t& messages_transferred)
{
  const empty_range none = empty_range();
  transmit_multiple_limit<EndpointSequence,
      const empty_range, const empty_range> limit(endpoints, none, none);
  typedef endpoint_sequence_adapter<EndpointSequence, Endpoint> adapter_type;
  typedef typename adapter_type::storage_type storage_type;
  return transmit_multiple_dispatch<const_buffer, storage_type>(
      buffer_sequences, limit, [&](mmsghdr* msgs, std::size_t count,
        endpoint_storage_base<storage_type>& storage) -> bool
      {
        adapter_type::set_destinations(msgs, count, endpoints, storage);
        return socket_ops::non_blocking_sendmmsg(s,
            msgs, count, flags, ec, messages_transferred);
      });
}

#endif // !defined(ASIO_HAS_IOCP)

template <typename BufferSequences, typename EndpointSequence,
    typename SizeSequence, typename FlagsSequence>
inline std::size_t receive_multiple(socket_type s,
    socket_ops::state_type state, BufferSequences& buffer_sequences,
    EndpointSequence& endpoints, SizeSequence& sizes,
    FlagsSequence& out_flags, socket_base::message_flags flags,
    asio::error_code& ec)
{
  transmit_multiple_limit<EndpointSequence, SizeSequence, FlagsSequence>
    limit(endpoints, sizes, out_flags);
  return transmit_multiple_dispatch<mutable_buffer, void>(
      buffer_sequences, limit,
      [&](mmsghdr* msgs, std::size_t count,
        endpoint_storage_base<void>&) -> std::size_t
      {
        endpoint_sequence_adapter<EndpointSequence>::set_sources(
            msgs, count, endpoints);
        std::size_t n = socket_ops::sync_recvmmsg(s, state, msgs, count,
            flags, !is_same<remove_cv_t<FlagsSequence>,
              empty_range>::value, ec);
        if (!ec)
        {
          endpoint_sequence_adapter<EndpointSequence>::update_sources(
              msgs, n, endpoints);
          size_sequence_adapter<SizeSequence>::set_sizes(msgs, n, sizes);
          flags_sequence_adapter<FlagsSequence>::set_flags(
              msgs, n, out_flags);
        }
        return n;
      });
}

#if !defined(ASIO_HAS_IOCP)

template <typename BufferSequences, typename EndpointSequence,
    typename SizeSequence, typename FlagsSequence>
inline bool non_blocking_receive_multiple(socket_type s,
    BufferSequences& buffer_sequences, EndpointSequence& endpoints,
    SizeSequence& sizes, FlagsSequence& out_flags,
    socket_base::message_flags flags, asio::error_code& ec,
    std::size_t& messages_transferred)
{
  transmit_multiple_limit<EndpointSequence, SizeSequence, FlagsSequence>
    limit(endpoints, sizes, out_flags);
  return transmit_multiple_dispatch<mutable_buffer, void>(
      buffer_sequences, limit,
      [&](mmsghdr* msgs, std::size_t count,
        endpoint_storage_base<void>&) -> bool
      {
        endpoint_sequence_adapter<EndpointSequence>::set_sources(
            msgs, count, endpoints);
        if (!socket_ops::non_blocking_recvmmsg(s, msgs, count, flags,
              !is_same<remove_cv_t<FlagsSequence>, empty_range>::value,
              ec, messages_transferred))
          return false;
        if (!ec)
        {
          endpoint_sequence_adapter<EndpointSequence>::update_sources(
              msgs, messages_transferred, endpoints);
          size_sequence_adapter<SizeSequence>::set_sizes(
              msgs, messages_transferred, sizes);
          flags_sequence_adapter<FlagsSequence>::set_flags(
              msgs, messages_transferred, out_flags);
        }
        return true;
      });
}

#endif // !defined(ASIO_HAS_IOCP)

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // !defined(ASIO_WINDOWS_RUNTIME)

#endif // ASIO_DETAIL_BUFFER_SEQUENCES_ADAPTER_HPP
