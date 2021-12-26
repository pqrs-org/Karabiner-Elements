import Combine
import Foundation
import SwiftUI

final class Settings: ObservableObject {
    static let shared = Settings()

    private var coreConfigurationModel: KarabinerKitCoreConfigurationModel!

    init() {
        coreConfigurationModel = KarabinerKitConfigurationManager.shared().coreConfigurationModel

        checkForUpdatesOnStartup = coreConfigurationModel.globalConfigurationCheckForUpdatesOnStartup
        showIconInMenuBar = coreConfigurationModel.globalConfigurationShowInMenuBar
        showProfileNameInMenuBar = coreConfigurationModel.globalConfigurationShowProfileNameInMenuBar

        updateSystemDefaultProfileExists()
    }

    @Published var checkForUpdatesOnStartup: Bool {
        didSet {
            coreConfigurationModel.globalConfigurationCheckForUpdatesOnStartup = checkForUpdatesOnStartup
            coreConfigurationModel.save()
        }
    }

    @Published var showIconInMenuBar: Bool {
        didSet {
            coreConfigurationModel.globalConfigurationShowInMenuBar = showIconInMenuBar
            coreConfigurationModel.save()
            libkrbn_launch_menu()
        }
    }

    @Published var showProfileNameInMenuBar: Bool {
        didSet {
            coreConfigurationModel.globalConfigurationShowProfileNameInMenuBar = showProfileNameInMenuBar
            coreConfigurationModel.save()
            libkrbn_launch_menu()
        }
    }

    @Published var systemDefaultProfileExists: Bool = false
    private func updateSystemDefaultProfileExists() {
        systemDefaultProfileExists = libkrbn_system_core_configuration_file_path_exists()
    }

    func installSystemDefaultProfile() {
        // Ensure karabiner.json exists before copy.
        coreConfigurationModel.save()

        let url = URL(fileURLWithPath: "/Library/Application Support/org.pqrs/Karabiner-Elements/scripts/copy_current_profile_to_system_default_profile.applescript")
        guard let script = NSAppleScript(contentsOf: url, error: nil) else { return }
        script.executeAndReturnError(nil)

        updateSystemDefaultProfileExists()
    }

    func removeSystemDefaultProfile() {
        let url = URL(fileURLWithPath: "/Library/Application Support/org.pqrs/Karabiner-Elements/scripts/remove_system_default_profile.applescript")
        guard let script = NSAppleScript(contentsOf: url, error: nil) else { return }
        script.executeAndReturnError(nil)

        updateSystemDefaultProfileExists()
    }
}
