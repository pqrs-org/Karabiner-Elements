#!/usr/bin/python3

import os
import re
import sys
from pathlib import Path
from itertools import chain

topDirectory = Path(__file__).resolve(True).parents[1]

with topDirectory.joinpath('version').open() as versionFile:
    version = versionFile.readline().strip()

    for templateFilePath in chain(topDirectory.rglob('*.h.in'),
                                  topDirectory.rglob('*.hpp.in'),
                                  topDirectory.rglob('*.plist.in'),
                                  topDirectory.rglob('*.xml.in')):
        # Skip vendor directory
        if re.search(r'/vendor/', str(templateFilePath)):
            continue

        replacedFilePath = Path(re.sub(r'\.in$', '', str(templateFilePath)))
        needsUpdate = False

        with templateFilePath.open('r') as templateFile:
            templateLines = templateFile.readlines()
            replacedLines = []

            if replacedFilePath.exists():
                with replacedFilePath.open('r') as replacedFile:
                    replacedLines = replacedFile.readlines()
                    while len(replacedLines) < len(templateLines):
                        replacedLines.append('')
            else:
                replacedLines = templateLines

            for index, templateLine in enumerate(templateLines):
                line = templateLine
                line = line.replace('@VERSION@', version)

                if (replacedLines[index] != line):
                    needsUpdate = True
                    replacedLines[index] = line

        if needsUpdate:
            with replacedFilePath.open('w') as replacedFile:
                print("Update " + str(replacedFilePath))
                replacedFile.write(''.join(replacedLines))
