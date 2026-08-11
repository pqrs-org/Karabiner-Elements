import AppKit

func completePendingApplicationTermination() {
  Task { @MainActor in
    // `requestApplicationTermination` returned `.terminateLater`, so complete
    // that pending request. Calling `terminate` here would start a new request
    // and invoke `applicationShouldTerminate` again.
    NSApplication.shared.reply(toApplicationShouldTerminate: true)
  }
}

@MainActor
func requestApplicationTermination(
  _ requestTermination: () -> Bool
) -> NSApplication.TerminateReply {
  // Keep the main run loop active while the C++ components are being stopped.
  // Some monitors dispatch synchronously to the main queue during cleanup.
  if requestTermination() {
    return .terminateLater
  }

  return .terminateNow
}
