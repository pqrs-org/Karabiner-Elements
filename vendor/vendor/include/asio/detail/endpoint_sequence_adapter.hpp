//
// detail/endpoint_sequence_adapter.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_ENDPOINT_SEQUENCE_ADAPTER_HPP
#define ASIO_DETAIL_ENDPOINT_SEQUENCE_ADAPTER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if !defined(ASIO_WINDOWS_RUNTIME)

#include <cstddef>
#include <new>
#include "asio/detail/empty_range.hpp"
#include "asio/detail/socket_types.hpp"
#include "asio/detail/type_traits.hpp"
#include "asio/detail/unwrap_ref.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

// Helper trait to determine whether an endpoint may be used without a copy.
template <typename Reference, typename Endpoint>
struct is_borrowed_endpoint
  : integral_constant<bool,
      is_void<Endpoint>::value
        || (is_lvalue_reference<Reference>::value
          && is_same<remove_cvref_t<Reference>, Endpoint>::value)
        || is_same<decay_t<Reference>, reference_wrapper<Endpoint>>::value
        || is_same<decay_t<Reference>,
          reference_wrapper<const Endpoint>>::value>
{
};

// Helper class to hold the endpoints copied for the duration of a send.
template <typename Endpoint>
class endpoint_storage_base
{
public:
  template <typename Source>
  const Endpoint& push_back(Source&& source)
  {
    Endpoint* e = new (static_cast<void*>(data_ + size_))
      Endpoint(static_cast<Source&&>(source));
    ++size_;
    return *e;
  }

protected:
  explicit endpoint_storage_base(void* data)
    : data_(static_cast<Endpoint*>(data)),
      size_(0)
  {
  }

  void destroy()
  {
    while (size_ > 0)
      data_[--size_].~Endpoint();
  }

private:
  Endpoint* data_;
  std::size_t size_;
};

template <>
class endpoint_storage_base<void>
{
};

template <typename Endpoint, std::size_t N>
class endpoint_storage : public endpoint_storage_base<Endpoint>
{
public:
  endpoint_storage()
    : endpoint_storage_base<Endpoint>(storage_)
  {
  }

  ~endpoint_storage()
  {
    this->destroy();
  }

private:
  endpoint_storage(const endpoint_storage&) = delete;
  endpoint_storage& operator=(const endpoint_storage&) = delete;

  alignas(Endpoint) unsigned char storage_[N * sizeof(Endpoint)];
};

template <std::size_t N>
class endpoint_storage<void, N> : public endpoint_storage_base<void>
{
};

// Helper class to translate a range of endpoints, one per message, into the
// native message representation.
template <typename EndpointSequence, typename Endpoint = void>
class endpoint_sequence_adapter
{
private:
  typedef decltype(*unwrap_ref(declval<EndpointSequence&>()).begin())
    reference;

public:
  // The type of endpoint copied for the duration of a send, if any.
  typedef conditional_t<is_borrowed_endpoint<reference, Endpoint>::value,
      void, Endpoint> storage_type;

  // Set the destination of each message from the corresponding endpoint.
  static void set_destinations(mmsghdr* msgs, std::size_t count,
      EndpointSequence& endpoints, endpoint_storage_base<storage_type>& storage)
  {
    auto it = unwrap_ref(endpoints).begin();
    for (std::size_t i = 0; i < count; ++i, ++it)
    {
      const auto& endpoint = get_destination(it, storage);
      ASIO_MMSGHDR_HDR_PTR(msgs[i]).msg_name = const_cast<void*>(
          static_cast<const void*>(endpoint.data()));
      ASIO_MMSGHDR_HDR_PTR(msgs[i]).msg_namelen =
        static_cast<socklen_t>(endpoint.size());
    }
  }

  // Point each message's source address at the corresponding endpoint.
  static void set_sources(mmsghdr* msgs,
      std::size_t count, EndpointSequence& endpoints)
  {
    auto it = unwrap_ref(endpoints).begin();
    for (std::size_t i = 0; i < count; ++i, ++it)
    {
      ASIO_MMSGHDR_HDR_PTR(msgs[i]).msg_name = unwrap_ref(*it).data();
      ASIO_MMSGHDR_HDR_PTR(msgs[i]).msg_namelen =
        static_cast<socklen_t>(unwrap_ref(*it).capacity());
    }
  }

  // Resize each endpoint to the source address length of the corresponding
  // received message.
  static void update_sources(const mmsghdr* msgs,
      std::size_t count, EndpointSequence& endpoints)
  {
    auto it = unwrap_ref(endpoints).begin();
    for (std::size_t i = 0; i < count; ++i, ++it)
      unwrap_ref(*it).resize(ASIO_MMSGHDR_HDR_PTR(msgs[i]).msg_namelen);
  }

private:
  template <typename Iterator>
  static auto get_destination(const Iterator& it,
      endpoint_storage_base<void>&) -> decltype(unwrap_ref(*it))
  {
    return unwrap_ref(*it);
  }

  template <typename Iterator, typename E>
  static const E& get_destination(const Iterator& it,
      endpoint_storage_base<E>& storage)
  {
    return storage.push_back(unwrap_ref(*it));
  }
};

template <typename Endpoint>
class endpoint_sequence_adapter<empty_range, Endpoint>
{
public:
  typedef void storage_type;

  static void set_destinations(mmsghdr*, std::size_t,
      const empty_range&, endpoint_storage_base<void>&)
  {
  }

  static void set_sources(mmsghdr*, std::size_t, const empty_range&)
  {
  }

  static void update_sources(const mmsghdr*, std::size_t, const empty_range&)
  {
  }
};

template <typename Endpoint>
class endpoint_sequence_adapter<const empty_range, Endpoint>
  : public endpoint_sequence_adapter<empty_range, Endpoint>
{
};

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // !defined(ASIO_WINDOWS_RUNTIME)

#endif // ASIO_DETAIL_ENDPOINT_SEQUENCE_ADAPTER_HPP
