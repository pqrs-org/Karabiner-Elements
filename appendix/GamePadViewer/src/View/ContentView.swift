import SwiftUI

struct ContentView: View {
  @ObservedObject var inputMonitoringAlertData = InputMonitoringAlertData.shared
  @ObservedObject var eventObserver = EventObserver.shared

  var body: some View {
    VStack {
      Text("counter: \(eventObserver.counter)")
      Text(
        "right stick X: \(eventObserver.rightStickX.value) (acceleration: \(eventObserver.rightStickX.acceleration))"
      )
      Text(
        "right stick Y: \(eventObserver.rightStickY.value) (acceleration: \(eventObserver.rightStickY.acceleration))"
      )

      ZStack(alignment: .center) {
        Circle()
          .stroke(Color.gray, lineWidth: 2)
          .frame(width: 100, height: 100)

        Line(
          x: eventObserver.rightStickX.acceleration / 20.0,
          y: eventObserver.rightStickY.acceleration / 20.0
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
