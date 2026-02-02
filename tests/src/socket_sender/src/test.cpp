#include "socket_sender.hpp"
#include <boost/ut.hpp>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

std::string make_tmp_socket_path(const char* tag) {
  std::string tmpl = std::string("/tmp/krbn_socket_sender_") + tag + ".XXXXXX";
  std::vector<char> buf(tmpl.begin(), tmpl.end());
  buf.push_back('\0');
  auto dir = mkdtemp(buf.data());
  if (!dir) {
    return std::string("/tmp/krbn_socket_sender_") + tag + ".sock";
  }
  return std::string(dir) + "/sock";
}

void cleanup_socket_path(const std::string& path) {
  ::unlink(path.c_str());
  if (auto p = path.find_last_of('/'); p != std::string::npos) {
    ::rmdir(path.substr(0, p).c_str());
  }
}

int make_server(const std::string& path) {
  ::unlink(path.c_str());

  int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) return -1;

  sockaddr_un addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

  if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 ||
      ::listen(fd, 4) != 0) {
    ::close(fd);
    return -1;
  }
  return fd;
}

int accept_with_timeout(int listen_fd, int timeout_ms) {
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(listen_fd, &rfds);
  timeval tv;
  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;
  if (::select(listen_fd + 1, &rfds, nullptr, nullptr, &tv) <= 0) {
    return -1;
  }
  return ::accept(listen_fd, nullptr, nullptr);
}

std::string read_line(int fd) {
  std::string out;
  for (;;) {
    char c = 0;
    auto n = ::read(fd, &c, 1);
    if (n <= 0 || c == '\n') return out;
    out.push_back(c);
  }
}

} // namespace

int main(void) {
  using namespace boost::ut;

  "basic send"_test = [] {
    const auto path = make_tmp_socket_path("basic");
    int listen_fd = make_server(path);
    expect(listen_fd >= 0);

    std::string received;
    std::thread server([&] {
      int conn = accept_with_timeout(listen_fd, 2000);
      if (conn >= 0) {
        timeval tv{2, 0};
        ::setsockopt(conn, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        received = read_line(conn);
        ::close(conn);
      }
    });

    krbn::socket_sender sender;
    expect(sender.send(path, "HELLO"));

    server.join();
    expect(received == "HELLO");

    ::close(listen_fd);
    cleanup_socket_path(path);
  };

  "persistent connection reuse"_test = [] {
    const auto path = make_tmp_socket_path("reuse");
    int listen_fd = make_server(path);
    expect(listen_fd >= 0);

    std::string line1, line2;
    std::thread server([&] {
      // Expect both messages on a single connection
      int conn = accept_with_timeout(listen_fd, 2000);
      if (conn >= 0) {
        timeval tv{2, 0};
        ::setsockopt(conn, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        line1 = read_line(conn);
        line2 = read_line(conn);
        ::close(conn);
      }
    });

    krbn::socket_sender sender;
    expect(sender.send(path, "MSG1"));
    expect(sender.send(path, "MSG2"));

    server.join();
    expect(line1 == "MSG1");
    expect(line2 == "MSG2");

    ::close(listen_fd);
    cleanup_socket_path(path);
  };

  "reconnect after server restart"_test = [] {
    const auto path = make_tmp_socket_path("reconn");
    int listen_fd = make_server(path);
    expect(listen_fd >= 0);

    // Send first message to original server
    std::string first_msg;
    std::thread server1([&] {
      int conn = accept_with_timeout(listen_fd, 2000);
      if (conn >= 0) {
        timeval tv{2, 0};
        ::setsockopt(conn, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        first_msg = read_line(conn);
        ::close(conn);
      }
    });

    krbn::socket_sender sender;
    expect(sender.send(path, "BEFORE"));
    server1.join();
    expect(first_msg == "BEFORE");

    // Kill and restart server
    ::close(listen_fd);
    ::unlink(path.c_str());
    listen_fd = make_server(path);
    expect(listen_fd >= 0);

    // Send second message — should reconnect automatically
    std::string second_msg;
    std::thread server2([&] {
      int conn = accept_with_timeout(listen_fd, 2000);
      if (conn >= 0) {
        timeval tv{2, 0};
        ::setsockopt(conn, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        second_msg = read_line(conn);
        ::close(conn);
      }
    });

    expect(sender.send(path, "AFTER"));
    server2.join();
    expect(second_msg == "AFTER");

    ::close(listen_fd);
    cleanup_socket_path(path);
  };

  "fails gracefully when server not running"_test = [] {
    krbn::socket_sender sender;
    expect(!sender.send("/tmp/krbn_socket_sender_nonexistent.sock", "NOPE"));
  };

  "rejects too-long endpoint path"_test = [] {
    krbn::socket_sender sender;
    std::string long_path(200, 'x');
    expect(!sender.send(long_path, "NOPE"));
  };

  return 0;
}
