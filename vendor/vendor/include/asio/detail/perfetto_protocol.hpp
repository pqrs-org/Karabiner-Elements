//
// detail/perfetto_protocol.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_PERFETTO_PROTOCOL_HPP
#define ASIO_DETAIL_PERFETTO_PROTOCOL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_PERFETTO_HANDLER_TRACKING)

#include <cstddef>
#include <cstring>
#include "asio/detail/assert.hpp"
#include "asio/detail/cstdint.hpp"
#include "asio/detail/type_traits.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {
namespace perfetto_protocol {

enum class wire_type : uint32_t
{
  varint = 0,
  fixed64 = 1,
  length_delimited = 2
};

enum class trace_field : uint32_t
{
  packet = 1
};

enum class trace_packet_field : uint32_t
{
  clock_snapshot = 6,
  timestamp = 8,
  trusted_packet_sequence_id = 10,
  track_event = 11,
  sequence_flags = 13,
  timestamp_clock_id = 58,
  track_descriptor = 60
};

enum class clock_snapshot_field : uint32_t
{
  clocks = 1,
  primary_trace_clock = 2
};

enum class clock_field : uint32_t
{
  clock_id = 1,
  timestamp = 2
};

enum class track_event_field : uint32_t
{
  debug_annotations = 4,
  type = 9,
  track_uuid = 11,
  name = 23,
  flow_ids = 47,
  terminating_flow_ids = 48
};

enum class debug_annotation_field : uint32_t
{
  bool_value = 2,
  uint_value = 3,
  int_value = 4,
  string_value = 6,
  pointer_value = 7,
  name = 10
};

enum class track_descriptor_field : uint32_t
{
  uuid = 1,
  name = 2,
  process = 3,
  thread = 4,
  parent_uuid = 5
};

enum class process_descriptor_field : uint32_t
{
  pid = 1,
  cmdline = 2,
  process_name = 6
};

enum class thread_descriptor_field : uint32_t
{
  pid = 1,
  tid = 2,
  thread_name = 5
};

enum class track_event_type
{
  slice_begin = 1,
  slice_end = 2,
  instant = 3,
  counter = 4
};

enum class sequence_flags : uint32_t
{
  incremental_state_cleared = 1
};

enum class builtin_clock : uint32_t
{
  realtime = 1,
  monotonic = 3,
  boottime = 6
};

struct limits
{
  enum
  {
    // Longest string kept in a packet, in bytes (longer values are truncated).
    max_string = 128,

    // Upper bound on the size of a single encoded packet, in bytes.
    max_packet = 4096,

    // Capacity of each pooled encode buffer (must exceed max_packet).
    buffer_bytes = 32768
  };
};

struct annotation
{
  enum value_kind
  {
    kind_none,
    kind_int,
    kind_uint,
    kind_pointer,
    kind_bool,
    kind_string
  };

  const char* name;
  value_kind kind;
  int64_t int_value;
  uint64_t uint_value;
  const void* pointer_value;
  bool bool_value;
  const char* string_value;

  static annotation make_pointer(const char* name, const void* value)
  {
    annotation a = annotation();
    a.name = name;
    a.kind = kind_pointer;
    a.pointer_value = value;
    return a;
  }

  template <typename Value,
      enable_if_t<is_enum<Value>::value || is_integral<Value>::value, int> = 0>
  static annotation make_uint(const char* name, Value value)
  {
    typedef typename conditional_t<is_enum<Value>::value,
        underlying_type<Value>, type_identity<Value> >::type integer_type;
    annotation a = annotation();
    a.name = name;
    a.kind = kind_uint;
    a.uint_value = static_cast<uint64_t>(static_cast<integer_type>(value));
    return a;
  }

  template <typename Value,
      enable_if_t<is_enum<Value>::value || is_integral<Value>::value, int> = 0>
  static annotation make_int(const char* name, Value value)
  {
    typedef typename conditional_t<is_enum<Value>::value,
        underlying_type<Value>, type_identity<Value> >::type integer_type;
    annotation a = annotation();
    a.name = name;
    a.kind = kind_int;
    a.int_value = static_cast<int64_t>(static_cast<integer_type>(value));
    return a;
  }

  static annotation make_bool(const char* name, bool value)
  {
    annotation a = annotation();
    a.name = name;
    a.kind = kind_bool;
    a.bool_value = value;
    return a;
  }

  static annotation make_string(const char* name, const char* value)
  {
    annotation a = annotation();
    a.name = name;
    a.kind = kind_string;
    a.string_value = value;
    return a;
  }
};

class encoder
{
public:
  encoder(unsigned char* buf, uint32_t start)
    : buf_(buf),
      pos_(start)
  {
  }

  uint32_t size() const
  {
    return pos_;
  }

  void put(unsigned char b)
  {
    buf_[pos_++] = b;
  }

  void varint(uint64_t value)
  {
    while (value >= 0x80)
    {
      put(static_cast<unsigned char>(value) | 0x80);
      value >>= 7;
    }
    put(static_cast<unsigned char>(value));
  }

