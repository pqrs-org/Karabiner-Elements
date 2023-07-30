#!/usr/bin/python3

'''Replace @VERSION@'''

import re
from pathlib import Path
from itertools import chain


def update_version():
    '''Replace @VERSION@'''

    top_directory = Path(__file__).resolve(True).parents[1]

    with top_directory.joinpath('version').open(encoding='utf-8') as version_file:
        version = version_file.readline().strip()

        for template_file_path in chain(top_directory.rglob('*.hpp.in'),
                                        top_directory.rglob('*.h.in'),
                                        top_directory.rglob('*.plist.in'),
                                        top_directory.rglob('*.xml.in')):
            # Skip vendor directory
            if re.search(r'/vendor/', str(template_file_path)):
                continue

            replaced_file_path = Path(
                re.sub(r'\.in$', '', str(template_file_path)))
            needs_update = False

            with template_file_path.open('r') as template_file:
                template_lines = template_file.readlines()
                replaced_lines = []

                if replaced_file_path.exists():
                    with replaced_file_path.open(encoding='utf-8') as replaced_file:
                        replaced_lines = replaced_file.readlines()
                        while len(replaced_lines) < len(template_lines):
                            replaced_lines.append('')
                else:
                    replaced_lines = template_lines

                for index, template_line in enumerate(template_lines):
                    line = template_line
                    line = line.replace('@VERSION@', version)

                    if replaced_lines[index] != line:
                        needs_update = True
                        replaced_lines[index] = line

            if needs_update:
                with replaced_file_path.open('w', encoding='utf-8') as replaced_file:
                    print("Update " + str(replaced_file_path))
                    replaced_file.write(''.join(replaced_lines))


if __name__ == "__main__":
    update_version()
