import Foundation
import ServiceManagement

func registerService(_ service: SMAppService) {
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
  let coreServices: [SMAppService] = [
    SMAppService.agent(plistName: "org.pqrs.karabiner.agent.karabiner_grabber.plist"),
    SMAppService.agent(plistName: "org.pqrs.karabiner.karabiner_session_monitor.plist"),
    SMAppService.daemon(plistName: "org.pqrs.Karabiner-VirtualHIDDevice-Daemon.plist"),
    SMAppService.daemon(plistName: "org.pqrs.karabiner.karabiner_grabber.plist"),
  ]

  let consoleUserServerAgentService = SMAppService.agent(
    plistName: "org.pqrs.karabiner.karabiner_console_user_server.plist")
  let notificationWindowAgentService = SMAppService.agent(
    plistName: "org.pqrs.karabiner.NotificationWindow.plist")

  var allServices: [SMAppService] = []
  for s in coreServices {
    allServices.append(s)
  }
  allServices.append(consoleUserServerAgentService)
  allServices.append(notificationWindowAgentService)

  for argument in CommandLine.arguments {
    if argument == "register-core" {
      for s in coreServices {
        registerService(s)
      }
      exit(0)

    } else if argument == "unregister-core" {
      for s in coreServices {
        unregisterService(s)
      }
      exit(0)

    } else if argument == "register-console-user-server-agent" {
      registerService(consoleUserServerAgentService)
      exit(0)

    } else if argument == "unregister-console-user-server-agent" {
      unregisterService(consoleUserServerAgentService)
      exit(0)

    } else if argument == "register-notification-window-agent" {
      registerService(notificationWindowAgentService)
      exit(0)

    } else if argument == "unregister-notification-window-agent" {
      unregisterService(notificationWindowAgentService)
      exit(0)

    } else if argument == "status" {
      for s in allServices {
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
  print("    Karabiner-Elements-Services subcommand")
  print("")
  print("Subcommands:")
  print("    register-core")
  print("    unregister-core")
  print("    register-console-user-server-agent")
  print("    register-notification-window-agent")
  print("    unregister-console-user-server-agent")
  print("    unregister-notification-window-agent")
  print("    status")

  exit(0)
}

RunLoop.main.run()
