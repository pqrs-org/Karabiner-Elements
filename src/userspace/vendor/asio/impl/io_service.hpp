//
// impl/io_service.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_IO_SERVICE_HPP
#define ASIO_IMPL_IO_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/handler_type_requirements.hpp"
#include "asio/detail/service_registry.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

template <typename Service>
inline Service& use_service(io_service& ios)
{
  // Check that Service meets the necessary type requirements.
  (void)static_cast<io_service::service*>(static_cast<Service*>(0));
  (void)static_cast<const io_service::id*>(&Service::id);

  return ios.service_registry_->template use_service<Service>();
}

template <>
inline detail::io_service_impl& use_service<detail::io_service_impl>(
    io_service& ios)
{
  return ios.impl_;
}

template <typename Service>
inline void add_service(io_service& ios, Service* svc)
{
  // Check that Service meets the necessary type requirements.
  (void)static_cast<io_service::service*>(static_cast<Service*>(0));
  (void)static_cast<const io_service::id*>(&Service::id);

  ios.service_registry_->template add_service<Service>(svc);
}

template <typename Service>
inline bool has_service(io_service& ios)
{
  // Check that Service meets the necessary type requirements.
  (void)static_cast<io_service::service*>(static_cast<Service*>(0));
  (void)static_cast<const io_service::id*>(&Service::id);

  return ios.service_registry_->template has_service<Service>();
}

} // namespace asio

#include "asio/detail/pop_options.hpp"

#if defined(ASIO_HAS_IOCP)
# include "asio/detail/win_iocp_io_service.hpp"
#else
# include "asio/detail/task_io_service.hpp"
#endif

#include "asio/detail/push_options.hpp"

namespace asio {

template <typename CompletionHandler>
inline ASIO_INITFN_RESULT_TYPE(CompletionHandler, void ())
io_service::dispatch(ASIO_MOVE_ARG(CompletionHandler) handler)
{
  // If you get an error on the following line it means that your handler does
  // not meet the documented type requirements for a CompletionHandler.
  ASIO_COMPLETION_HANDLER_CHECK(CompletionHandler, handler) type_check;

  detail::async_result_init<
    CompletionHandler, void ()> init(
      ASIO_MOVE_CAST(CompletionHandler)(handler));

  impl_.dispatch(init.handler);

  return init.result.get();
}

template <typename CompletionHandler>
inline ASIO_INITFN_RESULT_TYPE(CompletionHandler, void ())
io_service::post(ASIO_MOVE_ARG(CompletionHandler) handler)
{
  // If you get an error on the following line it means that your handler does
  // not meet the documented type requirements for a CompletionHandler.
  ASIO_COMPLETION_HANDLER_CHECK(CompletionHandler, handler) type_check;

  detail::async_result_init<
    CompletionHandler, void ()> init(
      ASIO_MOVE_CAST(CompletionHandler)(handler));

  impl_.post(init.handler);

  return init.result.get();
}

template <typename Handler>
#if defined(GENERATING_DOCUMENTATION)
unspecified
#else
inline detail::wrapped_handler<io_service&, Handler>
#endif
io_service::wrap(Handler handler)
{
  return detail::wrapped_handler<io_service&, Handler>(*this, handler);
}

inline io_service::work::work(asio::io_service& io_service)
  : io_service_impl_(io_service.impl_)
{
  io_service_impl_.work_started();
}

inline io_service::work::work(const work& other)
  : io_service_impl_(other.io_service_impl_)
{
  io_service_impl_.work_started();
}

inline io_service::work::~work()
{
  io_service_impl_.work_finished();
}

inline asio::io_service& io_service::work::get_io_service()
{
  return io_service_impl_.get_io_service();
}

inline asio::io_service& io_service::service::get_io_service()
{
  return owner_;
}

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IMPL_IO_SERVICE_HPP
