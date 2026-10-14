#!/usr/bin/env python3
"""Patch the retail XOnline Dashboard with selectable UIX Lite features."""

import argparse
import hashlib
import os
import struct
import sys

SHA256 = "ffe013d6067025281bc3c9bc9f9b62c99107baafdfb1005c2e98ae6dbc74714d"
CODE_VA = 0x00158D00
HERE = os.path.dirname(os.path.abspath(__file__))

FEATURES = ["colors", "textures", "video720", "redirect", "fontpath"]
DEFAULT_FEATURES = set(FEATURES)

# (virtual address, retail bytes, payload symbol, opcode, feature)
HOOKS = [
    (0x00051247, "e87da10200", "patch_xonline_return",       0xE8, "redirect"),
    (0x00099CB2, "d9442404d9", "patch_xonline_rgba",         0xE9, "colors"),
    (0x00099E2A, "8b44240489", "patch_xonline_colors",       0xE9, "colors"),
    (0x0009FC7C, "e8ab110000", "patch_xonline_xbx_size",     0xE8, "textures"),
    (0x0009FD17, "e8e0110000", "patch_xonline_xbx_open",     0xE8, "textures"),
    (0x0004D727, "e8b48f0900", "patch_xonline_video_device", 0xE9, "video720"),
    (0x00098EA2, "ff35c43519", "patch_xonline_video_ui",     0xE9, "video720"),
]

FONT_OFFSET = 0x3ECC
OLD_FONT = b"\\Device\\Harddisk0\\partition2\\xodash\\Xbox.xtf\0"
NEW_FONT = b"\\Device\\Harddisk0\\partition2\\Xbox.xtf\0"


def die(message):
    sys.exit("xonline patch: " + message)


class Xbe:
    def __init__(self, buf):
        self.buf = buf
        self.base = struct.unpack_from("<I", buf, 0x104)[0]
        self.nsec = struct.unpack_from("<I", buf, 0x11C)[0]
        self.section_table = struct.unpack_from("<I", buf, 0x120)[0]

    def sections(self):
        for index in range(self.nsec):
            offset = self.section_table - self.base + index * 0x38
            flags, va, size, raw, raw_size = struct.unpack_from("<5I", self.buf, offset)
            yield flags, va, size, raw, raw_size

    def offset(self, va):
        for _flags, section_va, _size, raw, raw_size in self.sections():
            if section_va <= va < section_va + raw_size:
                return raw + va - section_va
        die(f"address {va:08x} is not mapped")


def parse_features(args):
    selected = set(DEFAULT_FEATURES)
    if args.only:
        selected = {item.strip() for item in args.only.split(",") if item.strip()}
    if args.skip:
        selected -= {item.strip() for item in args.skip.split(",") if item.strip()}
    unknown = selected - set(FEATURES)
    if unknown:
        die(
            f"unknown feature(s) {', '.join(sorted(unknown))}; "
            f"pick from {', '.join(FEATURES)}"
        )
    return selected


def main():
    parser = argparse.ArgumentParser(
        description="Patch a retail XOnline Dashboard XBE with UIX Lite features."
    )
    parser.add_argument(
        "--only",
        help="comma-separated features to apply: " + ",".join(FEATURES),
    )
    parser.add_argument(
        "--skip",
        help="comma-separated features to leave out: " + ",".join(FEATURES),
    )
    parser.add_argument("input", help="retail xonlinedash.xbe")
    parser.add_argument("output", help="where to write the patched XBE")
    args = parser.parse_args()
    selected = parse_features(args)

    if os.path.abspath(args.input) == os.path.abspath(args.output):
        die("input and output must differ")

    buf = bytearray(open(args.input, "rb").read())
    if hashlib.sha256(buf).hexdigest() != SHA256:
        die("unsupported xonlinedash.xbe")

    hooks = [hook for hook in HOOKS if hook[4] in selected]
    symbols = {}
    xbe = Xbe(buf)

    if hooks:
        blob = open(os.path.join(HERE, "bin", "xonline_skin.bin"), "rb").read()
        with open(os.path.join(HERE, "bin", "symbols.txt")) as symbol_file:
            for line in symbol_file:
                address, _kind, name = line.split()
                symbols[name] = int(address, 16)

        cave = xbe.offset(CODE_VA)
        if any(buf[cave:cave + len(blob)]):
            die("payload cave is not empty")
        buf[cave:cave + len(blob)] = blob

    for va, old_hex, symbol, opcode, _feature in hooks:
        offset = xbe.offset(va)
        old = bytes.fromhex(old_hex)
        if buf[offset:offset + 5] != old:
            die(f"hook mismatch at {va:08x}")
        relative = (symbols[symbol] - va - 5) & 0xFFFFFFFF
        buf[offset:offset + 5] = bytes([opcode]) + struct.pack("<I", relative)

    if "fontpath" in selected:
        if buf[FONT_OFFSET:FONT_OFFSET + len(OLD_FONT)] != OLD_FONT:
            die("font path mismatch at file offset 00003ecc")
        replacement = NEW_FONT + bytes(len(OLD_FONT) - len(NEW_FONT))
        buf[FONT_OFFSET:FONT_OFFSET + len(OLD_FONT)] = replacement

    with open(args.output, "wb") as output:
        output.write(buf)

    enabled = ", ".join(feature for feature in FEATURES if feature in selected)
    print(f"{args.output}: {enabled or 'no features'}")
    print(hashlib.sha256(buf).hexdigest())


if __name__ == "__main__":
    main()

