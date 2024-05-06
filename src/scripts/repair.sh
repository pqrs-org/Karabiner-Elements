#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin
export PATH

if [ ! -d /Applications/Karabiner-Elements.app ]; then
    echo "Restore /Applications/Karabiner-Elements.app"

    tar -C /Applications -xf '/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-Elements.app.tar.gz'

    '/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_cli' --lsregister-karabiner-elements
fi
