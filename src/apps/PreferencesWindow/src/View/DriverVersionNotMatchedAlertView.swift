import SwiftUI

struct DriverVersionNotMatchedAlertView: View {
    var body: some View {
        VStack(alignment: .leading, spacing: 20.0) {
            Label("Driver Alert",
                  systemImage: "exclamationmark.triangle")
                .font(.system(size: 24))

            Text("The current virtual keyboard and mouse driver is outdated.\nPlease deactivate driver and restart macOS to upgrade the driver.")
                .fixedSize(horizontal: false, vertical: true)

            Text("How to upgrade the driver:")

            VStack(alignment: .leading, spacing: 10.0) {
                Text("1. Press the following button to deactivate driver.\n(The administrator password will be required.)")
                    .fixedSize(horizontal: false, vertical: true)

                DeactivateDriverButton()
                    .padding(.vertical, 10)
                    .padding(.leading, 20)

                Text("2. Restart macOS.")
                    .fontWeight(.bold)

                Text("3. Activate the driver after restarting macOS.")
            }
        }
        .padding()
        .frame(width: 500)
    }
}

struct DriverVersionNotMatchedAlertView_Previews: PreviewProvider {
    static var previews: some View {
        Group {
            DriverVersionNotMatchedAlertView()
                .previewLayout(.sizeThatFits)
        }
    }
}
