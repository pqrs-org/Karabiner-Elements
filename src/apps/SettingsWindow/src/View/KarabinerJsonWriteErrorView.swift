import SwiftUI

struct KarabinerJsonWriteErrorView: View {
  let error: krbn_configuration_file_access

  private var hint: String {
    switch error {
    case krbn_configuration_file_access_file_not_writable:
      return "settings.setup.configuration.file_not_writable"
    case krbn_configuration_file_access_directory_not_writable:
      return "settings.setup.configuration.directory_not_writable"
    case krbn_configuration_file_access_permissions_unsupported:
      return "settings.setup.configuration.permissions_unsupported"
    default:
      return "settings.setup.configuration.ownership_ignored"
    }
  }

  var body: some View {
    VStack(spacing: 20) {
      AppLocalizedLabel("settings.setup.configuration.not_writable", systemImage: ErrorBorder.icon)
        .font(.title)
      AppLocalizedText(hint)
        .multilineTextAlignment(.center)
    }
    .padding()
    .frame(width: 850)
    .textSelection(.enabled)
  }
}
