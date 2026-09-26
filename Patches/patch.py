#!/usr/bin/env python3
"""Patch a retail 5960 xboxdash.xbe into UIX Lite's dashboard.

Only the retail 5960 XBE is accepted (checked by hash), and the input is
never modified. The patches are:

  drives      N: O: for HDD0 partitions 6/7, P: Q: R: for a second drive
  video720    1280x720 progressive on HDTV when 720p + widescreen are enabled
  xipsig      load modified XIPs (skip the signature and digest checks)
  dvdregion   play DVDs from any region
  skins       Theseus-style skins, switchable live from the Skins menu
  discimages  boot default.iso / default.cci title folders (Cerbios)

All of them are applied unless you pick with --only or --skip.

  python3 patch.py xboxdash.xbe xboxdash_uixlite.xbe
  python3 patch.py --skip video720,dvdregion xboxdash.xbe out.xbe

The code the patches add is built from src/ by build.sh into bin/; the
in-place edits are listed below with the instructions they write.
"""

import argparse
import hashlib
import os
import struct
import sys

RETAIL_MD5 = "08d3a6f99184679aa13008d6397bacce"
RETAIL_SHA1 = "4d8d6b1d3f7ace05cbaf03565382b0e6e40c706c"

FEATURES = ["drives", "video720", "xipsig", "dvdregion", "skins", "discimages", "ftp"]

HERE = os.path.dirname(os.path.abspath(__file__))
CODE_VA = 0x001FE000         # link address in link.ld
SECTION_NAME = b".uixlite"
SECTION_FLAGS = 0x7          # writable, preload, executable
NEW_HEADERS_VA = 0x00010E00  # free header space in retail (0x10dde..0x11b20 is unused)

# Bits in patch_features (src/uixlite.h): startup work patch_start does per feature
FEATURE_BITS = {"drives": 0x1, "ftp": 0x2}

# 5-byte call / jmp redirects into the patch code:
# (address, retail bytes, symbol, opcode, features that need it)
HOOKS = [
    (0x0002DCA1, "e85afdffff", "patch_start",                 0xE8, set(FEATURE_BITS)),       # main: run the dashboard
    (0x0002CE6F, "e85cb41100", "patch_create_device",         0xE8, {"video720"}),            # D3D setup: create the device
    (0x0002D89D, "e80ea40200", "patch_material_init",         0xE8, {"skins"}),               # startup: build the material table
    (0x0006172F, "e85cf5ffff", "patch_texture_from_xip",      0xE8, {"skins"}),               # texture cache miss: XIP lookup
    (0x0003A490, "b8f0fd1700", "patch_config_getfunctionmap", 0xE9, {"skins", "discimages"}), # theConfig's function table
]

# In-place edits: (address, retail bytes, new bytes, feature, what)
INPLACE = [
    # XIP loader. A retail XIP carries a signed table of chunk digests; any
    # mismatch reboots the box (push 4 / call HalReturnToFirmware).
    (0x0003D4C8, "75", "eb", "xipsig", "jmp: don't reboot when the signature section is missing"),
    (0x0003D520, "75", "eb", "xipsig", "jmp: don't reboot when the digest table is empty"),
    (0x0003D5BA, "6a04ff1518200100", "9090909090909090", "xipsig", "nop: don't reboot when the table's signature fails"),
    (0x0003D863, "0f8385000000", "909090909090", "xipsig", "nop: read chunks the table doesn't list"),
    (0x0003D880, "756c", "9090", "xipsig", "nop: ignore a chunk digest mismatch"),
    # The second XIP reader, same three checks
    (0x0003EB13, "75", "eb", "xipsig", "jmp: don't reboot when the signature section is missing"),
    (0x0003EBE7, "0f8483", "e98400", "xipsig", "je -> jmp (same target): skip the reboot"),
    (0x0003EC54, "74", "eb", "xipsig", "jmp: ignore a digest mismatch"),

    # DVD player setup. Retail refuses a disc whose region doesn't match the
    # console's; instead, try regions 1..6 until the player opens.
    (0x000667F7, "74", "eb", "dvdregion", "jmp: skip the console region check"),
    (0x00066833,
     "8b874401000033c98a4c241233d2b20149d2e2f6d2526a4450e807fc12008b8f440100008db748010000566a4451e85cfb12"
     "008bd081e20000008081fa0000008089442418752c3d008083870f8510010000be05000000e851f20d006a006a00687cab02"
     "0057e80209fdff5f5e5d5b83c47cc3",
     "c64424120133c98b87440100008a4c241233d2b20149d2e2f6d2526a4450e802fc12008b8f440100008db748010000566a44"
     "51e857fb12008bd081e20000008081fa00000080894424187527fe442412807c24120676b090909090909090909090909090"
     "909090909090909090909090909090",
     "dvdregion",
     """region loop:
        mov  byte [esp+0x12], 1          ; region = 1
        xor  ecx, ecx
    try:
        mov  eax, [edi+0x144]            ; player
        mov  cl, [esp+0x12]
        xor  edx, edx
        mov  dl, 1
        dec  ecx
        shl  dl, cl
        not  dl                          ; region mask
        push edx
        push 0x44
        push eax
        call 0x196458                    ; set the system region
        mov  ecx, [edi+0x144]
        lea  esi, [edi+0x148]
        push esi
        push 0x44
        push ecx
        call 0x1963c2                    ; open the player
        mov  edx, eax
        and  edx, 0x80000000
        cmp  edx, 0x80000000
        mov  [esp+0x18], eax
        jne  0x668a6                     ; opened
        inc  byte [esp+0x12]
        cmp  byte [esp+0x12], 6
        jbe  try
        nop x28                          ; all six failed: carry on as retail's success path"""),
]


