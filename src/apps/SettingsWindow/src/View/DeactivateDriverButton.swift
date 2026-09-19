import SwiftUI

struct DeactivateDriverButton: View {
  @State private var showingProgress: Bool = false
  @State private var showingResult: Bool = false
  @State private var status: Int32 = 0

  var body: some View {
    VStack(alignment: .leading) {
      Button(
        action: {
          self.showingProgress = true
          self.showingResult = false

          VirtualHIDDeviceManager.shared.deactivateDriver(completion: { status in
            Task { @MainActor in
              self.status = status
              self.showingProgress = false
              self.showingResult = true
            }
          })
        },
        label: {
          AppLocalizedLabel(
            "settings.setup.driver.deactivate",
            systemImage: "star.fill")
        })

      if self.showingProgress {
        AppLocalizedText("settings.setup.driver.deactivating")
          .padding(.bottom, 20)
      }

      if self.showingResult {
        VStack(alignment: .leading) {
          if self.status == 0 {
            AppLocalizedText("settings.setup.driver.deactivated")
              .bold()
          } else {
            AppLocalizedText(
              "settings.setup.driver.deactivation_failed", arguments: ["status": String(self.status)]
            )
            .bold()
          }
        }.padding(.bottom, 20)
      }
    }
  }
}
