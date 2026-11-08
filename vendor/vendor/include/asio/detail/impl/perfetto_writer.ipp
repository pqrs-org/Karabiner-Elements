//
// detail/impl/perfetto_writer.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_PERFETTO_WRITER_IPP
#define ASIO_DETAIL_IMPL_PERFETTO_WRITER_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_PERFETTO_HANDLER_TRACKING)

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <new>
#include "asio/detail/perfetto_writer.hpp"
#include "asio/detail/perfetto_protocol.hpp"

#if defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
# include "asio/detail/socket_types.hpp"
#else // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
# include <cerrno>
# include <fcntl.h>
# include <pthread.h>
# include <sched.h>
# include <time.h>
# include <unistd.h>
# include <sys/types.h>
# if defined(__linux__)
#  include <sys/syscall.h>
# endif // defined(__linux__)
#endif // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {
namespace perfetto_writer {

config::config()
  : writer_idle_sleep_ns(1000000),
    writer_cpu(-1),
    max_file_size(0),
    max_files(0),
    reclaim_idle_ns(100000000ULL), // 100 milliseconds
    flush_interval_ns(250000000ULL) // 250 milliseconds
{
}

config config::from_env()
{
  config cfg;

  const char* env_path = std::getenv("ASIO_PERFETTO_OUTPUT_PATH");
  if (env_path && *env_path)
    cfg.output_path = env_path;

  uint64_t v = 0;
  if (parse_env_ns("ASIO_PERFETTO_WRITER_IDLE_MS", v))
    cfg.writer_idle_sleep_ns = static_cast<int64_t>(v);
  if (parse_env_uint("ASIO_PERFETTO_WRITER_CPU", v))
    cfg.writer_cpu = static_cast<int>(v);
  if (parse_env_uint("ASIO_PERFETTO_MAX_FILE_SIZE", v))
    cfg.max_file_size = v;
  if (parse_env_uint("ASIO_PERFETTO_MAX_FILES", v))
    cfg.max_files = static_cast<unsigned>(v);
  if (parse_env_ns("ASIO_PERFETTO_RECLAIM_IDLE_MS", v))
    cfg.reclaim_idle_ns = v;
  if (parse_env_ns("ASIO_PERFETTO_FLUSH_INTERVAL_MS", v))
    cfg.flush_interval_ns = v;

  return cfg;
}

bool config::parse_env_uint(const char* name, uint64_t& value)
{
  const char* s = std::getenv(name);
  if (!s || !*s)
    return false;
  errno = 0;
  char* end = 0;
  long long v = std::strtoll(s, &end, 10);
  if (end == s || *end != '\0' || errno != 0 || v < 0)
    return false;
  value = static_cast<uint64_t>(v);
  return true;
}

bool config::parse_env_ns(const char* name, uint64_t& ns)
{
  uint64_t ms = 0;
  if (!parse_env_uint(name, ms))
    return false;
  const uint64_t max_ms = 3600000;
  if (ms > max_ms)
    ms = max_ms;
  ns = ms * 1000000ULL;
  return true;
}

void buffer_lease::lock(state& s)
{
  if (lock_depth_++ != 0)
    return;

  locked_.store(1, std::memory_order_seq_cst);
  buffer* buf = buf_.load(std::memory_order_relaxed);
  if (buf == 0 || revoked_.load(std::memory_order_seq_cst))
  {
    // No lease, or ours was revoked. Take a fresh buffer, hand off the old.
    revoked_.store(0, std::memory_order_relaxed);
    buffer* fresh = s.pool().acquire();
    if (buf != 0 && buf_.compare_exchange_strong(buf, fresh,
          std::memory_order_acq_rel, std::memory_order_relaxed))
      s.enqueue(buf);
    else
      buf_.store(fresh, std::memory_order_relaxed);
  }
}

buffer* buffer_lease::renew(state& s, buffer* full)
{
  buffer* fresh = s.pool().acquire();
  if (buf_.compare_exchange_strong(full, fresh,
        std::memory_order_acq_rel, std::memory_order_relaxed))
    s.enqueue(full);
  else
    buf_.store(fresh, std::memory_order_relaxed);
  return fresh;
}

buffer* buffer_lease::try_revoke(unsigned bounded_spins)
{
  buffer* b = buf_.load(std::memory_order_relaxed);
  if (b == 0)
    return 0; // nothing leased

  // Revoke, then wait for any in-flight handler to complete.
  revoked_.store(1, std::memory_order_seq_cst);
  for (unsigned spins = 0;
      locked_.load(std::memory_order_seq_cst) != 0; ++spins)
  {
    // At shutdown, give up on a thread stuck in a handler rather than hang.
    if (bounded_spins != 0 && spins >= bounded_spins)
    {
      revoked_.store(0, std::memory_order_relaxed);
      return 0;
    }
    thread_state::pause_cpu();
  }

  // Take the buffer only if the producer still holds it, else cancel and leave.
  if (!buf_.compare_exchange_strong(b, static_cast<buffer*>(0),
        std::memory_order_acquire, std::memory_order_relaxed))
  {
    revoked_.store(0, std::memory_order_relaxed);
    return 0;
  }

  return b;
}

bool file::open(const std::string& path)
{
#if defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  handle_ = ::CreateFileA(path.c_str(), GENERIC_WRITE,
      FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
#else // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  int flags = O_WRONLY | O_CREAT | O_TRUNC;
# if defined(O_CLOEXEC)
  flags |= O_CLOEXEC;
# endif // defined(O_CLOEXEC)
  handle_ = ::open(path.c_str(), flags, 0644);
#endif // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  return is_open();
}

std::size_t file::write(const unsigned char* data, std::size_t len)
{
  std::size_t done = 0;
#if defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  while (done < len)
  {
    std::size_t remaining = len - done;
    DWORD chunk = remaining > 0x7FFFFFFF
      ? 0x7FFFFFFF : static_cast<DWORD>(remaining);
    DWORD written = 0;
    if (!::WriteFile(handle_, data + done, chunk, &written, 0))
      break;
    if (written == 0)
      break;
    done += written;
  }
#else // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  while (done < len)
  {
    ssize_t n = ::write(handle_, data + done, len - done);
    if (n < 0)
    {
      if (errno == EINTR)
        continue;
      break;
    }
    if (n == 0)
      break;
    done += static_cast<std::size_t>(n);
  }
#endif // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  return done;
}

void file::close()
{
#if defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  if (handle_ != INVALID_HANDLE_VALUE)
  {
    ::CloseHandle(handle_);
    handle_ = INVALID_HANDLE_VALUE;
  }
#else // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  if (handle_ >= 0)
  {
    ::close(handle_);
    handle_ = -1;
  }
#endif // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
}

void file::remove(const std::string& path)
{
#if defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  ::DeleteFileA(path.c_str());
#else // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  ::unlink(path.c_str());
#endif // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
}

bool file_writer::ensure_open(uint32_t sequence_id)
{
  if (open_attempted_.load(std::memory_order_acquire) != 0)
    return file_opened_successfully_.load(std::memory_order_relaxed) != 0;

  mutex::scoped_lock lock(open_mutex_);
  if (open_attempted_.load(std::memory_order_relaxed) != 0)
    return file_opened_successfully_.load(std::memory_order_relaxed) != 0;

  // File 0 uses the original output path. A non-rotating trace is unchanged.
  bool ok = file_.open(file_path(0));
  if (ok)
    write_clock_snapshot(sequence_id);

  file_opened_successfully_.store(ok ? 1 : 0, std::memory_order_relaxed);
  open_attempted_.store(1, std::memory_order_release);
  return ok;
}

void file_writer::write_bytes(const unsigned char* data, std::size_t len)
{
  bytes_in_file_ += file_.write(data, len);
}

void file_writer::rotate(uint32_t sequence_id)
{
  file_.close();

  ++file_index_;
  if (max_files_ != 0 && file_index_ >= max_files_)
    file::remove(file_path(file_index_ - max_files_));

  bool ok = file_.open(file_path(file_index_));
  bytes_in_file_ = 0;
  if (ok)
    write_clock_snapshot(sequence_id);
}

std::string file_writer::file_path(unsigned index)
{
  if (index == 0)
    return output_path_;
  return output_path_ + "." + std::to_string(index);
}

void file_writer::write_clock_snapshot(uint32_t sequence_id)
{
  using namespace perfetto_protocol;

  // Read both clocks back to back so the snapshot maps monotonic to wall-clock.
  uint64_t monotonic_ns = static_cast<uint64_t>(
      chrono::duration_cast<chrono::nanoseconds>(
        chrono::steady_clock::now().time_since_epoch()).count());
  uint64_t realtime_ns = static_cast<uint64_t>(
      chrono::duration_cast<chrono::nanoseconds>(
        chrono::system_clock::now().time_since_epoch()).count());

  unsigned char header[limits::max_packet];
  clock_snapshot_packet packet = clock_snapshot_packet();
  packet.hdr.timestamp = monotonic_ns;
  packet.hdr.sequence_id = sequence_id;
  packet.monotonic_ns = monotonic_ns;
  packet.realtime_ns = realtime_ns;
  std::size_t header_len = encode(header, 0, packet);
  write_bytes(header, header_len);
}

void thread_registry::add(thread_state* ts)
{
  ts->registry_next_ = head_.load(std::memory_order_relaxed);
  while (!head_.compare_exchange_weak(ts->registry_next_, ts,
        std::memory_order_release, std::memory_order_relaxed))
  {
    // Retry.
  }
}

thread_state* thread_registry::next(thread_state* ts) const
{
  return ts->registry_next_;
}

int identity::read_process_id()
{
#if defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  return static_cast<int>(::GetCurrentProcessId());
#else // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  return static_cast<int>(::getpid());
#endif // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
}

uint64_t identity::entropy()
{
  uint64_t v = 0;
#if defined(__APPLE__) || defined(__FreeBSD__) \
  || defined(__OpenBSD__) || defined(__NetBSD__)
  ::arc4random_buf(&v, sizeof(v));
  if (v != 0)
    return v;
#elif defined(__linux__) && defined(__GLIBC__) \
  && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 25))
  if (::getentropy(&v, sizeof(v)) == 0 && v != 0)
    return v;
