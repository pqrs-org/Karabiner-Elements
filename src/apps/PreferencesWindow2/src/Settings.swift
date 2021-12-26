import Combine
import Foundation
import SwiftUI

final class Settings: ObservableObject {
    static let shared = Settings()

    private var coreConfigurationModel: KarabinerKitCoreConfigurationModel!

    init() {
        coreConfigurationModel = KarabinerKitConfigurationManager.shared().coreConfigurationModel
        showMenu = coreConfigurationModel.globalConfigurationShowInMenuBar
    }

    @Published var showMenu: Bool {
        didSet {
            coreConfigurationModel.globalConfigurationShowInMenuBar = showMenu
            coreConfigurationModel.save()
            libkrbn_launch_menu()
        }
    }
}
