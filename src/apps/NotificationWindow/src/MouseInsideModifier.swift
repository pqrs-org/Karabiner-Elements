// https://gist.github.com/importRyan/c668904b0c5442b80b6f38a980595031

import SwiftUI

struct MouseInsideModifier: ViewModifier {
  let mouseIsInside: (Bool) -> Void

  init(_ mouseIsInside: @escaping (Bool) -> Void) {
    self.mouseIsInside = mouseIsInside
  }

  func body(content: Content) -> some View {
    content.background(
      GeometryReader { proxy in
        Representable(
          mouseIsInside: mouseIsInside,
          frame: proxy.frame(in: .global))
      }
    )
  }

  private struct Representable: NSViewRepresentable {
    let mouseIsInside: (Bool) -> Void
    let frame: NSRect

    func makeCoordinator() -> Coordinator {
      let coordinator = Coordinator()
      coordinator.mouseIsInside = mouseIsInside
      return coordinator
    }

    class Coordinator: NSResponder {
      var mouseIsInside: ((Bool) -> Void)?

      override func mouseEntered(with _: NSEvent) {
        mouseIsInside?(true)
      }

      override func mouseExited(with _: NSEvent) {
        mouseIsInside?(false)
      }
    }

    func makeNSView(context: Context) -> NSView {
      let view = NSView(frame: frame)

      let options: NSTrackingArea.Options = [
        .mouseEnteredAndExited,
        .inVisibleRect,
        .activeAlways,
      ]

      let trackingArea = NSTrackingArea(
        rect: frame,
        options: options,
        owner: context.coordinator,
        userInfo: nil)

      view.addTrackingArea(trackingArea)

      return view
    }

    func updateNSView(_: NSView, context _: Context) {}

    static func dismantleNSView(_ nsView: NSView, coordinator _: Coordinator) {
      nsView.trackingAreas.forEach { nsView.removeTrackingArea($0) }
    }
  }
}
