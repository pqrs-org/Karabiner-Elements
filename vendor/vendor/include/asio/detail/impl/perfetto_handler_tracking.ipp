//
// detail/impl/perfetto_handler_tracking.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_PERFETTO_HANDLER_TRACKING_IPP
#define ASIO_DETAIL_IMPL_PERFETTO_HANDLER_TRACKING_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_PERFETTO_HANDLER_TRACKING)

#include <cstddef>
#include "asio/detail/perfetto_handler_tracking.hpp"
#include "asio/detail/perfetto_writer.hpp"
#include "asio/detail/perfetto_protocol.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {

void perfetto_handler_tracking::init()
{
  perfetto_writer::init();
}

perfetto_handler_tracking::location::location(
    const char* file, int line, const char* func)
  : file_(file),
    line_(line),
    func_(func),
    emitted_(false)
{
  using namespace perfetto_protocol;

  if (!file_)
    return;

  perfetto_writer::thread_state* ts = perfetto_writer::this_thread();
  if (!ts)
    return;

  char loc_buf[256];
  location_label(loc_buf, sizeof(loc_buf), file_, line_);

  annotation a = annotation::make_string("location", loc_buf);
  track_event_packet packet = track_event_packet();
  packet.event_type = track_event_type::slice_begin;
  packet.track_uuid = ts->thread_track_uuid();
  packet.name = func_;
  packet.annotations = &a;
  packet.annotation_count = 1;
  packet.hdr.timestamp = ts->timestamp();
  ts->emit(packet);
  emitted_ = true;
}

perfetto_handler_tracking::location::~location()
{
  using namespace perfetto_protocol;

  if (!emitted_)
    return;

  perfetto_writer::thread_state* ts = perfetto_writer::this_thread();
  if (!ts)
    return;

  uint64_t now = ts->timestamp();
  track_event_packet packet = track_event_packet();
  packet.hdr.timestamp = now;
  packet.event_type = track_event_type::slice_end;
  packet.track_uuid = ts->thread_track_uuid();
  ts->emit(packet);
}

void perfetto_handler_tracking::creation(
    perfetto_handler_tracking::tracked_handler& h,
    const char* object_type, void* object,
    uintmax_t /*native_handle*/, const char* op_name)
{
  using namespace perfetto_protocol;

  perfetto_writer::thread_state* ts = perfetto_writer::this_thread();
  if (!ts)
    return;

  h.id_ = perfetto_writer::next_handler_id();
  h.object_ = object;
  h.object_type_ = object_type;
  h.op_name_ = op_name;

  char name_buf[256];
  uint32_t name_len = op_label(
      name_buf, sizeof(name_buf), object_type, object, op_name);

  annotation a = annotation::make_uint("handler_id", h.id_);

  // Creation starts two flows: one to the invocation, one via reactor events.
  uint64_t out_flows[2] =
  {
    perfetto_writer::salt() ^ h.id_,
    perfetto_writer::salt() ^ h.id_ ^ reactor_flow_bit
  };
  track_event_packet packet = track_event_packet();
  packet.event_type = track_event_type::instant;
  packet.track_uuid = ts->thread_track_uuid();
  packet.name = name_buf;
  packet.name_len = name_len;
  packet.flow_ids = out_flows;
  packet.flow_id_count = 2;
  packet.annotations = &a;
  packet.annotation_count = 1;
  packet.hdr.timestamp = ts->timestamp();
  ts->emit(packet);
}

void perfetto_handler_tracking::creation(execution_context&,
    perfetto_handler_tracking::tracked_handler& h,
    const char* object_type, void* object,
    uintmax_t native_handle, const char* op_name)
{
  creation(h, object_type, object, native_handle, op_name);
}

perfetto_handler_tracking::completion::completion(
    const perfetto_handler_tracking::tracked_handler& h)
  : id_(h.id_),
    object_(h.object_),
    object_type_(h.object_type_),
    op_name_(h.op_name_),
    reactor_linked_(h.reactor_linked_),
    invoked_(false)
{
}

perfetto_handler_tracking::completion::~completion()
{
  if (invoked_ || id_)
    completion_end(id_, invoked_, reactor_linked_);
}

