// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#import <Foundation/Foundation.h>

void pqrs_osx_process_info_create_globally_unique_string(char* buffer, size_t buffer_size) {
  strlcpy(buffer, NSProcessInfo.processInfo.globallyUniqueString.UTF8String, buffer_size);
}

int pqrs_osx_process_info_process_identifier(void) {
  return NSProcessInfo.processInfo.processIdentifier;
}

void pqrs_osx_process_info_disable_sudden_termination(void) {
  [NSProcessInfo.processInfo disableSuddenTermination];
}

void pqrs_osx_process_info_enable_sudden_termination(void) {
  [NSProcessInfo.processInfo enableSuddenTermination];
}