#endif // defined(__linux__) && defined(__GLIBC__)
       //   && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 25))
  uint64_t t1 = static_cast<uint64_t>(
      chrono::duration_cast<chrono::nanoseconds>(
        chrono::system_clock::now().time_since_epoch()).count());
  uint64_t t2 = static_cast<uint64_t>(
      chrono::duration_cast<chrono::nanoseconds>(
        chrono::steady_clock::now().time_since_epoch()).count());
  uint64_t mix = t1;
  mix ^= t2 * 0x9E3779B97F4A7C15ULL;
  mix ^= static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&v)) << 17;
  mix ^= static_cast<uint64_t>(read_process_id()) * 0xD1B54A32D192ED03ULL;
  mix += 0x9E3779B97F4A7C15ULL;
  mix = (mix ^ (mix >> 30)) * 0xBF58476D1CE4E5B9ULL;
  mix = (mix ^ (mix >> 27)) * 0x94D049BB133111EBULL;
  mix = mix ^ (mix >> 31);
  return mix != 0 ? mix : 0x9E3779B97F4A7C15ULL;
}

#if defined(__GLIBC__) && !defined(ASIO_WINDOWS) \
  && !defined(ASIO_WINDOWS_RUNTIME)
// program_invocation_short_name is a glibc C global. Its <errno.h> declaration
// needs _GNU_SOURCE, which asio does not set. Declare it here with C linkage.
extern "C" char* program_invocation_short_name;
#endif // defined(__GLIBC__) && !defined(ASIO_WINDOWS)
       //   && !defined(ASIO_WINDOWS_RUNTIME)

