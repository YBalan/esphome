#!/usr/bin/env python3
"""Turn the addresses from the "BMS Last Panic Backtrace" sensor into function names.

Usage (paste the whole sensor text, or just the addresses):
    python tools/decode_panic.py "0x400d1234 0x400e5678 0x40081abc"
    python tools/decode_panic.py --elf saved/firmware-2026-10-25.elf "0x400d1234 ..."

The ELF must be the one built for the firmware that was running when the panic
happened. A rebuild changes addresses, so copy firmware.elf somewhere safe each time
you flash a build, and pass that copy with --elf.
"""

import argparse
import datetime
import pathlib
import re
import subprocess
import sys

PROJECT = pathlib.Path(__file__).resolve().parent.parent
# Native ESP-IDF builds (ESPHome 2026.7+) put the ELF in build/; PlatformIO builds in .pioenvs/.
ELF_CANDIDATES = [
    PROJECT / ".esphome/build/jkbms-gateway-v2/build/firmware.elf",
    PROJECT / ".esphome/build/jkbms-gateway-v2/.pioenvs/jkbms-gateway-v2/firmware.elf",
]
DEFAULT_ELF = next((p for p in ELF_CANDIDATES if p.exists()), ELF_CANDIDATES[0])
# Where ESPHome keeps the compiler: native ESP-IDF cache first, then PlatformIO.
TOOLCHAIN_GLOBS = [
    (pathlib.Path.home() / "AppData/Local/esphome/Cache/idf/tools/xtensa-esp-elf", "*/xtensa-esp-elf/bin"),
    (pathlib.Path.home() / ".esphome/cache/idf/tools/xtensa-esp-elf", "*/xtensa-esp-elf/bin"),
    (pathlib.Path.home() / ".platformio/packages", "toolchain-xtensa-esp-elf/bin"),
]


def find_addr2line() -> pathlib.Path:
    for root, pattern in TOOLCHAIN_GLOBS:
        for bin_dir in sorted(root.glob(pattern), reverse=True):
            for name in ("xtensa-esp32-elf-addr2line.exe", "xtensa-esp32-elf-addr2line"):
                candidate = bin_dir / name
                if candidate.exists():
                    return candidate
    sys.exit("addr2line not found in the ESPHome toolchain cache; pass --addr2line")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("text", nargs="+", help="backtrace text or addresses (0x40xxxxxx ...)")
    parser.add_argument("--elf", type=pathlib.Path, default=DEFAULT_ELF, help="firmware.elf of the flashed build")
    parser.add_argument("--addr2line", type=pathlib.Path, default=None)
    args = parser.parse_args()

    addresses = re.findall(r"0x[0-9a-fA-F]{8}", " ".join(args.text))
    if not addresses:
        sys.exit("no 0x........ addresses found in the input")
    if not args.elf.exists():
        sys.exit(f"ELF not found: {args.elf}")

    built = datetime.datetime.fromtimestamp(args.elf.stat().st_mtime)
    print(f"ELF: {args.elf}  (built {built:%Y-%m-%d %H:%M:%S})")
    print("It must be the build that was running; otherwise the names below are wrong.\n")

    addr2line = args.addr2line or find_addr2line()
    result = subprocess.run(
        [str(addr2line), "-pfiaC", "-e", str(args.elf), *addresses],
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
