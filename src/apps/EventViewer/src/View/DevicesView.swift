import SwiftUI

struct DevicesView: View {
    @ObservedObject var devicesJsonString = DevicesJsonString.shared

    var body: some View {
        VStack(alignment: .leading, spacing: 12.0) {
            GroupBox(label: Text("Devices")) {
                VStack(alignment: .leading, spacing: 12.0) {
                    ScrollView {
                        HStack {
                            VStack(alignment: .leading) {
                                Text(devicesJsonString.text)
                                    .lineLimit(nil)
                                    .font(.custom("Menlo", size: 11.0))
                            }

                            Spacer()
                        }
                        .background(Color(NSColor.textBackgroundColor))
                    }
                }
                .padding(6.0)
            }
            
            Spacer()
        }
        .padding()
    }
}

struct DevicesView_Previews: PreviewProvider {
    static var previews: some View {
        DevicesView()
    }
}