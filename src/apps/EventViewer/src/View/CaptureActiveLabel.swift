import Foundation
import SwiftUI

struct CaptureActiveLabel: View {
  let text: String

  @Environment(\.accessibilityReduceMotion) private var reduceMotion

  var body: some View {
    Label {
      Text(text)
    } icon: {
      // Do not use animation(_:value:) with repeatForever here. On the initial
      // capture, its transaction can also animate layout and focus changes,
      // causing the label to move. TimelineView updates only the opacity.
      TimelineView(.animation(minimumInterval: 1.0 / 30.0, paused: reduceMotion)) { context in
        Image(systemName: "circle.fill")
          .opacity(indicatorOpacity(at: context.date))
      }
    }
    .foregroundStyle(.green)
  }

  private func indicatorOpacity(at date: Date) -> Double {
    if reduceMotion {
      return 1
    }

    let fullCycleDuration = 2.0
    let phase =
      date.timeIntervalSinceReferenceDate
      .truncatingRemainder(dividingBy: fullCycleDuration) / fullCycleDuration
    let dimmingProgress = (1 - cos(2 * .pi * phase)) / 2
    return 1 - 0.65 * dimmingProgress
  }
}

struct CaptureWaitingForDeviceAccessLabel: View {
  var body: some View {
    HStack(spacing: 6) {
      ProgressView()
        .controlSize(.small)

      Text("Waiting for device access...")
        .lineLimit(1)
        .fixedSize(horizontal: true, vertical: false)
    }
    .foregroundStyle(.secondary)
  }
}
