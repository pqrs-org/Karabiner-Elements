import Foundation
import SwiftUI

@MainActor
public class SysextdLogMessages {
  public static let shared = SysextdLogMessages()

  let streamer = RealtimeCommandStreamer()
  var stream: RealtimeTextStream { streamer.stream }

  public func update() {
    stream.clear()

    streamer.start(
      launchPath: "/usr/bin/log",
      arguments: [
        "show",
        "--predicate", "sender == \"sysextd\" or sender CONTAINS \"org.pqrs\"",
        "--info",
        "--debug",
        "--signpost",
        "--loss",
        "--last", "2h",
      ],
      environment: [
        "LC_ALL": "C"
      ]
    )
  }
}
