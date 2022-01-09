import SwiftUI

struct DevicesView: View {
    @ObservedObject private var settings = Settings.shared
    @ObservedObject private var connectedDevices = ConnectedDevices.shared

    var body: some View {
        VStack(alignment: .leading, spacing: 12.0) {
            ScrollView {
                VStack(alignment: .leading, spacing: 0.0) {
                    // swiftformat:disable:next unusedArguments
                    ForEach($connectedDevices.connectedDevices) { $connectedDevice in
                        HStack(alignment: .center, spacing: 0) {
                            Text("\(connectedDevice.product) \(connectedDevice.manufacturer)")
                        }
                        .padding(12.0)

                        Divider()
                    }

                    Spacer()
                }
            }
            .background(Color(NSColor.textBackgroundColor))
        }
        .padding()
    }
}

struct DevicesView_Previews: PreviewProvider {
    static var previews: some View {
        DevicesView()
    }
}
