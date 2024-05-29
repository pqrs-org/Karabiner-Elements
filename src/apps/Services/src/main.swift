import Foundation
import ServiceManagement

enum Subcommand: String {
  case registerCoreDaemons = "register-core-daemons"
  case unregisterCoreDaemons = "unregister-core-daemons"

  case registerCoreAgents = "register-core-agents"
  case unregisterCoreAgents = "unregister-core-agents"

  case registerMenuAgent = "register-menu-agent"
  case unregisterMenuAgent = "unregister-menu-agent"

  case registerNotificationWindowAgent = "register-notification-window-agent"
  case unregisterNotificationWindowAgent = "unregister-notification-window-agent"

  case status = "status"
}

func registerService(_ service: SMAppService) {
  // Regarding daemons, performing the following steps causes inconsistencies in the user approval database,
  // so the process will not start again until it is unregistered and then registered again.
  //
  // 1. Register a daemon.
  // 2. Approve the daemon.
  // 3. The database is reset using `sfltool resetbtm`.
  // 4. Restart macOS.
  //
  // When this happens, the service status becomes .notFound.
  // So, if the service status is .notFound, we call unregister before register to avoid this issue.
  //
  // Another case where it becomes .notFound is when it has never actually been registered before.
  // Even in this case, calling unregister will not have any negative impact.

  if service.status == .notFound {
    unregisterService(service)
  }

  do {
    try service.register()
    print("Successfully registered \(service)")
  } catch {
    // Note:
    // When calling `SMAppService.daemon.register`, if user approval has not been granted, an `Operation not permitted` error will be returned.
    // To call `register` for all agents and daemons, the loop continues even if an error occurs.
    // Therefore, only log output will be performed here.
    print("Unable to register \(error)")
  }
}

func unregisterService(_ service: SMAppService) {
  do {
    try service.unregister()
    print("Successfully unregistered \(service)")
  } catch {
    print("Unable to unregister \(error)")
  }
}

RunLoop.main.perform {
  let coreDaemons: [SMAppService] = [
    SMAppService.daemon(
      plistName: "org.pqrs.service.daemon.Karabiner-VirtualHIDDevice-Daemon.plist"),
    SMAppService.daemon(
      plistName: "org.pqrs.service.daemon.karabiner_grabber.plist"),
  ]

  let coreAgents: [SMAppService] = [
    SMAppService.agent(
      plistName: "org.pqrs.service.agent.karabiner_console_user_server.plist"),
    SMAppService.agent(
      plistName: "org.pqrs.service.agent.karabiner_grabber.plist"),
    SMAppService.agent(
      plistName: "org.pqrs.service.agent.karabiner_session_monitor.plist"),
  ]

  let menuAgentService = SMAppService.agent(
    plistName: "org.pqrs.service.agent.Karabiner-Menu.plist")

  let notificationWindowAgentService = SMAppService.agent(
    plistName: "org.pqrs.service.agent.Karabiner-NotificationWindow.plist")

  var allServices: [SMAppService] = []
  for s in coreDaemons {
    allServices.append(s)
  }
  for s in coreAgents {
    allServices.append(s)
  }
  allServices.append(menuAgentService)
  allServices.append(notificationWindowAgentService)

  if CommandLine.arguments.count > 1 {
    let subcommand = CommandLine.arguments[1]

    switch Subcommand(rawValue: subcommand) {
    case .registerCoreDaemons:
      for s in coreDaemons {
        registerService(s)
      }
      exit(0)

    case .unregisterCoreDaemons:
      for s in coreDaemons {
        unregisterService(s)
      }
      exit(0)

    case .registerCoreAgents:
      for s in coreAgents {
        registerService(s)
      }
      exit(0)

    case .unregisterCoreAgents:
      for s in coreAgents {
        unregisterService(s)
      }
      exit(0)

    case .registerMenuAgent:
      registerService(menuAgentService)
      exit(0)

    case .unregisterMenuAgent:
      unregisterService(menuAgentService)
      exit(0)

    case .registerNotificationWindowAgent:
      registerService(notificationWindowAgentService)
      exit(0)

    case .unregisterNotificationWindowAgent:
      unregisterService(notificationWindowAgentService)
      exit(0)

    case .status:
      for s in allServices {
        switch s.status {
        case .notRegistered:
          // A service that was once registered but then unregistered becomes notRegistered.
          print("\(s) notRegistered")
        case .enabled:
          print("\(s) enabled")
        case .requiresApproval:
          print("\(s) requiresApproval")
        case .notFound:
          // A service that has never been registered becomes notFound. Resetting the database with `sfltool resetbtm` also results in notFound.
          print("\(s) notFound")
        @unknown default:
          print("\(s) unknown \(s.status)")
        }
      }
      exit(0)

    default:
      print("Unknown subcommand \(subcommand)")
      exit(1)
    }
  }

  print("Usage:")
  print("    Karabiner-Elements-Services subcommand")
  print("")
  print("Subcommands:")
  print("    \(Subcommand.registerCoreDaemons.rawValue)")
  print("    \(Subcommand.unregisterCoreDaemons.rawValue)")

  print("    \(Subcommand.registerCoreAgents.rawValue)")
  print("    \(Subcommand.unregisterCoreAgents.rawValue)")

  print("    \(Subcommand.registerMenuAgent.rawValue)")
  print("    \(Subcommand.unregisterMenuAgent.rawValue)")

  print("    \(Subcommand.registerNotificationWindowAgent.rawValue)")
  print("    \(Subcommand.unregisterNotificationWindowAgent.rawValue)")

  print("    \(Subcommand.status.rawValue)")

  exit(0)
}

RunLoop.main.run()
