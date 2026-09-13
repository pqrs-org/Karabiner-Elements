import SwiftUI

@main
struct KarabinerConsoleUserServerApp: App {
  @ObservedObject private var localization = AppLocalization.shared
  @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate
  @ObservedObject private var state = ConsoleUserServerUIState.shared

  private let version =
    Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? ""

  private func localized(_ key: String) -> String {
    AppLanguage.text(key, locale: AppLanguage.locale(for: state.uiLanguage))
  }

  private var quitLabel: some View {
    Label(localized("menu_bar_extra.quit"), systemImage: "xmark.rectangle")
      .labelStyle(.titleAndIcon)
  }

  var body: some Scene {
    MenuBarExtra(
      isInserted: Binding(
        // Keep the menu bar extra inserted while the configuration is loading.
        // If MenuBarExtra is initially created with isInserted == false and inserted only after
        // the asynchronous configuration load completes, macOS does not persist its position
        // after the user Command-drags it in the menu bar.
        get: { !state.configurationLoaded || state.menuVisible },
        set: { _ in }
      ),
      content: {
        Text("Karabiner-Elements \(version)")

        Divider()

        Label(localized("menu_bar_extra.profiles"), systemImage: "person.3")
          .labelStyle(.titleAndIcon)

        ForEach(state.profiles) { profile in
          Button(
            action: {
              state.selectProfile(profile)
            },
            label: {
              if profile.selected {
                Label(profile.name, systemImage: "checkmark")
                  .labelStyle(.titleAndIcon)
              } else {
                Label(profile.name, image: "clear")
                  .labelStyle(.titleAndIcon)
              }
            }
          )
        }

        Divider()

        Button(
          action: {
            console_user_server_launch_settings()
          },
          label: {
            Label(localized("menu_bar_extra.settings"), systemImage: "gear")
              .labelStyle(.titleAndIcon)
          }
        )

        if state.menuSettings.enableMultitouchExtension {
          Button(
            action: {
              KarabinerAppHelper.shared.openMultitouchExtensionSettings()
            },
            label: {
              Label(
                localized("menu_bar_extra.multitouch_settings"),
                systemImage: "rectangle.and.hand.point.up.left.filled"
              )
              .labelStyle(.titleAndIcon)
            }
          )
        }

        Button(
          action: {
            console_user_server_check_for_updates(false)
          },
          label: {
            Label(localized("menu_bar_extra.check_for_updates"), systemImage: "network")
              .labelStyle(.titleAndIcon)
          }
        )

        if state.menuSettings.showAdditionalMenuItems {
          Button(
            action: {
              console_user_server_check_for_updates(true)
            },
            label: {
              Label(localized("menu_bar_extra.check_for_beta_updates"), systemImage: "hare")
                .labelStyle(.titleAndIcon)
            }
          )
        }

        Button(
          action: {
            console_user_server_launch_event_viewer()
          },
          label: {
            Label(localized("menu_bar_extra.launch_event_viewer"), systemImage: "magnifyingglass")
              .labelStyle(.titleAndIcon)
          }
        )

        Divider()

        Button(
          action: {
            console_user_server_restart()
          },
          label: {
            Label(localized("menu_bar_extra.restart"), systemImage: "arrow.clockwise")
              .labelStyle(.titleAndIcon)
          }
        )

        if state.menuSettings.showQuitConfirmationMenu {
          Menu(
            content: {
              Text(verbatim: localized("menu_bar_extra.quit_confirmation"))

              Divider()

              Button(
                action: {
                  console_user_server_quit()
                },
                label: {
                  quitLabel
                }
              )
            },
            label: {
              quitLabel
            }
          )
        } else {
          Button(
            action: {
              console_user_server_quit()
            },
            label: {
              quitLabel
            }
          )
        }
      },
      label: {
        HStack(spacing: 8.0) {
          if state.menuSettings.showIcon {
            Image("menu")
              .environment(\.displayScale, 2.0)
          }
          if state.menuSettings.showProfileName {
            Text(state.selectedProfileName)
          }
        }
      }
    )

    Settings {}
  }
}

@MainActor
final class AppDelegate: NSObject, NSApplicationDelegate {
  func applicationDidFinishLaunching(_: Notification) {
    console_user_server_start(completePendingApplicationTermination)
    ConsoleUserServerUIState.shared.start()
    _ = NotificationWindowManager.shared
  }

  func applicationShouldTerminate(_: NSApplication) -> NSApplication.TerminateReply {
    // Keep AppKit running until the asynchronous C++ component cleanup finishes.
    requestApplicationTermination {
      console_user_server_async_request_termination()
    }
  }

  func applicationWillTerminate(_: Notification) {
    console_user_server_finalize()
  }
}
