//
// detail/perfetto_writer.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_PERFETTO_WRITER_HPP
#define ASIO_DETAIL_PERFETTO_WRITER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_PERFETTO_HANDLER_TRACKING)

#include <atomic>
#include <cstddef>
#include <string>
#include "asio/detail/cstdint.hpp"
#include "asio/detail/chrono.hpp"
#include "asio/detail/mutex.hpp"
#include "asio/detail/thread.hpp"
#include "asio/detail/tss_ptr.hpp"
#include "asio/detail/perfetto_protocol.hpp"
#if defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
# include "asio/detail/socket_types.hpp"
#endif // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)

#include "asio/detail/push_options.hpp"

namespace asio {
ASIO_INLINE_NAMESPACE_BEGIN
namespace detail {
namespace perfetto_writer {

class state;
class thread_state;

struct config
{
  // Output file path. Defaults to "asio_handler_tracking.<pid>.pftrace".
  std::string output_path;

  // Writer-thread idle backoff, in nanoseconds. Set to 0 to busy poll.
  int64_t writer_idle_sleep_ns;

  // CPU to pin the writer thread to. Set to -1 to leave unpinned.
  int writer_cpu;

  // Maximum file size before rotation occurs. Set to 0 to disable rotation.
  uint64_t max_file_size;

  // Retain at most this many rotated files. Set to 0 to retain all of them.
  unsigned max_files;

  // A thread's leased buffer is reclaimed after it has been idle this long,
  // in nanoseconds. Set to 0 to disable reclamation.
  uint64_t reclaim_idle_ns;

  // The time, in nanoseconds, that records may sit in a thread's buffer before
  // being flushed to the writer thread. Set to 0 to disable periodic flushing.
  uint64_t flush_interval_ns;

  // Construct with the default settings.
  ASIO_DECL config();

  // Construct from the ASIO_PERFETTO_* environment variables.
  ASIO_DECL static config from_env();

private:
  ASIO_DECL static bool parse_env_uint(const char* name, uint64_t& value);

  ASIO_DECL static bool parse_env_ns(const char* name, uint64_t& ns);
};

struct buffer
{
  std::size_t len;
  buffer* next;
  unsigned char data[perfetto_protocol::limits::buffer_bytes];
};

class buffer_pool
{
public:
  // Construct an empty pool.
  buffer_pool()
    : free_head_(0)
  {
  }

  // Take a buffer from the free list, allocating one if the list is empty.
  buffer* acquire()
  {
    mutex::scoped_lock lock(mutex_);
    buffer* b = free_head_;
    if (b)
      free_head_ = b->next;
    else
      b = new buffer;
    b->len = 0;
    return b;
  }

  // Return a buffer to the free list.
  void release(buffer* b)
  {
    mutex::scoped_lock lock(mutex_);
    b->next = free_head_;
    free_head_ = b;
  }

private:
  mutex mutex_;
  buffer* free_head_;
};

class alignas(64) buffer_lease
{
public:
  // Construct an unleased, unlocked lease.
  buffer_lease()
    : buf_(0),
      locked_(0),
      revoked_(0),
      heartbeat_(0),
      lock_depth_(0)
  {
  }

  // The lease is locked if a handler is in-progress on this thread.
  bool locked() const
  {
    return lock_depth_ != 0;
  }

  // The buffer currently leased, or null if there is none.
  buffer* current() const
  {
    return buf_.load(std::memory_order_relaxed);
  }

  // Time of the last unlock, used to detect if the lease is idle.
  uint64_t last_active_ns() const
  {
    return heartbeat_.load(std::memory_order_relaxed);
  }

  // Whether a handler is currently in progress, safe for the drainer to poll.
  bool in_use() const
  {
    return locked_.load(std::memory_order_acquire) != 0;
  }

  // Lock the lease while a handler is in-progress, to prevent revocation.
  ASIO_DECL void lock(state& s);

  // Unlock the lease at the end of a handler.
  void unlock(uint64_t now)
  {
    // Nothing to unlock if a matching lock was skipped (e.g. on alloc failure).
    if (lock_depth_ == 0)
      return;
    if (--lock_depth_ == 0)
    {
      heartbeat_.store(now, std::memory_order_relaxed);
      locked_.store(0, std::memory_order_release);
    }
  }

  // Hand back a full buffer and lease a fresh one.
  ASIO_DECL buffer* renew(state& s, buffer* full);

