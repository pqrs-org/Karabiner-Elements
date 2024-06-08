import Foundation
import ServiceManagement

enum Subcommand: String {
  case registerCoreDaemons = "register-core-daemons"
  case unregisterCoreDaemons = "unregister-core-daemons"

  case status = "status"
  case running = "running"
}

let coreDaemonServiceNames = [
  "org.pqrs.service.daemon.Karabiner-VirtualHIDDevice-Daemon",
  "org.pqrs.service.daemon.karabiner_grabber",
]

RunLoop.main.perform {
  let coreDaemons = coreDaemonServiceNames.map {
    SMAppService.daemon(plistName: "\($0).plist")
  }

  var allServices: [SMAppService] = []
  for s in coreDaemons {
    allServices.append(s)
  }

  if CommandLine.arguments.count > 1 {
    let subcommand = CommandLine.arguments[1]

    switch Subcommand(rawValue: subcommand) {
    case .registerCoreDaemons:
      ServiceManagementHelper.register(services: coreDaemons)
      exit(0)

    case .unregisterCoreDaemons:
      ServiceManagementHelper.unregister(services: coreDaemons)
      exit(0)

    case .status:
      ServiceManagementHelper.printStatuses(services: allServices)
      exit(0)

    case .running:
      var exitCode: Int32 = 0
      libkrbn_initialize()
      for n in coreDaemonServiceNames {
        n.withCString {
          if !libkrbn_services_daemon_running($0) {
            exitCode = 1
          }
        }
      }
      libkrbn_terminate()
      exit(exitCode)

    default:
      print("Unknown subcommand \(subcommand)")
      exit(1)
    }
  }

  print("Usage:")
  print("    'Karabiner-Elements Privileged Daemons' subcommand")
  print("")
  print("Subcommands:")
  print("    \(Subcommand.registerCoreDaemons.rawValue)")
  print("    \(Subcommand.unregisterCoreDaemons.rawValue)")

  print("    \(Subcommand.status.rawValue)")
  print("    \(Subcommand.running.rawValue)")

  exit(0)
}

RunLoop.main.run()
