import SwiftUI

struct SettingsAlertView: View {
  @ObservedObject private var settingsChecker = SettingsChecker.shared
  @FocusState var focus: Bool

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .leading, spacing: 6.0) {
        if settingsChecker.keyboardTypeEmpty {
          VStack(alignment: .leading, spacing: 20.0) {
            Label(
              "Please select the keyboard type",
              systemImage: "gearshape"
            )
            .font(.system(size: 24))

            KeyboardTypeSelectorView()
          }
        }
      }
      .padding()
      .frame(width: 850)

      SheetCloseButton {
        ContentViewStates.shared.showSettingsAlert = false
      }
    }
    .onAppear {
      focus = true
    }
  }
}

struct SettingsAlertView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      SettingsAlertView()
        .previewLayout(.sizeThatFits)
    }
  }
}
