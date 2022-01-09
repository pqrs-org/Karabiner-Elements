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
                            Text("\(connectedDevice.productName) \(connectedDevice.manufacturerName)")

                            Spacer()

                            VStack(alignment: .trailing, spacing: 0) {
                                HStack(alignment: .center, spacing: 0) {
                                    Text("Vendor ID: ")
                                        .font(.caption)
                                    Text(String(connectedDevice.vendorId)
                                        .padding(toLength: 6, withPad: " ", startingAt: 0))
                                        .font(.custom("Menlo", size: 11.0))
                                }
                                HStack(alignment: .center, spacing: 0) {
                                    Text("Product ID: ")
                                        .font(.caption)
                                    Text(String(connectedDevice.productId)
                                        .padding(toLength: 6, withPad: " ", startingAt: 0))
                                        .font(.custom("Menlo", size: 11.0))
                                }
                            }

                            HStack(alignment: .center, spacing: 0) {
                                if connectedDevice.isKeyboard {
                                    Image(systemName: "keyboard")
                                }
                            }
                            .frame(width: 20.0)

                            HStack(alignment: .center, spacing: 0) {
                                if connectedDevice.isPointingDevice {
                                    Image(systemName: "capsule.portrait")
                                }
                            }
                            .frame(width: 20.0)
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
