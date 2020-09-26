import Cocoa

@NSApplicationMain
public class AppDelegate: NSObject, NSApplicationDelegate, NSTabViewDelegate {
  @IBOutlet var window: NSWindow!
  @IBOutlet var eventQueue: EventQueue!
  @IBOutlet var keyResponder: KeyResponder!
  @IBOutlet var frontmostApplicationController: FrontmostApplicationController!
  @IBOutlet var variablesController: VariablesController!
  @IBOutlet var devicesController: DevicesController!
  @IBOutlet var inputMonitoringAlertWindowController: InputMonitoringAlertWindowController!

  public func applicationDidFinishLaunching(_: Notification) {
    libkrbn_initialize()

    setKeyResponder()
    setWindowProperty(self)
    eventQueue.setup()
    frontmostApplicationController.setup()
    variablesController.setup()
    devicesController.setup()

    DispatchQueue.main.asyncAfter(deadline: .now() + 3.0) { [weak self] in
      guard let self = self else { return }

      if !self.eventQueue.observed() {
        self.inputMonitoringAlertWindowController.show()
      }
    }
  }

  // Note:
  // We have to set NSSupportsSuddenTermination `NO` to use `applicationWillTerminate`.
  public func applicationWillTerminate(_: Notification) {
    libkrbn_terminate()
  }

  public func applicationShouldTerminateAfterLastWindowClosed(_: NSApplication) -> Bool {
    return true
  }

  public func tabView(_ tabView: NSTabView, didSelect _: NSTabViewItem?) {
    if tabView.identifier?.rawValue == "Main" {
      setKeyResponder()
    }
  }

  func setKeyResponder() {
    window.makeFirstResponder(keyResponder)
  }

  @IBAction func setWindowProperty(_: Any) {
    // ----------------------------------------
    if UserDefaults.standard.bool(forKey: "kForceStayTop") {
      window.level = .floating
    } else {
      window.level = .normal
    }

    // ----------------------------------------
    if UserDefaults.standard.bool(forKey: "kShowInAllSpaces") {
      window.collectionBehavior.insert(.canJoinAllSpaces)
    } else {
      window.collectionBehavior.remove(.canJoinAllSpaces)
    }

    window.collectionBehavior.insert(.managed)
    window.collectionBehavior.remove(.moveToActiveSpace)
    window.collectionBehavior.remove(.transient)
  }
}
