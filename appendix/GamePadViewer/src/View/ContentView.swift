import SwiftUI

struct ContentView: View {
  @ObservedObject var inputMonitoringAlertData = InputMonitoringAlertData.shared
  @ObservedObject var eventObserver = EventObserver.shared
  @ObservedObject var stickManager = StickManager.shared

  var body: some View {
    VStack {
      VStack(alignment: .leading) {
        Text("counter: \(eventObserver.counter)")

        StickInfo(label: "Right Stick X", stick: stickManager.rightStickX)
        StickInfo(label: "Right Stick Y", stick: stickManager.rightStickY)
      }

      ZStack(alignment: .center) {
        Circle()
          .stroke(Color.gray, lineWidth: 2)
          .frame(width: 100, height: 100)

        Line(
          x: stickManager.rightStickX.lastAcceleration / 5.0,
          y: stickManager.rightStickY.lastAcceleration / 5.0
        )
        .stroke(Color.red, lineWidth: 2)
        .frame(width: 100, height: 100)
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

struct StickInfo: View {
  let label: String
  @ObservedObject var stick: StickManager.Stick

  var body: some View {
    VStack(alignment: .leading) {
      Text("\(label):")
      Text("    value: \(stick.value)")
      Text("    interval: \(stick.lastInterval)")
      Text("    acceleration: \(stick.lastAcceleration))")
    }
  }
}

struct Line: Shape {
  let x: CGFloat  // -1.0 ... 1.0
  let y: CGFloat  // -1.0 ... 1.0

  func path(in rect: CGRect) -> Path {
    var path = Path()

    path.move(to: CGPoint(x: rect.maxX / 2.0, y: rect.maxY / 2.0))
    // x:1.0  => rect.maxX
    // x:0.0  => rect.maxX / 2.0
    // x:-1.0 => 0.0
    path.addLine(
      to: CGPoint(
        x: (x + 1.0) / 2.0 * rect.maxX,
        y: (y + 1.0) / 2.0 * rect.maxY
      ))

    return path
  }
}

struct ContentView_Previews: PreviewProvider {
  static var previews: some View {
    ContentView()
  }
}
