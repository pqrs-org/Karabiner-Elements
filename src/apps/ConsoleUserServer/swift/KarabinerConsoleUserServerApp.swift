import SwiftUI

private func consoleUserServerTerminated() {
  Task { @MainActor in
    NSApplication.shared.terminate(nil)
  }
}

@main
struct KarabinerConsoleUserServerApp: App {
  @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate
  @ObservedObject private var state = ConsoleUserServerUIState.shared

  private let version =
    Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? ""

  private var quitLabel: some View {
    Label("Quit Karabiner-Elements", systemImage: "xmark.rectangle")
      .labelStyle(.titleAndIcon)
  }

  var body: some Scene {
    MenuBarExtra(
      isInserted: Binding(
        get: { state.menuVisible },
        set: { _ in }
      ),
      content: {
        Text("Karabiner-Elements \(version)")

        Divider()

        Label("Profiles", systemImage: "person.3")
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
            Label("Settings...", systemImage: "gear")
              .labelStyle(.titleAndIcon)
          }
        )

        if state.menuSettings.enableMultitouchExtension {
          Button(
            action: {
              guard
                let url = NSWorkspace.shared.urlForApplication(
                  withBundleIdentifier: "org.pqrs.Karabiner-MultitouchExtension")
              else { return }

              NSWorkspace.shared.openApplication(
                at: url,
                configuration: NSWorkspace.OpenConfiguration())
            },
            label: {
              Label(
                "Multitouch Extension Settings...",
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
            Label("Check for updates...", systemImage: "network")
              .labelStyle(.titleAndIcon)
          }
        )

        if state.menuSettings.showAdditionalMenuItems {
          Button(
            action: {
              console_user_server_check_for_updates(true)
            },
            label: {
              Label("Check for beta updates...", systemImage: "hare")
                .labelStyle(.titleAndIcon)
            }
          )
        }

        Button(
          action: {
            console_user_server_launch_event_viewer()
          },
          label: {
            Label("Launch EventViewer...", systemImage: "magnifyingglass")
              .labelStyle(.titleAndIcon)
          }
        )

        Divider()

        Button(
          action: {
            console_user_server_restart()
          },
          label: {
            Label("Restart Karabiner-Elements", systemImage: "arrow.clockwise")
              .labelStyle(.titleAndIcon)
          }
        )

        if state.menuSettings.askForConfirmationBeforeQuitting {
          Menu(
            content: {
              Text("Are you sure you want to quit?")

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
    console_user_server_start(consoleUserServerTerminated)
    ConsoleUserServerUIState.shared.start()
    _ = NotificationWindowManager.shared
  }

  func applicationWillTerminate(_: Notification) {
    console_user_server_terminate()
  }
}
