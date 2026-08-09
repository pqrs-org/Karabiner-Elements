import SwiftUI

@MainActor
public class SystemExtensions {
  public static let shared = SystemExtensions()

  let streamer = CommandOutputStreamer()

  public func update() {
    streamer.clear()

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
