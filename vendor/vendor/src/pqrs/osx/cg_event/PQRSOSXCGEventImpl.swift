// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

import AppKit
import IOKit.hidsystem

@_cdecl("pqrs_osx_cg_event_get_aux_control_button")
public func pqrs_osx_cg_event_get_aux_control_button(
  _ event: CGEvent,
  _ value: UnsafeMutablePointer<UInt16>?
) -> Bool {
  guard
    let value,
    let nsEvent = NSEvent(cgEvent: event),
    nsEvent.type == .systemDefined,
    nsEvent.subtype.rawValue == Int16(NX_SUBTYPE_AUX_CONTROL_BUTTONS)
  else {
    return false
  }

  let data1 = UInt32(bitPattern: Int32(nsEvent.data1))
  value.pointee = UInt16((data1 & 0xffff_0000) >> 16)

  return true
}
