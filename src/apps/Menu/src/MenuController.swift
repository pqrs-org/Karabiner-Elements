import AppKit

public class MenuController: NSObject, NSMenuDelegate {
  static let shared = MenuController()

  var menu: NSMenu!
  var statusItem: NSStatusItem?
  var menuIcon: NSImage?

  override public init() {
    super.init()
    menu = NSMenu(title: "Karabiner-Elements")
  }

  public func setup() {
    menu.delegate = self

    menuIcon = NSImage(named: "MenuIcon")

    statusItem = NSStatusBar.system.statusItem(
      withLength: NSStatusItem.variableLength
    )
    statusItem?.button?.font = NSFont.systemFont(ofSize: NSFont.smallSystemFontSize)
    statusItem?.menu = menu

    NotificationCenter.default.addObserver(
      forName: LibKrbn.Settings.didConfigurationLoad,
      object: nil,
      queue: .main
    ) { [weak self] _ in
      guard let self = self else { return }

      self.terminateIfHidden()
      self.setStatusItemImage()
      self.setStatusItemTitle()
    }

    LibKrbn.Settings.shared.start()
  }

  func terminateIfHidden() {
    if !LibKrbn.Settings.shared.showIconInMenuBar,
      !LibKrbn.Settings.shared.showProfileNameInMenuBar
    {
      NSApplication.shared.terminate(self)
    }
  }

  func setStatusItemImage() {
    var showImage = false

    if LibKrbn.Settings.shared.showIconInMenuBar {
      showImage = true
    }

    if showImage {
      statusItem?.button?.imagePosition = .imageLeft
      statusItem?.button?.image = menuIcon
    } else {
      statusItem?.button?.image = nil
    }
  }

  func setStatusItemTitle() {
    var title = ""

    if LibKrbn.Settings.shared.showProfileNameInMenuBar {
      if LibKrbn.Settings.shared.showIconInMenuBar {
        // Add padding
        title += " "
      }

      LibKrbn.Settings.shared.profiles.forEach { profile in
        if profile.selected {
          title += profile.name
        }
      }
    }

    statusItem?.button?.title = title

    if title == "" {
      statusItem?.button?.imagePosition = .imageOnly
    } else {
      statusItem?.button?.imagePosition = .imageLeft
    }
  }

  public func menuNeedsUpdate(_ menu: NSMenu) {
    //
    // Clear existing items
    //

    menu.removeAllItems()

    //
    // Append items
    //

    let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as? String ?? ""
    menu.addItem(
      withTitle: "Karabiner-Elements \(version)",
      action: nil,
      keyEquivalent: "")

    // Profiles

    menu.addItem(NSMenuItem.separator())

    do {
      let newItem = NSMenuItem(
        title: "Profiles",
        action: nil,
        keyEquivalent: "")

      newItem.image = NSImage(
        systemSymbolName: "person.3",
        accessibilityDescription: nil)

      menu.addItem(newItem)
    }

    LibKrbn.Settings.shared.profiles.forEach { profile in
      let newItem = NSMenuItem(
        title: profile.name,
        action: #selector(profileSelected),
        keyEquivalent: "")

      newItem.target = self
      newItem.representedObject = profile.id
      newItem.indentationLevel = 1

      if profile.selected {
        newItem.state = .on
      } else {
        newItem.state = .off
      }

      menu.addItem(newItem)
    }

    // Others

    menu.addItem(NSMenuItem.separator())

    do {
      let newItem = NSMenuItem(
        title: "Settings...",
        action: #selector(openSettings),
        keyEquivalent: "")

      newItem.image = NSImage(
        systemSymbolName: "gearshape",
        accessibilityDescription: nil)

      newItem.target = self
      menu.addItem(newItem)
    }
    do {
      let newItem = NSMenuItem(
        title: "Launch EventViewer...",
        action: #selector(launchEventViewer),
        keyEquivalent: "")

      newItem.image = NSImage(
        systemSymbolName: "magnifyingglass",
        accessibilityDescription: nil)

      newItem.target = self
      menu.addItem(newItem)
    }

    // Restart

    menu.addItem(NSMenuItem.separator())

    do {
      let newItem = NSMenuItem(
        title: "Restart Karabiner-Elements",
        action: #selector(restartKarabiner),
        keyEquivalent: "")

      newItem.image = NSImage(
        systemSymbolName: "arrow.clockwise",
        accessibilityDescription: nil)

      newItem.target = self
      menu.addItem(newItem)
    }

    // Quit

    do {
      let newItem = NSMenuItem(
        title: "Quit Karabiner-Elements",
        action: #selector(quitKarabiner),
        keyEquivalent: "")

      newItem.image = NSImage(
        systemSymbolName: "xmark.circle.fill",
        accessibilityDescription: nil)

      newItem.target = self
      menu.addItem(newItem)
    }
  }

  @objc
  func profileSelected(sender: NSMenuItem) {
    if let id = sender.representedObject as? UUID {
      LibKrbn.Settings.shared.profiles.forEach { profile in
        if id == profile.id {
          LibKrbn.Settings.shared.selectProfile(profile)
        }
      }

      setStatusItemTitle()
    }
  }

  @objc
  func openSettings(_: Any) {
    libkrbn_launch_settings()
  }

  @objc
  func launchEventViewer(_: Any) {
    libkrbn_launch_event_viewer()
  }

  @objc
  func restartKarabiner(_: Any) {
    libkrbn_launchctl_restart_console_user_server()
  }

  @objc
  func quitKarabiner(_: Any) {
    KarabinerAppHelper.shared.quitKarabiner(
      askForConfirmation: LibKrbn.Settings.shared.askForConfirmationBeforeQuitting)
  }
}
