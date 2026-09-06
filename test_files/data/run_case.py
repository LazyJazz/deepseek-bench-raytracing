#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path


def main():
    if len(sys.argv) != 6:
        return 2
    renderer, comparator, scene, reference, output = sys.argv[1:]
    output_path = Path(output)
    output_path.unlink(missing_ok=True)
    try:
        result = subprocess.run([renderer, scene, output], stdin=subprocess.DEVNULL,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                text=True, timeout=100)
    except subprocess.TimeoutExpired:
        print("renderer timed out")
        return 1
    if result.returncode != 0:
        print("renderer exited with code {}".format(result.returncode))
        print(result.stdout[-4000:])
        return 1
    if not output_path.is_file():
        print("renderer did not create the requested PNG")
        return 1
    compared = subprocess.run([comparator, output, reference], stdin=subprocess.DEVNULL)
    return compared.returncode


if __name__ == "__main__":
    sys.exit(main())