void perfetto_handler_tracking::completion::invocation_begin()
{
  do_invocation_begin(id_, reactor_linked_,
      object_type_, object_, op_name_, 0, 0);
  invoked_ = true;
}

void perfetto_handler_tracking::completion::invocation_begin(
    const asio::error_code& ec)
{
  using namespace perfetto_protocol;

  annotation a[2];
  a[0] = annotation::make_string("ec.category", ec.category().name());
  a[1] = annotation::make_int("ec.value", ec.value());
  do_invocation_begin(id_, reactor_linked_,
      object_type_, object_, op_name_, a, 2);
  invoked_ = true;
}

void perfetto_handler_tracking::completion::invocation_begin(
    const asio::error_code& ec, std::size_t bytes_transferred)
{
  using namespace perfetto_protocol;

  annotation a[3];
  a[0] = annotation::make_string("ec.category", ec.category().name());
  a[1] = annotation::make_int("ec.value", ec.value());
  a[2] = annotation::make_uint("bytes_transferred", bytes_transferred);
  do_invocation_begin(id_, reactor_linked_,
      object_type_, object_, op_name_, a, 3);
  invoked_ = true;
}

void perfetto_handler_tracking::completion::invocation_begin(
    const asio::error_code& ec, int signal_number)
{
  using namespace perfetto_protocol;

  annotation a[3];
  a[0] = annotation::make_string("ec.category", ec.category().name());
  a[1] = annotation::make_int("ec.value", ec.value());
  a[2] = annotation::make_int("signal_number", signal_number);
  do_invocation_begin(id_, reactor_linked_,
      object_type_, object_, op_name_, a, 3);
  invoked_ = true;
}

void perfetto_handler_tracking::completion::invocation_begin(
    const asio::error_code& ec, const char* arg)
{
  using namespace perfetto_protocol;

  annotation a[3];
  a[0] = annotation::make_string("ec.category", ec.category().name());
  a[1] = annotation::make_int("ec.value", ec.value());
  a[2] = annotation::make_string("arg", arg);
  do_invocation_begin(id_, reactor_linked_,
      object_type_, object_, op_name_, a, 3);
  invoked_ = true;
}

void perfetto_handler_tracking::completion::invocation_end()
{
  if (invoked_)
  {
    do_invocation_end(id_);
    invoked_ = false;
  }
  id_ = 0;
}

void perfetto_handler_tracking::completion::do_invocation_begin(
    uint64_t id, bool reactor_linked, const char* object_type,
    const void* object, const char* op_name,
    const perfetto_protocol::annotation* annotations,
    std::size_t annotation_count)
{
  using namespace perfetto_protocol;

  perfetto_writer::thread_state* ts = perfetto_writer::this_thread();
  if (!ts)
    return;

  // Lock for the whole invocation, even when untracked. This keeps lock and
  // unlock balanced. An untracked handler (no id) opens no slice.
  ts->lock();
  if (id == 0)
    return;

  char name_buf[256];
  uint32_t name_len = completion_label(name_buf,
      sizeof(name_buf), object_type, object, op_name);

  uint64_t terminating_flows[2] =
  {
    perfetto_writer::salt() ^ id,
    perfetto_writer::salt() ^ id ^ reactor_flow_bit
  };
  track_event_packet packet = track_event_packet();
  packet.event_type = track_event_type::slice_begin;
  packet.track_uuid = ts->thread_track_uuid();
  packet.name = name_buf;
  packet.name_len = name_len;
  packet.terminating_flow_ids = terminating_flows;
  packet.terminating_flow_id_count = reactor_linked ? 2 : 1;
  packet.annotations = annotations;
  packet.annotation_count = annotation_count;
  packet.hdr.timestamp = ts->timestamp();
  ts->emit(packet);
}

void perfetto_handler_tracking::completion::do_invocation_end(uint64_t id)
{
  using namespace perfetto_protocol;

  perfetto_writer::thread_state* ts = perfetto_writer::this_thread();
  if (!ts)
    return;

  // Close the slice only for a tracked handler, matching do_invocation_begin,
  // but always release the lock it took.
  if (id != 0)
  {
    uint64_t now = ts->timestamp();
    track_event_packet packet = track_event_packet();
    packet.hdr.timestamp = now;
    packet.event_type = track_event_type::slice_end;
    packet.track_uuid = ts->thread_track_uuid();
    ts->emit(packet);
  }

  ts->unlock();
}

