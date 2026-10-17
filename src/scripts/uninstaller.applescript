try
    display dialog "Are you sure you want to remove Karabiner-Elements?" buttons {"Cancel", "OK"}
    if the button returned of the result is "OK" then
        try
            do shell script "test -f '/Applications/Karabiner-Elements.app/Contents/Resources/uninstall.sh'"
            try
                do shell script "'/Applications/Karabiner-Elements.app/Contents/Helpers/Karabiner-Elements Non-Privileged Agents v3.app/Contents/MacOS/Karabiner-Elements Non-Privileged Agents v3' unregister-core-agents"

                do shell script "bash '/Library/Application Support/org.pqrs/Karabiner-DriverKit-VirtualHIDDevice/scripts/uninstall/deactivate_driver.sh'"

                display dialog "Removing Karabiner-Elements files.\nosascript will ask the administrator password to complete." buttons {"OK"}
                do shell script "bash '/Applications/Karabiner-Elements.app/Contents/Resources/uninstall.sh'" with administrator privileges

                display dialog "Karabiner-Elements has been uninstalled.\nPlease restart macOS." buttons {"OK"}
            on error
                display alert "Failed to uninstall Karabiner-Elements."
            end try
        on error
            display alert "Karabiner-Elements is not installed."
        end try
    end if
on error
    display alert "Karabiner-Elements uninstallation was canceled."
end try