  // Attempt to forcibly revoke a lease. Fails if it's locked.
  ASIO_DECL buffer* try_revoke(unsigned bounded_spins);

private:
  std::atomic<buffer*> buf_;
  std::atomic<unsigned> locked_;
  std::atomic<unsigned> revoked_;
  std::atomic<uint64_t> heartbeat_;
  unsigned lock_depth_;
};

class file
{
public:
  // Construct with no open file.
  file()
#if defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
    : handle_(INVALID_HANDLE_VALUE)
#else // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
    : handle_(-1)
#endif // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  {
  }

  // Whether the file is currently open.
  bool is_open() const
  {
#if defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
    return handle_ != INVALID_HANDLE_VALUE;
#else // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
    return handle_ >= 0;
#endif // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  }

  // Create/truncate the file at path for writing. Returns true it opened.
  ASIO_DECL bool open(const std::string& path);

  // Append exactly len bytes, looping over short writes. Returns the number of
  // bytes actually written (less than len only on error).
  ASIO_DECL std::size_t write(const unsigned char* data, std::size_t len);

  // Close the file if it is open.
  ASIO_DECL void close();

  // Delete the file at path. Used to prune old rotated files.
  ASIO_DECL static void remove(const std::string& path);

private:
#if defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  HANDLE handle_;
#else // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
  int handle_;
#endif // defined(ASIO_WINDOWS) || defined(ASIO_WINDOWS_RUNTIME)
};

class file_writer
{
public:
  // Construct a writer for the given path, with optional rotation limits.
  file_writer(const std::string& output_path,
      uint64_t max_file_size, unsigned max_files)
    : open_attempted_(0),
      file_opened_successfully_(0),
      output_path_(output_path),
      max_file_size_(max_file_size),
      max_files_(max_files),
      bytes_in_file_(0),
      file_index_(0)
  {
  }

  // Ensure we have an open file. Write the initial clock snapshot if it wasn't.
  ASIO_DECL bool ensure_open(uint32_t sequence_id);

  // Append raw bytes to the current file, counting them towards rotation.
  ASIO_DECL void write_bytes(const unsigned char* data, std::size_t len);

  // Whether the current file has reached the configured size limit.
  bool should_rotate() const
  {
    return max_file_size_ != 0 && bytes_in_file_ >= max_file_size_;
  }

  // Close current file and open next. Write clock snapshot to new file.
  ASIO_DECL void rotate(uint32_t sequence_id);

private:
  // The path of the file with the given rotation index.
  ASIO_DECL std::string file_path(unsigned index);

  // Write the clock snapshot packet that heads each file.
  ASIO_DECL void write_clock_snapshot(uint32_t sequence_id);

  file file_;
  mutex open_mutex_;
  std::atomic<unsigned> open_attempted_;
  std::atomic<unsigned> file_opened_successfully_;
  std::string output_path_;
  uint64_t max_file_size_;
  unsigned max_files_;
  uint64_t bytes_in_file_;
  unsigned file_index_;
};

// Append-only registry of every thread_state ever created.
class thread_registry
{
public:
  // Construct an empty registry.
  thread_registry()
    : head_(0)
  {
  }

  // Push a thread_state onto the registry.
  ASIO_DECL void add(thread_state* ts);

  // The most recently added thread_state, or null if there are none.
  thread_state* head() const
  {
    return head_.load(std::memory_order_acquire);
  }

  // The thread_state added just before the given one, or null at the end.
  ASIO_DECL thread_state* next(thread_state* ts) const;

private:
  std::atomic<thread_state*> head_;
};

// Manages various tracking identifiers.
class identity
{
public:
  // Construct, generating the salt and reading the process details.
  identity()
    : salt_(entropy()),
      sequence_id_prefix_(make_sequence_id_prefix()),
      process_track_uuid_(salt_), // track counter 0 is reserved for this.
      next_handler_id_(1),
      next_track_counter_(1),
      next_seq_counter_(1),
      process_id_(read_process_id()),
      process_name_(read_process_name()),
      process_command_line_(read_process_command_line())
  {
  }

  // Salt mixed into flow ids so traces from different processes do not collide.
  uint64_t salt() const
  {
    return salt_;
  }

  // The 16-bit sequence-id prefix, carried in the high bits of every sequence
  // id (and the writer's own), and seeding each file's clock-snapshot sequence.
  uint16_t sequence_id_prefix() const
  {
    return sequence_id_prefix_;
  }

  // The process track's uuid, parent of every thread track.
  uint64_t process_track_uuid() const
  {
    return process_track_uuid_;
  }

  // The native process id.
  int process_id() const
  {
    return process_id_;
  }

