import AppKit

@main
class AppDelegate: NSObject, NSApplicationDelegate {
    var timer: Timer?
    var count: Int = 0
    
    func applicationDidFinishLaunching(_ aNotification: Notification) {
        print("applicationDidFinishLaunching")
    }

    public func applicationShouldTerminate(_: NSApplication) -> NSApplication.TerminateReply {
        print("applicationShouldTerminate")
        
        for count in 1...10 {
            print("i \(count)")
            sleep(1)
        }

        return .terminateNow
    }
}

