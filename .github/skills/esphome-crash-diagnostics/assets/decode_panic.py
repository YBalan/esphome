#!/usr/bin/env python3
"""Turn the addresses from a "Last Panic Backtrace" sensor into function names.

Usage (paste the whole sensor text, or just the addresses), from the device folder:
    python decode_panic.py "0x400d1234 0x400e5678 0x40081abc"
    python decode_panic.py --elf saved/firmware-2026-10-25.elf "0x400d1234 ..."

The ELF must be the one built for the firmware that was running when the panic
happened. A rebuild changes addresses, so copy firmware.elf somewhere safe each time
you flash a build, and pass that copy with --elf. Without --elf the newest
.esphome/build/*/build/firmware.elf (native ESP-IDF build) or
.esphome/build/*/.pioenvs/*/firmware.elf (PlatformIO build) under the current folder is used.
"""

import argparse
import datetime
import pathlib
import re
import subprocess
import sys

# Where ESPHome keeps the compiler: native ESP-IDF cache first, then PlatformIO.
TOOLCHAIN_GLOBS = [
    (pathlib.Path.home() / "AppData/Local/esphome/Cache/idf/tools/xtensa-esp-elf", "*/xtensa-esp-elf/bin"),
    (pathlib.Path.home() / ".esphome/cache/idf/tools/xtensa-esp-elf", "*/xtensa-esp-elf/bin"),
    (pathlib.Path.home() / ".platformio/packages", "toolchain-xtensa-esp-elf/bin"),
]


def newest_elf() -> pathlib.Path | None:
    candidates = list(pathlib.Path.cwd().glob(".esphome/build/*/build/firmware.elf"))
    candidates += pathlib.Path.cwd().glob(".esphome/build/*/.pioenvs/*/firmware.elf")
    return max(candidates, key=lambda p: p.stat().st_mtime) if candidates else None


def find_addr2line(chip: str) -> pathlib.Path:
    for root, pattern in TOOLCHAIN_GLOBS:
        for bin_dir in sorted(root.glob(pattern), reverse=True):
            for name in (f"xtensa-{chip}-elf-addr2line.exe", f"xtensa-{chip}-elf-addr2line"):
                candidate = bin_dir / name
                if candidate.exists():
                    return candidate
    sys.exit(f"addr2line for {chip} not found in the ESPHome toolchain cache; pass --addr2line")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("text", nargs="+", help="backtrace text or addresses (0x40xxxxxx ...)")
    parser.add_argument("--elf", type=pathlib.Path, default=None, help="firmware.elf of the flashed build")
    parser.add_argument("--chip", default="esp32", help="esp32, esp32s2 or esp32s3 (Xtensa only)")
    parser.add_argument("--addr2line", type=pathlib.Path, default=None)
    args = parser.parse_args()

    addresses = re.findall(r"0x[0-9a-fA-F]{8}", " ".join(args.text))
    if not addresses:
        sys.exit("no 0x........ addresses found in the input")
    elf = args.elf or newest_elf()
    if elf is None or not elf.exists():
        sys.exit("firmware.elf not found: pass --elf <path>")

    built = datetime.datetime.fromtimestamp(elf.stat().st_mtime)
    print(f"ELF: {elf}  (built {built:%Y-%m-%d %H:%M:%S})")
    print("It must be the build that was running; otherwise the names below are wrong.\n")

    addr2line = args.addr2line or find_addr2line(args.chip)
    result = subprocess.run(
        [str(addr2line), "-pfiaC", "-e", str(elf), *addresses],
        capture_output=True,
        text=True,
        check=False,
    )
    print(result.stdout.rstrip())
    if result.returncode != 0:
        print(result.stderr.rstrip(), file=sys.stderr)
        sys.exit(result.returncode)


if __name__ == "__main__":
    main()