void perfetto_handler_tracking::completion::completion_end(
    uint64_t id, bool invoked, bool reactor_linked)
{
  using namespace perfetto_protocol;

  perfetto_writer::thread_state* ts = perfetto_writer::this_thread();
  if (!ts)
    return;

  if (invoked)
  {
    if (id != 0)
    {
      uint64_t now = ts->timestamp();
      annotation a = annotation::make_bool("exception", true);
      track_event_packet packet = track_event_packet();
      packet.hdr.timestamp = now;
      packet.event_type = track_event_type::slice_end;
      packet.track_uuid = ts->thread_track_uuid();
      packet.annotations = &a;
      packet.annotation_count = 1;
      ts->emit(packet);
    }
    ts->unlock();
  }
  else if (id != 0)
  {
    annotation a = annotation::make_bool("abandoned", true);
    uint64_t terminating_flows[2] =
    {
      perfetto_writer::salt() ^ id,
      perfetto_writer::salt() ^ id ^ reactor_flow_bit
    };
    track_event_packet packet = track_event_packet();
    packet.event_type = track_event_type::instant;
    packet.track_uuid = ts->thread_track_uuid();
    packet.name = "abandoned";
    packet.terminating_flow_ids = terminating_flows;
    packet.terminating_flow_id_count = reactor_linked ? 2 : 1;
    packet.annotations = &a;
    packet.annotation_count = 1;
    packet.hdr.timestamp = ts->timestamp();
    ts->emit(packet);
  }
}

void perfetto_handler_tracking::operation(execution_context&,
    const char* object_type, void* object,
    uintmax_t /*native_handle*/, const char* op_name)
{
  using namespace perfetto_protocol;

  perfetto_writer::thread_state* ts = perfetto_writer::this_thread();
  if (!ts)
    return;

  char name_buf[256];
  uint32_t name_len = op_label(name_buf,
      sizeof(name_buf), object_type, object, op_name);

  track_event_packet packet = track_event_packet();
  packet.event_type = track_event_type::instant;
  packet.track_uuid = ts->thread_track_uuid();
  packet.name = name_buf;
  packet.name_len = name_len;
  packet.hdr.timestamp = ts->timestamp();
  ts->emit(packet);
}

void perfetto_handler_tracking::reactor_registration(
    execution_context& context, uintmax_t native_handle, uintmax_t registration)
{
  using namespace perfetto_protocol;

  perfetto_writer::thread_state* ts = perfetto_writer::this_thread();
  if (!ts)
    return;

  annotation a[2];
  a[0] = annotation::make_uint("native_handle", native_handle);
  a[1] = annotation::make_uint("registration", registration);

  char name_buf[256];
  reactor_event_label(name_buf, sizeof(name_buf),
      static_cast<const void*>(&context), "registration");

  track_event_packet packet = track_event_packet();
  packet.event_type = track_event_type::instant;
  packet.track_uuid = ts->thread_track_uuid();
  packet.name = name_buf;
  packet.annotations = a;
  packet.annotation_count = 2;
  packet.hdr.timestamp = ts->timestamp();
  ts->emit(packet);
}

void perfetto_handler_tracking::reactor_deregistration(
    execution_context& context, uintmax_t native_handle, uintmax_t registration)
{
  using namespace perfetto_protocol;

  perfetto_writer::thread_state* ts = perfetto_writer::this_thread();
  if (!ts)
    return;

  annotation a[2];
  a[0] = annotation::make_uint("native_handle", native_handle);
  a[1] = annotation::make_uint("registration", registration);

  char name_buf[256];
  reactor_event_label(name_buf, sizeof(name_buf),
      static_cast<const void*>(&context), "deregistration");

  track_event_packet packet = track_event_packet();
  packet.event_type = track_event_type::instant;
  packet.track_uuid = ts->thread_track_uuid();
  packet.name = name_buf;
  packet.annotations = a;
  packet.annotation_count = 2;
  packet.hdr.timestamp = ts->timestamp();
  ts->emit(packet);
}

