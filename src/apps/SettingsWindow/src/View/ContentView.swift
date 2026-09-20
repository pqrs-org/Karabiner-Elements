import SwiftUI

struct ContentView: View {
  @ObservedObject private var debugPreview = DebugAlertPreviewState.shared
  @ObservedObject private var contentViewStates = ContentViewStates.shared
  @ObservedObject private var settings = Settings.shared

  private let padding = 6.0

  private var karabinerJsonPermissionError: Bool {
    settings.configurationLoadState == krbn_core_configuration_load_state_permission_error
      || (settings.configurationLoaded && contentViewStates.karabinerJsonPermissionError)
  }

  var body: some View {
    ZStack(alignment: .top) {
      if karabinerJsonPermissionError {
        KarabinerJsonPermissionErrorView()
      } else if settings.configurationLoaded {
        ContentMainView()

        if contentViewStates.displayedAlert == .doctor {
          OverlayAlertView {
            DoctorAlertView()
          }
        } else if contentViewStates.displayedAlert == .servicesNotRunning {
          OverlayAlertView {
            ServicesNotRunningAlertView()
          }
        } else if contentViewStates.displayedAlert == .settings {
          OverlayAlertView {
            SettingsAlertView()
          }
        } else if contentViewStates.displayedAlert == .consoleUserServerNotConnected {
          OverlayAlertView {
            ConsoleUserServerNotConnectedAlertView()
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
    .animation(.easeInOut(duration: 0.2), value: contentViewStates.toast)
    .animation(.easeInOut(duration: 0.2), value: debugPreview.alert?.isToast)
    .frame(
      minWidth: 1100,
      maxWidth: .infinity,
      minHeight: 680,
      maxHeight: .infinity
    )
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
