import SwiftUI

struct ContentView: View {
  @ObservedObject var inputMonitoringAlertData = InputMonitoringAlertData.shared
  @ObservedObject var eventObserver = EventObserver.shared
  @ObservedObject var rightStick = StickManager.shared.rightStick

  let circleSize = 100.0
  let indicatorSize = 10.0
  let accelerationScale = 10.0

  var body: some View {
    VStack {
      VStack(alignment: .leading) {
        Text("counter: \(eventObserver.counter)")
        Text("deadzoneEnteredAt: \(rightStick.deadzoneEnteredAt)")
        Text("deadzoneLeftAt: \(rightStick.deadzoneLeftAt)")
        Text("startingStroke: \(rightStick.startingStroke ? "true":"false")")
        Text("Right Stick radian: \(rightStick.radian)")
        Text("Right Stick magnitude: \(rightStick.magnitude)")
        Text("Right stick holding acceleration: \(rightStick.holdingAcceleration)")
        Text("Right stick holding magnitude: \(rightStick.holdingMagnitude)")

        StickSensorInfo(label: "Right Stick X", stick: rightStick.horizontal)
        StickSensorInfo(label: "Right Stick Y", stick: rightStick.vertical)
      }

      ZStack(alignment: .topLeading) {
        Circle()
          .stroke(Color.gray, lineWidth: 2)
          .frame(width: circleSize, height: circleSize)

        Circle()
          .fill(Color.blue)
          .frame(width: indicatorSize, height: indicatorSize)
          .padding(
            .leading,
            circleSize / 2.0 + cos(rightStick.radian)
              * rightStick.magnitude * circleSize / 2.0 - indicatorSize / 2.0
          )
          .padding(
            .top,
            circleSize / 2.0 + sin(rightStick.radian)
              * rightStick.magnitude * circleSize / 2.0 - indicatorSize / 2.0
          )

        Path { path in
          path.move(to: CGPoint(x: circleSize / 2.0, y: circleSize / 2.0))
          path.addLine(
            to: CGPoint(
              x: circleSize / 2.0 + cos(rightStick.radian)
                * rightStick.holdingAcceleration * accelerationScale * circleSize
                / 2.0,
              y: circleSize / 2.0 + sin(rightStick.radian)
                * rightStick.holdingAcceleration * accelerationScale * circleSize / 2.0
            ))
        }
        .stroke(Color.red, lineWidth: 2)
        .frame(width: circleSize, height: circleSize)
      }
    }
    .alert(isPresented: inputMonitoringAlertData.showing) {
      InputMonitoringAlertView()
    }
    .frame(
      minWidth: 1100,
      maxWidth: .infinity,
      minHeight: 650,
      maxHeight: .infinity)
  }
}

struct StickSensorInfo: View {
  let label: String
  @ObservedObject var stick: StickManager.StickSensor

  var body: some View {
    VStack(alignment: .leading) {
      Text("\(label):")
      Text("    value: \(stick.lastDoubleValue)")
    }
  }
}

struct ContentView_Previews: PreviewProvider {
  static var previews: some View {
    ContentView()
  }
}
