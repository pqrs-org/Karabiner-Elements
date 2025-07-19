import SwiftUI

@main
struct KarabinerMenuApp: App {
  @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

  @ObservedObject private var settings = LibKrbn.Settings.shared

  private let version =
    Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? ""

  init() {
    libkrbn_initialize()
    settings.watch()
  }

  var body: some Scene {
    MenuBarExtra(
      content: {
        Text("Karabiner-Elements \(version)")

        Divider()

        Label("Profiles", systemImage: "person.3")
          .labelStyle(.titleAndIcon)

        ForEach(settings.profiles) { profile in
          Button(
            action: {
              settings.selectProfile(profile)
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
            libkrbn_launch_settings()
          },
          label: {
            Label("Settings...", systemImage: "gearshape")
              .labelStyle(.titleAndIcon)
          }
        )

        Button(
          action: {
            libkrbn_updater_check_for_updates_stable_only()
          },
          label: {
            Label("Check for updates...", systemImage: "network")
              .labelStyle(.titleAndIcon)
          }
        )

        if settings.showAdditionalMenuItems {
          Button(
            action: {
              libkrbn_updater_check_for_updates_with_beta_version()
            },
            label: {
              Label("Check for beta updates", systemImage: "hare")
                .labelStyle(.titleAndIcon)
            }
          )
        }

        Button(
          action: {
            libkrbn_launch_event_viewer()
          },
          label: {
            Label("Launch EventViewer...", systemImage: "magnifyingglass")
              .labelStyle(.titleAndIcon)
          }
        )

        Divider()

        Button(
          action: {
            libkrbn_services_restart_console_user_server_agent()
          },
          label: {
            Label("Restart Karabiner-Elements", systemImage: "arrow.clockwise")
              .labelStyle(.titleAndIcon)
          }
        )

        Button(
          action: {
            KarabinerAppHelper.shared.quitKarabiner(
              askForConfirmation: settings.askForConfirmationBeforeQuitting,
              quitFrom: .menu)
          },
          label: {
            Label("Quit Karabiner-Elements", systemImage: "xmark.circle.fill")
              .labelStyle(.titleAndIcon)
          }
        )
      },
      label: {
        HStack(spacing: 8.0) {
          if settings.showIconInMenuBar {
            Image("menu")
              .environment(\.displayScale, 2.0)
          }
          if settings.showProfileNameInMenuBar {
            Text(settings.selectedProfileName())
          }
        }
      }
    )
  }
}

class AppDelegate: NSObject, NSApplicationDelegate {
  public func applicationWillTerminate(_: Notification) {
    print("terminate")
    libkrbn_terminate()
  }
}
