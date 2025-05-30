import SwiftUI

struct ContentView: View {
  @ObservedObject private var inputMonitoringAlertData = InputMonitoringAlertData.shared

  var body: some View {
    ZStack {
      HStack {
        StickView(stick: StickManager.shared.leftStick)
        StickView(stick: StickManager.shared.rightStick)
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

struct StickView: View {
  @ObservedObject var stick: StickManager.Stick

  private let circleSize = 100.0
  private let indicatorSize = 10.0

  var body: some View {
    HStack {
      Grid {
        GridRow {
          Text("horizontal:")
            .gridColumnAlignment(.trailing)
          Text("\(stick.horizontal.lastDoubleValue)")
        }
        GridRow {
          Text("vertical:")

          Text("\(stick.vertical.lastDoubleValue)")
        }
        GridRow {
          Text("magnitude:")

          Text("\(stick.magnitude)")
        }
        GridRow {
          Text("radian:")

          Text("\(stick.radian)")
        }
      }
      .frame(width: 200)

      ZStack(alignment: .topLeading) {
        Circle()
          .stroke(.gray, lineWidth: 2)
          .frame(width: circleSize, height: circleSize)

        Circle()
          .fill(.blue)
          .frame(width: indicatorSize, height: indicatorSize)
          .padding(
            .leading,
            circleSize / 2.0 + cos(stick.radian) * stick.magnitude * circleSize / 2.0
              - indicatorSize
              / 2.0
          )
          .padding(
            .top,
            circleSize / 2.0 - sin(stick.radian) * stick.magnitude * circleSize / 2.0
              - indicatorSize
              / 2.0
          )
      }
      .frame(
        width: circleSize + indicatorSize * 2,
        height: circleSize + indicatorSize * 2)
    }
  }
}
