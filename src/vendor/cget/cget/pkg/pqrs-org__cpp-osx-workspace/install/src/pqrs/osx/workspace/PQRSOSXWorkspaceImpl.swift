// (C) Copyright Takayama Fumihiko 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

import AppKit

@_cdecl("pqrs_osx_workspace_find_application_url_by_bundle_identifier")
func pqrs_osx_workspace_find_application_url_by_bundle_identifier(
  _ bundleIdentifierPtr: UnsafePointer<Int8>,
  _ buffer: UnsafeMutablePointer<Int8>,
  _ bufferSize: Int32
) {
  let bundleIdentifier = String(cString: bundleIdentifierPtr)
  let urlString =
    NSWorkspace.shared.urlForApplication(withBundleIdentifier: bundleIdentifier)?.absoluteString
    ?? ""

  _ = urlString.utf8CString.withUnsafeBufferPointer { ptr in
    strlcpy(buffer, ptr.baseAddress, Int(bufferSize))
  }
}
