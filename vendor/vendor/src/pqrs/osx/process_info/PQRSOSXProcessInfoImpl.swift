// (C) Copyright Takayama Fumihiko 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

import AppKit

@_cdecl("pqrs_osx_process_info_create_globally_unique_string")
func pqrs_osx_process_info_create_globally_unique_string(
  _ buffer: UnsafeMutablePointer<Int8>,
  _ bufferSize: Int32
) {
  _ = ProcessInfo.processInfo.globallyUniqueString.utf8CString.withUnsafeBufferPointer { ptr in
    strlcpy(buffer, ptr.baseAddress, Int(bufferSize))
  }
}

@_cdecl("pqrs_osx_process_info_process_identifier")
func process_identifier() -> Int32 {
  return ProcessInfo.processInfo.processIdentifier
}

@_cdecl("pqrs_osx_process_info_disable_sudden_termination")
func disable_sudden_termination() {
  ProcessInfo.processInfo.disableSuddenTermination()
}

@_cdecl("pqrs_osx_process_info_enable_sudden_termination")
func enable_sudden_termination() {
  ProcessInfo.processInfo.enableSuddenTermination()
}
