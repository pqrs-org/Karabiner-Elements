import SwiftUI

@MainActor
public class SystemExtensions {
  public static let shared = SystemExtensions()

  let streamer = RealtimeCommandStreamer()
  var stream: RealtimeTextStream { streamer.stream }

  public func update() {
    stream.clear()

    streamer.start(
      launchPath: "/usr/bin/systemextensionsctl",
      arguments: [
        "list"
      ],
      environment: [
        "LC_ALL": "C"
      ]
    )
  }
}
