#!/usr/bin/env python3
"""Create a deterministic VisualXIP patch package from one payload/symbol build."""

from __future__ import print_function

import argparse
import hashlib
import os
import zipfile


def digest(path):
    value = hashlib.sha256()
    with open(path, "rb") as source:
        while True:
            block = source.read(65536)
            if not block:
                break
            value.update(block)
    return value.hexdigest()


def parse_symbols(path):
    names = []
    seen = set()
    with open(path, "r") as source:
        for number, line in enumerate(source, 1):
            fields = line.split()
            if len(fields) < 3:
                continue
            name = fields[2]
            if name in seen:
                raise ValueError("duplicate symbol %s on line %d" % (name, number))
            seen.add(name)
            names.append(name)
    if not names:
        raise ValueError("the symbols file contains no symbols")
    return names


def zip_entry(archive, name, data):
    info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
    info.compress_type = zipfile.ZIP_DEFLATED
    info.external_attr = 0o100644 << 16
    archive.writestr(info, data)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--payload", required=True)
    parser.add_argument("--symbols", required=True)
    args = parser.parse_args()

    if os.path.getsize(args.payload) == 0:
        raise ValueError("the payload is empty")
    symbols = parse_symbols(args.symbols)
    with open(args.manifest, "r") as source:
        manifest_text = source.read()
    replacements = {
        "{payload_name}": os.path.basename(args.payload),
        "{symbols_name}": os.path.basename(args.symbols),
        "{payload_sha256}": digest(args.payload),
        "{symbols_sha256}": digest(args.symbols),
        "{required_symbols}": ",".join(symbols),
    }
    for token, value in replacements.items():
        if token not in manifest_text:
            raise ValueError("manifest template is missing " + token)
        manifest_text = manifest_text.replace(token, value)
    manifest = manifest_text.replace("\r\n", "\n").encode("utf-8")

    output_dir = os.path.dirname(os.path.abspath(args.output))
    if not os.path.isdir(output_dir):
        os.makedirs(output_dir)
    with open(args.payload, "rb") as source:
        payload = source.read()
    with open(args.symbols, "rb") as source:
        symbol_data = source.read()
    with zipfile.ZipFile(args.output, "w") as archive:
        zip_entry(archive, "manifest.ini", manifest)
        zip_entry(archive, os.path.basename(args.payload), payload)
        zip_entry(archive, os.path.basename(args.symbols), symbol_data)
    print("Created %s" % args.output)


if __name__ == "__main__":
    main()