  // Read the current process id from the OS (used to detect a fork).
  ASIO_DECL static int read_process_id();

  // The short process name, or empty if it could not be read.
  const std::string& process_name() const
  {
    return process_name_;
  }

  // The process command line, or empty if it could not be read.
  const std::string& process_command_line() const
  {
    return process_command_line_;
  }

  // Allocate the next globally-unique handler id.
  uint64_t next_handler_id()
  {
    return next_handler_id_.fetch_add(1, std::memory_order_relaxed);
  }

  // Allocate a fresh per-thread sequence id.
  uint32_t next_sequence_id()
  {
    uint32_t low = next_seq_counter_.fetch_add(
        1, std::memory_order_relaxed) & 0xFFFF;
    uint32_t sid = (static_cast<uint32_t>(sequence_id_prefix_) << 16) | low;
    return sid == 0 ? 0x8000 : sid; // Must never be 0.
  }

  // Allocate a fresh thread-track uuid.
  uint64_t new_track_uuid()
  {
    return salt_ ^ next_track_counter_.fetch_add(1, std::memory_order_relaxed);
  }

private:
  // A single 64-bit draw from the best entropy source available, used only to
  // salt id spaces so that traces from different processes do not collide.
  ASIO_DECL static uint64_t entropy();

  // A 16-bit prefix for sequence ids, folded down from a fresh entropy draw.
  static uint16_t make_sequence_id_prefix()
  {
    uint64_t s = entropy();
    return static_cast<uint16_t>(s ^ (s >> 16) ^ (s >> 32) ^ (s >> 48));
  }

  // Best-effort short process name.
  ASIO_DECL static std::string read_process_name();

  // Best-effort command line, where it is cheaply available.
  ASIO_DECL static std::string read_process_command_line();

  uint64_t salt_;
  uint16_t sequence_id_prefix_;
  uint64_t process_track_uuid_;
  std::atomic<uint64_t> next_handler_id_;
  std::atomic<uint64_t> next_track_counter_;
  std::atomic<uint32_t> next_seq_counter_;
  int process_id_;
  std::string process_name_;
  std::string process_command_line_;
};

// Process-global tracking state.
class state
{
public:
  // Construct from the given configuration.
  state(const config& cfg = config::from_env())
    : writer_idle_sleep_ns_(cfg.writer_idle_sleep_ns),
      reclaim_idle_ns_(cfg.reclaim_idle_ns),
      flush_interval_ns_(cfg.flush_interval_ns),
      last_reclaim_ns_(0),
      writer_cpu_(cfg.writer_cpu),
      writer_started_(0),
      stop_writer_(0),
      identity_(),
      file_writer_(output_path(cfg, identity_.process_id()),
          cfg.max_file_size, cfg.max_files),
      writer_queue_(0),
      process_descriptor_written_(false),
      last_described_thread_(0)
  {
  }

  // The shared buffer pool.
  buffer_pool& pool()
  {
    return pool_;
  }

  // The tracking identifiers (salt, process details, id allocators).
  identity& ids()
  {
    return identity_;
  }

  // The periodic flush interval, in nanoseconds.
  uint64_t flush_interval_ns() const
  {
    return flush_interval_ns_;
  }

  // This thread's state pointer, or null before its first tracked event.
  thread_state* current_thread()
  {
    return current_thread_;
  }

  // Record this thread's state pointer.
  void set_current_thread(thread_state* ts)
  {
    current_thread_ = ts;
  }

  // The CPU to pin the writer thread to, or -1 to leave it unpinned.
  int writer_cpu() const
  {
    return writer_cpu_;
  }

  // The writer thread's idle sleep interval, in nanoseconds.
  int64_t writer_idle_sleep_ns() const
  {
    return writer_idle_sleep_ns_;
  }

  // Whether a shutdown has been requested.
  bool stop_requested() const
  {
    return stop_writer_.load(std::memory_order_acquire) != 0;
  }

  // The process-global instance, created on first use.
  static state* instance(const config* cfg = 0)
  {
    static state* s = cfg ? new state(*cfg) : new state;
    return s;
  }

  // Add a thread_state to the registry.
  void register_thread(thread_state* ts)
  {
    registry_.add(ts);
  }

  // Hand a full buffer to the writer queue.
  ASIO_DECL void enqueue(buffer* buf);

  // Open the output file and start the writer thread. Called once from init().
  ASIO_DECL void start();

  // Used as an atexit handler. Drains everything and waits for the trace tail
  // to reach the file before the process exits.
  ASIO_DECL static void shutdown();

