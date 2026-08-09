import SwiftUI

struct KarabinerJsonPermissionErrorView: View {
  var body: some View {
    VStack(spacing: 20.0) {
      Label(
        "karabiner.json couldn't be loaded because access was denied",
        systemImage: ErrorBorder.icon
      )
      .font(.title)

      Text(
        "Move karabiner.json to a location that Karabiner-Elements can access, or grant Full Disk Access to Karabiner-Elements and its background services. Then restart Karabiner-Elements."
      )
      .multilineTextAlignment(.center)
    }
    .padding()
    .frame(width: 850)
    .textSelection(.enabled)
  }
}
