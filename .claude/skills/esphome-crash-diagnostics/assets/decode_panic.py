#!/usr/bin/env python3
"""Turn the addresses from a "Last Panic Backtrace" sensor into function names.

Usage (paste the whole sensor text, or just the addresses), from the device folder:
    python decode_panic.py "0x400d1234 0x400e5678 0x40081abc"
    python decode_panic.py "<Last Panic text> <Last Panic Backtrace text>"
    python decode_panic.py --elf elf-archive/firmware-2026-10-25_1031-10573ef7.elf "0x400d1234 ..."

The ELF must be the one built for the firmware that was running when the panic
happened; a rebuild changes every address. With the elf_archive hook (assets/elf_archive/)
each build copies its ELF to elf-archive/firmware-<date>-<id>.elf, <id> being the first 8
hex digits of its SHA-256, which is the "elf=<id>" the Last Panic sensor shows. When the
pasted text contains "elf=<id>" the matching file is picked from the archive and its hash
is checked, so a wrong ELF is refused. Without an id, or without the archive, the newest
.esphome/build/*/build/firmware.elf (native ESP-IDF build) or
.esphome/build/*/.pioenvs/*/firmware.elf (PlatformIO build) under the current folder is
used and you must compare its id with the sensor yourself.
"""

import argparse
import datetime
import hashlib
import pathlib
import re
import subprocess
import sys

ARCHIVE = pathlib.Path.cwd() / "elf-archive"
# Where ESPHome keeps the compiler: native ESP-IDF cache first, then PlatformIO.
TOOLCHAIN_GLOBS = [
    (pathlib.Path.home() / "AppData/Local/esphome/Cache/idf/tools/xtensa-esp-elf", "*/xtensa-esp-elf/bin"),
    (pathlib.Path.home() / ".esphome/cache/idf/tools/xtensa-esp-elf", "*/xtensa-esp-elf/bin"),
    (pathlib.Path.home() / ".platformio/packages", "toolchain-xtensa-esp-elf/bin"),
]


def build_elfs() -> list[pathlib.Path]:
    candidates = list(pathlib.Path.cwd().glob(".esphome/build/*/build/firmware.elf"))
    candidates += pathlib.Path.cwd().glob(".esphome/build/*/.pioenvs/*/firmware.elf")
    return sorted(candidates, key=lambda p: p.stat().st_mtime, reverse=True)


def elf_id(path: pathlib.Path) -> str:
    """First 8 hex digits of the ELF's SHA-256: the "elf=" value the device reports."""
    digest = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()[:8]


def pick_elf(wanted_id: str | None) -> pathlib.Path | None:
    if wanted_id is None:
        elfs = build_elfs()
        return elfs[0] if elfs else None
    for path in sorted(ARCHIVE.glob(f"firmware-*-{wanted_id}.elf"), reverse=True):
        return path
    for path in build_elfs():
        if elf_id(path) == wanted_id:
            return path
    archived = ", ".join(p.name for p in sorted(ARCHIVE.glob("firmware-*.elf"))) or "nothing archived yet"
    sys.exit(f"no ELF with build id {wanted_id} in {ARCHIVE} or the build folder.\nArchived: {archived}")


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

    text = " ".join(args.text)
    addresses = re.findall(r"0x[0-9a-fA-F]{8}", text)
    if not addresses:
        sys.exit("no 0x........ addresses found in the input")
    match = re.search(r"elf=([0-9a-fA-F]{8})", text)
    wanted_id = match.group(1).lower() if match else None

    elf = args.elf or pick_elf(wanted_id)
    if elf is None or not elf.exists():
        sys.exit("firmware.elf not found: pass --elf <path>")

    file_time = datetime.datetime.fromtimestamp(elf.stat().st_mtime)
    actual_id = elf_id(elf)
    print(f"ELF: {elf}  (file time {file_time:%Y-%m-%d %H:%M:%S}, id {actual_id})")
    if wanted_id is None:
        print("No elf=<id> in the input: compare this id with the elf= value of the Last Panic sensor.\n")
    elif actual_id != wanted_id:
        sys.exit(f"WRONG ELF: the device ran build {wanted_id}, this file is {actual_id}. The names would be wrong.")
    else:
        print(f"Matches the build the device ran (elf={wanted_id}).\n")

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
