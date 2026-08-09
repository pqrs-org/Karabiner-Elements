import Foundation
import SwiftUI

@MainActor
public class SysextdLogMessages {
  public static let shared = SysextdLogMessages()

  let streamer = CommandOutputStreamer()

  public func update() {
    streamer.clear()

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
