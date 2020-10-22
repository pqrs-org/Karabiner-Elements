import SwiftUI

struct DriverVersionNotMatchedAlertView: View {
    public static let title = "Driver Alert"

    var body: some View {
        VStack(alignment: .leading, spacing: 20.0) {
            HStack {
                Image(decorative: "baseline_warning_black_48pt")
                    .resizable()
                    .frame(width: 48.0, height: 48.0)
                Text(DriverVersionNotMatchedAlertView.title)
            }

            Text("The current virtual keyboard and mouse driver is outdated.\nPlease deactivate driver and restart macOS to upgrade the driver.")
                .fixedSize(horizontal: false, vertical: true)

            Text("How to upgrade the driver:")

            VStack(alignment: .leading, spacing: 10.0) {
                Text("1. Press the following button to deactivate driver.\n(The administrator password will be required.)")
                    .fixedSize(horizontal: false, vertical: true)

                DeactivateDriverButton()

                Text("2. Restart macOS.")
                    .fontWeight(.bold)
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
