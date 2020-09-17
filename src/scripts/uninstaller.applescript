try
    display dialog "Are you sure you want to remove Karabiner-Elements?" buttons {"Cancel", "OK"}
    if the button returned of the result is "OK" then
        try
            do shell script "test -f '/Library/Application Support/org.pqrs/Karabiner-Elements/uninstall.sh'"
            try
                do shell script "bash '/Library/Application Support/org.pqrs/Karabiner-DriverKit-VirtualHIDDevice/scripts/uninstall/deactivate_driver.sh'"
                display dialog "Removing Karabiner-Elements files.\nosascript will ask the administrator password to complete." buttons {"OK"}
                do shell script "bash '/Library/Application Support/org.pqrs/Karabiner-Elements/uninstall.sh'" with administrator privileges
                display dialog "Karabiner-Elements has been uninstalled.\nPlease restart your system." buttons {"OK"}
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