  template <typename Field>
  void tag(Field field_number, wire_type wt)
  {
    varint((static_cast<uint64_t>(field_number) << 3)
        | static_cast<uint64_t>(wt));
  }

  void fixed64(uint64_t value)
  {
    unsigned char b[8];
    for (int i = 0; i < 8; ++i)
    {
      b[i] = static_cast<unsigned char>(value & 0xFF);
      value >>= 8;
    }
    std::memcpy(buf_ + pos_, b, 8);
    pos_ += 8;
  }

  template <typename Field, typename Value,
      enable_if_t<is_enum<Value>::value || is_integral<Value>::value, int> = 0>
  void varint_field(Field field_number, Value value)
  {
    typedef typename conditional_t<is_enum<Value>::value,
        underlying_type<Value>, type_identity<Value> >::type integer_type;
    tag(field_number, wire_type::varint);
    varint(static_cast<uint64_t>(static_cast<integer_type>(value)));
  }

  template <typename Field>
  void fixed64_field(Field field_number, uint64_t value)
  {
    tag(field_number, wire_type::fixed64);
    fixed64(value);
  }

  template <typename Field>
  void bytes_field(Field field_number, const void* data, uint32_t size)
  {
    tag(field_number, wire_type::length_delimited);
    varint(static_cast<uint64_t>(size));
    if (size != 0)
    {
      std::memcpy(buf_ + pos_, data, size);
      pos_ += size;
    }
  }

  template <typename Field>
  void string_field(Field field_number, const char* value, uint32_t len)
  {
    uint32_t size = value
      ? (len ? len : static_cast<uint32_t>(std::strlen(value))) : 0;
    if (size > limits::max_string)
      size = limits::max_string;
    bytes_field(field_number, value, size);
  }

  template <typename Field>
  void string_field(Field field_number, const char* value)
  {
    string_field(field_number, value, 0);
  }

  template <typename Field>
  uint32_t begin_submessage(Field field_number)
  {
    tag(field_number, wire_type::length_delimited);
    uint32_t offset = pos_;
    pos_ += 4; // Reserved for length prefix.
    return offset;
  }

