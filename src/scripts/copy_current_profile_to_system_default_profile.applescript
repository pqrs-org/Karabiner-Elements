try
    do shell script "'/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_cli' --copy-current-profile-to-system-default-profile" with administrator privileges
on error
    display alert "Failed to copy current profile to system default profile."
end try
