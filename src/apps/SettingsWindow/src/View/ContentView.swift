import SwiftUI

struct ContentView: View {
  static let defaultContentSize = NSSize(width: 1100, height: 680)

  @ObservedObject private var debugPreview = DebugAlertPreviewState.shared
  @ObservedObject private var contentViewStates = ContentViewStates.shared
  @ObservedObject private var settings = Settings.shared
  @State private var configurationFileAccess = krbn_configuration_file_access_ok

  private let padding = 6.0

  private var karabinerJsonPermissionError: Bool {
    settings.configurationLoadState == krbn_core_configuration_load_state_permission_error
      || (settings.configurationLoaded && contentViewStates.karabinerJsonPermissionError)
  }

  var body: some View {
    ZStack(alignment: .top) {
      if karabinerJsonPermissionError {
        KarabinerJsonPermissionErrorView()
      } else if configurationFileAccess != krbn_configuration_file_access_ok {
        KarabinerJsonWriteErrorView(error: configurationFileAccess)
      } else if contentViewStates.displayedAlert == .karabinerJsonParseError {
        // `.karabinerJsonParseError` is published only after a parse error persists for three seconds of
        // awake time, so transient errors while an external process rewrites
        // karabiner.json do not show an alert.
        KarabinerJsonParseErrorView()
      } else if settings.configurationLoadState
        == krbn_core_configuration_load_state_other_error
      {
        UnexpectedSettingsLoadErrorView(message: settings.settingsSnapshotProcessingErrorMessage)
      } else if settings.configurationLoaded {
        ContentMainView()

        if contentViewStates.displayedAlert == .consoleUserServerNotConnected {
          OverlayAlertView {
            ConsoleUserServerNotConnectedAlertView()
          }
        } else if contentViewStates.displayedAlert == .servicesNotRunning {
          OverlayAlertView {
            ServicesNotRunningAlertView()
          }
        } else if contentViewStates.displayedAlert == .settings {
          OverlayAlertView {
            SettingsAlertView()
          }
        } else if contentViewStates.displayedAlert == .virtualHidDeviceServiceClientNotConnected {
          OverlayAlertView {
            VirtualHidDeviceServiceClientNotConnectedAlertView()
          }
        } else if contentViewStates.displayedAlert == .driverVersionMismatched {
          OverlayAlertView {
            DriverVersionMismatchedAlertView()
          }
        } else if contentViewStates.displayedAlert == .driverNotConnected {
          OverlayAlertView {
            DriverNotConnectedAlertView()
          }
        }
      } else {
        // Keep the settings UI unavailable until karabiner.json has loaded successfully.
        // A core_configuration created after a parse error contains fallback default values;
        // exposing and editing those values could overwrite karabiner.json and discard the
        // user's existing settings. A persistent parse error becomes the `.karabinerJsonParseError` alert and
        // is displayed by KarabinerJsonParseErrorView after a three-second grace period. Until then, show
        // progress instead of stale or fallback settings.
        ProgressView()
      }

      if let alert = debugPreview.alert, alert.isToast {
        // SettingsLanguage below applies to child views, not this view's environment.
        DebugToastPreview(message: AppLanguage.text(alert.title, locale: settings.uiLocale)) {
          if debugPreview.alert == alert {
            debugPreview.alert = nil
          }
        }
        .id(alert)
        .padding(.top, 16)
        .transition(.move(edge: .top).combined(with: .opacity))
        .zIndex(2)
      } else if let alert = debugPreview.alert, alert.setup == nil {
        DebugAlertsView.previewView(alert)
          .modifier(
            LocalizationPreviewInteraction {
              debugPreview.alert = nil
            }
          )
          .zIndex(2)
      }

      if let toast = contentViewStates.toast, debugPreview.alert?.isToast != true {
        ToastView(toast: toast) {
          contentViewStates.dismissToast()
        }
        .padding(.top, 16)
        .transition(.move(edge: .top).combined(with: .opacity))
        .zIndex(1)
      }
    }
    .modifier(SettingsLanguage())
    .background(IconOnlyToolbarConfigurator().frame(width: 0, height: 0))
    .task {
      // Poll off MainActor: configuration paths may point to external or network volumes.
      // Rechecking also lets the user fix permissions without restarting Settings.
      while !Task.isCancelled {
        let result = await Task.detached(priority: .utility) {
          krbn_check_configuration_file_access()
        }.value
        guard !Task.isCancelled else { return }
        configurationFileAccess = result
        if result == krbn_configuration_file_access_ok && !settings.configurationLoaded {
          // Permission/ownership fixes may not change file contents, so the file monitor
          // can suppress their notifications. Retry until loading has recovered; C++ only
          // retries failed loads and leaves an already loaded configuration untouched.
          krbn_async_retry_failed_configuration_load()
        }
        do {
          try await Task.sleep(for: .seconds(3))
        } catch {
          return
        }
      }
    }
    .animation(.easeInOut(duration: 0.2), value: contentViewStates.toast)
    .animation(.easeInOut(duration: 0.2), value: debugPreview.alert?.isToast)
    .frame(
      minWidth: Self.defaultContentSize.width,
      maxWidth: .infinity,
      minHeight: Self.defaultContentSize.height,
      maxHeight: .infinity
    )
  }
}

struct UnexpectedSettingsLoadErrorView: View {
  let message: String

  var body: some View {
    VStack(spacing: 20) {
      AppLocalizedLabel(
        "settings.setup.configuration.load_error",
        systemImage: ErrorBorder.icon
      )
      .font(.title)

      if !message.isEmpty {
        Text(message)
          .modifier(ErrorBorder())
      }
    }
    .padding()
    .frame(width: 850)
    .textSelection(.enabled)
  }
}

// macOS sheets can reset the locale instead of inheriting it from their presenter.
// Apply this inside each sheet and observe settings for changes while it is open.
struct SettingsLanguage: ViewModifier {
  @ObservedObject private var settings = Settings.shared
  @ObservedObject private var localization = AppLocalization.shared

  func body(content: Content) -> some View {
    content.environment(\.locale, settings.uiLocale)
  }
}