  // Emit any owed descriptors, then drain the handed-off buffers to the file.
  // Returns whether anything was written.
  ASIO_DECL bool drain_all();

  // Reclaim the leased buffers of every idle thread.
  ASIO_DECL void reclaim_scan();

  // Flush every registered thread's partial (not-yet-handed-off) buffer to the
  // file. Used at shutdown to capture the trace tail.
  ASIO_DECL void flush_partials();

private:
  // The output file path: the configured path if set, otherwise the default
  // "asio_handler_tracking.<pid>.pftrace".
  ASIO_DECL static std::string output_path(
      const config& cfg, int process_id);

  // Emit the process descriptor (once per file) plus any thread descriptors
  // registered since the last call. Returns whether any were written.
  ASIO_DECL bool emit_descriptors();

  // Write one full buffer to the file, rotating (and re-describing the new
  // file) at the buffer boundary if the file is now full.
  ASIO_DECL void write_buffer(buffer* buf);

  // Reclaim thread ts's leased buffer via the lease protocol, flushing its
  // partial and returning it to the pool.
  ASIO_DECL void reclaim(thread_state* ts, unsigned bounded_spins = 0);

  // Encode and write the process / thread descriptors on the writer sequence.
  ASIO_DECL void emit_process_descriptor();
  ASIO_DECL void emit_thread_descriptor(thread_state* ts);

  // The sequence id carrying writer-emitted packets (clock snapshot,
  // descriptors). Matches the clock snapshot written by file_writer.
  uint32_t writer_sequence_id() const
  {
    uint32_t sid = static_cast<uint32_t>(identity_.sequence_id_prefix()) << 16;
    return sid ? sid : 0x8000;
  }

  // Monotonic nanoseconds, for stamping writer-emitted packets. Matches the
  // clock used for per-event timestamps (see thread_state::timestamp()).
  static uint64_t drainer_now_ns()
  {
    return static_cast<uint64_t>(
        chrono::duration_cast<chrono::nanoseconds>(
          chrono::steady_clock::now().time_since_epoch()).count());
  }

  // Writer-thread idle backoff, in nanoseconds. 0 means busy-poll.
  int64_t writer_idle_sleep_ns_;

  // Idle time before a thread's leased buffer is reclaimed, in nanoseconds.
  uint64_t reclaim_idle_ns_;

  // Interval at which a thread's buffered records are flushed, in nanoseconds.
  uint64_t flush_interval_ns_;

  // Time of the last reclaim scan, used to rate-limit reclamation.
  uint64_t last_reclaim_ns_;

  // CPU to pin the writer thread to, or -1 to leave it unpinned.
  int writer_cpu_;

  // Once-guard used to start the writer thread on the first flush.
  std::atomic<unsigned> writer_started_;

  // Used to initiate a clean shutdown.
  std::atomic<unsigned> stop_writer_;

  // The writer thread, owned here and joined at shutdown.
  asio::detail::thread writer_thread_;

  // The tracking identifiers (salt, process details, id allocators).
  identity identity_;

  // Writes encoded buffers to the output file, rotating as configured.
  file_writer file_writer_;

  // Queue of full buffers ready to be written.
  std::atomic<buffer*> writer_queue_;

  // The shared pool of reusable buffers.
  buffer_pool pool_;

  // The registry of every thread_state, walked by the drainer.
  thread_registry registry_;

  // Drainer-only bookkeeping, reset on each rotation so every file is
  // self-describing.
  bool process_descriptor_written_;
  thread_state* last_described_thread_;

  // The calling thread's state pointer, or null before its first tracked event.
  tss_ptr<thread_state> current_thread_;
};

// Per-thread state. Outlives the thread and sits in the registry until exit.
class thread_state
{
public:
  // Construct, allocating this thread's sequence id and track uuid, then
  // publishing into the registry.
  thread_state()
    : sequence_id_(0),
      thread_track_uuid_(0),
      tid_(0),
      last_event_ns_(0),
      last_flush_ns_(0),
      any_packet_emitted_(false),
      registry_next_(0)
  {
    state* s = state::instance();
    sequence_id_ = s->ids().next_sequence_id();
    thread_track_uuid_ = s->ids().new_track_uuid();
    tid_ = this_thread_id();

    // Publish into the registry last, once every field the writer reads is set.
    s->register_thread(this);
  }

