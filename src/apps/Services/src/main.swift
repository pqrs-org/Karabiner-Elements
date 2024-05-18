import Foundation
import ServiceManagement

let daemonPlistNames = [
  "org.pqrs.Karabiner-VirtualHIDDevice-Daemon.plist"
]

RunLoop.main.perform {
  for argument in CommandLine.arguments {
    if argument == "register" {
      for daemonPlistName in daemonPlistNames {
        let s = SMAppService.daemon(plistName: daemonPlistName)
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
      for daemonPlistName in daemonPlistNames {
        let s = SMAppService.daemon(plistName: daemonPlistName)
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
      for daemonPlistName in daemonPlistNames {
        let s = SMAppService.daemon(plistName: daemonPlistName)
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
  print("    SMAppServiceExample register|unregister|status")
  exit(0)
}

RunLoop.main.run()