def die(msg):
    sys.exit(f"patch.py: {msg}")


class Xbe:
    def __init__(self, buf):
        self.buf = buf
        self.base, = struct.unpack_from("<I", buf, 0x104)
        self.nsec, self.sec_va = struct.unpack_from("<II", buf, 0x11C)

    def u32(self, va):
        return struct.unpack_from("<I", self.buf, va - self.base)[0]

    def sections(self):
        out = []
        for i in range(self.nsec):
            o = self.sec_va - self.base + i * 0x38
            flags, va, vsize, raw, rawsize = struct.unpack_from("<5I", self.buf, o)
            out.append({"flags": flags, "va": va, "vsize": vsize, "raw": raw, "rawsize": rawsize})
        return out

    def file_offset(self, va):
        for s in self.sections():
            if s["va"] <= va < s["va"] + s["rawsize"]:
                return s["raw"] + va - s["va"]
        die(f"{va:08x} is not in any section")


def add_section(buf, blob):
    """Append a section for the patch code.

    The section header table is copied to free header space with the new
    entry on the end, so nothing else in the header moves: only the section
    count, the table pointer and the image size change.
    """
    x = Xbe(buf)
    last = x.sections()[-1]
    va = (last["va"] + last["vsize"] + 0xFFF) & ~0xFFF
    if va != CODE_VA:
        die(f"patch code is linked at {CODE_VA:08x} but the section would start at {va:08x}")
    raw = (len(buf) + 0xFFF) & ~0xFFF
    size = (len(blob) + 0xF) & ~0xF

    table = bytes(buf[x.sec_va - x.base:x.sec_va - x.base + x.nsec * 0x38])
    new_table = NEW_HEADERS_VA
    refcounts = new_table + (x.nsec + 1) * 0x38      # head / tail shared page counts (WORD each)
    name = refcounts + 4
    end = name + len(SECTION_NAME) + 1
    header_end = struct.unpack_from("<I", buf, 0x108)[0] + x.base
    if any(buf[new_table - x.base:end - x.base]) or end > 0x11B20 or end > header_end:
        die("header space for the new section table isn't free")

    hdr = struct.pack("<9I", SECTION_FLAGS, va, size, raw, size, name, 0, refcounts, refcounts + 2) + bytes(20)
    o = new_table - x.base
    buf[o:o + len(table)] = table
    buf[o + len(table):o + len(table) + 0x38] = hdr
    buf[name - x.base:name - x.base + len(SECTION_NAME)] = SECTION_NAME

    struct.pack_into("<II", buf, 0x11C, x.nsec + 1, new_table)
    struct.pack_into("<I", buf, 0x10C, va + size - x.base)   # size of image

    if len(buf) < raw:
        buf += bytes(raw - len(buf))
    buf += blob + bytes(((size + 0xFFF) & ~0xFFF) - len(blob))
    return va


def main():
    ap = argparse.ArgumentParser(description="Patch a retail 5960 xboxdash.xbe into UIX Lite's dashboard.")
    ap.add_argument("input", help="retail 5960 xboxdash.xbe")
    ap.add_argument("output", help="where to write the patched XBE")
    ap.add_argument("--only", help="comma-separated features to apply")
    ap.add_argument("--skip", help="comma-separated features to leave out")
    args = ap.parse_args()

    want = set(FEATURES)
    if args.only:
        want = set(args.only.split(","))
    if args.skip:
        want -= set(args.skip.split(","))
    bad = want - set(FEATURES)
    if bad:
        die(f"unknown feature(s) {', '.join(sorted(bad))}; pick from {', '.join(FEATURES)}")
    if os.path.abspath(args.input) == os.path.abspath(args.output):
        die("write to a new file; the input is left alone")

    buf = bytearray(open(args.input, "rb").read())
    if hashlib.md5(buf).hexdigest() != RETAIL_MD5 or hashlib.sha1(buf).hexdigest() != RETAIL_SHA1:
        die("that isn't the retail 5960 xboxdash.xbe")

    hooks = [h for h in HOOKS if h[4] & want]
    if hooks:
        blob = open(os.path.join(HERE, "bin", "uixlite.bin"), "rb").read()
        syms = {}
        for line in open(os.path.join(HERE, "bin", "symbols.txt")):
            addr, _, sym = line.split()
            syms[sym] = int(addr, 16)
        va = add_section(buf, blob)
        bits = sum(b for f, b in FEATURE_BITS.items() if f in want)
        o = Xbe(buf).file_offset(syms["patch_features"])
        struct.pack_into("<I", buf, o, bits)

    x = Xbe(buf)
    for va, orig, sym, op, _ in hooks:
        o = x.file_offset(va)
        if bytes(buf[o:o + 5]) != bytes.fromhex(orig):
            die(f"hook site {va:08x} doesn't hold the retail bytes")
        rel = (syms[sym] - (va + 5)) & 0xFFFFFFFF
        buf[o:o + 5] = bytes([op]) + struct.pack("<I", rel)

    for va, orig, new, feature, _ in INPLACE:
        if feature not in want:
            continue
        orig, new = bytes.fromhex(orig), bytes.fromhex(new)
        if len(orig) != len(new):
            die(f"in-place edit at {va:08x} changes length")
        o = x.file_offset(va)
        if bytes(buf[o:o + len(orig)]) != orig:
            die(f"{va:08x} doesn't hold the retail bytes")
        buf[o:o + len(new)] = new

    open(args.output, "wb").write(buf)
    print(f"{args.output}: {', '.join(f for f in FEATURES if f in want)}")


if __name__ == "__main__":
    main()
