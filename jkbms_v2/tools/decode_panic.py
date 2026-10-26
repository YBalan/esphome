#!/usr/bin/env python3
"""Turn the addresses from the "BMS Last Panic Backtrace" sensor into function names.

Usage (paste the sensor text, or just the addresses):
    python tools/decode_panic.py "0x400d1234 0x400e5678 0x40081abc"
    python tools/decode_panic.py "<BMS Last Panic text> <BMS Last Panic Backtrace text>"
    python tools/decode_panic.py --elf elf-archive/firmware-2026-10-26_0944-10c1fea0.elf "0x400d1234 ..."

The ELF must be the one built for the firmware that was running when the panic
happened. Every build copies its ELF to elf-archive/ (elf_archive/ hook), named
firmware-<date>-<id>.elf. When the pasted text contains "elf=<id>" (it is part of
"BMS Last Panic") the matching file is picked from the archive and its hash checked.
Without an id the newest build/firmware.elf is used and you must compare its id with
the "elf=" value yourself.
"""

import argparse
import datetime
import hashlib
import pathlib
import re
import subprocess
import sys

PROJECT = pathlib.Path(__file__).resolve().parent.parent
ARCHIVE = PROJECT / "elf-archive"
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


def elf_id(path: pathlib.Path) -> str:
    """First 8 hex digits of the ELF's SHA-256: the "elf=" value the device reports."""
    digest = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()[:8]


def pick_elf(wanted_id: str | None) -> pathlib.Path:
    if wanted_id is None:
        return DEFAULT_ELF
    for path in sorted(ARCHIVE.glob(f"firmware-*-{wanted_id}.elf"), reverse=True):
        return path
    for path in ELF_CANDIDATES:
        if path.exists() and elf_id(path) == wanted_id:
            return path
    archived = ", ".join(p.name for p in sorted(ARCHIVE.glob("firmware-*.elf"))) or "nothing archived yet"
    sys.exit(f"no ELF with build id {wanted_id} in {ARCHIVE} or the build folder.\nArchived: {archived}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("text", nargs="+", help="backtrace text or addresses (0x40xxxxxx ...)")
    parser.add_argument("--elf", type=pathlib.Path, default=None, help="firmware.elf of the flashed build")
    parser.add_argument("--addr2line", type=pathlib.Path, default=None)
    args = parser.parse_args()

    text = " ".join(args.text)
    addresses = re.findall(r"0x[0-9a-fA-F]{8}", text)
    if not addresses:
        sys.exit("no 0x........ addresses found in the input")
    match = re.search(r"elf=([0-9a-fA-F]{8})", text)
    wanted_id = match.group(1).lower() if match else None

    elf = args.elf or pick_elf(wanted_id)
    if not elf.exists():
        sys.exit(f"ELF not found: {elf}")

    file_time = datetime.datetime.fromtimestamp(elf.stat().st_mtime)
    actual_id = elf_id(elf)
    print(f"ELF: {elf}  (file time {file_time:%Y-%m-%d %H:%M:%S}, id {actual_id})")
    if wanted_id is None:
        print("No elf=<id> in the input: compare this id with the elf= value of BMS Last Panic.\n")
    elif actual_id != wanted_id:
        sys.exit(f"WRONG ELF: the device ran build {wanted_id}, this file is {actual_id}. The names would be wrong.")
    else:
        print(f"Matches the build the device ran (elf={wanted_id}).\n")

    addr2line = args.addr2line or find_addr2line()
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
