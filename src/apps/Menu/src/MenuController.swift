public class MenuController: NSObject, NSMenuDelegate {
    static let shared = MenuController()

    var menu: NSMenu!
    var statusItem: NSStatusItem?
    var menuIcon: NSImage?
    var observers: KarabinerKitSmartObserverContainer?

    override public init() {
        super.init()
        menu = NSMenu(title: "Karabiner-Elements")
    }

    public func setup() {
        terminateIfHidden()

        menu.delegate = self

        menuIcon = NSImage(named: "MenuIcon")

        statusItem = NSStatusBar.system.statusItem(
            withLength: NSStatusItem.variableLength
        )
        statusItem?.button?.font = NSFont.systemFont(ofSize: NSFont.smallSystemFontSize)
        statusItem?.menu = menu

        setStatusItemImage()
        setStatusItemTitle()

        observers = KarabinerKitSmartObserverContainer()

        let center = NotificationCenter.default
        let o = center.addObserver(forName: NSNotification.Name(rawValue: kKarabinerKitConfigurationIsLoaded), object: nil, queue: .main) { [weak self] _ in
            guard let self = self else { return }

            self.terminateIfHidden()
            self.setStatusItemImage()
            self.setStatusItemTitle()
        }
        observers?.addObserver(o, notificationCenter: center)
    }

    func terminateIfHidden() {
        var terminate = false

        if let coreConfigurationModel = KarabinerKitConfigurationManager.shared()?.coreConfigurationModel {
            if !coreConfigurationModel.globalConfigurationShowInMenuBar,
               !coreConfigurationModel.globalConfigurationShowProfileNameInMenuBar
            {
                terminate = true
            }
        }

        if terminate {
            NSApplication.shared.terminate(self)
        }
    }

    func setStatusItemImage() {
        var showImage = false

        if let coreConfigurationModel = KarabinerKitConfigurationManager.shared().coreConfigurationModel {
            if coreConfigurationModel.globalConfigurationShowInMenuBar {
                showImage = true
            }
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

        if let coreConfigurationModel = KarabinerKitConfigurationManager.shared().coreConfigurationModel {
            if coreConfigurationModel.globalConfigurationShowProfileNameInMenuBar {
                title = coreConfigurationModel.selectedProfileName
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

        let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as! String
        menu.addItem(withTitle: "Karabiner-Elements \(version)",
                     action: nil,
                     keyEquivalent: "")

        // Profiles

        menu.addItem(NSMenuItem.separator())
        menu.addItem(withTitle: "Profiles",
                     action: nil,
                     keyEquivalent: "")

        if let coreConfigurationModel = KarabinerKitConfigurationManager.shared().coreConfigurationModel {
            for i in 0 ..< coreConfigurationModel.profilesCount {
                let newItem = NSMenuItem(title: coreConfigurationModel.profileName(at: i),
                                         action: #selector(profileSelected),
                                         keyEquivalent: "")

                newItem.target = self
                newItem.representedObject = UInt(i)
                newItem.indentationLevel = 1

                if coreConfigurationModel.profileSelected(at: i) {
                    newItem.state = .on
                } else {
                    newItem.state = .off
                }

                menu.addItem(newItem)
            }
        }

        // Others

        menu.addItem(NSMenuItem.separator())
        do {
            let newItem = NSMenuItem(title: "Preferences...",
                                     action: #selector(openPreferences),
                                     keyEquivalent: "")
            newItem.target = self
            menu.addItem(newItem)
        }
        do {
            let newItem = NSMenuItem(title: "Launch EventViewer...",
                                     action: #selector(launchEventViewer),
                                     keyEquivalent: "")
            newItem.target = self
            menu.addItem(newItem)
        }

        // Quit

        menu.addItem(NSMenuItem.separator())
        do {
            let newItem = NSMenuItem(title: "Quit Karabiner-Elements",
                                     action: #selector(quitKarabiner),
                                     keyEquivalent: "")
            newItem.target = self
            menu.addItem(newItem)
        }
    }

    @objc
    func profileSelected(sender: NSMenuItem) {
        let index = sender.representedObject as! UInt
        if let coreConfigurationModel = KarabinerKitConfigurationManager.shared().coreConfigurationModel {
            coreConfigurationModel.selectProfile(at: index)
            coreConfigurationModel.save()

            setStatusItemTitle()
        }
    }

    @objc
    func openPreferences(_: Any) {
        libkrbn_launch_preferences()
    }

    @objc
    func launchEventViewer(_: Any) {
        libkrbn_launch_event_viewer()
    }

    @objc
    func quitKarabiner(_: Any) {
        KarabinerKit.quitKarabinerWithConfirmation()
    }
}
