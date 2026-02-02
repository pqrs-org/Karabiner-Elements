#include "console_user_server/socket_command_handler.hpp"
#include "dispatcher_utility.hpp"
#include <boost/ut.hpp>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <thread>
#include <unistd.h>

namespace {
std::string make_tmp_socket_path(void) {
  char tmpl[] = "/tmp/krbn_socket_command_handler.XXXXXX";
  auto dir = mkdtemp(tmpl);
  if (!dir) {
    return "/tmp/krbn_socket_command_handler.sock";
  }
  return std::string(dir) + "/sock";
}

bool bind_and_listen_unix_stream(int& listen_fd, const std::string& path) {
  ::unlink(path.c_str());

  listen_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    return false;
  }

  sockaddr_un addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

  if (::bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
    ::close(listen_fd);
    listen_fd = -1;
    return false;
  }

  if (::listen(listen_fd, 1) != 0) {
    ::close(listen_fd);
    listen_fd = -1;
    return false;
  }

  return true;
}

std::string read_line(int fd) {
  std::string out;
  for (;;) {
    char c = 0;
    auto n = ::read(fd, &c, 1);
    if (n < 0) {
      // Treat timeouts as "no more data".
      return out;
    }
    if (n == 0) {
      return out;
    }
    if (c == '\n') {
      return out;
    }
    out.push_back(c);
  }
}

int accept_with_timeout(int listen_fd, int timeout_ms) {
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(listen_fd, &rfds);

  timeval tv;
  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;

  int r = ::select(listen_fd + 1, &rfds, nullptr, nullptr, &tv);
  if (r <= 0) {
    return -1;
  }
  return ::accept(listen_fd, nullptr, nullptr);
}
} // namespace

int main(void) {
  using namespace boost::ut;

  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();

  "socket_command_handler sends and reuses connection"_test = [] {
    const auto socket_path = make_tmp_socket_path();

    int listen_fd = -1;
    expect(bind_and_listen_unix_stream(listen_fd, socket_path));

    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
    std::string line1;
    std::string line2;

    std::thread server([&] {
      int conn_fd = accept_with_timeout(listen_fd, 2000);
      if (conn_fd >= 0) {
        // Avoid hanging the test if the second write never arrives.
        timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        ::setsockopt(conn_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        line1 = read_line(conn_fd);
        line2 = read_line(conn_fd);
        ::close(conn_fd);
      }

      {
        std::lock_guard<std::mutex> lock(mutex);
        done = true;
      }
      cv.notify_one();
    });

    krbn::console_user_server::socket_command_handler handler;

    handler.run(std::string("{\"endpoint\":\"") + socket_path + "\",\"command\":\"PING1\"}");
    handler.run(std::string("{\"endpoint\":\"") + socket_path + "\",\"command\":\"PING2\"}");

    {
      std::unique_lock<std::mutex> lock(mutex);
      cv.wait_for(lock, std::chrono::seconds(2), [&] { return done; });
    }

    expect(line1 == "PING1");
    expect(line2 == "PING2");

    ::close(listen_fd);
    ::unlink(socket_path.c_str());
    if (auto p = socket_path.find_last_of('/'); p != std::string::npos) {
      ::rmdir(socket_path.substr(0, p).c_str());
    }

    server.join();
  };

  "socket_command_handler reconnects after server restart"_test = [] {
    const auto socket_path = make_tmp_socket_path();

    int listen_fd = -1;
    expect(bind_and_listen_unix_stream(listen_fd, socket_path));

    // Phase 1: send to original server
    std::string phase1_msg;
    std::thread server1([&] {
      int conn_fd = accept_with_timeout(listen_fd, 2000);
      if (conn_fd >= 0) {
        timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        ::setsockopt(conn_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        phase1_msg = read_line(conn_fd);
        ::close(conn_fd);
      }
    });

    krbn::console_user_server::socket_command_handler handler;
    handler.run(std::string("{\"endpoint\":\"") + socket_path + "\",\"command\":\"PHASE1\"}");

    server1.join();
    expect(phase1_msg == "PHASE1");

    // Kill and restart server
    ::close(listen_fd);
    ::unlink(socket_path.c_str());
    listen_fd = -1;
    expect(bind_and_listen_unix_stream(listen_fd, socket_path));

    // Phase 2: handler should reconnect
    std::string phase2_msg;
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;

    std::thread server2([&] {
      int conn_fd = accept_with_timeout(listen_fd, 2000);
      if (conn_fd >= 0) {
        timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        ::setsockopt(conn_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        phase2_msg = read_line(conn_fd);
        ::close(conn_fd);
      }
      {
        std::lock_guard<std::mutex> lock(mutex);
        done = true;
      }
      cv.notify_one();
    });

    handler.run(std::string("{\"endpoint\":\"") + socket_path + "\",\"command\":\"PHASE2\"}");

    {
      std::unique_lock<std::mutex> lock(mutex);
      cv.wait_for(lock, std::chrono::seconds(3), [&] { return done; });
    }

    expect(phase2_msg == "PHASE2");

    ::close(listen_fd);
    ::unlink(socket_path.c_str());
    if (auto p = socket_path.find_last_of('/'); p != std::string::npos) {
      ::rmdir(socket_path.substr(0, p).c_str());
    }

    server2.join();
  };

  return 0;
}
