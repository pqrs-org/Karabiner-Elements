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

          let palmThreshold = userSettings.palmThreshold
          let targetArea = userSettings.targetArea
          let areaWidth = areaSize.width * targetArea.size.width
          let areaHeight = areaSize.height * targetArea.size.height
          let areaLeading = areaSize.width * targetArea.origin.x
          let areaTop = areaSize.height * (1.0 - targetArea.origin.y - targetArea.size.height)
          let areaX25 = areaSize.width * (targetArea.origin.x + targetArea.size.width * 0.25)
          let areaX50 = areaSize.width * (targetArea.origin.x + targetArea.size.width * 0.5)
          let areaX75 = areaSize.width * (targetArea.origin.x + targetArea.size.width * 0.75)
          let areaY25 =
            areaSize.height * (1.0 - targetArea.origin.y - targetArea.size.height * 0.25)
          let areaY50 = areaSize.height * (1.0 - targetArea.origin.y - targetArea.size.height * 0.5)
          let areaY75 =
            areaSize.height * (1.0 - targetArea.origin.y - targetArea.size.height * 0.75)

          RoundedRectangle(cornerRadius: 10.0)
            .fill(Color.gray)
            .frame(width: areaWidth, height: areaHeight)
            .padding(.leading, areaLeading)
            .padding(.top, areaTop)

          Path { path in
            path.addLines([
              CGPoint(x: areaX50, y: areaTop),
              CGPoint(x: areaX50, y: areaTop + areaHeight),
            ])

            path.addLines([
              CGPoint(x: areaLeading, y: areaY50),
              CGPoint(x: areaLeading + areaWidth, y: areaY50),
            ])
          }.stroke(.black, lineWidth: 1)

          Path { path in
            path.addLines([
              CGPoint(x: areaX25, y: areaTop),
              CGPoint(x: areaX25, y: areaTop + areaHeight),
            ])

            path.addLines([
              CGPoint(x: areaX75, y: areaTop),
              CGPoint(x: areaX75, y: areaTop + areaHeight),
            ])

            path.addLines([
              CGPoint(x: areaLeading, y: areaY25),
              CGPoint(x: areaLeading + areaWidth, y: areaY25),
            ])

            path.addLines([
              CGPoint(x: areaLeading, y: areaY75),
              CGPoint(x: areaLeading + areaWidth, y: areaY75),
            ])
          }.stroke(.black, style: StrokeStyle(lineWidth: 1, dash: [2]))

          ForEach(fingerManager.states) { state in
            if state.touchedPhysically || state.touchedFixed || state.palmed {
              let minDiameter = 5.0
              let thresholdDiameter = 15 + palmThreshold * 5
              let diameter =
                minDiameter + (thresholdDiameter - minDiameter) * (state.size / palmThreshold)
              let palmedColor = Color.purple
              let color = state.ignored ? Color.black : (state.palmed ? palmedColor : Color.red)
              let leading = areaSize.width * state.point.x - (diameter / 2)
              let top = areaSize.height * (1.0 - state.point.y) - (diameter / 2)

              if state.touchedFixed {
                Circle()
                  .fill(color)
                  .frame(width: diameter, height: diameter)
                  .padding(.leading, leading)
                  .padding(.top, top)
                  .clipped()
              }

              let palmThresholdLeading = areaSize.width * state.point.x - (thresholdDiameter / 2)
              let palmThresholdTop =
                areaSize.height * (1.0 - state.point.y) - (thresholdDiameter / 2)

              Circle()
                .stroke(
                  !state.touchedFixed ? Color.black : (state.palmed ? Color.gray : palmedColor),
                  style: StrokeStyle(lineWidth: 2)
                )
                .frame(width: thresholdDiameter, height: thresholdDiameter)
                .padding(.leading, palmThresholdLeading)
                .padding(.top, palmThresholdTop)
                .clipped()

              Text("\(String(format: "%.1f", state.size))")
                .padding(.leading, palmThresholdLeading)
                .padding(.top, palmThresholdTop + thresholdDiameter)
                .clipped()
            }
          }
        }
        .frame(
          width: areaSize.width,
          height: areaSize.height,
          alignment: .topLeading
        )

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
    .onAppear {
      FingerManager.shared.trackStates = true
    }
    .onDisappear {
      FingerManager.shared.trackStates = false
    }
  }
}
