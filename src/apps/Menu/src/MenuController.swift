public class MenuController: NSObject, NSMenuDelegate {
    @IBOutlet var menu: NSMenu!
    var statusItem: NSStatusItem?
    var menuIcon: NSImage?
    var observers: KarabinerKitSmartObserverContainer?

    public func setup() {
        terminateIfHidden()

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

        while let item = menu.item(at: 0) {
            if item.title == "endoflist" {
                break
            }

            menu.removeItem(at: 0)
        }

        //
        // Append items
        //

        if let coreConfigurationModel = KarabinerKitConfigurationManager.shared().coreConfigurationModel {
            for i in 0 ..< coreConfigurationModel.profilesCount {
                let newItem = NSMenuItem(title: coreConfigurationModel.profileName(at: i),
                                         action: #selector(profileSelected),
                                         keyEquivalent: "")

                newItem.target = self
                newItem.representedObject = UInt(i)

                if coreConfigurationModel.profileSelected(at: i) {
                    newItem.state = .on
                } else {
                    newItem.state = .off
                }

                menu.insertItem(newItem, at: Int(i))
            }
        }
    }

    @objc func profileSelected(sender: NSMenuItem) {
        let index = sender.representedObject as! UInt
        if let coreConfigurationModel = KarabinerKitConfigurationManager.shared().coreConfigurationModel {
            coreConfigurationModel.selectProfile(at: index)
            coreConfigurationModel.save()

            setStatusItemTitle()
        }
    }

    @IBAction func openPreferences(_: Any) {
        libkrbn_launch_preferences()
    }

    @IBAction func checkForUpdates(_: Any) {
        libkrbn_check_for_updates_stable_only()
    }

    @IBAction func launchEventViewer(_: Any) {
        libkrbn_launch_event_viewer()
    }

    @IBAction func quitKarabiner(_: Any) {
        KarabinerKit.quitKarabinerWithConfirmation()
    }
}
