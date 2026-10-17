try
    do shell script "'/Applications/Karabiner-Elements.app/Contents/Helpers/karabiner_cli' --copy-current-profile-to-system-default-profile" with administrator privileges
    display dialog "The system default profile has been updated." buttons {"OK"}
on error
    display alert "Failed to copy current profile to system default profile."
end try