std::string identity::read_process_name()
{
#if defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  char path[MAX_PATH];
  DWORD n = ::GetModuleFileNameA(0, path, static_cast<DWORD>(sizeof(path)));
  if (n == 0)
    return std::string();
  std::string s(path, path + n);
  std::string::size_type slash = s.find_last_of("\\/");
  return slash == std::string::npos ? s : s.substr(slash + 1);
#elif defined(__APPLE__) || defined(__FreeBSD__) \
  || defined(__OpenBSD__) || defined(__NetBSD__)
  const char* p = ::getprogname();
  return p ? std::string(p) : std::string();
#elif defined(__GLIBC__)
  return program_invocation_short_name
    ? std::string(program_invocation_short_name) : std::string();
#else // defined(__GLIBC__)
  return std::string();
#endif // defined(__GLIBC__)
}

std::string identity::read_process_command_line()
{
#if defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  const char* c = ::GetCommandLineA();
  return c ? std::string(c) : std::string();
#elif defined(__linux__)
  std::FILE* f = std::fopen("/proc/self/cmdline", "rb");
  if (!f)
    return std::string();
  std::string s;
  int ch;
  while ((ch = std::fgetc(f)) != EOF)
    s.push_back(ch == '\0' ? ' ' : static_cast<char>(ch));
  std::fclose(f);
  while (!s.empty() && s[s.size() - 1] == ' ')
    s.erase(s.size() - 1);
  return s;
#else // defined(__linux__)
  return std::string();
#endif // defined(__linux__)
}