  void end_submessage(uint32_t length_offset)
  {
    uint32_t len = pos_ - (length_offset + 4);
    buf_[length_offset + 0] =
      static_cast<unsigned char>((len & 0x7F) | 0x80);
    buf_[length_offset + 1] =
      static_cast<unsigned char>(((len >> 7) & 0x7F) | 0x80);
    buf_[length_offset + 2] =
      static_cast<unsigned char>(((len >> 14) & 0x7F) | 0x80);
    buf_[length_offset + 3] =
      static_cast<unsigned char>((len >> 21) & 0x7F);
  }

private:
  unsigned char* buf_;
  uint32_t pos_;
};

struct packet_header
{
  uint32_t sequence_id;
  uint64_t timestamp;
  bool incremental_state_cleared;
};

inline void encode(encoder& enc, const packet_header& hdr)
{
  enc.varint_field(trace_packet_field::timestamp, hdr.timestamp);
  enc.varint_field(trace_packet_field::timestamp_clock_id,
      builtin_clock::monotonic);
  enc.varint_field(trace_packet_field::trusted_packet_sequence_id,
      hdr.sequence_id);
  if (hdr.incremental_state_cleared)
  {
    enc.varint_field(trace_packet_field::sequence_flags,
        sequence_flags::incremental_state_cleared);
  }
}

struct track_event_packet
{
  packet_header hdr;
  track_event_type event_type;
  uint64_t track_uuid;
  const char* name;
  uint32_t name_len;
  const uint64_t* flow_ids;
  std::size_t flow_id_count;
  const uint64_t* terminating_flow_ids;
  std::size_t terminating_flow_id_count;
  const annotation* annotations;
  std::size_t annotation_count;
};

inline void encode(encoder& enc, const track_event_packet& packet)
{
  uint32_t packet_len = enc.begin_submessage(trace_field::packet);
  encode(enc, packet.hdr);

  uint32_t te_len = enc.begin_submessage(trace_packet_field::track_event);

  enc.varint_field(track_event_field::type, packet.event_type);
  enc.varint_field(track_event_field::track_uuid, packet.track_uuid);

  if (packet.name)
    enc.string_field(track_event_field::name, packet.name, packet.name_len);

  for (std::size_t i = 0; i < packet.flow_id_count; ++i)
    enc.fixed64_field(track_event_field::flow_ids, packet.flow_ids[i]);
  for (std::size_t i = 0; i < packet.terminating_flow_id_count; ++i)
  {
    enc.fixed64_field(track_event_field::terminating_flow_ids,
        packet.terminating_flow_ids[i]);
  }

  for (std::size_t i = 0; i < packet.annotation_count; ++i)
  {
    const annotation& a = packet.annotations[i];
    uint32_t da_len = enc.begin_submessage(
        track_event_field::debug_annotations);
    if (a.name)
      enc.string_field(debug_annotation_field::name, a.name);
    switch (a.kind)
    {
    case annotation::kind_int:
      enc.varint_field(debug_annotation_field::int_value, a.int_value);
      break;
    case annotation::kind_uint:
      enc.varint_field(debug_annotation_field::uint_value, a.uint_value);
      break;
    case annotation::kind_pointer:
      enc.varint_field(debug_annotation_field::pointer_value,
          reinterpret_cast<uintptr_t>(a.pointer_value));
      break;
    case annotation::kind_bool:
      enc.varint_field(debug_annotation_field::bool_value,
          a.bool_value ? 1 : 0);
      break;
    case annotation::kind_string:
      enc.string_field(debug_annotation_field::string_value,
          a.string_value ? a.string_value : "");
      break;
    case annotation::kind_none:
    default:
      break;
    }
    enc.end_submessage(da_len);
  }

  enc.end_submessage(te_len);
  enc.end_submessage(packet_len);
}

struct clock_snapshot_packet
{
  packet_header hdr;
  uint64_t monotonic_ns;
  uint64_t realtime_ns;
};

inline void encode(encoder& enc, const clock_snapshot_packet& packet)
{
  uint32_t packet_len = enc.begin_submessage(trace_field::packet);
  encode(enc, packet.hdr);

  uint32_t cs_len = enc.begin_submessage(trace_packet_field::clock_snapshot);

  uint32_t mono_len = enc.begin_submessage(clock_snapshot_field::clocks);
  enc.varint_field(clock_field::clock_id, builtin_clock::monotonic);
  enc.varint_field(clock_field::timestamp, packet.monotonic_ns);
  enc.end_submessage(mono_len);
  uint32_t real_len = enc.begin_submessage(clock_snapshot_field::clocks);
  enc.varint_field(clock_field::clock_id, builtin_clock::realtime);
  enc.varint_field(clock_field::timestamp, packet.realtime_ns);
  enc.end_submessage(real_len);

  uint32_t boot_len = enc.begin_submessage(clock_snapshot_field::clocks);
  enc.varint_field(clock_field::clock_id, builtin_clock::boottime);
  enc.varint_field(clock_field::timestamp, packet.monotonic_ns);
  enc.end_submessage(boot_len);

  enc.varint_field(clock_snapshot_field::primary_trace_clock,
      builtin_clock::monotonic);
  enc.end_submessage(cs_len);

  enc.end_submessage(packet_len);
}

struct process_descriptor_packet
{
  packet_header hdr;
  uint64_t descriptor_uuid;
  const char* descriptor_name;
  int32_t pid;
  const char* process_name;
  const char* cmdline;
};

inline void encode(encoder& enc, const process_descriptor_packet& packet)
{
  uint32_t packet_len = enc.begin_submessage(trace_field::packet);
  encode(enc, packet.hdr);

  uint32_t td_len = enc.begin_submessage(trace_packet_field::track_descriptor);
  enc.varint_field(track_descriptor_field::uuid, packet.descriptor_uuid);
  if (packet.descriptor_name)
    enc.string_field(track_descriptor_field::name, packet.descriptor_name);

  uint32_t pd_len = enc.begin_submessage(track_descriptor_field::process);
  enc.varint_field(process_descriptor_field::pid, packet.pid);
  if (packet.process_name)
  {
    enc.string_field(process_descriptor_field::process_name,
        packet.process_name);
  }
  if (packet.cmdline)
    enc.string_field(process_descriptor_field::cmdline, packet.cmdline);
  enc.end_submessage(pd_len);

  enc.end_submessage(td_len);
  enc.end_submessage(packet_len);
}

struct thread_descriptor_packet
{
  packet_header hdr;
  uint64_t descriptor_uuid;
  uint64_t descriptor_parent_uuid;
  int32_t pid;
  int32_t tid;
};

inline void encode(encoder& enc, const thread_descriptor_packet& packet)
{
  uint32_t packet_len = enc.begin_submessage(trace_field::packet);
  encode(enc, packet.hdr);

  uint32_t td_len = enc.begin_submessage(trace_packet_field::track_descriptor);
  enc.varint_field(track_descriptor_field::uuid, packet.descriptor_uuid);
  if (packet.descriptor_parent_uuid != 0)
  {
    enc.varint_field(track_descriptor_field::parent_uuid,
        packet.descriptor_parent_uuid);
  }

  uint32_t thr_len = enc.begin_submessage(track_descriptor_field::thread);
  enc.varint_field(thread_descriptor_field::pid, packet.pid);
  enc.varint_field(thread_descriptor_field::tid, packet.tid);
  enc.end_submessage(thr_len);

  enc.end_submessage(td_len);
  enc.end_submessage(packet_len);
}

template <typename Packet>
uint32_t encode(unsigned char* out, uint32_t start, const Packet& packet)
{
  encoder enc(out, start);
  encode(enc, packet);

  // The callers guarantee max_packet bytes of headroom at start.
  ASIO_ASSERT(enc.size() - start <= limits::max_packet);

  return enc.size();
}

} // namespace perfetto_protocol
} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_PERFETTO_HANDLER_TRACKING)

#endif // ASIO_DETAIL_PERFETTO_PROTOCOL_HPP