  // Strictly increasing per-thread timestamp so that same-nanosecond events
  // nest deterministically.
  uint64_t timestamp()
  {
    uint64_t ns = static_cast<uint64_t>(
        chrono::duration_cast<chrono::nanoseconds>(
          chrono::steady_clock::now().time_since_epoch()).count());
    if (ns <= last_event_ns_)
      ns = ++last_event_ns_;
    else
      last_event_ns_ = ns;
    return ns;
  }

  // Open a handler scope spanning the whole invocation, leasing the buffer up
  // front. The matching unlock() closes it.
  void lock()
  {
    lease_.lock(*state::instance());
  }

  // Close the handler scope opened by lock(), stamping its exit time.
  void unlock()
  {
    lease_.unlock(last_event_ns_);
  }

  // Append one encoded packet to the leased buffer. Inside a handler this is a
  // plain append into the already-leased buffer. Outside any handler it wraps
  // itself in a self-contained scope.
  template <typename Packet>
  void emit(Packet& packet)
  {
    using namespace perfetto_protocol;

    packet.hdr.sequence_id = sequence_id_;
    if (!any_packet_emitted_)
    {
      packet.hdr.incremental_state_cleared = true;
      any_packet_emitted_ = true;
    }

    state* s = state::instance();
    if (lease_.locked())
    {
      append_packet(*s, packet);
    }
    else
    {
      lease_.lock(*s);
      append_packet(*s, packet);
      lease_.unlock(last_event_ns_);
    }
  }

  // This thread's track uuid, stamped on every packet it emits.
  uint64_t thread_track_uuid() const
  {
    return thread_track_uuid_;
  }

  // Yield the rest of this thread's time slice to another runnable thread.
  ASIO_DECL static void yield_cpu();

  // Emit a CPU relax/pause hint for a busy-poll spin loop.
  ASIO_DECL static void pause_cpu();

private:
  friend class state;
  friend class thread_registry;

  // Native OS thread id, matching other tools' thread listings.
  ASIO_DECL static int this_thread_id();

  // Append one encoded packet into the leased buffer in place, handing the
  // buffer off and leasing a fresh one when it fills or the flush interval
  // elapses.
  template <typename Packet>
  void append_packet(state& s, Packet& packet)
  {
    using namespace perfetto_protocol;

    buffer* buf = lease_.current();

    // Hand a full buffer off to the writer and lease a fresh one.
    if (buf->len + limits::max_packet > limits::buffer_bytes)
    {
      buf = lease_.renew(s, buf);
      last_flush_ns_ = last_event_ns_;
    }

    buf->len = encode(buf->data, buf->len, packet);

    // Bound how long records sit buffered before reaching the file. This caps
    // the tail lost on abnormal termination to about one flush interval.
    if (s.flush_interval_ns() != 0
        && last_event_ns_ - last_flush_ns_ >= s.flush_interval_ns())
    {
      lease_.renew(s, buf);
      last_flush_ns_ = last_event_ns_;
    }
  }

  // The trusted sequence id identifying this thread's packet stream.
  uint32_t sequence_id_;

  // This thread's track uuid, the track its events are recorded on.
  uint64_t thread_track_uuid_;

  // Native OS thread id, recorded in this thread's descriptor.
  int tid_;

  // Timestamp of the most recent event, keeping per-thread time monotonic.
  uint64_t last_event_ns_;

  // Timestamp of the last renewal. Used to drive periodic flushing.
  uint64_t last_flush_ns_;

  // Whether any packet has been emitted yet, to clear incremental state once.
  bool any_packet_emitted_;

  // Manages ownership / reclamation of the buffer.
  buffer_lease lease_;

  // Next thread in the registry. Set at construction and never updated, as
  // threads stay in the registry until program exit.
  thread_state* registry_next_;
};

// Returns the calling thread's writer state, creating it on first use.
ASIO_DECL thread_state* this_thread();

// The process-global flow-id salt (state's trace-identity salt).
ASIO_DECL uint64_t salt();

// Allocate the next globally-unique handler id.
ASIO_DECL uint64_t next_handler_id();

// Initialise the writer (open the file and start the writer thread).
ASIO_DECL void init();

// Initialise the writer with an explicit configuration.
ASIO_DECL void init(const config& cfg);

} // namespace perfetto_writer
} // namespace detail
ASIO_INLINE_NAMESPACE_END
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_PERFETTO_HANDLER_TRACKING)

#if defined(ASIO_HEADER_ONLY)
# include "asio/detail/impl/perfetto_writer.ipp"
#endif // defined(ASIO_HEADER_ONLY)

#endif // ASIO_DETAIL_PERFETTO_WRITER_HPP
