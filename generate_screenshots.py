#!/usr/bin/env python3
import os
import time
from pathlib import Path
import sys
import json
from subprocess import check_call
from typing import Any
from enum import StrEnum
import shutil

THIS_DIR = Path(__file__).absolute().parent
DEBUG_CHEATSHEET = "DEBUG_CHEATSHEET"
DEBUG_TIME = "DEBUG_TIME"
DEBUG_BBOX = "DEBUG_BBOX"


def make_env(flags: list[str] | None) -> dict[str, str]:
    env = os.environ.copy()
    if flags is None:
        return env
    cflags = []
    for f in flags:
        cflags.append(f"-D{f}")
    env["CFLAGS"] = " ".join(cflags)
    return env


def make_gif(inputs: list[Path], output: Path) -> None:
    check_call(
        [
            "convert",
            "-delay", "70",
            "-loop", "0",
            "-dispose", "previous",
            *[str(name) for name in inputs],
            str(output),
        ]
    )


def pebble(*args: str, flags: None | list[str] = None) -> None:
    check_call(["pebble"] + list(args), env=make_env(flags))


def animated_screenshots(platforms: list[str]) -> None:
    hour = 3
    frames_dir = (THIS_DIR / "screenshot_animation_frames")
    shutil.rmtree(frames_dir, ignore_errors=True)
    frames_dir.mkdir(exist_ok=True)
    for platform in platforms:
        platform_dir = frames_dir / platform
        platform_dir.mkdir()
    for minute in range(60):
        for platform in platforms:
            pebble("build", flags=[f"HOUR_OVERRIDE={hour}", f"MINUTE_OVERRIDE={minute}"])
            pebble("install", "--emulator", platform)
            pebble("screenshot", "--emulator", platform, str(frames_dir / platform / f"screenshot_{hour:02d}{minute:02d}.png"))
    for platform in platforms:
        make_gif(sorted((frames_dir / platform).glob("*.png")), THIS_DIR / f"animated_{platform}.gif")


def static_screenshots(platforms: list[str], flags: list[str], name: str) -> None:
    pebble("build", flags=flags)
    for platform in platforms:
        pebble("emu-time-format", "--format", "12h", "--emulator", platform)
        pebble("install", "--emulator", platform)
        pebble("screenshot", "--emulator", platform, f"screenshot_{platform}_{name}.png")


def main() -> None:
    pebble("wipe")
    pebble("kill")
    with open(THIS_DIR / "package.json", mode="r") as f:
        package = json.load(f)
    platforms = package["pebble"]["targetPlatforms"]
    static_screenshots(platforms, ["HOUR_OVERRIDE=7", "MINUTE_OVERRIDE=52"], "0752")
    static_screenshots(platforms, [DEBUG_CHEATSHEET], "guide")
    animated_screenshots(platforms)
    pebble("wipe")
    pebble("kill")


if __name__ == "__main__":
    main()
