#!/usr/bin/python3

import subprocess
import sys


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} command")
        return 1

    command = sys.argv[1]
    full_output = []
    count = 0

    with subprocess.Popen(
        command,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    ) as process:
        assert process.stdout is not None

        for line in process.stdout:
            full_output.append(line)

            print(".", end="", flush=True)
            count += 1
            if count >= 70:
                print()
                count = 0

    print()

    if process.returncode != 0:
        print("".join(full_output))
        return 99

    return 0


if __name__ == "__main__":
    sys.exit(main())
