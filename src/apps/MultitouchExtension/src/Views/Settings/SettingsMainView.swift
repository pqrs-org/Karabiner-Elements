import SwiftUI

struct SettingsMainView: View {
  @ObservedObject private var userSettings = UserSettings.shared
  @ObservedObject private var fingerManager = FingerManager.shared

  private let areaSize = CGSize(width: 280.0, height: 200.0)

  var body: some View {
    VStack(alignment: .leading, spacing: 25.0) {
      GroupBox(label: Text("Basic")) {
        VStack(alignment: .leading, spacing: 25.0) {
          HStack {
            Toggle(isOn: $userSettings.openAtLogin) {
              Text("Open at login")
            }
            .switchToggleStyle()

            Text("(Default: off)")

            Spacer()
          }

          VStack(alignment: .leading) {
            HStack {
              Toggle(isOn: $userSettings.hideIconInDock) {
                Text("Hide icon in Dock")
              }
              .switchToggleStyle()

              Text("(Default: off)")

              Spacer()
            }

            Text("(You need to restart app to enable/disable this option)")
          }
        }
        .padding(6.0)
      }

      GroupBox(label: Text("Area")) {
        VStack(alignment: .leading, spacing: 10.0) {
          VStack(alignment: .center) {
            HStack {
              Text("Ignored Area (Top)")

              IntTextField(
                value: $userSettings.ignoredAreaTop,
                range: 0...100,
                step: 5,
                width: 40)
            }

            HStack {
              VStack {
                Text("Ignored")
                Text("Area")
                Text("(Left)")

                IntTextField(
                  value: $userSettings.ignoredAreaLeft,
                  range: 0...100,
                  step: 5,
                  width: 40)
              }

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
                  .padding(
                    .top, areaSize.height * (1.0 - targetArea.origin.y - targetArea.size.height))

                ForEach(fingerManager.states) { state in
                  if state.touchedPhysically {
                    let diameter = 10.0

                    Circle()
                      .stroke(
                        state.ignored ? Color.black : Color.red, style: StrokeStyle(lineWidth: 2)
                      )
                      .frame(width: diameter)
                      .padding(.leading, areaSize.width * state.point.x - (diameter / 2))
                      .padding(.top, areaSize.height * (1.0 - state.point.y) - (diameter / 2))
                  }

                  if state.touchedFixed {
                    let diameter = 5.0

                    Circle()
                      .fill(
                        state.ignored ? Color.black : Color.red
                      )
                      .frame(width: diameter)
                      .padding(.leading, areaSize.width * state.point.x - (diameter / 2))
                      .padding(.top, areaSize.height * (1.0 - state.point.y) - (diameter / 2))
                  }
                }
              }

              VStack {
                Text("Ignored")
                Text("Area")
                Text("(Right)")

                IntTextField(
                  value: $userSettings.ignoredAreaRight,
                  range: 0...100,
                  step: 5,
                  width: 40)
              }
            }

            HStack {
              Text("Ignored Area (Bottom)")

              IntTextField(
                value: $userSettings.ignoredAreaBottom,
                range: 0...100,
                step: 5,
                width: 40)
            }
          }
        }
      }
      .padding(6.0)

      Spacer()
    }.padding()
  }
}

struct SettingsMainView_Previews: PreviewProvider {
  static var previews: some View {
    SettingsMainView()
  }
}
