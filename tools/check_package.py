#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Paket içerik denetimi (CI).

- Beklenen dosyalar var mı (liblist, delta.lst, istemci/sunucu ikilileri, kurulum aracı)?
- Orijinal oyun verisi sayılabilecek dosya türleri pakete girmiş mi? (girmemeli)
- Linux: istemci .so dosyası SDL sembolü dışa aktarıyor ya da SDL2'ye bağımlı mı? (olmamalı)
- Linux: ikililerin istediği en yüksek GLIBC sürümünü raporla ve sınırı denetle.
"""
import argparse
import os
import re
import subprocess
import sys

MODS = ("lambda_hl", "lambda_op4", "lambda_bs")
# Orijinal oyun içeriği türleri: haritalar, dokular, modeller, sesler, videolar, paketler, kayıtlar
FORBIDDEN_EXT = {".bsp", ".wad", ".mdl", ".spr", ".wav", ".mp3", ".avi", ".bik", ".pak", ".sav", ".dem", ".bmp"}
ALLOWED_EXT = {".dll", ".so", ".exe", ".gam", ".lst", ".txt", ".md", ".cfg", ".png", ".tga", ".ttf", ".otf", ".json", ".vdf", ""}


def fail(msg, errors):
    errors.append(msg)
    print("ERROR:", msg)


def check_tree(stage, platform, errors):
    lib = ".dll" if platform == "windows" else ".so"
    exe = "lambda-setup.exe" if platform == "windows" else "lambda-setup"
    for mod in MODS:
        for rel in ("liblist.gam", "delta.lst", "lambda/lambda_mod.txt", "cl_dlls/client" + lib, "dlls/hl" + lib):
            if not os.path.isfile(os.path.join(stage, mod, rel)):
                fail(f"missing {mod}/{rel}", errors)
    if not os.path.isfile(os.path.join(stage, "lambda_tools", exe)):
        fail(f"missing lambda_tools/{exe}", errors)
    for root, _dirs, files in os.walk(stage):
        for name in files:
            ext = os.path.splitext(name)[1].lower()
            rel = os.path.relpath(os.path.join(root, name), stage)
            if ext in FORBIDDEN_EXT:
                fail(f"original game data type must not be packaged: {rel}", errors)
            elif ext not in ALLOWED_EXT and not name.endswith((".pdb", ".dbg")):
                fail(f"unexpected file type in package: {rel}", errors)


def run(cmd):
    return subprocess.run(cmd, check=True, capture_output=True, text=True).stdout


def check_linux_symbols(stage, max_glibc, errors):
    worst = (0, 0)
    for mod in MODS:
        for rel in ("cl_dlls/client.so", "dlls/hl.so"):
            path = os.path.join(stage, mod, rel)
            if not os.path.isfile(path):
                continue
            exported = run(["nm", "-D", "--defined-only", path])
            leaked = [l for l in exported.splitlines() if re.search(r"\bSDL_", l)]
            if leaked:
                fail(f"{mod}/{rel} exports SDL symbols (would clash with the engine's SDL2): {leaked[:5]}", errors)
            undefined = run(["nm", "-D", "--undefined-only", path])
            sdl_undef = [l for l in undefined.splitlines() if re.search(r"\bSDL_", l)]
            if sdl_undef:
                fail(f"{mod}/{rel} links SDL symbols dynamically: {sdl_undef[:5]}", errors)
            needed = run(["objdump", "-p", path])
            if "libSDL2" in needed:
                fail(f"{mod}/{rel} depends on libSDL2 at load time", errors)
            for m in re.finditer(r"NEEDED\s+(\S+)", needed):
                if "/" in m.group(1):
                    fail(f"{mod}/{rel} has a build-machine path in DT_NEEDED: {m.group(1)}", errors)
            for m in re.finditer(r"GLIBC_(\d+)\.(\d+)", undefined + needed):
                worst = max(worst, (int(m.group(1)), int(m.group(2))))
    print(f"Highest required glibc symbol version: {worst[0]}.{worst[1]}")
    limit = tuple(int(x) for x in max_glibc.split("."))
    if worst > limit:
        fail(f"binaries require glibc {worst[0]}.{worst[1]}, above the supported baseline {max_glibc}", errors)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("stage")
    ap.add_argument("--platform", choices=("windows", "linux"), required=True)
    ap.add_argument("--max-glibc", default="2.31")
    args = ap.parse_args()
    errors = []
    check_tree(args.stage, args.platform, errors)
    if args.platform == "linux":
        check_linux_symbols(args.stage, args.max_glibc, errors)
    if errors:
        print(f"\nPackage check failed with {len(errors)} error(s).")
        return 1
    print("Package check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
