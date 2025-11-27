import SwiftUI

// A stream that emits the text displayed by RealtimeText.
@MainActor
final class RealtimeTextStream {
  // Reusing a single AsyncStream across multiple views can cause new events to stop arriving.
  // (e.g., repeatedly creating/destroying a view)
  // To avoid the issue, we have to create an independent AsyncStream for each view.
  private var continuations: [ObjectIdentifier: AsyncStream<Void>.Continuation] = [:]
  private(set) var text: String = ""

  func makeSignal(for owner: AnyObject) -> AsyncStream<Void> {
    let id = ObjectIdentifier(owner)
    return AsyncStream(bufferingPolicy: .bufferingNewest(1)) { c in
      continuations[id] = c
      c.onTermination = { @Sendable _ in
        Task { @MainActor in
          self.continuations.removeValue(forKey: id)
        }
      }
    }
  }

  func setText(_ s: String) {
    text = s

    for c in continuations.values {
      c.yield(())
    }
  }

  func appendText(_ s: String) {
    text += s

    for c in continuations.values {
      c.yield(())
    }
  }
}

// A view for displaying frequently updated text.
struct RealtimeText: NSViewRepresentable {
  let stream: RealtimeTextStream
  let font: NSFont

  func makeCoordinator() -> Coordinator { Coordinator() }

  func makeNSView(context: Context) -> NSScrollView {
    let tv = NSTextView()
    tv.isEditable = false
    tv.isRichText = false
    // Because the text updates invalidate the selection, disable text selection.
    tv.isSelectable = false
    // Disable background drawing to reduce rendering overhead.
    tv.drawsBackground = false
    tv.font = font
    tv.textContainerInset = CGSize.init(width: 8, height: 8)
    tv.textContainer?.lineFragmentPadding = 0
    // Wrap text.
    tv.textContainer?.widthTracksTextView = true
    tv.isHorizontallyResizable = false
    tv.isVerticallyResizable = true
    // Without this setting, the height collapses to 0 and the text is not rendered on macOS 13.
    tv.autoresizingMask = [.width]

    let scroll = NSScrollView()
    // Disable background drawing to reduce rendering overhead.
    scroll.drawsBackground = false
    scroll.hasVerticalScroller = true
    scroll.hasHorizontalScroller = false
    scroll.autohidesScrollers = true
    scroll.documentView = tv

    context.coordinator.start(textView: tv, stream: stream)

    return scroll
  }

  func updateNSView(_ nsView: NSScrollView, context: Context) {
  }

  static func dismantleNSView(_ nsView: NSScrollView, coordinator: Coordinator) {
    coordinator.stop()
  }

  @MainActor
  final class Coordinator {
    private var task: Task<Void, Never>?
    private weak var textView: NSTextView?

    func start(textView: NSTextView, stream: RealtimeTextStream) {
      stop()

      self.textView = textView
      textView.string = stream.text

      task = Task { [weak textView] in
        guard let textView else { return }

        let signal = stream.makeSignal(for: self)

        for await _ in signal {
          textView.string = stream.text

          if Task.isCancelled {
            break
          }
        }
      }
    }

    func stop() {
      task?.cancel()
      task = nil
      textView = nil
    }

    deinit {
      task?.cancel()
    }
  }
}
