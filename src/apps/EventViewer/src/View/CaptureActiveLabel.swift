import AppKit
import QuartzCore
import SwiftUI

struct CaptureActiveLabel: View {
  let text: String

  @Environment(\.accessibilityReduceMotion) private var reduceMotion

  var body: some View {
    Label {
      Text(text)
    } icon: {
      // Keep the pulse on its own layer. Updating a TimelineView every frame can
      // repeatedly trigger SwiftUI layout on macOS 13, even without input events.
      CaptureActivityIndicator(animate: !reduceMotion)
        .frame(width: 10, height: 10)
        .accessibilityHidden(true)
    }
    .foregroundStyle(.green)
  }
}

private struct CaptureActivityIndicator: NSViewRepresentable {
  let animate: Bool

  func makeNSView(context: Context) -> IndicatorView {
    IndicatorView()
  }

  func updateNSView(_ view: IndicatorView, context: Context) {
    view.animate = animate
  }

  static func dismantleNSView(_ view: IndicatorView, coordinator: ()) {
    view.animate = false
  }

  final class IndicatorView: NSView {
    private static let pulseKey = "capturePulse"

    var animate = false {
      didSet { updateAnimation() }
    }

    override init(frame frameRect: NSRect) {
      super.init(frame: frameRect)
      wantsLayer = true
      updateColor()
    }

    required init?(coder: NSCoder) {
      fatalError("init(coder:) has not been implemented")
    }

    override func layout() {
      super.layout()
      CATransaction.begin()
      CATransaction.setDisableActions(true)
      layer?.cornerRadius = min(bounds.width, bounds.height) / 2
      CATransaction.commit()
    }

    override func viewDidMoveToWindow() {
      super.viewDidMoveToWindow()
      updateAnimation()
    }

    override func viewDidChangeEffectiveAppearance() {
      super.viewDidChangeEffectiveAppearance()
      updateColor()
    }

    private func updateColor() {
      effectiveAppearance.performAsCurrentDrawingAppearance {
        layer?.backgroundColor = NSColor.systemGreen.cgColor
      }
    }

    private func updateAnimation() {
      guard let layer else { return }
      guard animate, window != nil else {
        // The model opacity stays at 1, so removing the animation leaves a solid dot.
        layer.removeAnimation(forKey: Self.pulseKey)
        return
      }
      guard layer.animation(forKey: Self.pulseKey) == nil else { return }

      let pulse = CABasicAnimation(keyPath: "opacity")
      pulse.fromValue = 1.0
      pulse.toValue = 0.35
      pulse.duration = 1.0
      pulse.autoreverses = true
      pulse.repeatCount = .infinity
      pulse.timingFunction = CAMediaTimingFunction(name: .easeInEaseOut)
      layer.add(pulse, forKey: Self.pulseKey)
    }
  }
}

struct CaptureWaitingForDeviceAccessLabel: View {
  var body: some View {
    HStack(spacing: 6) {
      ProgressView()
        .controlSize(.small)

      AppLocalizedText("event_viewer.capture.waiting")
    }
    .foregroundStyle(.secondary)
  }
}
