import SwiftUI

struct ComplexModificationsAdvancedView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared

  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 24.0) {
        GroupBox(label: Text("type:basic parameters")) {
          VStack(alignment: .leading, spacing: 12.0) {
            HStack {
              Text("to_if_alone_timeout_milliseconds:")

              IntTextField(
                value: $settings.complexModificationsParameterToIfAloneTimeoutMilliseconds,
                range: 0...10000,
                step: 100,
                width: 50)

              Text("(Default value is 1000)")
            }

            Divider()

            HStack {
              Text("to_if_held_down_threshold_milliseconds:")

              IntTextField(
                value: $settings.complexModificationsParameterToIfHeldDownThresholdMilliseconds,
                range: 0...10000,
                step: 100,
                width: 50)

              Text("(Default value is 500)")
            }

            Divider()

            HStack {
              Text("to_delayed_action_delay_milliseconds:")

              IntTextField(
                value: $settings.complexModificationsParameterToDelayedActionDelayMilliseconds,
                range: 0...10000,
                step: 100,
                width: 50)

              Text("(Default value is 500)")
            }

            Divider()

            HStack {
              Text("simultaneous_threshold_milliseconds:")

              IntTextField(
                value: $settings.complexModificationsParameterSimultaneousThresholdMilliseconds,
                range: 0...1000,
                step: 20,
                width: 50)

              Text("(Default value is 50)")
            }
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: Text("type:mouse_motion_to_scroll parameters")) {
          VStack(alignment: .leading, spacing: 12.0) {
            HStack {
              Text("speed:")

              IntTextField(
                value: $settings.complexModificationsParameterMouseMotionToScrollSpeed,
                range: 0...10000,
                step: 10,
                width: 50)

              Text("% (Default value is 100)")
            }
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }
      }
      .padding()
    }
  }
}
