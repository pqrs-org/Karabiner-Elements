import SwiftUI

struct IgnoredAreaView: View {
  @ObservedObject private var userSettings = UserSettings.shared
  @ObservedObject private var fingerManager = FingerManager.shared

  private let areaSize = CGSize(width: 280.0, height: 200.0)

  var body: some View {
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
            if state.touchedPhysically || state.touchedFixed {
              let diameter = 10.0
              let color = state.ignored ? Color.black : Color.red
              let leading = areaSize.width * state.point.x - (diameter / 2)
              let top = areaSize.height * (1.0 - state.point.y) - (diameter / 2)

              Circle()
                .stroke(color, style: StrokeStyle(lineWidth: 2))
                .frame(width: diameter)
                .padding(.leading, leading)
                .padding(.top, top)

              if state.touchedFixed {
                Circle()
                  .fill(color)
                  .frame(width: diameter)
                  .padding(.leading, leading)
                  .padding(.top, top)
              }
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
