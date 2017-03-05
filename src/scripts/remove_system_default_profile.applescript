try
    do shell script "'/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_cli' --remove-system-default-profile" with administrator privileges
on error
    display alert "Failed to remove the system default profile."
end try
