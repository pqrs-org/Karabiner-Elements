import Foundation
import ServiceManagement

enum Subcommand: String {
  case registerCoreDaemons = "register-core-daemons"
  case unregisterCoreDaemons = "unregister-core-daemons"

  case status = "status"
}

RunLoop.main.perform {
  let coreDaemons: [SMAppService] = [
    SMAppService.daemon(
      plistName: "org.pqrs.service.daemon.Karabiner-VirtualHIDDevice-Daemon.plist"),
    SMAppService.daemon(
      plistName: "org.pqrs.service.daemon.karabiner_grabber.plist"),
  ]

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

    default:
      print("Unknown subcommand \(subcommand)")
      exit(1)
    }
  }

  print("Usage:")
  print("    Karabiner-Elements-Daemons subcommand")
  print("")
  print("Subcommands:")
  print("    \(Subcommand.registerCoreDaemons.rawValue)")
  print("    \(Subcommand.unregisterCoreDaemons.rawValue)")

  print("    \(Subcommand.status.rawValue)")

  exit(0)
}

RunLoop.main.run()
