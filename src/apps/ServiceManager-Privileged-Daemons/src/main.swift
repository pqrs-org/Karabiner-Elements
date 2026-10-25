import Foundation
import ServiceManagement

enum Subcommand: String {
  case registerCoreDaemons = "register-core-daemons"
  case unregisterCoreDaemons = "unregister-core-daemons"
  case coreDaemonsEnabled = "core-daemons-enabled"

  case status
  case running
}

RunLoop.main.perform {
  let coreDaemonPlistNames = [
    "org.pqrs.service.daemon.Karabiner-VirtualHIDDevice-Daemon-v3.plist",
    "org.pqrs.service.daemon.Karabiner-Core-Service-v3.plist",
  ]

  let coreDaemons = coreDaemonPlistNames.map {
    SMAppService.daemon(plistName: $0)
  }

  var allServices: [SMAppService] = []
  for s in coreDaemons {
    allServices.append(s)
  }

  if CommandLine.arguments.count > 1 {
    let subcommand = CommandLine.arguments[1]

    switch Subcommand(rawValue: subcommand) {
    case .registerCoreDaemons:
      exit(ServiceManagementHelper.register(services: coreDaemons) ? 0 : 1)

    case .unregisterCoreDaemons:
      ServiceManagementHelper.unregister(services: coreDaemons)
      exit(0)

    case .coreDaemonsEnabled:
      if ServiceManagementHelper.enabled(services: coreDaemons) {
        print("enabled")
        exit(0)
      } else {
        print("There are services that are not enabled")
        exit(1)
      }

    case .status:
      ServiceManagementHelper.printStatuses(services: allServices)
      exit(0)

    case .running:
      var exitCode: Int32 = 0
      for plistName in coreDaemonPlistNames {
        guard
          let label = ServiceManagementHelper.launchdLabel(
            plistName: plistName,
            directoryName: "LaunchDaemons"
          )
        else {
          exitCode = 1
          continue
        }

        label.withCString {
          if !krbn_daemon_running($0) {
            print("\(label) is not running")
            exitCode = 1
          }
        }
      }
      exit(exitCode)

    default:
      print("Unknown subcommand \(subcommand)")
      exit(1)
    }
  }

  print("Usage:")
  print("    'Karabiner-Elements Privileged Daemons v3' subcommand")
  print("")
  print("Subcommands:")
  print("    \(Subcommand.registerCoreDaemons.rawValue)")
  print("    \(Subcommand.unregisterCoreDaemons.rawValue)")
  print("    \(Subcommand.coreDaemonsEnabled.rawValue)")

  print("    \(Subcommand.status.rawValue)")
  print("    \(Subcommand.running.rawValue)")

  exit(0)
}

RunLoop.main.run()