void perfetto_handler_tracking::reactor_events(
    execution_context& context, uintmax_t registration, unsigned events)
{
  using namespace perfetto_protocol;

  perfetto_writer::thread_state* ts = perfetto_writer::this_thread();
  if (!ts)
    return;

  annotation a[4];
  a[0] = annotation::make_uint("registration", registration);
  a[1] = annotation::make_bool("read",
      (events & ASIO_HANDLER_REACTOR_READ_EVENT) != 0);
  a[2] = annotation::make_bool("write",
      (events & ASIO_HANDLER_REACTOR_WRITE_EVENT) != 0);
  a[3] = annotation::make_bool("error",
      (events & ASIO_HANDLER_REACTOR_ERROR_EVENT) != 0);

  char name_buf[256];
  reactor_event_label(name_buf, sizeof(name_buf),
      static_cast<const void*>(&context), "events");

  track_event_packet packet = track_event_packet();
  packet.event_type = track_event_type::instant;
  packet.track_uuid = ts->thread_track_uuid();
  packet.name = name_buf;
  packet.annotations = a;
  packet.annotation_count = 4;
  packet.hdr.timestamp = ts->timestamp();
  ts->emit(packet);
}

void perfetto_handler_tracking::reactor_operation(tracked_handler& h,
    const char* op_name, const asio::error_code& ec)
{
  using namespace perfetto_protocol;

  perfetto_writer::thread_state* ts = perfetto_writer::this_thread();
  if (!ts)
    return;

  // A reactor event was emitted for this async operation. The completion
  // handler should terminate the reactor flow to complete the creation ->
  // event -> completion chain.
  h.reactor_linked_ = true;

  annotation a[2];
  a[0] = annotation::make_string("ec.category", ec.category().name());
  a[1] = annotation::make_int("ec.value", ec.value());

  char name_buf[256];
  reactor_label(name_buf, sizeof(name_buf),
      h.object_type_, h.object_, h.op_name_, op_name);

  track_event_packet packet = track_event_packet();
  packet.event_type = track_event_type::instant;
  packet.track_uuid = ts->thread_track_uuid();
  packet.name = name_buf;
  uint64_t reactor_flow[1] =
    { perfetto_writer::salt() ^ h.id_ ^ reactor_flow_bit };
  packet.flow_ids = reactor_flow;
  packet.flow_id_count = 1;
  packet.annotations = a;
  packet.annotation_count = 2;
  packet.hdr.timestamp = ts->timestamp();
  ts->emit(packet);
}

void perfetto_handler_tracking::reactor_operation(tracked_handler& h,
    const char* op_name, const asio::error_code& ec,
    std::size_t bytes_transferred)
{
  using namespace perfetto_protocol;

  perfetto_writer::thread_state* ts = perfetto_writer::this_thread();
  if (!ts)
    return;

  // A reactor event was emitted for this async operation. The completion
  // handler should terminate the reactor flow to complete the initiation ->
  // event -> completion chain.
  h.reactor_linked_ = true;

  annotation a[3];
  a[0] = annotation::make_string("ec.category", ec.category().name());
  a[1] = annotation::make_int("ec.value", ec.value());
  a[2] = annotation::make_uint("bytes_transferred", bytes_transferred);

  char name_buf[256];
  reactor_label(name_buf, sizeof(name_buf),
      h.object_type_, h.object_, h.op_name_, op_name);

  track_event_packet packet = track_event_packet();
  packet.event_type = track_event_type::instant;
  packet.track_uuid = ts->thread_track_uuid();
  packet.name = name_buf;
  uint64_t reactor_flow[1] =
    { perfetto_writer::salt() ^ h.id_ ^ reactor_flow_bit };
  packet.flow_ids = reactor_flow;
  packet.flow_id_count = 1;
  packet.annotations = a;
  packet.annotation_count = 3;
  packet.hdr.timestamp = ts->timestamp();
  ts->emit(packet);
}

perfetto_handler_tracking::scheduler_operation::scheduler_operation(
    const void* scheduler, const char* name)
  : active_(false)
{
  using namespace perfetto_protocol;

  perfetto_writer::thread_state* ts = perfetto_writer::this_thread();
  if (!ts)
    return;

  char name_buf[256];
  op_label(name_buf, sizeof(name_buf), "scheduler", scheduler, name);

  track_event_packet packet = track_event_packet();
  packet.event_type = track_event_type::slice_begin;
  packet.track_uuid = ts->thread_track_uuid();
  packet.name = name_buf;
  packet.hdr.timestamp = ts->timestamp();
  ts->emit(packet);
  active_ = true;
}

