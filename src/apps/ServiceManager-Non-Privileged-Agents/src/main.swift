import Foundation
import ServiceManagement

enum Subcommand: String {
  case registerCoreAgents = "register-core-agents"
  case unregisterCoreAgents = "unregister-core-agents"
  case coreAgentsEnabled = "core-agents-enabled"

  case registerMultitouchExtensionAgent = "register-multitouch-extension-agent"
  case unregisterMultitouchExtensionAgent = "unregister-multitouch-extension-agent"

  case status
  case running
}

RunLoop.main.perform {
  let coreAgentPlistNames = [
    "org.pqrs.service.agent.Karabiner-Core-Service-v3.plist",
    // unregisterCoreAgents may be invoked by Karabiner-Console-User-Server.
    // Unregistering its own service can terminate the caller before the operation completes,
    // so unregister Karabiner-Console-User-Server last.
    "org.pqrs.service.agent.Karabiner-Console-User-Server-v3.plist",
  ]

  let coreAgents = coreAgentPlistNames.map {
    SMAppService.agent(plistName: $0)
  }

  let multitouchExtensionAgentService = SMAppService.agent(
    plistName: "org.pqrs.service.agent.Karabiner-MultitouchExtension-v3.plist")

  var allServices: [SMAppService] = []
  for s in coreAgents {
    allServices.append(s)
  }
  allServices.append(multitouchExtensionAgentService)

  if CommandLine.arguments.count > 1 {
    let subcommand = CommandLine.arguments[1]

    switch Subcommand(rawValue: subcommand) {
    case .registerCoreAgents:
      exit(ServiceManagementHelper.register(services: coreAgents) ? 0 : 1)

    case .unregisterCoreAgents:
      ServiceManagementHelper.unregister(services: coreAgents)
      exit(0)

    case .coreAgentsEnabled:
      if ServiceManagementHelper.enabled(services: coreAgents) {
        print("enabled")
        exit(0)
      } else {
        print("There are services that are not enabled")
        exit(1)
      }

    case .registerMultitouchExtensionAgent:
      exit(
        ServiceManagementHelper.register(services: [multitouchExtensionAgentService]) ? 0 : 1)

    case .unregisterMultitouchExtensionAgent:
      ServiceManagementHelper.unregister(services: [multitouchExtensionAgentService])
      exit(0)

    case .status:
      ServiceManagementHelper.printStatuses(services: allServices)
      exit(0)

    case .running:
      var exitCode: Int32 = 0
      for plistName in coreAgentPlistNames {
        guard
          let label = ServiceManagementHelper.launchdLabel(
            plistName: plistName,
            directoryName: "LaunchAgents"
          )
        else {
          exitCode = 1
          continue
        }

        label.withCString {
          if !krbn_agent_running($0) {
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
  print("    'Karabiner-Elements Non-Privileged Agents v3' subcommand")
  print("")
  print("Subcommands:")
  print("    \(Subcommand.registerCoreAgents.rawValue)")
  print("    \(Subcommand.unregisterCoreAgents.rawValue)")
  print("    \(Subcommand.coreAgentsEnabled.rawValue)")

  print("    \(Subcommand.registerMultitouchExtensionAgent.rawValue)")
  print("    \(Subcommand.unregisterMultitouchExtensionAgent.rawValue)")

  print("    \(Subcommand.status.rawValue)")
  print("    \(Subcommand.running.rawValue)")

  exit(0)
}

RunLoop.main.run()
