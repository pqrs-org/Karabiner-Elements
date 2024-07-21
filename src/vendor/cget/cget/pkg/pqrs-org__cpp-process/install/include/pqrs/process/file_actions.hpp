#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <spawn.h>

namespace pqrs {
namespace process {
class file_actions final {
public:
  file_actions(void) {
    posix_spawn_file_actions_init(&actions_);
  }

  ~file_actions(void) {
    posix_spawn_file_actions_destroy(&actions_);
  }

  posix_spawn_file_actions_t* get_actions(void) {
    return &actions_;
  }

  int addclose(int file_descriptor) {
    return posix_spawn_file_actions_addclose(&actions_,
                                             file_descriptor);
  }

  int addopen(int file_descriptor, const char* path, int flag, mode_t mode) {
    return posix_spawn_file_actions_addopen(&actions_,
                                            file_descriptor,
                                            path,
                                            flag,
                                            mode);
  }

  int adddup2(int file_descriptor, int newfiledes) {
    return posix_spawn_file_actions_adddup2(&actions_,
                                            file_descriptor,
                                            newfiledes);
  }

  int addinherit_np(int file_descriptor) {
    return posix_spawn_file_actions_addinherit_np(&actions_,
                                                  file_descriptor);
  }

private:
  posix_spawn_file_actions_t actions_;
};
} // namespace process
} // namespace pqrs
