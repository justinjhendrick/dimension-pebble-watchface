#!/usr/bin/env python3
import os
import time
from pathlib import Path
import sys
import json
from subprocess import check_call
from typing import Any

THIS_DIR = Path(__file__).absolute().parent

def pebble(*args: str, **kwargs: Any) -> None:
    check_call(["pebble"] + list(args), **kwargs)

def main() -> int:
    # TODO: animated gif screenshots
    with open(THIS_DIR / "package.json", mode="r") as f:
        package = json.load(f)
    for cflags in ["", f"-DDEBUG_CHEATSHEET"]:
        env = os.environ.copy()
        env["CFLAGS"] = cflags
        pebble("build", env=env)
        for platform in package["pebble"]["targetPlatforms"]:
            pebble("wipe")
            pebble("kill")
            pebble("emu-time-format", "--format", "12h", "--emulator", platform)
            pebble("install", "--emulator", platform)
            time.sleep(1.0)
            if cflags == "":
                pebble("screenshot", f"screenshot_{platform}.png")
            else:
                pebble("screenshot", f"screenshot_{platform}_guide.png")
        pebble("wipe")
        pebble("kill")
    return 0

if __name__ == "__main__":
    sys.exit(main())
