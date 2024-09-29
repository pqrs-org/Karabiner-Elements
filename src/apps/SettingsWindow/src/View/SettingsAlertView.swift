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
              "Please select the keyboard type you'd like to use",
              systemImage: "gearshape"
            )
            .font(.system(size: 24))

            Text(
              "The keyboard type selected here will be used in Karabiner-Elements, regardless of the physical keyboard you're using."
            )

            VStack {
              KeyboardTypeSelectorView()
            }
            .padding(20.0)
            .foregroundColor(Color.infoForeground)
            .background(Color.infoBackground)
            .cornerRadius(8)

            Text(
              "Note: You can change it later from the Virtual Keyboard settings."
            )
          }
        }
      }
      .padding()
      .frame(width: 650)

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