class writer_thread
{
public:
  explicit writer_thread(state* s)
    : s_(s)
  {
  }

  void operator()()
  {
    pin_thread(s_->writer_cpu());

    const bool spin = (s_->writer_idle_sleep_ns() == 0);
    unsigned idle = 0;
    for (;;)
    {
      bool drained = s_->drain_all();

      // Reclaim idle threads' leased buffers to keep the pool bounded.
      s_->reclaim_scan();

      // On shutdown, drain and flush everything, then exit so the join returns.
      if (s_->stop_requested())
      {
        while (s_->drain_all()) {}
        s_->flush_partials();
        return;
      }

      if (drained)
      {
        idle = 0;
        continue;
      }

      // Nothing ready. Back off in stages.
      if (spin)
        thread_state::pause_cpu();
      else if (idle < 1024)
        thread_state::yield_cpu();
      else if (idle < 1024 + 512)
        sleep(20000L);
      else
        sleep(s_->writer_idle_sleep_ns());
      ++idle;
    }
  }

private:
  static void sleep(int64_t ns)
  {
#if defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
    ::Sleep(ns < 1000000 ? 1 : static_cast<DWORD>(ns / 1000000));
#else // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
    timespec ts;
    ts.tv_sec = static_cast<time_t>(ns / 1000000000);
    ts.tv_nsec = static_cast<long>(ns % 1000000000);
    ::nanosleep(&ts, 0);
#endif // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  }

  static void pin_thread(int cpu)
  {
    if (cpu < 0)
      return;
#if defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
# if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0601)
    GROUP_AFFINITY ga = {};
    ga.Group = static_cast<WORD>(cpu / 64);
    ga.Mask = static_cast<KAFFINITY>(1) << (cpu % 64);
    ::SetThreadGroupAffinity(::GetCurrentThread(), &ga, 0);
# else // defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0601)
    if (cpu < 64)
      ::SetThreadAffinityMask(::GetCurrentThread(),
          static_cast<DWORD_PTR>(1) << cpu);
# endif // defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0601)
#elif defined(__linux__)
    const int bits = static_cast<int>(8 * sizeof(unsigned long));
    const int word = cpu / bits;
    if (word >= 64)
      return;
    unsigned long mask[64] = { 0 };
    mask[word] = 1UL << (cpu % bits);
    ::syscall(SYS_sched_setaffinity, 0,
        (word + 1) * sizeof(unsigned long), mask);
#else // defined(__linux__)
    (void)cpu;
#endif // defined(__linux__)
  }

  state* s_;
};

void state::enqueue(buffer* buf)
{
  buf->next = writer_queue_.load(std::memory_order_relaxed);
  while (!writer_queue_.compare_exchange_weak(buf->next, buf,
        std::memory_order_release, std::memory_order_relaxed))
  {
    // Retry.
  }

#if !defined(ASIO_HAS_THREADS)
  // No writer thread. Drain synchronously on this (only) thread.
  drain_all();
#endif // !defined(ASIO_HAS_THREADS)
}

void state::start()
{
  file_writer_.ensure_open(writer_sequence_id());

  // Once only: spawn the writer thread and register the shutdown drain.
  if (writer_started_.load(std::memory_order_acquire) == 0)
  {
    unsigned expected = 0;
    if (writer_started_.compare_exchange_strong(
          expected, 1, std::memory_order_acq_rel))
    {
#if defined(ASIO_HAS_THREADS)
      writer_thread w(this);
# if !defined(ASIO_NO_EXCEPTIONS)
      try
      {
# endif // !defined(ASIO_NO_EXCEPTIONS)
        writer_thread_ = asio::detail::thread(w);
# if !defined(ASIO_NO_EXCEPTIONS)
      }
      catch (...)
      {
        // The spawn failed. Roll back so a later event retries, and skip the
        // atexit drain, as there is no writer thread to wait for.
        writer_started_.store(0, std::memory_order_release);
        return;
      }
# endif // !defined(ASIO_NO_EXCEPTIONS)
#endif // defined(ASIO_HAS_THREADS)
      std::atexit(&state::shutdown);
    }
  }
}

void state::shutdown()
{
  state* s = instance();
  if (s->writer_started_.load(std::memory_order_acquire) == 0)
    return;

#if defined(ASIO_HAS_THREADS)
  // A forked child inherits this atexit handler and writer_started_ == 1 but
  // not the writer thread. Waiting for it would hang forever. Bail if the
  // process id no longer matches the one that started the writer.
  if (s->ids().process_id() != identity::read_process_id())
    return;
  s->stop_writer_.store(1, std::memory_order_release);
  s->writer_thread_.join();
#else // defined(ASIO_HAS_THREADS)
  while (s->drain_all()) {}
  s->flush_partials();
#endif // defined(ASIO_HAS_THREADS)
}

bool state::drain_all()
{
  bool wrote = emit_descriptors();

  buffer* batch = writer_queue_.exchange(0, std::memory_order_acquire);
  if (batch == 0)
    return wrote;

  // The queue is a LIFO stack. Reverse it to write in production order.
  buffer* fifo = 0;
  while (batch != 0)
  {
    buffer* next = batch->next;
    batch->next = fifo;
    fifo = batch;
    batch = next;
  }

  while (fifo != 0)
  {
    buffer* next = fifo->next;
    write_buffer(fifo);
    pool_.release(fifo);
    fifo = next;
  }

  return true;
}

void state::reclaim_scan()
{
  // Cap on the revoke wait for the periodic scan. Leases are pre-filtered to
  // idle ones. This only bounds the rare race where a producer re-locks.
  const unsigned reclaim_scan_max_spins = 64;

  if (reclaim_idle_ns_ == 0)
    return;

  // Rate-limit scans to at most one per idle interval.
  uint64_t now = drainer_now_ns();
  if (now - last_reclaim_ns_ < reclaim_idle_ns_)
    return;
  last_reclaim_ns_ = now;

  for (thread_state* t = registry_.head(); t != 0; t = registry_.next(t))
  {
    if (t->lease_.current() == 0)
      continue;
    if (t->lease_.in_use())
      continue;
    uint64_t last_active = t->lease_.last_active_ns();
    if (now <= last_active || now - last_active < reclaim_idle_ns_)
      continue;
    reclaim(t, reclaim_scan_max_spins);
  }
}

void state::flush_partials()
{
  if (!file_writer_.ensure_open(writer_sequence_id()))
    return;
  emit_descriptors();
  for (thread_state* t = registry_.head(); t != 0; t = registry_.next(t))
    reclaim(t, 4096);
}

std::string state::output_path(const config& cfg, int process_id)
{
  if (!cfg.output_path.empty())
    return cfg.output_path;

  using namespace std; // For snprintf (or equivalent).
  char buf[64];
#if defined(ASIO_HAS_SNPRINTF)
  snprintf(buf, sizeof(buf),
      "asio_handler_tracking.%d.pftrace", process_id);
#elif defined(ASIO_HAS_SECURE_RTL)
  sprintf_s(buf, sizeof(buf),
      "asio_handler_tracking.%d.pftrace", process_id);
#else // defined(ASIO_HAS_SECURE_RTL)
  sprintf(buf, "asio_handler_tracking.%d.pftrace", process_id);
#endif // defined(ASIO_HAS_SECURE_RTL)
  return buf;
}

bool state::emit_descriptors()
{
  if (!file_writer_.ensure_open(writer_sequence_id()))
    return false;

  bool wrote = false;
  if (!process_descriptor_written_)
  {
    emit_process_descriptor();
    process_descriptor_written_ = true;
    wrote = true;
  }

  // The entries above last_described_thread_ are the ones not yet described.
  thread_state* head = registry_.head();
  for (thread_state* t = head; t != last_described_thread_;
      t = registry_.next(t))
  {
    emit_thread_descriptor(t);
    wrote = true;
  }
  last_described_thread_ = head;
  return wrote;
}

void state::write_buffer(buffer* buf)
{
  file_writer_.write_bytes(buf->data, buf->len);
  if (file_writer_.should_rotate())
  {
    file_writer_.rotate(writer_sequence_id());
    process_descriptor_written_ = false;
    last_described_thread_ = 0;
    emit_descriptors();
  }
}

void state::reclaim(thread_state* t, unsigned bounded_spins)
{
  buffer* b = t->lease_.try_revoke(bounded_spins);
  if (b == 0)
    return;
  if (b->len != 0)
    write_buffer(b);
  pool_.release(b);
}

void state::emit_process_descriptor()
{
  using namespace perfetto_protocol;

  const std::string& name = identity_.process_name();
  const std::string& cmdline = identity_.process_command_line();
  const char* pname = name.empty() ? 0 : name.c_str();
  process_descriptor_packet packet = process_descriptor_packet();
  packet.hdr.sequence_id = writer_sequence_id();
  packet.hdr.timestamp = drainer_now_ns();
  packet.descriptor_uuid = identity_.process_track_uuid();
  packet.descriptor_name = pname;
  packet.pid = identity_.process_id();
  packet.process_name = pname;
  packet.cmdline = cmdline.empty() ? 0 : cmdline.c_str();

  unsigned char buf[limits::max_packet];
  std::size_t len = encode(buf, 0, packet);
  file_writer_.write_bytes(buf, len);
}

void state::emit_thread_descriptor(thread_state* ts)
{
  using namespace perfetto_protocol;

  thread_descriptor_packet packet = thread_descriptor_packet();
  packet.hdr.sequence_id = writer_sequence_id();
  packet.hdr.timestamp = drainer_now_ns();
  packet.descriptor_uuid = ts->thread_track_uuid_;
  packet.descriptor_parent_uuid = identity_.process_track_uuid();
  packet.pid = identity_.process_id();
  packet.tid = ts->tid_;

  unsigned char buf[limits::max_packet];
  std::size_t len = encode(buf, 0, packet);
  file_writer_.write_bytes(buf, len);
}

void thread_state::yield_cpu()
{
#if defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  ::SwitchToThread();
#else // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  ::sched_yield();
#endif // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
}

void thread_state::pause_cpu()
{
#if defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  YieldProcessor();
#elif defined(__x86_64__) || defined(__i386__)
  __asm__ __volatile__("pause" ::: "memory");
#elif defined(__aarch64__) || defined(__arm__)
  __asm__ __volatile__("yield" ::: "memory");
#else // defined(__aarch64__) || defined(__arm__)
  // No relax hint on this architecture. Spin with a plain loop.
#endif // defined(__aarch64__) || defined(__arm__)
}

int thread_state::this_thread_id()
{
#if defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  return static_cast<int>(::GetCurrentThreadId());
#elif defined(__linux__)
  return static_cast<int>(::syscall(SYS_gettid));
#elif defined(__APPLE__)
  uint64_t tid64 = 0;
  ::pthread_threadid_np(0, &tid64);
  return static_cast<int>(tid64);
#elif defined(ASIO_HAS_PTHREADS)
  return static_cast<int>(reinterpret_cast<uintptr_t>(
      reinterpret_cast<void*>(::pthread_self())));
#else // defined(ASIO_HAS_PTHREADS)
  return 0;
#endif // defined(ASIO_HAS_PTHREADS)
}

thread_state* this_thread()
{
  state* s = state::instance();
  thread_state* ts = s->current_thread();
  if (!ts)
  {
    // Start the writer and register the atexit drain before the first event.
    s->start();
    ts = new (std::nothrow) thread_state;
    s->set_current_thread(ts);
  }
  return ts;
}

uint64_t salt()
{
  return state::instance()->ids().salt();
}

uint64_t next_handler_id()
{
  return state::instance()->ids().next_handler_id();
}

void init()
{
  state::instance()->start();
}

void init(const config& cfg)
{
  state::instance(&cfg)->start();
}

} // namespace perfetto_writer
} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_PERFETTO_HANDLER_TRACKING)

#endif // ASIO_DETAIL_IMPL_PERFETTO_WRITER_IPP
