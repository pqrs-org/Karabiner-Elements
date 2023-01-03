#!/bin/bash

spec="project-with-codesign.yml"

if [[ -z $(bash ../../../scripts/get-codesign-identity.sh) ]]; then
  spec="project-without-codesign.yml"
fi

echo "Use $spec"
xcodegen generate --spec $spec
