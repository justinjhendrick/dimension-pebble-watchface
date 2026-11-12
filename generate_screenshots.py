#!/usr/bin/env python3
import os
import time
from pathlib import Path
import sys
import json
from subprocess import check_call

THIS_DIR = Path(__file__).absolute().parent

def main() -> int:
    with open(THIS_DIR / "package.json", mode="r") as f:
        package = json.load(f)
    for cflags in ["", f"-DDEBUG_CHEATSHEET"]:
        env = os.environ.copy()
        env["CFLAGS"] = cflags
        check_call(["pebble", "build"], env=env)
        for platform in package["pebble"]["targetPlatforms"]:
            check_call(["pebble", "wipe"])
            check_call(["pebble", "kill"])
            check_call(["pebble", "install", "--emulator", platform])
            time.sleep(2.0)
            if cflags == "":
                check_call(["pebble", "screenshot", f"screenshot_{platform}.png"])
            else:
                check_call(["pebble", "screenshot", f"screenshot_{platform}_guide.png"])
        check_call(["pebble", "wipe"])
        check_call(["pebble", "kill"])
    return 0

if __name__ == "__main__":
    sys.exit(main())
