import Foundation
import ServiceManagement

let agentPlistNames = [
  "org.pqrs.karabiner.agent.karabiner_grabber.plist",
  "org.pqrs.karabiner.karabiner_console_user_server.plist",
  "org.pqrs.karabiner.karabiner_session_monitor.plist",
  "org.pqrs.karabiner.NotificationWindow.plist",
]

let daemonPlistNames = [
  "org.pqrs.Karabiner-VirtualHIDDevice-Daemon.plist",
  "org.pqrs.karabiner.karabiner_grabber.plist",
]

RunLoop.main.perform {
  //
  // Prepare services
  //

  var services: [SMAppService] = []

  for n in agentPlistNames {
    services.append(SMAppService.agent(plistName: n))
  }
  for n in daemonPlistNames {
    services.append(SMAppService.daemon(plistName: n))
  }

  //
  // Process
  //

  for argument in CommandLine.arguments {
    if argument == "register" {
      for s in services {
        do {
          try s.register()
          print("Successfully registered \(s)")
        } catch {
          print("Unable to register \(error)")
          exit(1)
        }
      }
      exit(0)

    } else if argument == "unregister" {
      for s in services {
        do {
          try s.unregister()
          print("Successfully unregistered \(s)")
        } catch {
          print("Unable to unregister \(error)")
          exit(1)
        }
      }
      exit(0)

    } else if argument == "status" {
      for s in services {
        switch s.status {
        case .notRegistered:
          print("\(s) notRegistered")
        case .enabled:
          print("\(s) enabled")
        case .requiresApproval:
          print("\(s) requiresApproval")
        case .notFound:
          print("\(s) notFound")
        @unknown default:
          print("\(s) unknown \(s.status)")
        }
      }

      exit(0)
    }
  }

  print("Usage:")
  print("    Karabiner-Elements-Services register|unregister|status")
  exit(0)
}

RunLoop.main.run()
