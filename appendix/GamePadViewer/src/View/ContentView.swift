import SwiftUI

struct ContentView: View {
  @ObservedObject var inputMonitoringAlertData = InputMonitoringAlertData.shared
  @ObservedObject var eventObserver = EventObserver.shared
  @ObservedObject var rightStick = StickManager.shared.rightStick

  let circleSize = 100.0
  let indicatorSize = 10.0

  var body: some View {
    VStack {
      VStack(alignment: .leading) {
        Text("counter: \(eventObserver.counter)")
        Text("horizontal: \(rightStick.horizontal.lastDoubleValue)")
        Text("vertical: \(rightStick.vertical.lastDoubleValue)")
        Divider()
        Text("radian: \(rightStick.radian)")
        Text("magnitude: \(rightStick.magnitude)")
        Text("strokeAcceleration: \(rightStick.strokeAcceleration)")
        Divider()
        Text("radianDiff \(rightStick.radianDiff)")
        Text("deltaHorizontal: \(rightStick.deltaHorizontal)")
        Text("deltaVertical: \(rightStick.deltaVertical)")
        Text("deltaRadian: \(rightStick.deltaRadian)")
        Text("deltaMagnitude: \(rightStick.deltaMagnitude)")
      }
      .frame(width: 400)

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
              x: circleSize / 2.0 + cos(rightStick.radian) * rightStick.strokeAcceleration
                * circleSize / 2.0,
              y: circleSize / 2.0 + sin(rightStick.radian) * rightStick.strokeAcceleration
                * circleSize / 2.0
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

struct ContentView_Previews: PreviewProvider {
  static var previews: some View {
    ContentView()
  }
}
