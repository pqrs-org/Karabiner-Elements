import SwiftUI

struct SettingsMainView: View {
  @ObservedObject private var userSettings = UserSettings.shared
  @ObservedObject private var fingerStatus = FingerStatusManager.shared

  private let areaSize = CGSize(width: 280.0, height: 200.0)

  var body: some View {
    VStack(alignment: .leading, spacing: 25.0) {
      HStack {
        Text("SettingsMainView")

        Text("\(userSettings.targetArea.origin.y)")

        Spacer()
      }

      GroupBox(label: Text("Area")) {
        VStack(alignment: .leading, spacing: 10.0) {
          ZStack(alignment: .topLeading) {
            RoundedRectangle(cornerRadius: 10.0)
              .fill(Color.gray.opacity(0.5))
              .frame(width: areaSize.width, height: areaSize.height)

            let targetArea = userSettings.targetArea

            RoundedRectangle(cornerRadius: 10.0)
              .fill(Color.gray)
              .frame(
                width: areaSize.width * targetArea.size.width,
                height: areaSize.height * targetArea.size.height
              )
              .padding(.leading, areaSize.width * targetArea.origin.x)
              .padding(.top, areaSize.height * (1.0 - targetArea.origin.y - targetArea.size.height))

            ForEach(fingerStatus.entries) { entry in
              if entry.touchedPhysically {
                let diameter = 10.0

                Circle()
                  .fill(Color.red)
                  .frame(width: diameter)
                  .padding(.leading, areaSize.width * entry.point.x - (diameter / 2))
                  .padding(.top, areaSize.height * (1.0 - entry.point.y) - (diameter / 2))
              }
            }
          }
        }
      }

      Spacer()
    }.padding()
  }
}

struct SettingsMainView_Previews: PreviewProvider {
  static var previews: some View {
    SettingsMainView()
  }
}
