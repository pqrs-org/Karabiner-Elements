import Combine
import Foundation
import SwiftUI

final class Settings: ObservableObject {
    static let shared = Settings()

    private var coreConfigurationModel: KarabinerKitCoreConfigurationModel!

    init() {
        coreConfigurationModel = KarabinerKitConfigurationManager.shared().coreConfigurationModel

        showIconInMenuBar = coreConfigurationModel.globalConfigurationShowInMenuBar
        showProfileNameInMenuBar = coreConfigurationModel.globalConfigurationShowProfileNameInMenuBar
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
}
