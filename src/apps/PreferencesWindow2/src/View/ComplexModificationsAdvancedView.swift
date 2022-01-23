import SwiftUI

struct ComplexModificationsAdvancedView: View {
    @ObservedObject private var settings = Settings.shared

    var body: some View {
        VStack(alignment: .leading, spacing: 24.0) {
            GroupBox(label: Text("Basic parameters")) {
                VStack(alignment: .leading, spacing: 12.0) {
                    HStack {
                        Text("to_if_alone_timeout_milliseconds:")

                        IntTextField(value: $settings.complexModificationsParameterToIfAloneTimeoutMilliseconds,
                                     range: 0 ... 10000,
                                     step: 100,
                                     width: 50)

                        Text("(Default value is 1000)")
                        Spacer()
                    }

                    Divider()

                    HStack {
                        Text("to_if_held_down_threshold_milliseconds:")

                        IntTextField(value: $settings.complexModificationsParameterToIfHeldDownThresholdMilliseconds,
                                     range: 0 ... 10000,
                                     step: 100,
                                     width: 50)

                        Text("(Default value is 500)")
                        Spacer()
                    }

                    Divider()

                    HStack {
                        Text("to_delayed_action_delay_milliseconds:")

                        IntTextField(value: $settings.complexModificationsParameterToDelayedActionDelayMilliseconds,
                                     range: 0 ... 10000,
                                     step: 100,
                                     width: 50)

                        Text("(Default value is 500)")
                        Spacer()
                    }
                }
                .padding(6.0)
            }

            Spacer()
        }
        .padding()
    }
}

struct ComplexModificationsAdvancedView_Previews: PreviewProvider {
    static var previews: some View {
        ComplexModificationsAdvancedView()
    }
}