perfetto_handler_tracking::scheduler_operation::~scheduler_operation()
{
  using namespace perfetto_protocol;

  if (!active_)
    return;

  perfetto_writer::thread_state* ts = perfetto_writer::this_thread();
  if (!ts)
    return;

  uint64_t now = ts->timestamp();
  track_event_packet packet = track_event_packet();
  packet.hdr.timestamp = now;
  packet.event_type = track_event_type::slice_end;
  packet.track_uuid = ts->thread_track_uuid();
  ts->emit(packet);
}

char* perfetto_handler_tracking::append(char* p, char* end, const char* s)
{
  if (s)
    while (*s != '\0' && p != end)
      *p++ = *s++;
  return p;
}

char* perfetto_handler_tracking::append_ptr(char* p, char* end, const void* ptr)
{
  static const char digits[] = "0123456789abcdef";
  uintptr_t v = reinterpret_cast<uintptr_t>(ptr);
  p = append(p, end, "0x");
  int shift = static_cast<int>(sizeof(uintptr_t) * 8) - 4;
  while (shift > 0 && ((v >> shift) & 0xF) == 0)
    shift -= 4;
  for (; shift >= 0 && p != end; shift -= 4)
    *p++ = digits[(v >> shift) & 0xF];
  return p;
}

char* perfetto_handler_tracking::append_uint(char* p, char* end, unsigned value)
{
  char tmp[10];
  int n = 0;
  do
  {
    tmp[n++] = static_cast<char>('0' + value % 10);
    value /= 10;
  }
  while (value != 0 && n < 10);
  while (n > 0 && p != end)
    *p++ = tmp[--n];
  return p;
}

uint32_t perfetto_handler_tracking::object_label(char* buf, std::size_t n,
    const char* object_type, const void* object, const char* op_name,
    const char* suffix)
{
  if (n == 0)
    return 0;
  char* p = buf;
  char* end = buf + (n - 1);
  p = append(p, end, object_type);
  if (p != end)
    *p++ = '@';
  p = append_ptr(p, end, object);
  if (p != end)
    *p++ = '.';
  p = append(p, end, op_name);
  if (suffix)
  {
    if (p != end)
      *p++ = ' ';
    p = append(p, end, suffix);
  }
  *p = '\0';
  return static_cast<uint32_t>(p - buf);
}

uint32_t perfetto_handler_tracking::op_label(char* buf, std::size_t n,
    const char* object_type, const void* object, const char* op_name)
{
  return object_label(buf, n, object_type, object, op_name, 0);
}

uint32_t perfetto_handler_tracking::completion_label(char* buf, std::size_t n,
    const char* object_type, const void* object, const char* op_name)
{
  return object_label(buf, n, object_type, object, op_name, "completion");
}

void perfetto_handler_tracking::reactor_label(char* buf, std::size_t n,
    const char* object_type, const void* object, const char* op_name,
    const char* reactor_op)
{
  object_label(buf, n, object_type, object, op_name, reactor_op);
}

void perfetto_handler_tracking::reactor_event_label(char* buf,
    std::size_t n, const void* reactor, const char* suffix)
{
  if (n == 0)
    return;
  char* p = buf;
  char* end = buf + (n - 1);
  p = append(p, end, "reactor@");
  p = append_ptr(p, end, reactor);
  if (p != end)
    *p++ = ' ';
  p = append(p, end, suffix);
  *p = '\0';
}

void perfetto_handler_tracking::location_label(char* buf,
    std::size_t n, const char* file, int line)
{
  if (n == 0)
    return;
  char* p = buf;
  char* end = buf + (n - 1);
  p = append(p, end, file);
  if (p != end)
    *p++ = ':';
  p = append_uint(p, end, line < 0 ? 0u : static_cast<unsigned>(line));
  *p = '\0';
}

} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_PERFETTO_HANDLER_TRACKING)

#endif // ASIO_DETAIL_IMPL_PERFETTO_HANDLER_TRACKING_IPP
