import SwiftUI

struct DoctorAlertView: View {
  @ObservedObject private var doctor = Doctor.shared

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center, spacing: 20.0) {
        if !doctor.karabinerJSONParseErrorMessage.isEmpty {
          Label(
            "karabiner.json couldn't be loaded due to a parse error",
            systemImage: ErrorBorder.icon
          )
          .font(.title)

          Text("It looks like the file was edited manually and now contains invalid JSON.")

          Text(doctor.karabinerJSONParseErrorMessage)
            .modifier(ErrorBorder())
        }
      }
      .padding()
      .frame(width: 850)

      SheetCloseButton {
        ContentViewStates.shared.dismissCurrentAlert()
      }
    }
  }
}
