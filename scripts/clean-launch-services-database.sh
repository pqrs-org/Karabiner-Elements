#!/bin/bash

set -u # forbid undefined variables
set -e # forbid command failure

# We have to check the running macOS version due to the following requirements:
# - NSWorkspace.shared.urlsForApplications is available since macOS 12.
# - swift regex literals is available since macOS 13.

major=$(sw_vers -productVersion | awk -F. '{print $1}')
if [ $major -ge 13 ]; then
    swift scripts/clean-launch-services-database.swift
else
    echo "Skip: clean-launch-services-database.swift is available since macOS 13"
fi
