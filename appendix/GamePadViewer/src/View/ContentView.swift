import SwiftUI

struct ContentView: View {
  @ObservedObject private var inputMonitoringAlertData = InputMonitoringAlertData.shared

  var body: some View {
    ZStack {
      HStack {
        InformationView()
        StickView()
        PointerView()
      }

      if inputMonitoringAlertData.showing {
        OverlayAlertView {
          InputMonitoringAlertView()
        }
      }
    }
    .frame(
      minWidth: 1100,
      maxWidth: .infinity,
      minHeight: 650,
      maxHeight: .infinity)
  }
}

struct InformationView: View {
  @ObservedObject private var eventObserver = EventObserver.shared
  @ObservedObject private var rightStick = StickManager.shared.rightStick
  @ObservedObject private var converter = StickManager.shared.converter

  var body: some View {
    VStack(alignment: .leading) {
      Group {
        Text("counter: \(eventObserver.counter)")
        Text("horizontal: \(rightStick.horizontal.lastDoubleValue)")
        Text("vertical: \(rightStick.vertical.lastDoubleValue)")
      }
      Divider()
      Group {
        Text("radian: \(rightStick.radian)")
        Text("magnitude: \(rightStick.magnitude)")
      }
      Divider()
      Group {
        Text("deltaMagnitude: \(rightStick.deltaMagnitude)")
        Text("history.count: \(rightStick.history.count)")
      }
      Divider()
      Group {
        Text("continuedMovementMagnitude: \(converter.continuedMovementMagnitude)")
      }

      Divider()
      Group {
        Text("pointerX \(converter.pointerX)")
        Text("pointerY \(converter.pointerY)")
      }
    }
    .frame(width: 350)
  }
}

struct StickView: View {
  @ObservedObject private var rightStick = StickManager.shared.rightStick

  private let circleSize = 100.0
  private let indicatorSize = 10.0

  var body: some View {
    ZStack(alignment: .topLeading) {
      Circle()
        .stroke(.gray, lineWidth: 2)
        .frame(width: circleSize, height: circleSize)

      Circle()
        .fill(.blue)
        .frame(width: indicatorSize, height: indicatorSize)
        .padding(
          .leading,
          circleSize / 2.0 + cos(rightStick.radian)
            * rightStick.magnitude * circleSize / 2.0 - indicatorSize / 2.0
        )
        .padding(
          .top,
          circleSize / 2.0 - sin(rightStick.radian)
            * rightStick.magnitude * circleSize / 2.0 - indicatorSize / 2.0
        )
    }
    .frame(
      width: circleSize + indicatorSize * 2,
      height: circleSize + indicatorSize * 2)
  }
}

struct PointerView: View {
  @ObservedObject private var rightStick = StickManager.shared.rightStick
  @ObservedObject private var converter = StickManager.shared.converter

  private let boxWidth = 400.0
  private let boxHeight = 225.0
  private let indicatorSize = 10.0
  private let gridCount = 10

  var body: some View {
    ZStack(alignment: .topLeading) {
      Rectangle()
        .stroke(.gray, lineWidth: 2)
        .frame(width: boxWidth, height: boxHeight)

      Path { path in
        for i in 1..<gridCount {
          path.addLines([
            CGPoint(
              x: (boxWidth * Double(i)) / Double(gridCount),
              y: 0.0
            ),
            CGPoint(
              x: (boxWidth * Double(i)) / Double(gridCount),
              y: boxHeight
            ),
          ])

          path.addLines([
            CGPoint(
              x: 0.0,
              y: (boxHeight * Double(i)) / Double(gridCount)
            ),
            CGPoint(
              x: boxWidth,
              y: (boxHeight * Double(i)) / Double(gridCount)
            ),
          ])

        }
      }
      .stroke(.gray)
      .frame(width: boxWidth, height: boxHeight)

      Circle()
        .fill(.blue)
        .frame(width: indicatorSize, height: indicatorSize)
        .padding(.leading, converter.pointerX * boxWidth - indicatorSize / 2)
        .padding(.top, converter.pointerY * boxHeight - indicatorSize / 2)
    }
    .frame(
      width: boxWidth + indicatorSize * 2,
      height: boxHeight + indicatorSize * 2)
  }
}

struct ContentView_Previews: PreviewProvider {
  static var previews: some View {
    ContentView()
  }
}
