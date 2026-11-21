import AppKit
import QuartzCore
import SwiftUI

@MainActor
final class TransientStatusPanel {
  private let panel = NSPanel(
    contentRect: .zero,
    styleMask: [.borderless, .nonactivatingPanel],
    backing: .buffered,
    defer: true)
  private var hostingController: NSHostingController<TransientStatusPanelContent>?
  private var presentationID: UUID?

  init() {
    panel.isOpaque = false
    panel.backgroundColor = .clear
    panel.hasShadow = true
    panel.hidesOnDeactivate = false
    panel.ignoresMouseEvents = true
    panel.collectionBehavior = [.transient, .ignoresCycle]
  }

  func update(
    presentationID: UUID?,
    message: String,
    systemImage: String,
    relativeTo view: NSView
  ) {
    let shouldRestartPresentation = self.presentationID != presentationID
    self.presentationID = presentationID

    let content = TransientStatusPanelContent(message: message, systemImage: systemImage)
    if let hostingController {
      hostingController.rootView = content
    } else {
      let hostingController = NSHostingController(rootView: content)
      self.hostingController = hostingController
      panel.contentViewController = hostingController
    }

    guard presentationID != nil else {
      dismissPanel()
      return
    }

    guard shouldRestartPresentation else { return }

    panel.alphaValue = 0

    DispatchQueue.main.async { [weak self, weak view] in
      guard
        let self,
        let view,
        self.presentationID == presentationID,
        let window = view.window,
        let hostingController = self.hostingController
      else { return }

      hostingController.view.layoutSubtreeIfNeeded()
      let fittingSize = hostingController.view.fittingSize
      let panelSize = NSSize(
        width: ceil(fittingSize.width),
        height: ceil(fittingSize.height))

      let viewRectInWindow = view.convert(view.bounds, to: nil)
      let viewRectOnScreen = window.convertToScreen(viewRectInWindow)
      var origin = NSPoint(
        x: viewRectOnScreen.minX - panelSize.width - 8,
        y: viewRectOnScreen.midY - panelSize.height / 2)

      if let visibleFrame = window.screen?.visibleFrame {
        origin.x = max(visibleFrame.minX + 8, origin.x)
        origin.y = min(
          max(visibleFrame.minY + 8, origin.y),
          visibleFrame.maxY - panelSize.height - 8)
      }

      self.panel.setFrame(NSRect(origin: origin, size: panelSize), display: true)
      if self.panel.parent !== window {
        self.panel.parent?.removeChildWindow(self.panel)
        window.addChildWindow(self.panel, ordered: .above)
      }

      self.panel.orderFront(nil)
      self.animate(for: presentationID)
    }
  }

  func dismiss() {
    presentationID = nil
    dismissPanel()
  }

  private func animate(for presentationID: UUID?) {
    guard self.presentationID == presentationID else { return }

    NSAnimationContext.runAnimationGroup { context in
      context.duration = 0.15
      context.timingFunction = CAMediaTimingFunction(name: .easeInEaseOut)
      panel.animator().alphaValue = 1
    } completionHandler: { [weak self] in
      guard let self, self.presentationID == presentationID else { return }

      NSAnimationContext.runAnimationGroup { context in
        context.duration = 1.5
        context.timingFunction = CAMediaTimingFunction(name: .easeInEaseOut)
        self.panel.animator().alphaValue = 0
      }
    }
  }

  private func dismissPanel() {
    panel.parent?.removeChildWindow(panel)
    panel.orderOut(nil)
    panel.alphaValue = 1
  }
}

private struct TransientStatusPanelContent: View {
  let message: String
  let systemImage: String

  var body: some View {
    Label(message, systemImage: systemImage)
      .padding(.horizontal, 10)
      .padding(.vertical, 7)
      .background(
        Color(nsColor: .windowBackgroundColor),
        in: RoundedRectangle(cornerRadius: 8)
      )
      .overlay {
        RoundedRectangle(cornerRadius: 8)
          .stroke(Color(nsColor: .separatorColor), lineWidth: 0.5)
      }
      .fixedSize()
  }
}
